
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <grx20.h>
#include <grxkeys.h>


#include "trs_iodefs.h"
#include "trs.h"
#include "z80.h"
#include "trs_disk.h"
#include "trs_uart.h"
#include "trs_imp_exp.h"

#include <dirent.h>
#include <assert.h>
#include <io.h>
#include "trs_metafile.h"
#include "trs_djgpp.h"

static int min(int x, int y) {
    return x < y ? x : y;
}

#define ENTRY_TYPE_NONE 0
#define ENTRY_TYPE_CASSETTE 1
#define ENTRY_TYPE_SUBDIR 2
#define ENTRY_TYPE_PARENTDIR 3

typedef char cassette_instructions[256];
typedef char cassette_description[256];
typedef char cassette_entry_fn[sizeof(cassette_filename_buffer) - 16];
typedef char cassette_simple_filename[128];

typedef char cassette_display_name[32];

typedef struct cassette_entry {
    cassette_display_name display_name;
    cassette_simple_filename filename;
    unsigned char entry_type;
} cassette_entry;

typedef struct cassette_table {
    unsigned int size;
    cassette_entry pEntries[];
} cassette_table;



struct dialogOption {
    char key;
    const char *display;
    int enabled;
};

struct optionDisplay {
    GrTextOption gtr;
    size_t optionCount;
    int xmin;
    int xmax;
    int y;
    struct dialogOption options[10];
};

static void drawOptions(const struct optionDisplay *o, int toggleIndex) {
    char buf[64];
    int dx = (o->xmax - o->xmin) / (int)o->optionCount;
    for(int indx = 0; indx < (int)(o->optionCount) ; indx++) {
        GrTextOption cpy = o ->gtr;
        cpy.txo_xalign = GR_ALIGN_CENTER;
        if (!o->options[indx].enabled) {
            cpy.txo_fgcolor.v = COLOR_DISABLED;
        } else if (toggleIndex == indx) {
            cpy.txo_bgcolor.v = GrWhite();
        }
        safe_strcpy(buf, o->options[indx].display, sizeof(buf));
        GrDrawString(buf, (int) strlen(buf),o->xmin + dx * (1 + 2*indx) / 2, o->y, &cpy);
    }
}

int matchOption(const struct optionDisplay *o, char c) {
    for(int i=0;i<(int)(o->optionCount);i++) {
        if(o->options[i].enabled && o->options[i].key == c) {
            drawOptions(o, i);
            usleep(50000);
            drawOptions(o, -1);
            return i;
        }
    }
    return -1;
}

static void show_help(char *disp_name, struct mem_block *meta);

static void fill_standard_text_option(GrTextOption *grt) {
    memset(grt, 0, sizeof(GrTextOption));
    // Use the GrFont_PC8x14 - presumably i can assume 8 pixels wide and 14 pixels high
    // so I don't have to use the text measurement functions.
    grt->txo_font = &GrFont_PC8x14;
    grt->txo_fgcolor.v = COLOR_PRIMARY;
    grt->txo_bgcolor.v = GrBlack();
    grt->txo_direct = GR_TEXT_RIGHT;
    grt->txo_xalign = GR_ALIGN_CENTER;
    grt->txo_yalign = GR_ALIGN_CENTER;
    grt->txo_chrtype = GR_BYTE_TEXT;
}


static unsigned char get_cassette_entry_type(struct dirent *dir) {
    if (!dir) {
        return ENTRY_TYPE_NONE;
    }
    if (dir->d_type == DT_DIR && dir->d_namlen > 1 && dir->d_name[0] != '.') {
        return ENTRY_TYPE_SUBDIR;
    }
    if (dir->d_type != DT_REG) {
        return ENTRY_TYPE_NONE;
    }
    if (dir->d_namlen < 4) {
        return ENTRY_TYPE_NONE;
    }
    char *p;
    p = (dir->d_name) + (dir->d_namlen) - 1;
    if (tolower(*(p--)) != 's')
        return 0;
    if (tolower(*(p--)) != 'a')
        return 0;
    if (tolower(*(p--)) != 'c')
        return 0;
    if (tolower(*(p--)) != '.')
        return 0;

    return ENTRY_TYPE_CASSETTE;

}

