
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>

#include <dir.h>
#include <dos.h>
#include <process.h>
#include <string.h>
/* #include <graphics.h> */

#define MAX_DESCRIPTION_LEN 256

#define MAX_CONFIG_LEN 128

int show_wip = 0;

struct emu {
    char dir[13];
    struct emu *next;
};

struct emu *emu_list;

char emus_base[MAXPATH+14];

char start_wd[MAXPATH+14];

int is_submenu;

int emu_count() {
    struct emu *p;
    int res;
    for(res=0,p=emu_list;p;p=p->next) {
        res += 1;
    }
    return res;
}

struct emu *get_emu(int index) {
    struct emu *p;
    if (index <= 0) {
        return emu_list;
    }
    for(p=emu_list;;p=p->next,index--) {
        if (index <= 0 || !(p->next)) {
            return p;
        }
    }
}

int _emu_compare(const void * v1, const void * v2) {
    const struct emu *a1,*a2;
    a1 = (const struct emu *)v1;
    a2 = (const struct emu *)v2;
    return strcmpi(a1->dir, a2->dir);
}

void reset_emu_base() {
    int l;
    strcpy(emus_base, start_wd);

    l = strlen(emus_base);
    if (!l || emus_base[l-1] != '\\') {
        emus_base[l] = '\\';
        emus_base[l+1] = 0;
    }
    strcat(emus_base, "EMUS\\");
    is_submenu = 0;
}

void sort_emus() {
    int cnt,i;
    struct emu *p,*table,*tmp;

    cnt = emu_count();
    table = (struct emu *)calloc(cnt, sizeof(struct emu));
    if (!table) {
        return;
    }
    for(i=0,p=emu_list;p;p = p->next,i++) {
        table[i] = *p;
    }
    qsort(table, cnt, sizeof(struct emu), _emu_compare);
    
    for(i=0,p=emu_list;p;p = p->next,i++) {
        tmp = p->next;
        *p = table[i];
        p->next = tmp;
    }
    
    free(table);
}

void free_emus() {
    struct emu *p;
    while (emu_list) {
        p = emu_list->next;
        free(emu_list);
        emu_list = p;
    }
}

/**
 * reads the emulator directories in emu_base and populates the emu_list.  As a side effect,
 * ensures that emu_base ends in a '/'
 * @return
 */
int read_emus() {
    int l;
    int find_res;
    int emu_counter;
    struct emu *first, *last, *p;
    struct ffblk dirblk;


    l = strlen(emus_base);
    if (!l || emus_base[l-1] != '\\') {
        emus_base[l] = '\\';
        emus_base[l+1] = 0;
    }
    strcat(emus_base, "*");
    first = last = 0;
    emu_counter = 0;
    for(find_res = findfirst(emus_base, &dirblk, FA_DIREC); find_res == 0 && emu_counter < 20; find_res = findnext(&dirblk)) {
        p = calloc(1, sizeof(struct emu));
        if (!p) {
            printf("calloc error %u\n", errno);
            return 0;
        }
        if (dirblk.ff_name[0] == '.') {
            continue;
        }
        if (dirblk.ff_name[0] == '_') {
            continue;
        }
        if (dirblk.ff_name[0] == 'W'
           && dirblk.ff_name[1] == 'I'
           && dirblk.ff_name[2] == 'P'
           && dirblk.ff_name[3] <= ' '
           && !show_wip
        ) {
            continue;
        }
        memcpy(p->dir, dirblk.ff_name, 13);
        p->dir[12] = 0;
        p->next = 0;
        if (!first) {
            first = p;
        }
        if (last) {
            last->next = p;
        }
        last = p;
        emu_counter ++;
    }

    free_emus();

    emu_list = first;
    l=strlen(emus_base);
    emus_base[l-1] = 0;

    sort_emus();
    return 1;
}

/**
 * Returns 0 if fail, 1 if successful launch, 2 if submenu.
 * If 2 is returned, the menus will have been reloaded
 */
int launch(int cur_emu) {
    char config[MAX_CONFIG_LEN+1];
    size_t config_len, pos;
    char dir[MAXPATH+14];
    struct emu *p;
    FILE * config_file;
    int exec_res;
    int subchk;
    char dostrspath[MAXPATH*14];
    char exepath[MAXPATH+14];
    char rpath[MAXPATH+15];

    p = get_emu(cur_emu);
    strcpy(dir, emus_base);
    strcat(dir, p->dir);
    if (chdir(dir)) {
        gotoxy(1,23);
        cprintf("DIR ERROR");
        return 0;
    }

    subchk = access("ISSUB.MRK", 0);
    if (subchk == 0) {
        /* file exists */
        strcpy(emus_base, dir);
        read_emus();
        is_submenu = 1;
        return 2;
    }


    config_file = fopen("CONFIG.XTR","rt");
    if (!config_file) {
        gotoxy(1,23);
        cprintf("CONFIG ERROR");
        return 0;
    }
    config_len = fread(config, 1, MAX_CONFIG_LEN, config_file);
    fclose(config_file);
    if (config_len > MAX_CONFIG_LEN) {
        config_len = MAX_CONFIG_LEN;
    }
    config[config_len] = 0;
    if (!config_len) {
        gotoxy(1,23);
        cprintf("CONFIG READ ERROR");
        return 0;
    }
    for(pos=0;pos<config_len;pos++) {
        if (config[pos] < ' ') {
            config[pos] = ' ';
        }
    }
    for(pos=config_len - 1; pos > 0 && config[pos] == ' '; pos--) {
        config[pos] = 0;
    }

    strcpy(exepath, start_wd);
    if (!strlen(exepath) || (exepath[strlen(exepath)-1]!= '\\')) {
        strcat(exepath, "\\");
    }
    strcpy(dostrspath, exepath);
    strcat(exepath,"KEYTRAP.COM");
    strcat(dostrspath, "DOSXTRS");

    /**
     * Even though ROMPATH shouldn't change, we set it before each spawn.  For some
     * reason, functions like cprintf seem to be blowing away some of the environment
     * changes we make.  This is a bit unsettling so need to watch for crashes, etc.
     */
    sprintf(rpath, "ROMPATH=%s", strlen(start_wd) ? start_wd : "\\");
    putenv(rpath);


    exec_res = spawnl(P_WAIT, exepath, "KEYTRAP.COM", dostrspath, config, 0);
    if (exec_res < 0) {
        gotoxy(1,23);
        cprintf("SPAWN FAIL [%s] [%s] [%s]", exepath, dostrspath, config);
        return 0;
    }


    return 1;
}

