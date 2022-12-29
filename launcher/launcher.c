
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>

#include <dir.h>
#include <dos.h>
#include <process.h>
/* #include <graphics.h> */

#define MAX_DESCRIPTION_LEN 256

#define MAX_CONFIG_LEN 128

struct emu {
    char dir[13];
    struct emu *next;
};

struct emu *emu_list;

char emus_base[MAXPATH+14];

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

int launch(int cur_emu) {
    char config[MAX_CONFIG_LEN+1];
    size_t config_len, pos;
    char dir[MAXPATH+14];
    struct emu *p;
    FILE * config_file;
    int exec_res;

    p = get_emu(cur_emu);
    strcpy(dir, emus_base);
    strcat(dir, p->dir);
    if (chdir(dir)) {
        gotoxy(1,23);
        cprintf("DIR ERROR");
        return 0;
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
    exec_res = spawnl(P_WAIT, "..\\..\\KEYTRAP.COM", "..\\..\\KEYTRAP.COM", "..\\..\\DOSXTRS", config, 0);
    if (exec_res < 0) {
        gotoxy(1,23);
        cprintf("SPAWN FAIL [%s]", config);
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
    int c,is_special,next_emu, cur_emu;
    is_special = 0;

    next_emu = 0;
    cur_emu = -1;

    for(;;) {
        if (cur_emu != next_emu) {
            cur_emu = next_emu;
            show_options(cur_emu);
        }
        c = getch();
        if (c == 0 && !is_special) { /*TODO - Some refereces imply I should check for 0xE0 too*/
            is_special = 1;
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
            continue;
        }
        if (c == 0xd) {
            if (launch(cur_emu)) {
                clrscr();
                show_options(cur_emu);
            }
            continue;
        }
    }
}


int init() {
    int l;
    struct ffblk dirblk; 
    int find_res;
    int emu_counter;
    struct emu *first, *last, *p;

    if (!getcwd(emus_base, MAXPATH)) {
        printf("getcwd error %u\n", errno);
        return 0;
    }

    l = strlen(emus_base);
    if (!l || emus_base[l-1] != '\\') {
        emus_base[l] = '\\';
        emus_base[l+1] = 0;
    }
    strcat(emus_base, "EMUS\\*");

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

    emu_list = first;
    l=strlen(emus_base);
    emus_base[l-1] = 0;
    return 1;

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
    return;
}