static void init_entry(cassette_entry *entry, struct dirent *de) {
    assert(entry != 0);
    memset(entry, 0, sizeof(cassette_entry));
    if (!de) {
        //Sentinel de==null --> is parentdir
        entry->entry_type = ENTRY_TYPE_PARENTDIR;
        strcpy(entry->display_name, "( Parent Dir )");
        strcpy(entry->filename, "..");
        return;
    }

    unsigned char entry_type = get_cassette_entry_type(de);
    assert(entry_type != ENTRY_TYPE_NONE);
    entry->entry_type = entry_type;
    strncpy(entry->filename, de->d_name, sizeof(cassette_simple_filename));
    entry->filename[sizeof(cassette_simple_filename) - 1] = 0;

    if (entry_type == ENTRY_TYPE_CASSETTE) {
        strncpy(entry->display_name, de->d_name, sizeof(cassette_display_name));
        entry->display_name[sizeof(cassette_display_name) - 1] = 0;
        char *p = strrchr(entry->display_name, '.');
        //Chop off extension in display name
        if (p) {
            *p = 0;
        }
    } else {
        const char *prefix = "< ";
        const char *suffix = " >";
        const unsigned int room = sizeof(cassette_display_name) - strlen(prefix) - strlen(suffix);
        strcpy(entry->display_name, prefix);
        strncpy(entry->display_name + strlen(prefix), de->d_name, room - strlen(prefix));
        entry->display_name[sizeof(cassette_display_name) - 1 - strlen(suffix)] = 0;
        strcat(entry->display_name, suffix);
    }
    unsigned int fnl = strlen(entry->display_name);
    for (unsigned int i = 0; i < fnl; i++) {
        entry->display_name[i] = toupper(entry->display_name[i]);
    }
}

static int compare_cassette_entries(const cassette_entry *e1, const cassette_entry *e2) {
    if (e1->display_name[0] == '(') {
        return e2->display_name[0] == '(' ? 0 : 1;
    }
    if (e2->display_name[0] == '(') {
        return -1;
    }
    return stricmp(e1->display_name, e2->display_name);
}

static cassette_table *load_cassette_table(char *base_dir, int nestCount) {
    struct dirent *de;
    DIR *d;
    int tableSize = 0;
    cassette_table *pRes;

    d = opendir(base_dir);
    if (!d) {
        joshlog("opendir failed\n");
        return 0;
    }
    while ((de = readdir(d))) {
        if (get_cassette_entry_type(de)) {
            tableSize++;
        }
    }
    closedir(d);
    if (nestCount) {
        tableSize++;
    }

    pRes = (cassette_table *) malloc(sizeof(cassette_table) + tableSize * sizeof(cassette_entry));
    if (!pRes) {
        return 0;
    }
    memset(pRes, 0, sizeof(cassette_table) + tableSize * sizeof(cassette_entry));

    d = opendir(base_dir);
    if (!d) {
        joshlog("opendir failed 2\n");
        free(pRes);
        return 0;
    }
    int ts2 = 0;
    if (nestCount) {
        init_entry(pRes->pEntries, 0);
        ts2 += 1;
    }
    while ((de = readdir(d)) && ts2 < tableSize) {
        if (get_cassette_entry_type(de)) {
            init_entry(pRes->pEntries + ts2, de);
            ts2 += 1;
        }
    }
    closedir(d);

    qsort(pRes->pEntries, ts2, sizeof(cassette_entry), (int (*)(const void *, const void *)) compare_cassette_entries);
    pRes->size = ts2;


    return pRes;
}

/** If base_dir is null, cassette_file name is assumed to be the full path */
static struct mem_block *load_cassette_meta(const char *base_dir, const char *cassette_filename) {
    char fn[2000];

    unsigned long dirlen = 0;
    if (base_dir) {
        strncpy(fn, base_dir, sizeof(fn) - 2);
        fn[sizeof(fn) - 1] = 0;
        dirlen = strlen(base_dir);
        if (dirlen) {
            char ec = fn[dirlen - 1];
            if (!(ec == '/' || ec == '\\')) {
                fn[dirlen++] = '/';
                fn[dirlen] = 0;
            }
        }
    }

    strncpy(fn + dirlen, cassette_filename, sizeof(fn) - dirlen );
    fn[sizeof(fn) - 1] = 0;
    unsigned long fulllen = strlen(fn);
    if (fulllen > 4 && stricmp(".cas", fn + (fulllen - 4)) == 0) {
        fn[fulllen - 3] = 'M';
        fn[fulllen - 2] = 'E';
        fn[fulllen - 1] = 'T';
    } else if (fulllen < (sizeof(fn) - 5)) {
        strcat(fn, ".MET");
    } else {
        return 0;
    }
    joshlog("Looking for metafile: %s\n", fn);
    if (!__file_exists(fn)) {
        return 0;
    }
    joshlog("metafile: %s exists!\n", fn);
    return read_mem_block_from_file(fn);
}