void show_options(int cur_emu) {
    static const char * title = "CHOOSE EMULATOR";




    struct emu *p;
    int opt_count;
    opt_count = 0;

    textbackground(0);
    textcolor(2);
    gotoxy((80 - strlen(title)) / 2, 2);
    cprintf("%s",title);



    for(p = emu_list; p ; p = p->next) {
        gotoxy(10+15 * (opt_count %4), 4+opt_count / 4);
        if (opt_count == cur_emu) {
            textbackground(7);
            textcolor(0);
        } else {
            textbackground(0);
            textcolor(15);
        }
        cprintf("%s", p->dir);
        opt_count+= 1;
    }
    if (is_submenu) {
        textbackground(0);
        textcolor(7);
        gotoxy(41,22);
        cprintf("<Press ESC to return to main menu>");
    }

    textbackground(0);
    textcolor(15);
    gotoxy(1,10);

}


int move(int cur_emu, int dx, int dy) {
    int x,y,r;

    x = cur_emu % 4;
    y = cur_emu / 4;

    x = x + dx;
    y = y + dy;

    if (x < 0) {
        x = 0;
    }
    if (x > 3) {
        x = 3;
    }
    if (y < 0) {
        y = 0;
    }
    r = y * 4 + x;
    if (r >= emu_count() ) {
        r = cur_emu;
    }
    return r;
}

void mainloop() {
    static const char escape_hatch[] = "exit";
    static const char wip_code[] = "wip";

    int c,is_special,next_emu, cur_emu, escape_counter, launch_res, wip_counter;
    is_special = 0;
    escape_counter = 0;
    wip_counter = 0;

    next_emu = 0;
    cur_emu = -1;

    for(;escape_hatch[escape_counter];) {
        if (cur_emu != next_emu) {
            cur_emu = next_emu;
            show_options(cur_emu);
        }
        c = getch();
        if (c == 0 && !is_special) { /*TODO - Some refereces imply I should check for 0xE0 too*/
            is_special = 1;
            escape_counter = 0;
            continue;
        }
        if (is_special) {
            switch (c) {
                case 0x48: next_emu = move(cur_emu, 0,-1); break;
                case 0x50: next_emu = move(cur_emu, 0, 1); break;
                case 0x4B: next_emu = move(cur_emu, -1, 0); break;
                case 0x4D: next_emu = move(cur_emu, 1, 0); break;
            }
            is_special = 0;
            escape_counter = 0;
            continue;
        }
        if (c == 27) { /*ESC, I think*/
            next_emu = 0;
            cur_emu = -1;
            escape_counter = 0;
            reset_emu_base();
            read_emus();
            clrscr();
            continue;
        }
        if (c == 0xd || c == 'z') {
            launch_res = launch(cur_emu);
            if (launch_res == 2) {
                /* sub menu */
                clrscr();
                show_options(0);
                next_emu = 0;
                cur_emu = -1;
            } else if (launch_res == 1) {
                /*
                 * If "z" was pressed then we don't clear the screen so we can see any console output
                 * that may have happened
                 */
                if (c == 0xd) {
                    clrscr();
                    show_options(cur_emu);
                } else {
                    printf("\nPress ESC\n");
                }
            }
            escape_counter = 0;
            continue;
        }
        if (c == escape_hatch[escape_counter]) {
            escape_counter++;
        } else {
            escape_counter = 0;
        }
        if (c == wip_code[wip_counter]) {
            wip_counter++;
            if (!wip_code[wip_counter]) {
                show_wip = 1;
                wip_counter = 0;
                read_emus();
                clrscr();
                show_options(0);
                next_emu = 0;
                cur_emu = -1;
                continue;
            }
        } else {
            wip_counter = 0;
        }
    }
}


int init() {
    int l;
    int find_res;
    int emu_counter;
    struct emu *first, *last, *p;

    emu_list = 0;

    if (!getcwd(start_wd, MAXPATH)) {
        printf("getcwd error %u\n", errno);
        return 0;
    }
    reset_emu_base();
    return read_emus();


}



void main(int argc, char **argv, char **env) {

    struct emu *p;
    int x;

    if (!init()) {
        exit(1);
        return;
    }


    textmode(C80);
    clrscr();
    mainloop();
    chdir(start_wd);
    return;
}