static int choose_cassette(cassette_entry_fn pDest) {
    assert(pDest != 0);
    int x, y;
    int insety = 50;
    int insetx = 80;
    int return_value = 0;
    typedef const char *choice;
    char baseDir[sizeof(cassette_filename_buffer)];
    strncpy(baseDir, cassette_base_directory, sizeof(cassette_filename_buffer));
    baseDir[sizeof(cassette_filename_buffer) - 1] = 0;
    const unsigned long initial_basedir_len = strlen(baseDir);
    int dirLevels = 0;
    int reload_table = 1;

    static const choice choices[] = {"(B)ack", "(S)elect", "(A)bout", "(C)ancel", "(N)ext"};
    static const char *chooseMsg = "Choose Cassette";
    cassette_table *pTable = 0;
    int currentEntry = 0;
    GrClearScreen(GrBlack());
    GrTextOption grt;
    fill_standard_text_option(&grt);
    const GrTextOption baset = grt;

    for (;;) {
        if (!pTable || reload_table) {
            reload_table = 0;
            if (pTable) {
                free(pTable);
            }
            pTable = load_cassette_table(baseDir, dirLevels);
            if (!pTable) {
                joshlog("ERR: Failed to load_cassette_table\n");
                goto done;
            }
            if (!pTable->size) {
                joshlog("ERR: No cassettes found\n");
                goto done;
            }
            currentEntry = 0;
        }
        grt = baset;


        GrFilledBox(insetx, insety, GrMaxX() - insetx, GrMaxY() - insety, GrBlack());
        GrBox(insetx, insety, GrMaxX() - insetx, GrMaxY() - insety, COLOR_BORDER);
        GrBox(insetx + 4, insety + 4, GrMaxX() - insetx - 4, GrMaxY() - insety - 4, COLOR_BORDER);

        x = GrMaxX() / 2;
        y = GrMaxY() / 2;

        GrDrawString((void *) chooseMsg, strlen(chooseMsg), x, y - 30, &grt);
        grt.txo_fgcolor.v = GrWhite();
        GrDrawString(pTable->pEntries[currentEntry].display_name, strlen(pTable->pEntries[currentEntry].display_name),
                     x, y - 10, &grt);
        grt = baset;


        /** DISPLAY DESCRIPTION */
        grt.txo_font = &GrFont_PC6x8;
        grt.txo_fgcolor.v = COLOR_SECONDARY;
        if (pTable->pEntries[currentEntry].entry_type == ENTRY_TYPE_CASSETTE ||
            pTable->pEntries[currentEntry].entry_type == ENTRY_TYPE_SUBDIR) {
            struct mem_block *pmeta = load_cassette_meta(baseDir, pTable->pEntries[currentEntry].filename);
            if (pmeta) {
                joshlog("Looking for description...\n");
                boundary b = find_meta_data(pmeta, "description");
                if (b.end > b.start) {
                    grt.txo_font = &GrFont_PC6x8;
                    grt.txo_fgcolor.v = COLOR_SECONDARY;
                    GrDrawString(pmeta->data + b.start, b.end - b.start, x, y + 5, &grt);
                }

                free_mem_block(pmeta);
            }
        } else if (pTable->pEntries[currentEntry].entry_type == ENTRY_TYPE_PARENTDIR) {
            char pdmsg[64];
            strcpy(pdmsg, "Return to the previous directory");
            GrDrawString(pdmsg, (int) strlen(pdmsg), x, y + 5, &grt);
        }
        grt = baset;
        /** END DISPLAY DESCRIPTION */



        if (dirLevels) {
            unsigned long curdirlen = strlen(baseDir);
            if (curdirlen > initial_basedir_len) {
                grt.txo_font = &GrFont_PC6x8;
                grt.txo_xalign = GR_ALIGN_LEFT;
                grt.txo_fgcolor.v = COLOR_SECONDARY;
                GrDrawString(baseDir + initial_basedir_len, (int) (curdirlen - initial_basedir_len), insetx + 14,
                             insety + 14, &grt);
                grt = baset;
            }
        }

        if (pTable->pEntries[currentEntry].entry_type == ENTRY_TYPE_CASSETTE) {
            int tracks[20];
            int m = find_cas_tracks2(baseDir, pTable->pEntries[currentEntry].filename, tracks, 20);
            char msg[32];
            sprintf(msg,"%d tracks", m);
            grt.txo_font = &GrFont_PC6x8;
            grt.txo_xalign = GR_ALIGN_LEFT;
            grt.txo_fgcolor.v = COLOR_SECONDARY;
            GrDrawString(msg, (int) strlen(msg), insetx + 14,
                         insety + 24, &grt);
            grt = baset;

        }

        for (int ii = 0; ii < 5; ii++) {
            GrDrawString((void *) choices[ii], (int) strlen(choices[ii]), x - 200 + ii * 100, y + 35, &grt);
        }

        int choice = -1;
        for (; choice < 0;) {
            GrKeyType key;
            key = GrKeyRead();
            //joshlog("KEY %u\n", key);
            if (key >= 'a' && key <= 'z') {
                key = key + ('A' - 'a');
            }
            switch (key) {
                case GrKey_Up:
                    //HACK - Up arrow key is like left but brings us all the way to the top
                    currentEntry = 0;
                    choice = 0;
                    break;
                case GrKey_Left:
                case 'B':
                    choice = 0;
                    break;
                case GrKey_Down:
                    //HACK - Down arrow key is like right but brings us all the way to the top
                    currentEntry = pTable->size - 1;
                    choice = 4;
                    break;
                case GrKey_Right:
                case 'N':
                    choice = 4;
                    break;
                case GrKey_Return:
                case 'S':
                    choice = 1;
                    break;
                case GrKey_Escape:
                case 'C':
                    choice = 3;
                    break;
                case 'A':
                    choice = 2;
                    break;
                default:
                    choice = -1;
            }
        }
        grt.txo_fgcolor.v = GrBlack();
        grt.txo_bgcolor.v = GrWhite();
        GrDrawString((void *) choices[choice], strlen(choices[choice]), x - 200 + choice * 100, y + 35, &grt);
        usleep(50000);

        if (choice == 0) {
            if (currentEntry > 0)
                currentEntry--;
        } else if (choice == 4) {
            if (currentEntry < (pTable->size - 1))
                currentEntry++;
        } else if (choice == 1) {
            //select
            cassette_entry *pSel = pTable->pEntries + currentEntry;
            if (pSel->entry_type == ENTRY_TYPE_CASSETTE) {
                strcpy(pDest, baseDir);
                strcat(pDest, "\\");
                strcat(pDest, pSel->filename);
                return_value = 1;
                break;
            } else if (pSel->entry_type == ENTRY_TYPE_SUBDIR) {
                strcat(baseDir, "\\");
                strcat(baseDir, pSel->filename);
                reload_table = 1;
                dirLevels++;
            } else if (pSel->entry_type == ENTRY_TYPE_PARENTDIR && dirLevels > 0) {
                char *p = strrchr(baseDir, '\\');
                if (p) {
                    *p = 0;
                    dirLevels--;
                    reload_table = 1;
                }
                joshlog("%s %d\n", baseDir, dirLevels);
            }
            continue;
        } else if (choice == 3) {
            //cancel
            break;
        } else if (choice == 2) {
            if (pTable->pEntries[currentEntry].entry_type == ENTRY_TYPE_CASSETTE ||
                pTable->pEntries[currentEntry].entry_type == ENTRY_TYPE_SUBDIR) {
                struct mem_block *pmeta = load_cassette_meta(baseDir, pTable->pEntries[currentEntry].filename);
                show_help(pTable->pEntries[currentEntry].display_name, pmeta);
                if (pmeta) {
                    free_mem_block(pmeta);
                }
                GrClearScreen(GrBlack());
            }
        }
        //grt.txo_fgcolor.v = GrBlack();
        //grt.txo_bgcolor.v = GrWhite();
        //if (key == 'Y') GrDrawString( "(Y)es",5,x-80,y+20,&grt );
        //else GrDrawString( "(N)o",5,x+80,y+20,&grt );
        //usleep(100000);

    }


    done:
    if (pTable) {
        free(pTable);
    }
    return return_value;
}


static void yesno_message_handler(joshem_modal_context *pContext);

int joshem_modal_ask_yn(const char *pPrompt) {
    void *c = (void *) (pPrompt ? pPrompt : "<NULL>");
    return joshem_do_modal(yesno_message_handler, c);
}


static void yesno_message_handler(joshem_modal_context *pContext) {
    char *message;
    int x, y;
    int insety = 50;
    int insetx = 80;
    message = (char *) (pContext->input);
    GrTextOption grt;
    fill_standard_text_option(&grt);

    GrClearScreen(GrBlack());


    GrFilledBox(insetx, insety, GrMaxX() - insetx, GrMaxY() - insety, GrBlack());
    GrBox(insetx, insety, GrMaxX() - insetx, GrMaxY() - insety, COLOR_BORDER);
    GrBox(insetx + 4, insety + 4, GrMaxX() - insetx - 4, GrMaxY() - insety - 4, COLOR_BORDER);

    x = GrMaxX() / 2;
    y = GrMaxY() / 2;
    joshlog("II %s %d %d\n", message, x, y);

    GrDrawString(message, strlen(message), x, y - 10, &grt);
    GrDrawString("(Y)es", 5, x - 80, y + 20, &grt);
    GrDrawString("(N)o", 5, x + 80, y + 20, &grt);

    GrKeyType key;
    for (;;) {
        key = GrKeyRead();
        if (key >= 'a' && key <= 'z') {
            key = key + ('A' - 'a');
        }
        if (key == 'Y' || key == 'N') {
            break;
        }
    }

    grt.txo_fgcolor.v = GrBlack();
    grt.txo_bgcolor.v = GrWhite();
    if (key == 'Y') GrDrawString("(Y)es", 5, x - 80, y + 20, &grt);
    else GrDrawString("(N)o", 5, x + 80, y + 20, &grt);
    usleep(100000);
    pContext->result = (key == 'Y') ? 1 : 0;

}

static void message_message_handler(joshem_modal_context *pContext);

void joshem_modal_message(const char *pPrompt) {
    void *c = (void *) (pPrompt ? pPrompt : "<NULL>");
    joshem_do_modal(message_message_handler, c);
}


static void message_message_handler(joshem_modal_context *pContext) {
    char *message;
    int x, y;
    int insety = 50;
    int insetx = 80;
    message = (char *) (pContext->input);
    GrTextOption grt;
    fill_standard_text_option(&grt);
    GrClearScreen(GrBlack());

    GrFilledBox(insetx, insety, GrMaxX() - insetx, GrMaxY() - insety, GrBlack());
    GrBox(insetx, insety, GrMaxX() - insetx, GrMaxY() - insety, COLOR_BORDER);
    GrBox(insetx + 4, insety + 4, GrMaxX() - insetx - 4, GrMaxY() - insety - 4, COLOR_BORDER);

    x = GrMaxX() / 2;
    y = GrMaxY() / 2;
    //joshlog("Modal Message: %s %d %d\n", message, x, y);

    GrDrawString(message, (int) strlen(message), x, y - 10, &grt);
    GrDrawString("Press (Enter)", 13, x, y + 20, &grt);

    GrKeyType key;
    for (;;) {
        key = GrKeyRead();
        if (key == GrKey_Return) {
            break;
        }
    }

    pContext->result = 0;

}


static int ask_question(const char *prompt, char *dest, unsigned int maxLen) {
    int x, y;
    int insety = 50;
    int insetx = 80;
    int retVal = 0;

    char *buf = malloc(maxLen + 2);
    if (!buf) {
        joshlog("ERR: ask_question alloc failed\n");
        return 0;
    }
    buf[0] = 0;
    unsigned int cur_len = 0;

    GrTextOption grt;
    fill_standard_text_option(&grt);
    GrClearScreen(GrBlack());

    GrFilledBox(insetx, insety, GrMaxX() - insetx, GrMaxY() - insety, GrBlack());
    GrBox(insetx, insety, GrMaxX() - insetx, GrMaxY() - insety, COLOR_BORDER);
    GrBox(insetx + 4, insety + 4, GrMaxX() - insetx - 4, GrMaxY() - insety - 4, COLOR_BORDER);

    for (;;) {
        x = GrMaxX() / 2;
        y = GrMaxY() / 2;
        GrFilledBox(x - 60, y - 10, x + 60, y + 10, GrBlack());
        GrBox(x - 60, y - 10, x + 60, y + 10, GrWhite());

        grt.txo_xalign = GR_ALIGN_LEFT;
        buf[cur_len] = (char) 0xB0;
        buf[cur_len + 1] = 0;
        GrDrawString(buf, (int) (cur_len + 1), x - 50, y, &grt);
        grt.txo_xalign = GR_ALIGN_CENTER;


        GrDrawString((void *) prompt, (int) strlen(prompt), x, y - 30, &grt);
        GrDrawString("(Enter) to accept", 17, x - 150, y + 30, &grt);
        GrDrawString("(Esc) to cancel", 15, x + 150, y + 30, &grt);

        GrKeyType key;
        for (;;) {
            key = GrKeyRead();
            if (key >= 'a' && key <= 'z') {
                key = key + ('A' - 'a');
            }
            if ((key >= 'A' && key <= 'Z') || (key >= '0' && key <= '9') || key == '_'
                || key == GrKey_BackSpace || key == GrKey_Return || key == GrKey_Escape)
                break;
        }
        if ((key >= 'A' && key <= 'Z') || (key >= '0' && key <= '9') || key == '_') {
            if (cur_len < maxLen) {
                buf[cur_len++] = (char) key;
            }
        } else if (key == GrKey_BackSpace) {
            if (cur_len > 0)
                cur_len--;
        } else if (key == GrKey_Return && cur_len > 0) {
            retVal = 1;
            break;
        } else if (key == GrKey_Escape) {
            retVal = 0;
            break;
        }

    }
    if (retVal) {
        memcpy(dest, buf, cur_len);
        dest[cur_len] = 0;
    }
    free(buf);
    return retVal;
}

static void create_display_name(const char *filename,  cassette_display_name  *  const display_name) {
    const char *slashpos = filename;
    const char *dotpos = filename;;
    const char *pos;
    for (pos = filename; *pos; pos++) {
        char c = *pos;
        if (c == '/' || c == '\\') {
            slashpos = dotpos = pos;
        } else if (c == '.') {
            dotpos = pos;
        }
    }
    if (slashpos == dotpos) {
        dotpos = pos;
    }
    ptrdiff_t len = dotpos - slashpos - 1;
    if (len > (sizeof(cassette_display_name) - 1)) {
        len = sizeof(cassette_display_name) - 1;
    }
    if (len > 0) {
        for(int i=0;i<len;i++) {
            (*display_name)[i] = toupper(slashpos[i+1]);
        }
        (*display_name)[len] = 0;
    } else {
        (*display_name)[0] = 0;
    }

}

static void cassette_control_handler(joshem_modal_context *pContext);

void joshem_cassette_control(joshem_cassette_control_args *pArgs) {
    joshem_cassette_control_args dmy;

    if (!pArgs) {
        memset(&dmy, 0, sizeof(dmy));
        strcpy(dmy.cassette_filename, "Hello");
        pArgs = &dmy;
    }


    joshem_do_modal(cassette_control_handler, (void *) pArgs);
}

static void show_help(char *disp_name, struct mem_block *meta) {
    int mx, my;
    int insety = 4;
    int insetx = 20;
    int retVal = 0;

    GrTextOption grt;
    fill_standard_text_option(&grt);
    const GrTextOption baset = grt;

    GrClearScreen(GrBlack());

    GrFilledBox(insetx, insety, GrMaxX() - insetx, GrMaxY() - insety, GrBlack());
    GrBox(insetx, insety, GrMaxX() - insetx, GrMaxY() - insety, COLOR_BORDER);
    GrBox(insetx + 4, insety + 4, GrMaxX() - insetx - 4, GrMaxY() - insety - 4, COLOR_BORDER);

    mx = GrMaxX() / 2;
    my = GrMaxY() / 2;
    grt.txo_xalign = GR_ALIGN_LEFT;
    grt.txo_yalign = GR_ALIGN_TOP;

    GrDrawString(disp_name, (int) strlen(disp_name), insetx + 10, insety+10, &grt);
    boundary description = find_meta_data(meta, "description");
    if (description.end > description.start) {
        GrDrawString("|", 1, insetx + 18 + 8 * strlen(disp_name), insety+10, &grt);

        GrDrawString(meta->data + description.start,
                     (int)(description.end - description.start),
                     insetx + 34 + 8 * (int)strlen(disp_name), insety+10, &grt);
    }

    boundary help = find_meta_data(meta, "help");
    if (help.end > help.start) {
        size_t offset = help.start;
        for(int line=0; line<17 && offset < help.end; line++) {
            size_t nx = find_next_line_offset(meta->data, help.end, offset);
            size_t s = offset; //Don't left-trim so we can preserve indents
            size_t e = rtrim_offset(meta->data, s, nx);
            if (e > s) {
                grt.txo_font = &GrFont_PC8x8;
                grt.txo_fgcolor.v = COLOR_SECONDARY;
                GrDrawString(meta->data + s, (int)(e-s), insetx + 12, insety+10 + 20 + line * 9, &grt);
            }
            offset = nx;
        }
    }




    GrKeyType key;
    for (;;) {
        key = GrKeyRead();
        if (key == GrKey_Return || key == GrKey_Escape) {
            break;
        }
    }
}

static void select_tape_for_reading(joshem_cassette_control_args *pArgs) {
    cassette_entry_fn e;
    if (choose_cassette(e)) {
        strcpy(pArgs->cassette_filename, e);
        pArgs->cassette_position = 0;
        pArgs->cassette_format = 1;
        pArgs->cassette_writable = 0;
        pArgs->initial_selection = 0;
        boundary  b;
        struct mem_block * p = load_cassette_meta(0, e);
        if (p) {
            b = find_meta_data(p, "help");
            if (b.end > b.start) {
                cassette_display_name x;
                create_display_name(e, &x);
                show_help(x, p);
            }
            free_mem_block(p);
        }
    }
    GrClearScreen(GrBlack());

}


//TODO - Maybe do not need this!
struct draw_cassette_result {
    int choose_new_tape;
};

static struct draw_cassette_result draw_cassette(joshem_cassette_control_args *pArgs) {
    cassette_display_name disp_name;
    create_display_name(pArgs->cassette_filename, &disp_name);

    struct draw_cassette_result res = {0};
    struct mem_block *meta = load_cassette_meta(0, pArgs->cassette_filename);

    struct track_list tracks;
    loadTrackList(0, pArgs->cassette_filename, meta, &tracks);

    int current_track = 0;
    while(current_track < tracks.track_count && tracks.tracks[current_track].offset < pArgs->cassette_position) {
        current_track++;
    }

    //show_help(disp_name, meta);

    //This is legal even if meta is null
    const boundary description = find_meta_data(meta, "description");
    const boundary help = find_meta_data(meta, "help");
    GrTextOption grt;
    fill_standard_text_option(&grt);
    const GrTextOption base_grt = grt;
    const int midx = GrMaxX() / 2;
    const int midy = GrMaxY() / 2 - 20;
    const int ry = GrMaxY() / 2 - 40;
    const int rx = GrMaxX() / 2 - 120;
    const int bx1 = rx * 3 / 4;
    const int bx2 = bx1 - rx / 10;
    const int by1 = ry;
    const int by2 = by1 - ry * 5 / 10;

    for(;;) {
        GrClearScreen(GrBlack());
        grt = base_grt;
        //GrFilledBox(midx - rx, midy - ry, midx + rx, midy + ry, COLOR_BORDER);
        GrBox(midx - rx, midy - ry, midx + rx, midy + ry, COLOR_BORDER);
        GrEllipse(midx - rx / 2, midy - ry / 5, rx / 7, ry / 4, COLOR_BORDER);
        GrEllipse(midx + rx / 2, midy - ry / 5, rx / 7, ry / 4, COLOR_BORDER);


        int poly[][2] = {
                {midx - bx1, midy + by1},
                {midx - bx2, midy + by2},
                {midx + bx2, midy + by2},
                {midx + bx1, midy + by1}
        };
        GrPolygon(4, poly, COLOR_BORDER);
        //GrLine(poly[0][0], poly[0][1], poly[1][0], poly[1][1], GrWhite());

        if (tracks.track_count > 0) {
            grt.txo_font = &GrFont_PC8x8;
            grt.txo_fgcolor.v = COLOR_SECONDARY_BRIGHT;
            grt.txo_xalign = GR_ALIGN_LEFT;
            grt.txo_yalign = GR_ALIGN_BOTTOM;
            char* track_name = tracks.tracks[current_track].track_name;
            GrDrawString(track_name, (int) strlen(track_name), midx - bx2, midy + by1 - 5, &grt);
            char track_posstr[20];
            sprintf(track_posstr, "%d", pArgs->cassette_position);
            grt.txo_xalign = GR_ALIGN_RIGHT;
            grt.txo_fgcolor.v = COLOR_SECONDARY;
            GrDrawString(track_posstr, (int) strlen(track_posstr), midx + bx2, midy + by1 - 5, &grt);
            grt = base_grt;
        }

        grt.txo_yalign = GR_ALIGN_CENTER;
        grt.txo_xalign = GR_ALIGN_CENTER;
        grt.txo_fgcolor.v = COLOR_SECONDARY;
        GrDrawString(disp_name, (int) strlen(disp_name), midx , midy - ry / 5, &grt);
        grt = base_grt;
        if (description.end > description.start) {
            grt.txo_yalign = GR_ALIGN_TOP;
            grt.txo_font = &GrFont_PC8x8;
            grt.txo_fgcolor.v = COLOR_SECONDARY_BRIGHT;
            grt.txo_xalign = GR_ALIGN_CENTER;
            GrDrawString(meta->data + description.start, (int)(description.end - description.start),
                         midx, midy - ry + 11, &grt);
            grt = base_grt;
        }

        struct optionDisplay move_options = {
                base_grt,4,midx - rx, midx + rx, midy + ry + 20,
                {
                {'S', "(S)tart",   tracks.track_count > 0 && current_track > 0},
                {'B', "(B)ack",    tracks.track_count > 0 &&  current_track > 0},
                {'F', "(F)orward", tracks.track_count > 0 && current_track < (tracks.track_count - 1)},
                {'E', "(E)nd",     tracks.track_count > 0 &&  current_track < (tracks.track_count - 1)},
        }};
        drawOptions(&move_options, -1);

        struct optionDisplay other_options = {
                base_grt, 2, midx - rx, midx + rx, midy + ry + 40,
            {
                    {'C', "(C)hange Tape",     TRUE},
                    {'A', "(A)bout This Tape", help.end > help.start},
                    }};

        drawOptions(&other_options,-1);

        char opt;
        for(;;) {
            GrKeyType  key = GrKeyRead();
            if (key >= 'a' && key <= 'z')
                key = key - 'a' + 'A';
            if (key == GrKey_Return) {
                opt = 'X';
                break;
            }
            switch (key) {
                case GrKey_Up: key = 'S'; break;
                case GrKey_Down: key = 'E'; break;
                case GrKey_Left: key = 'B'; break;
                case GrKey_Right: key = 'F'; break;
                default: break;
            }
            if (key != GrKey_NoKey && key < 127) {
                opt = (char)key;
                break;
            }
        }
        matchOption(&move_options, opt);
        matchOption(&other_options, opt);

        if (opt == 'X') {
            break;
        }
        if (opt == 'C') {
            res.choose_new_tape = 1;
            break;
        }
        if (opt == 'A') {
            show_help(disp_name, meta);
            GrClearScreen(GrBlack());
            continue;
        }

        int new_track = current_track;
        switch (opt) {
            case 'S': new_track = 0; break;
            case 'B': new_track = current_track > 0 ? current_track - 1 : current_track; break;
            case 'F': new_track = min(current_track+1, tracks.track_count - 1); break;
            case 'E': new_track = tracks.track_count - 1; break;
            default: break;
        }
        if (new_track != current_track && tracks.track_count > 0) {
            current_track = new_track;
            pArgs->cassette_position = tracks.tracks[current_track].offset;
            joshlog("Repositions %s to %d\n", pArgs->cassette_filename, pArgs->cassette_position);
        }
    }

    if (meta) {
        free_mem_block(meta);
        meta = 0;
    }
    return res;
}

static void cassette_control_handler(joshem_modal_context *pContext) {
    joshem_cassette_control_args *pArgs = pContext->input;
    cassette_entry_fn e;
    if (!pArgs) {
        return;
    }
    if (pArgs->view_current_status) {
        if (pArgs->initial_selection) {
            select_tape_for_reading(pArgs);
            if (pArgs->initial_selection) {
                return;
            }
        }
        struct draw_cassette_result res;
        for(;;) {
            res = draw_cassette(pArgs);
            if (!res.choose_new_tape) {
                break;
            }
            select_tape_for_reading(pArgs);
        }
        return;
    }
    if (pArgs->write_requested) {
        //A convenient buffer
        sprintf(pArgs->cassette_filename, "%s/%s", cassette_base_directory, CASSETTE_USER_DIRECTORY);
        mkdir(pArgs->cassette_filename, S_IWUSR);  //S_IWUSR ==> not read only

        if (ask_question("Save to which cassette?", e, 8)) {
            sprintf(pArgs->cassette_filename, "%s/%s/%s.CAS", cassette_base_directory, CASSETTE_USER_DIRECTORY, e);
        } else {
            sprintf(pArgs->cassette_filename, "%s/%s/_NOCAS.DMP", cassette_base_directory, CASSETTE_USER_DIRECTORY);
        }
        pArgs->cassette_format = 1;
        pArgs->cassette_position = (int) get_file_length(pArgs->cassette_filename);
        pArgs->cassette_writable = 1;

        return;
    }

    select_tape_for_reading(pArgs);


    //draw_cassette(pArgs);


    //GrKeyRead();
}

