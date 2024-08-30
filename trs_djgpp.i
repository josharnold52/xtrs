# 0 "/home/arnold/scm/xtrs/trs_djgpp.c"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/home/arnold/scm/xtrs/trs_djgpp.c"
# 34 "/home/arnold/scm/xtrs/trs_djgpp.c"
# 1 "/usr/i686-pc-msdosdjgpp/sys-include/stdio.h" 1 3 4
# 19 "/usr/i686-pc-msdosdjgpp/sys-include/stdio.h" 3 4
# 1 "/usr/i686-pc-msdosdjgpp/sys-include/sys/version.h" 1 3 4
# 20 "/usr/i686-pc-msdosdjgpp/sys-include/stdio.h" 2 3 4
# 1 "/usr/i686-pc-msdosdjgpp/sys-include/sys/djtypes.h" 1 3 4
# 43 "/usr/i686-pc-msdosdjgpp/sys-include/sys/djtypes.h" 3 4

# 43 "/usr/i686-pc-msdosdjgpp/sys-include/sys/djtypes.h" 3 4
typedef short __attribute__((__may_alias__)) __dj_short_a;
typedef int __attribute__((__may_alias__)) __dj_int_a;
typedef long __attribute__((__may_alias__)) __dj_long_a;
typedef long long __attribute__((__may_alias__)) __dj_long_long_a;
typedef unsigned short __attribute__((__may_alias__)) __dj_unsigned_short_a;
typedef unsigned int __attribute__((__may_alias__)) __dj_unsigned_int_a;
typedef unsigned long __attribute__((__may_alias__)) __dj_unsigned_long_a;
typedef unsigned long long __attribute__((__may_alias__)) __dj_unsigned_long_long_a;
typedef float __attribute__((__may_alias__)) __dj_float_a;
typedef double __attribute__((__may_alias__)) __dj_double_a;
typedef long double __attribute__((__may_alias__)) __dj_long_double_a;
# 21 "/usr/i686-pc-msdosdjgpp/sys-include/stdio.h" 2 3 4
# 48 "/usr/i686-pc-msdosdjgpp/sys-include/stdio.h" 3 4
typedef __builtin_va_list va_list;




typedef long unsigned int size_t;




typedef long signed int ssize_t;







typedef struct {
  ssize_t _cnt;
  char *_ptr;
  char *_base;
  size_t _bufsiz;
  int _flag;
  int _file;
  char *_name_to_remove;
  size_t _fillsize;
} FILE;

typedef unsigned long fpos_t;

extern FILE __dj_stdin, __dj_stdout, __dj_stderr;




void clearerr(FILE *_stream);
int fclose(FILE *_stream);
int feof(FILE *_stream);
int ferror(FILE *_stream);
int fflush(FILE *_stream);
int fgetc(FILE *_stream);
int fgetpos(FILE *_stream, fpos_t *_pos);
char * fgets(char *_s, int _n, FILE *_stream);
FILE * fopen(const char *_filename, const char *_mode);
int fprintf(FILE *_stream, const char *_format, ...);
int fputc(int _c, FILE *_stream);
int fputs(const char *_s, FILE *_stream);
size_t fread(void *_ptr, size_t _size, size_t _nelem, FILE *_stream);
FILE * freopen(const char *_filename, const char *_mode, FILE *_stream);
int fscanf(FILE *_stream, const char *_format, ...);
int fseek(FILE *_stream, long _offset, int _mode);
int fsetpos(FILE *_stream, const fpos_t *_pos);
long ftell(FILE *_stream);
size_t fwrite(const void *_ptr, size_t _size, size_t _nelem, FILE *_stream);
int getc(FILE *_stream);
int getchar(void);
char * gets(char *_s);
void perror(const char *_s);
int printf(const char *_format, ...);
int putc(int _c, FILE *_stream);
int putchar(int _c);
int puts(const char *_s);
int remove(const char *_filename);
int rename(const char *_old, const char *_new);
void rewind(FILE *_stream);
int scanf(const char *_format, ...);
void setbuf(FILE *_stream, char *_buf);
int setvbuf(FILE *_stream, char *_buf, int _mode, size_t _size);
int sprintf(char *_s, const char *_format, ...);
int sscanf(const char *_s, const char *_format, ...);
FILE * tmpfile(void);
char * tmpnam(char *_s);
int ungetc(int _c, FILE *_stream);
int vfprintf(FILE *_stream, const char *_format, va_list _ap);
int vprintf(const char *_format, va_list _ap);
int vsprintf(char *_s, const char *_format, va_list _ap);




int snprintf(char *str, size_t n, const char *fmt, ...);
int vfscanf(FILE *_stream, const char *_format, va_list _ap);
int vscanf(const char *_format, va_list _ap);
int vsnprintf(char *str, size_t n, const char *fmt, va_list ap);
int vsscanf(const char *_s, const char *_format, va_list _ap);
# 143 "/usr/i686-pc-msdosdjgpp/sys-include/stdio.h" 3 4
int dprintf(int _fd, const char *_format, ...) __attribute__ ((__format__ (__printf__, 2, 3)));
int fileno(FILE *_stream);
FILE * fdopen(int _fildes, const char *_type);
int mkstemp(char *_template);
int pclose(FILE *_pf);
FILE * popen(const char *_command, const char *_mode);
char * tempnam(const char *_dir, const char *_prefix);
int vdprintf(int _fd, const char *_format, va_list _ap) __attribute__ ((__format__ (__printf__, 2, 0)));



extern FILE __dj_stdprn, __dj_stdaux;





void _djstat_describe_lossage(FILE *_to_where);
int _doprnt(const char *_fmt, va_list _args, FILE *_f);
int _doscan(FILE *_f, const char *_fmt, va_list _args);
int _doscan_low(FILE *, int (*)(FILE *_get), int (*_unget)(int, FILE *), const char *_fmt, va_list _args);
int fpurge(FILE *_f);
int getw(FILE *_f);
char * mktemp(char *_template);
int putw(int _v, FILE *_f);
void setbuffer(FILE *_f, void *_buf, int _size);
void setlinebuf(FILE *_f);
int _rename(const char *_old, const char *_new);
int asprintf(char **_sp, const char *_format, ...) __attribute__((format (__printf__, 2, 3)));
char * asnprintf(char *_s, size_t *_np, const char *_format, ...) __attribute__((format (__printf__, 3, 4)));
int vasprintf(char **_sp, const char *_format, va_list _ap) __attribute__((format (__printf__, 2, 0)));
char * vasnprintf(char *_s, size_t *_np, const char *_format, va_list _ap) __attribute__((format (__printf__, 3, 0)));


typedef int off_t;



__extension__ typedef long long off64_t;


int fseeko(FILE *_stream, off_t _offset, int _mode);
off_t ftello(FILE *_stream);
int fseeko64(FILE *_stream, off64_t _offset, int _mode);
off64_t ftello64(FILE *_stream);
# 35 "/home/arnold/scm/xtrs/trs_djgpp.c" 2
# 1 "/usr/i686-pc-msdosdjgpp/sys-include/fcntl.h" 1 3 4
# 57 "/usr/i686-pc-msdosdjgpp/sys-include/fcntl.h" 3 4
# 1 "/usr/i686-pc-msdosdjgpp/sys-include/sys/types.h" 1 3 4
# 27 "/usr/i686-pc-msdosdjgpp/sys-include/sys/types.h" 3 4
typedef int blkcnt_t;
typedef int blksize_t;
typedef int dev_t;
typedef unsigned long fsblkcnt_t;
typedef unsigned long fsfilcnt_t;
typedef int ino_t;
typedef int mode_t;
typedef int nlink_t;


typedef int gid_t;
# 49 "/usr/i686-pc-msdosdjgpp/sys-include/sys/types.h" 3 4
__extension__ typedef long long offset_t;



typedef int pid_t;
# 66 "/usr/i686-pc-msdosdjgpp/sys-include/sys/types.h" 3 4
typedef int uid_t;
# 76 "/usr/i686-pc-msdosdjgpp/sys-include/sys/types.h" 3 4
typedef struct fd_set {
  unsigned char fd_bits [((256) + 7) / 8];
} fd_set;







typedef unsigned int time_t;
# 58 "/usr/i686-pc-msdosdjgpp/sys-include/fcntl.h" 2 3 4

struct flock {
  off_t l_len;
  pid_t l_pid;
  off_t l_start;
  short l_type;
  short l_whence;
};

struct flock64 {
  offset_t l_len;
  pid_t l_pid;
  offset_t l_start;
  short l_type;
  short l_whence;
};

extern int _fmode;

int open(const char *_path, int _oflag, ...);
int creat(const char *_path, mode_t _mode);
int fcntl(int _fildes, int _cmd, ...);
# 102 "/usr/i686-pc-msdosdjgpp/sys-include/fcntl.h" 3 4
extern int __djgpp_share_flags;
# 135 "/usr/i686-pc-msdosdjgpp/sys-include/fcntl.h" 3 4
unsigned _get_volume_info (const char *_path, int *_max_file_len, int *_max_path_len, char *_filesystype);
char _use_lfn (const char *_path);
char *_lfn_gen_short_fname (const char *_long_fname, char *_short_fname);
int _is_DOS83 (const char *_fname);




unsigned _lfn_get_ftime (int _handle, int _which);

char _preserve_fncase (void);
# 36 "/home/arnold/scm/xtrs/trs_djgpp.c" 2

# 1 "/usr/i686-pc-msdosdjgpp/sys-include/sys/stat.h" 1 3 4
# 51 "/usr/i686-pc-msdosdjgpp/sys-include/sys/stat.h" 3 4
struct stat {
  time_t st_atime;
  time_t st_ctime;
  dev_t st_dev;
  gid_t st_gid;
  ino_t st_ino;
  mode_t st_mode;
  time_t st_mtime;
  nlink_t st_nlink;
  off_t st_size;
  blksize_t st_blksize;
  uid_t st_uid;
  dev_t st_rdev;
};

int chmod(const char *_path, mode_t _mode);
int fchmod(int _fildes, mode_t _mode);
int fstat(int _fildes, struct stat *_buf);
int mkdir(const char *_path, mode_t _mode);
int mkfifo(const char *_path, mode_t _mode);
int stat(const char *_path, struct stat *_buf);
mode_t umask(mode_t _cmask);
# 93 "/usr/i686-pc-msdosdjgpp/sys-include/sys/stat.h" 3 4
void _fixpath(const char *, char *);
char * __canonicalize_path(const char *, char *, size_t);
unsigned short _get_magic(const char *, int);
int _is_executable(const char *, int, const char *);
int lstat(const char * _path, struct stat * _buf);
int mknod(const char *_path, mode_t _mode, dev_t _dev);
char * _truename(const char *, char *);
char * _truename_sfn(const char *, char *);
# 113 "/usr/i686-pc-msdosdjgpp/sys-include/sys/stat.h" 3 4
extern unsigned short _djstat_flags;
# 131 "/usr/i686-pc-msdosdjgpp/sys-include/sys/stat.h" 3 4
extern unsigned short _djstat_fail_bits;
# 38 "/home/arnold/scm/xtrs/trs_djgpp.c" 2

# 1 "/usr/i686-pc-msdosdjgpp/sys-include/sys/time.h" 1 3 4
# 23 "/usr/i686-pc-msdosdjgpp/sys-include/sys/time.h" 3 4
# 1 "/usr/i686-pc-msdosdjgpp/sys-include/time.h" 1 3 4
# 36 "/usr/i686-pc-msdosdjgpp/sys-include/time.h" 3 4
typedef int clock_t;
# 48 "/usr/i686-pc-msdosdjgpp/sys-include/time.h" 3 4
struct tm {
  int tm_sec;
  int tm_min;
  int tm_hour;
  int tm_mday;
  int tm_mon;
  int tm_year;
  int tm_wday;
  int tm_yday;
  int tm_isdst;
  char *__tm_zone;
  int __tm_gmtoff;
};

char * asctime(const struct tm *_tptr);
clock_t clock(void);
char * ctime(const time_t *_cal);
double difftime(time_t _t1, time_t _t0);
struct tm * gmtime(const time_t *_tod);
struct tm * localtime(const time_t *_tod);
time_t mktime(struct tm *_tptr);
size_t strftime(char * _s, size_t _n, const char * _format, const struct tm * _tptr);
time_t time(time_t *_tod);
# 81 "/usr/i686-pc-msdosdjgpp/sys-include/time.h" 3 4
extern char *tzname[2];

char * asctime_r(const struct tm * __restrict__ _tptr, char * __restrict__ _buf);
char * ctime_r(const time_t *_cal, char *_buf);
struct tm * gmtime_r(const time_t * __restrict__ _tod, struct tm * __restrict__ _tptr);
struct tm * localtime_r(const time_t * __restrict__ _tod, struct tm * __restrict__ _tptr);
void tzset(void);






struct timeval {
  time_t tv_sec;
  long tv_usec;
};

struct timezone {
  int tz_minuteswest;
  int tz_dsttime;
};



typedef long long uclock_t;


int gettimeofday(struct timeval *_tp, struct timezone *_tzp);
unsigned long rawclock(void);
int select(int _nfds, fd_set *_readfds, fd_set *_writefds, fd_set *_exceptfds, struct timeval *_timeout);
int settimeofday(struct timeval *_tp, ...);
void tzsetwall(void);
uclock_t uclock(void);







# 1 "/usr/i686-pc-msdosdjgpp/sys-include/sys/cdefs.h" 1 3 4
# 123 "/usr/i686-pc-msdosdjgpp/sys-include/time.h" 2 3 4

unsigned long long _rdtsc(void);

extern __inline__ __attribute__ ((__gnu_inline__)) unsigned long long
_rdtsc(void)
{

  return __builtin_ia32_rdtsc();





}
# 24 "/usr/i686-pc-msdosdjgpp/sys-include/sys/time.h" 2 3 4




struct itimerval {
  struct timeval it_interval;
  struct timeval it_value;
};





extern long __djgpp_clock_tick_interval;

int getitimer(int _which, struct itimerval *_value);
int setitimer(int _which, struct itimerval *_value, struct itimerval *_ovalue);
int utimes(const char *_file, struct timeval _tvp[2]);
# 40 "/home/arnold/scm/xtrs/trs_djgpp.c" 2
# 1 "/usr/i686-pc-msdosdjgpp/sys-include/sys/file.h" 1 3 4
# 28 "/usr/i686-pc-msdosdjgpp/sys-include/sys/file.h" 3 4
# 1 "/usr/i686-pc-msdosdjgpp/sys-include/unistd.h" 1 3 4
# 104 "/usr/i686-pc-msdosdjgpp/sys-include/unistd.h" 3 4
extern char *optarg;
extern int optind, opterr, optopt;

void __exit(int _status) __attribute__((noreturn));
void _exit(int _status) __attribute__((noreturn));
int access(const char *_path, int _amode);
unsigned int alarm(unsigned int _seconds);
int chdir(const char *_path);
int chown(const char *_path, uid_t _owner, gid_t _group);
int close(int _fildes);
size_t confstr(int _name, char *_buf, size_t _len);
char * ctermid(char *_s);
int dup(int _fildes);
int dup2(int _fildes, int _fildes2);
int execl(const char *_path, const char *_arg, ...);
int execle(const char *_path, const char *_arg, ...);
int execlp(const char *_file, const char *_arg, ...);
int execv(const char *_path, char *const _argv[]);
int execve(const char *_path, char *const _argv[], char *const _envp[]);
int execvp(const char *_file, char *const _argv[]);
int fchdir(int _fd);
pid_t fork(void);
long fpathconf(int _fildes, int _name);
char * getcwd(char *_buf, size_t _size);
gid_t getegid(void);
uid_t geteuid(void);
gid_t getgid(void);
int getgroups(int _gidsetsize, gid_t *_grouplist);
char * getlogin(void);
int getopt(int _argc, char *const _argv[], const char *_optstring);
pid_t getpgrp(void);
pid_t getpid(void);
pid_t getppid(void);
uid_t getuid(void);
int isatty(int _fildes);
int link(const char *_existing, const char *_new);
off_t lseek(int _fildes, off_t _offset, int _whence);
long pathconf(const char *_path, int _name);
int pause(void);
int pipe(int _fildes[2]);
ssize_t pwrite(int _fildes, const void *_buf, size_t _nbyte, off_t _offset);
ssize_t read(int _fildes, void *_buf, size_t _nbyte);
int rmdir(const char *_path);
int setgid(gid_t _gid);
int setpgid(pid_t _pid, pid_t _pgid);
pid_t setsid(void);
int setuid(uid_t uid);
unsigned int sleep(unsigned int _seconds);
long sysconf(int _name);
pid_t tcgetpgrp(int _fildes);
int tcsetpgrp(int _fildes, pid_t _pgrp_id);
char * ttyname(int _fildes);
int unlink(const char *_path);
ssize_t write(int _fildes, const void *_buf, size_t _nbyte);






char * basename(const char *_fn);
int brk(void *_heaptop);
char * dirname(const char *_fn);
int __file_exists(const char *_fn);
int fchown(int fd, uid_t owner, gid_t group);
int fsync(int _fd);
int ftruncate(int, off_t);
int getdtablesize(void);
int gethostname(char *buf, int size);
int getpagesize(void);
char * getwd(char *__buffer);
int lchown(const char * file, int owner, int group);
int lockf(int _fildes, int _cmd, off_t _len);
int llockf(int _fildes, int _cmd, offset_t _len);
offset_t llseek(int _fildes, offset_t _offset, int _whence);
int nice(int _increment);
int readlink(const char * __file, char * __buffer, size_t __size);
void * sbrk(int _delta);
int symlink (const char *, const char *);
int sync(void);
int truncate(const char*, off_t);
unsigned int usleep(unsigned int _useconds);

pid_t vfork(void);
# 29 "/usr/i686-pc-msdosdjgpp/sys-include/sys/file.h" 2 3 4
# 44 "/usr/i686-pc-msdosdjgpp/sys-include/sys/file.h" 3 4
int flock (int _fildes, int _op);
# 41 "/home/arnold/scm/xtrs/trs_djgpp.c" 2
# 1 "/usr/i686-pc-msdosdjgpp/sys-include/string.h" 1 3 4
# 33 "/usr/i686-pc-msdosdjgpp/sys-include/string.h" 3 4
void * memchr(const void *_s, int _c, size_t _n);
int memcmp(const void *_s1, const void *_s2, size_t _n);
void * memcpy(void * _dest, const void * _src, size_t _n);
void * memmove(void *_s1, const void *_s2, size_t _n);
void * memset(void *_s, int _c, size_t _n);
char * strcat(char * _s1, const char * _s2);
char * strchr(const char *_s, int _c);
int strcmp(const char *_s1, const char *_s2);
int strcoll(const char *_s1, const char *_s2);
char * strcpy(char * _s1, const char * _s2);
size_t strcspn(const char *_s1, const char *_s2);
char * strerror(int _errcode);
size_t strlen(const char *_s);
char * strncat(char * _s1, const char * _s2, size_t _n);
int strncmp(const char *_s1, const char *_s2, size_t _n);
char * strncpy(char * _s1, const char * _s2, size_t _n);
char * strpbrk(const char *_s1, const char *_s2);
char * strrchr(const char *_s, int _c);
size_t strspn(const char *_s1, const char *_s2);
char * strstr(const char *_s1, const char *_s2);
char * strtok(char * _s1, const char * _s2);
size_t strxfrm(char * _s1, const char * _s2, size_t _n);
# 63 "/usr/i686-pc-msdosdjgpp/sys-include/string.h" 3 4
int strerror_r(int _errnum, char *_strerrbuf, size_t _buflen);
char * strtok_r(char * _s1, const char * _s2, char ** _s3);



# 1 "/usr/i686-pc-msdosdjgpp/sys-include/sys/movedata.h" 1 3 4
# 36 "/usr/i686-pc-msdosdjgpp/sys-include/sys/movedata.h" 3 4
void dosmemget(unsigned long _offset, size_t _length, void *_buffer);
void dosmemput(const void *_buffer, size_t _length, unsigned long _offset);


void _dosmemgetb(unsigned long _offset, size_t _xfers, void *_buffer);
void _dosmemgetw(unsigned long _offset, size_t _xfers, void *_buffer);
void _dosmemgetl(unsigned long _offset, size_t _xfers, void *_buffer);
void _dosmemputb(const void *_buffer, size_t _xfers, unsigned long _offset);
void _dosmemputw(const void *_buffer, size_t _xfers, unsigned long _offset);
void _dosmemputl(const void *_buffer, size_t _xfers, unsigned long _offset);



void movedata(unsigned _source_selector, unsigned _source_offset,
        unsigned _dest_selector, unsigned _dest_offset,
        size_t _length);


void _movedatab(unsigned, unsigned, unsigned, unsigned, size_t);
void _movedataw(unsigned, unsigned, unsigned, unsigned, size_t);
void _movedatal(unsigned, unsigned, unsigned, unsigned, size_t);
# 69 "/usr/i686-pc-msdosdjgpp/sys-include/string.h" 2 3 4

int bcmp(const void *_ptr1, const void *_ptr2, int _length);
void bcopy(const void *_a, void *_b, size_t _len);
void bzero(void *ptr, size_t _len);
int ffs(int _mask);
char * index(const char *_string, int _c);
void * memccpy(void *_to, const void *_from, int _c, size_t _n);
int memicmp(const void *_s1, const void *_s2, size_t _n);
char * rindex(const char *_string, int _c);
char * stpcpy(char *_dest, const char *_src);
char * stpncpy(char *_dest, const char *_src, size_t _n);
char * strdup(const char *_s);
char * strndup(const char *_s, size_t _n);
size_t strlcat(char *_dest, const char *_src, size_t _size);
size_t strlcpy(char *_dest, const char *_src, size_t _size);
char * strlwr(char *_s);
int strcasecmp(const char *_s1, const char *_s2);
int stricmp(const char *_s1, const char *_s2);
int strncasecmp(const char *_s1, const char *_s2, size_t _n);
int strnicmp(const char *_s1, const char *_s2, size_t _n);
size_t strnlen(const char *_s, size_t _n);
char * strsep(char **_stringp, const char *_delim);
char * strupr(char *_s);
# 42 "/home/arnold/scm/xtrs/trs_djgpp.c" 2
# 1 "/usr/i686-pc-msdosdjgpp/sys-include/stdlib.h" 1 3 4
# 37 "/usr/i686-pc-msdosdjgpp/sys-include/stdlib.h" 3 4
extern int __dj_mb_cur_max;

typedef struct {
  int quot;
  int rem;
} div_t;

typedef struct {
  long quot;
  long rem;
} ldiv_t;







typedef unsigned short wchar_t;



void abort(void) __attribute__((noreturn));
int abs(int _i);
int atexit(void (*_func)(void));
double atof(const char *_s);
int atoi(const char *_s);
long atol(const char *_s);
void * bsearch(const void *_key, const void *_base, size_t _nelem,
  size_t _size, int (*_cmp)(const void *_ck, const void *_ce));
void * calloc(size_t _nelem, size_t _size);
div_t div(int _numer, int _denom);
void exit(int _status) __attribute__((noreturn));
void free(void *_ptr);
char * getenv(const char *_name);
long labs(long _i);
ldiv_t ldiv(long _numer, long _denom);
void * malloc(size_t _size);
int mblen(const char *_s, size_t _n);
size_t mbstowcs(wchar_t *_wcs, const char *_s, size_t _n);
int mbtowc(wchar_t *_pwc, const char *_s, size_t _n);
void qsort(void *_base, size_t _nelem, size_t _size,
       int (*_cmp)(const void *_e1, const void *_e2));
int rand(void);
void * realloc(void *_ptr, size_t _size);
void srand(unsigned _seed);
double strtod(const char *_s, char **_endptr);
long strtol(const char *_s, char **_endptr, int _base);
unsigned long strtoul(const char *_s, char **_endptr, int _base);
int system(const char *_s);
size_t wcstombs(char *_s, const wchar_t *_wcs, size_t _n);
int wctomb(char *_s, wchar_t _wchar);




typedef struct {
  long long int quot;
  long long int rem;
} lldiv_t;

void _Exit(int _status) __attribute__((noreturn));
long long int atoll(const char *_s);
long long int llabs(long long int _i);
lldiv_t lldiv(long long int _numer, long long int _denom);
float strtof(const char *_s, char **_endptr);
long double strtold(const char *_s, char **_endptr);
long long int strtoll(const char *_s, char **_endptr, int _base);
unsigned long long int strtoull(const char *_s, char **_endptr, int _base);





long a64l(const char *_string);
char * l64a(long _value);
char * mktemp(char *_template);
char * mkdtemp(char *_template);
int mkstemp(char *_template);
int putenv(char *_val);
char * realpath(const char *_path, char *_resolved);
int setenv(const char *_var, const char *_val, int _overwrite);
int unsetenv(const char *_var);



void * alloca(size_t _size);
long double _atold(const char *_s);
void cfree(void *_ptr);
double drand48(void);
char * ecvtbuf(double _val, int _nd, int *_dp, int *_sn, char *_bf);
char * ecvt(double _val, int _nd, int *_dp, int *_sn);
double erand48(unsigned short _state[3]);
char * fcvtbuf(double _val, int _nd, int *_dp, int *_sn, char *_bf);
char * fcvt(double _val, int _nd, int *_dp, int *_sn);
char * gcvt(double _val, int _nd, char *_buf);
char * getpass(const char *_prompt);
int getlongpass(const char *_prompt, char *_buffer, int _max_len);
char * itoa(int _value, char *_buffer, int _radix);
long jrand48(unsigned short _state[3]);
void lcong48(unsigned short _param[7]);
unsigned long lrand48(void);
long mrand48(void);
unsigned long nrand48(unsigned short _state[3]);
unsigned short *seed48(unsigned short _state_seed[3]);
void srand48(long _seedval);
int stackavail(void);
long double _strtold(const char *_s, char **_endptr);
void swab(const void *_from, void *_to, int _nbytes);
void * valloc (size_t _amt);






char * initstate (unsigned _seed, char *_arg_state, int _n);
char * setstate(char *_arg_state);
long random(void);
int srandom(int _seed);
# 174 "/usr/i686-pc-msdosdjgpp/sys-include/stdlib.h" 3 4
extern int __system_flags;
# 43 "/home/arnold/scm/xtrs/trs_djgpp.c" 2

# 1 "/usr/i686-pc-msdosdjgpp/sys-include/errno.h" 1 3 4
# 20 "/usr/i686-pc-msdosdjgpp/sys-include/errno.h" 3 4
extern int errno;
# 76 "/usr/i686-pc-msdosdjgpp/sys-include/errno.h" 3 4
extern char * sys_errlist[];
extern int sys_nerr;
extern const char * __sys_errlist[];
extern int __sys_nerr;
extern int _doserrno;
# 45 "/home/arnold/scm/xtrs/trs_djgpp.c" 2
# 1 "/home/arnold/scm/dostrs-build-extras/grx249/include/grx20.h" 1
# 144 "/home/arnold/scm/dostrs-build-extras/grx249/include/grx20.h"

# 144 "/home/arnold/scm/dostrs-build-extras/grx249/include/grx20.h"
typedef struct _GR_frameDriver GrFrameDriver;
typedef struct _GR_videoDriver GrVideoDriver;
typedef struct _GR_videoMode GrVideoMode;
typedef struct _GR_videoModeExt GrVideoModeExt;
typedef struct _GR_frame GrFrame;
typedef struct _GR_context GrContext;
# 161 "/home/arnold/scm/dostrs-build-extras/grx249/include/grx20.h"
typedef unsigned int GrColor;
# 171 "/home/arnold/scm/dostrs-build-extras/grx249/include/grx20.h"
typedef enum _GR_graphicsModes {
 GR_unknown_mode = (-1),

 GR_80_25_text = 0,
 GR_default_text,
 GR_width_height_text,
 GR_biggest_text,
 GR_320_200_graphics,
 GR_default_graphics,
 GR_width_height_graphics,
 GR_biggest_noninterlaced_graphics,
 GR_biggest_graphics,
 GR_width_height_color_graphics,
 GR_width_height_color_text,
 GR_custom_graphics,

 GR_NC_80_25_text,
 GR_NC_default_text,
 GR_NC_width_height_text,
 GR_NC_biggest_text,
 GR_NC_320_200_graphics,
 GR_NC_default_graphics,
 GR_NC_width_height_graphics,
 GR_NC_biggest_noninterlaced_graphics,
 GR_NC_biggest_graphics,
 GR_NC_width_height_color_graphics,
 GR_NC_width_height_color_text,
 GR_NC_custom_graphics,


 GR_width_height_bpp_graphics,
 GR_width_height_bpp_text,
 GR_custom_bpp_graphics,
 GR_NC_width_height_bpp_graphics,
 GR_NC_width_height_bpp_text,
 GR_NC_custom_bpp_graphics
} GrGraphicsMode;




typedef enum _GR_frameModes {

 GR_frameUndef,
 GR_frameText,
 GR_frameHERC1,
 GR_frameEGAVGA1,
 GR_frameEGA4,
 GR_frameSVGA4,
 GR_frameSVGA8,
 GR_frameVGA8X,
 GR_frameSVGA16,
 GR_frameSVGA24,
 GR_frameSVGA32L,
 GR_frameSVGA32H,

 GR_frameXWIN1 = GR_frameEGAVGA1,
 GR_frameXWIN4 = GR_frameSVGA4,
 GR_frameXWIN8 = GR_frameSVGA8,
 GR_frameXWIN16 = GR_frameSVGA16,
 GR_frameXWIN24 = GR_frameSVGA24,
 GR_frameXWIN32L = GR_frameSVGA32L,
 GR_frameXWIN32H = GR_frameSVGA32H,

 GR_frameWIN32_1 = GR_frameEGAVGA1,
 GR_frameWIN32_4 = GR_frameSVGA4,
 GR_frameWIN32_8 = GR_frameSVGA8,
 GR_frameWIN32_16 = GR_frameSVGA16,
 GR_frameWIN32_24 = GR_frameSVGA24,
 GR_frameWIN32_32L = GR_frameSVGA32L,
 GR_frameWIN32_32H = GR_frameSVGA32H,

 GR_frameSDL8 = GR_frameSVGA8,
 GR_frameSDL16 = GR_frameSVGA16,
 GR_frameSDL24 = GR_frameSVGA24,
 GR_frameSDL32L = GR_frameSVGA32L,
 GR_frameSDL32H = GR_frameSVGA32H,

 GR_frameSVGA8_LFB,
 GR_frameSVGA16_LFB,
 GR_frameSVGA24_LFB,
 GR_frameSVGA32L_LFB,
 GR_frameSVGA32H_LFB,

 GR_frameRAM1,
 GR_frameRAM4,
 GR_frameRAM8,
 GR_frameRAM16,
 GR_frameRAM24,
 GR_frameRAM32L,
 GR_frameRAM32H,
 GR_frameRAM3x8,

 GR_firstTextFrameMode = GR_frameText,
 GR_lastTextFrameMode = GR_frameText,
 GR_firstGraphicsFrameMode = GR_frameHERC1,
 GR_lastGraphicsFrameMode = GR_frameSVGA32H_LFB,
 GR_firstRAMframeMode = GR_frameRAM1,
 GR_lastRAMframeMode = GR_frameRAM3x8
} GrFrameMode;




typedef enum _GR_videoAdapters {
 GR_UNKNOWN = (-1),
 GR_VGA,
 GR_EGA,
 GR_HERC,
 GR_8514A,
 GR_S3,
 GR_XWIN,
 GR_WIN32,
 GR_LNXFB,
 GR_SDL,
 GR_MEM
} GrVideoAdapter;




struct _GR_videoDriver {
 char *name;
 enum _GR_videoAdapters adapter;
 struct _GR_videoDriver *inherit;
 struct _GR_videoMode *modes;
 int nmodes;
 int (*detect)(void);
 int (*init)(char *options);
 void (*reset)(void);
 GrVideoMode * (*selectmode)(GrVideoDriver *drv,int w,int h,int bpp,
     int txt,unsigned int *ep);
 unsigned drvflags;
};
# 313 "/home/arnold/scm/dostrs-build-extras/grx249/include/grx20.h"
struct _GR_videoMode {
 char present;
 char bpp;
 short width,height;
 short mode;
 int lineoffset;
 int privdata;
 struct _GR_videoModeExt *extinfo;
};







struct _GR_videoModeExt {
 enum _GR_frameModes mode;
 struct _GR_frameDriver *drv;
 char *frame;
 char cprec[3];
 char cpos[3];
 int flags;
 int (*setup)(GrVideoMode *md,int noclear);
 int (*setvsize)(GrVideoMode *md,int w,int h,GrVideoMode *result);
 int (*scroll)(GrVideoMode *md,int x,int y,int result[2]);
 void (*setbank)(int bk);
 void (*setrwbanks)(int rb,int wb);
 void (*loadcolor)(int c,int r,int g,int b);
 int LFB_Selector;
};




struct _GR_frameDriver {
    enum _GR_frameModes mode;
    enum _GR_frameModes rmode;
    int is_video;
    int row_align;
    int num_planes;
    int bits_per_pixel;
    long max_plane_size;
    int (*init)(GrVideoMode *md);
    GrColor (*readpixel)(GrFrame *c,int x,int y);
    void (*drawpixel)(int x,int y,GrColor c);
    void (*drawline)(int x,int y,int dx,int dy,GrColor c);
    void (*drawhline)(int x,int y,int w,GrColor c);
    void (*drawvline)(int x,int y,int h,GrColor c);
    void (*drawblock)(int x,int y,int w,int h,GrColor c);
    void (*drawbitmap)(int x,int y,int w,int h,char *bmp,int pitch,int start,GrColor fg,GrColor bg);
    void (*drawpattern)(int x,int y,int w,char patt,GrColor fg,GrColor bg);
    void (*bitblt)(GrFrame *dst,int dx,int dy,GrFrame *src,int x,int y,int w,int h,GrColor op);
    void (*bltv2r)(GrFrame *dst,int dx,int dy,GrFrame *src,int x,int y,int w,int h,GrColor op);
    void (*bltr2v)(GrFrame *dst,int dx,int dy,GrFrame *src,int x,int y,int w,int h,GrColor op);

    GrColor *(*getindexedscanline)(GrFrame *c,int x, int y, int w, int *indx);



    void (*putscanline)(int x, int y, int w,const GrColor *scl, GrColor op);


};




extern const struct _GR_driverInfo {
 struct _GR_videoDriver *vdriver;
 struct _GR_videoMode *curmode;
 struct _GR_videoMode actmode;
 struct _GR_frameDriver fdriver;
 struct _GR_frameDriver sdriver;
 struct _GR_frameDriver tdriver;
 enum _GR_graphicsModes mcode;
 int deftw,defth;
 int defgw,defgh;
 GrColor deftc,defgc;
 int vposx,vposy;
 int errsfatal;
 int moderestore;
 int splitbanks;
 int curbank;
 void (*mdsethook)(void);
 void (*setbank)(int bk);
 void (*setrwbanks)(int rb,int wb);
} * const GrDriverInfo;




int GrSetDriver(char *drvspec);
int GrSetMode(GrGraphicsMode which,...);
int GrSetViewport(int xpos,int ypos);
void GrSetModeHook(void (*hookfunc)(void));
void GrSetModeRestore(int restoreFlag);
void GrSetErrorHandling(int exitIfError);
void GrSetEGAVGAmonoDrawnPlane(int plane);
void GrSetEGAVGAmonoShownPlane(int plane);

unsigned GrGetLibraryVersion(void);
unsigned GrGetLibrarySystem(void);




GrGraphicsMode GrCurrentMode(void);
GrVideoAdapter GrAdapterType(void);
GrFrameMode GrCurrentFrameMode(void);
GrFrameMode GrScreenFrameMode(void);
GrFrameMode GrCoreFrameMode(void);

const GrVideoDriver *GrCurrentVideoDriver(void);
const GrVideoMode *GrCurrentVideoMode(void);
const GrVideoMode *GrVirtualVideoMode(void);
const GrFrameDriver *GrCurrentFrameDriver(void);
const GrFrameDriver *GrScreenFrameDriver(void);
const GrVideoMode *GrFirstVideoMode(GrFrameMode fmode);
const GrVideoMode *GrNextVideoMode(const GrVideoMode *prev);

int GrScreenX(void);
int GrScreenY(void);
int GrVirtualX(void);
int GrVirtualY(void);
int GrViewportX(void);
int GrViewportY(void);

int GrScreenIsVirtual(void);




int GrFrameNumPlanes(GrFrameMode md);
int GrFrameLineOffset(GrFrameMode md,int width);
long GrFramePlaneSize(GrFrameMode md,int w,int h);
long GrFrameContextSize(GrFrameMode md,int w,int h);

int GrNumPlanes(void);
int GrLineOffset(int width);
long GrPlaneSize(int w,int h);
long GrContextSize(int w,int h);
# 495 "/home/arnold/scm/dostrs-build-extras/grx249/include/grx20.h"
struct _GR_frame {
 char *gf_baseaddr[4];
 short gf_selector;
 char gf_onscreen;
 char gf_memflags;
 int gf_lineoffset;
 struct _GR_frameDriver *gf_driver;
};

struct _GR_context {
 struct _GR_frame gc_frame;
 struct _GR_context *gc_root;
 int gc_xmax;
 int gc_ymax;
 int gc_xoffset;
 int gc_yoffset;
 int gc_xcliplo;
 int gc_ycliplo;
 int gc_xcliphi;
 int gc_ycliphi;
 int gc_usrxbase;
 int gc_usrybase;
 int gc_usrwidth;
 int gc_usrheight;






};

extern const struct _GR_contextInfo {
 struct _GR_context current;
 struct _GR_context screen;
} * const GrContextInfo;

GrContext *GrCreateContext(int w,int h,char *memory[4],GrContext *where);
GrContext *GrCreateFrameContext(GrFrameMode md,int w,int h,char *memory[4],GrContext *where);
GrContext *GrCreateSubContext(int x1,int y1,int x2,int y2,const GrContext *parent,GrContext *where);
GrContext *GrSaveContext(GrContext *where);

GrContext *GrCurrentContext(void);
GrContext *GrScreenContext(void);

void GrDestroyContext(GrContext *context);
void GrResizeSubContext(GrContext *context,int x1,int y1,int x2,int y2);
void GrSetContext(const GrContext *context);

void GrSetClipBox(int x1,int y1,int x2,int y2);
void GrSetClipBoxC(GrContext *c,int x1,int y1,int x2,int y2);
void GrGetClipBox(int *x1p,int *y1p,int *x2p,int *y2p);
void GrGetClipBoxC(const GrContext *c,int *x1p,int *y1p,int *x2p,int *y2p);
void GrResetClipBox(void);
void GrResetClipBoxC(GrContext *c);

int GrMaxX(void);
int GrMaxY(void);
int GrSizeX(void);
int GrSizeY(void);
int GrLowX(void);
int GrLowY(void);
int GrHighX(void);
int GrHighY(void);
# 602 "/home/arnold/scm/dostrs-build-extras/grx249/include/grx20.h"
GrColor GrColorValue(GrColor c);
GrColor GrColorMode(GrColor c);
GrColor GrWriteModeColor(GrColor c);
GrColor GrXorModeColor(GrColor c);
GrColor GrOrModeColor(GrColor c);
GrColor GrAndModeColor(GrColor c);
GrColor GrImageModeColor(GrColor c);




extern const struct _GR_colorInfo {
 GrColor ncolors;
 GrColor nfree;
 GrColor black;
 GrColor white;
 unsigned int RGBmode;
 unsigned int prec[3];
 unsigned int pos[3];
 unsigned int mask[3];
 unsigned int round[3];
 unsigned int shift[3];
 unsigned int norm;
 struct {
  unsigned char r,g,b;
  unsigned int defined:1;
  unsigned int writable:1;
  unsigned long int nused;
 } ctable[256];
} * const GrColorInfo;

void GrResetColors(void);
void GrSetRGBcolorMode(void);
void GrRefreshColors(void);

GrColor GrNumColors(void);
GrColor GrNumFreeColors(void);

GrColor GrBlack(void);
GrColor GrWhite(void);

GrColor GrBuildRGBcolorT(int r,int g,int b);
GrColor GrBuildRGBcolorR(int r,int g,int b);
int GrRGBcolorRed(GrColor c);
int GrRGBcolorGreen(GrColor c);
int GrRGBcolorBlue(GrColor c);

GrColor GrAllocColor(int r,int g,int b);
GrColor GrAllocColorID(int r,int g,int b);
GrColor GrAllocColor2(long hcolor);
GrColor GrAllocColor2ID(long hcolor);
GrColor GrAllocCell(void);

GrColor *GrAllocEgaColors(void);

void GrSetColor(GrColor c,int r,int g,int b);
void GrFreeColor(GrColor c);
void GrFreeCell(GrColor c);

void GrQueryColor(GrColor c,int *r,int *g,int *b);
void GrQueryColorID(GrColor c,int *r,int *g,int *b);
void GrQueryColor2(GrColor c,long *hcolor);
void GrQueryColor2ID(GrColor c,long *hcolor);

int GrColorSaveBufferSize(void);
void GrSaveColors(void *buffer);
void GrRestoreColors(void *buffer);
# 766 "/home/arnold/scm/dostrs-build-extras/grx249/include/grx20.h"
typedef GrColor *GrColorTableP;
# 798 "/home/arnold/scm/dostrs-build-extras/grx249/include/grx20.h"
typedef struct {
 GrColor fbx_intcolor;
 GrColor fbx_topcolor;
 GrColor fbx_rightcolor;
 GrColor fbx_bottomcolor;
 GrColor fbx_leftcolor;
} GrFBoxColors;

void GrClearScreen(GrColor bg);
void GrClearContext(GrColor bg);
void GrClearContextC(GrContext *ctx, GrColor bg);
void GrClearClipBox(GrColor bg);
void GrPlot(int x,int y,GrColor c);
void GrLine(int x1,int y1,int x2,int y2,GrColor c);
void GrHLine(int x1,int x2,int y,GrColor c);
void GrVLine(int x,int y1,int y2,GrColor c);
void GrBox(int x1,int y1,int x2,int y2,GrColor c);
void GrFilledBox(int x1,int y1,int x2,int y2,GrColor c);
void GrFramedBox(int x1,int y1,int x2,int y2,int wdt,const GrFBoxColors *c);
int GrGenerateEllipse(int xc,int yc,int xa,int ya,int points[(1024 + 5)][2]);
int GrGenerateEllipseArc(int xc,int yc,int xa,int ya,int start,int end,int points[(1024 + 5)][2]);
void GrLastArcCoords(int *xs,int *ys,int *xe,int *ye,int *xc,int *yc);
void GrCircle(int xc,int yc,int r,GrColor c);
void GrEllipse(int xc,int yc,int xa,int ya,GrColor c);
void GrCircleArc(int xc,int yc,int r,int start,int end,int style,GrColor c);
void GrEllipseArc(int xc,int yc,int xa,int ya,int start,int end,int style,GrColor c);
void GrFilledCircle(int xc,int yc,int r,GrColor c);
void GrFilledEllipse(int xc,int yc,int xa,int ya,GrColor c);
void GrFilledCircleArc(int xc,int yc,int r,int start,int end,int style,GrColor c);
void GrFilledEllipseArc(int xc,int yc,int xa,int ya,int start,int end,int style,GrColor c);
void GrPolyLine(int numpts,int points[][2],GrColor c);
void GrPolygon(int numpts,int points[][2],GrColor c);
void GrFilledConvexPolygon(int numpts,int points[][2],GrColor c);
void GrFilledPolygon(int numpts,int points[][2],GrColor c);
void GrBitBlt(GrContext *dst,int x,int y,GrContext *src,int x1,int y1,int x2,int y2,GrColor op);
void GrBitBlt1bpp(GrContext *dst,int dx,int dy,GrContext *src,int x1,int y1,int x2,int y2,GrColor fg,GrColor bg);
void GrFloodFill(int x, int y, GrColor border, GrColor c);
void GrFloodSpill(int x1, int y1, int x2, int y2, GrColor old_c, GrColor new_c);
void GrFloodSpill2(int x1, int y1, int x2, int y2, GrColor old_c1, GrColor new_c1, GrColor old_c2, GrColor new_c2);
void GrFloodSpillC(GrContext *ctx, int x1, int y1, int x2, int y2, GrColor old_c, GrColor new_c);
void GrFloodSpillC2(GrContext *ctx, int x1, int y1, int x2, int y2, GrColor old_c1, GrColor new_c1, GrColor old_c2, GrColor new_c2);

GrColor GrPixel(int x,int y);
GrColor GrPixelC(GrContext *c,int x,int y);

const GrColor *GrGetScanline(int x1,int x2,int yy);
const GrColor *GrGetScanlineC(GrContext *ctx,int x1,int x2,int yy);
# 855 "/home/arnold/scm/dostrs-build-extras/grx249/include/grx20.h"
void GrPutScanline(int x1,int x2,int yy,const GrColor *c, GrColor op);
# 876 "/home/arnold/scm/dostrs-build-extras/grx249/include/grx20.h"
void GrPlotNC(int x,int y,GrColor c);
void GrLineNC(int x1,int y1,int x2,int y2,GrColor c);
void GrHLineNC(int x1,int x2,int y,GrColor c);
void GrVLineNC(int x,int y1,int y2,GrColor c);
void GrBoxNC(int x1,int y1,int x2,int y2,GrColor c);
void GrFilledBoxNC(int x1,int y1,int x2,int y2,GrColor c);
void GrFramedBoxNC(int x1,int y1,int x2,int y2,int wdt,const GrFBoxColors *c);
void GrBitBltNC(GrContext *dst,int x,int y,GrContext *src,int x1,int y1,int x2,int y2,GrColor op);

GrColor GrPixelNC(int x,int y);
GrColor GrPixelCNC(GrContext *c,int x,int y);
# 958 "/home/arnold/scm/dostrs-build-extras/grx249/include/grx20.h"
extern int _GR_textattrintensevideo;
# 997 "/home/arnold/scm/dostrs-build-extras/grx249/include/grx20.h"
typedef struct _GR_fontHeader {
 char *name;
 char *family;
 char proportional;
 char scalable;
 char preloaded;
 char modified;
 unsigned int width;
 unsigned int height;
 unsigned int baseline;
 unsigned int ulpos;
 unsigned int ulheight;
 unsigned int minchar;
 unsigned int numchars;
} GrFontHeader;

typedef struct _GR_fontChrInfo {
 unsigned int width;
 unsigned int offset;
} GrFontChrInfo;

typedef struct _GR_font {
 struct _GR_fontHeader h;
 char *bitmap;
 char *auxmap;
 unsigned int minwidth;
 unsigned int maxwidth;
 unsigned int auxsize;
 unsigned int auxnext;
 unsigned int *auxoffs[7];
 struct _GR_fontChrInfo chrinfo[1];
} GrFont;

extern GrFont GrFont_PC6x8;
extern GrFont GrFont_PC8x8;
extern GrFont GrFont_PC8x14;
extern GrFont GrFont_PC8x16;


GrFont *GrLoadFont(char *name);
GrFont *GrLoadConvertedFont(char *name,int cvt,int w,int h,int minch,int maxch);
GrFont *GrBuildConvertedFont(const GrFont *from,int cvt,int w,int h,int minch,int maxch);

void GrUnloadFont(GrFont *font);
void GrDumpFont(const GrFont *f,char *CsymbolName,char *fileName);
void GrDumpFnaFont(const GrFont *f, char *fileName);
void GrSetFontPath(char *path_list);

int GrFontCharPresent(const GrFont *font,int chr);
int GrFontCharWidth(const GrFont *font,int chr);
int GrFontCharHeight(const GrFont *font,int chr);
int GrFontCharBmpRowSize(const GrFont *font,int chr);
int GrFontCharBitmapSize(const GrFont *font,int chr);
int GrFontStringWidth(const GrFont *font,void *text,int len,int type);
int GrFontStringHeight(const GrFont *font,void *text,int len,int type);
int GrProportionalTextWidth(const GrFont *font,const void *text,int len,int type);

char *GrBuildAuxiliaryBitmap(GrFont *font,int chr,int dir,int ul);
char *GrFontCharBitmap(const GrFont *font,int chr);
char *GrFontCharAuxBmp(GrFont *font,int chr,int dir,int ul);

typedef union _GR_textColor {
 GrColor v;
 GrColorTableP p;
} GrTextColor;

typedef struct _GR_textOption {
 struct _GR_font *txo_font;
 union _GR_textColor txo_fgcolor;
 union _GR_textColor txo_bgcolor;
 char txo_chrtype;
 char txo_direct;
 char txo_xalign;
 char txo_yalign;
} GrTextOption;

typedef struct {
 struct _GR_font *txr_font;
 union _GR_textColor txr_fgcolor;
 union _GR_textColor txr_bgcolor;
 void *txr_buffer;
 void *txr_backup;
 int txr_width;
 int txr_height;
 int txr_lineoffset;
 int txr_xpos;
 int txr_ypos;
 char txr_chrtype;
} GrTextRegion;

int GrCharWidth(int chr,const GrTextOption *opt);
int GrCharHeight(int chr,const GrTextOption *opt);
void GrCharSize(int chr,const GrTextOption *opt,int *w,int *h);
int GrStringWidth(void *text,int length,const GrTextOption *opt);
int GrStringHeight(void *text,int length,const GrTextOption *opt);
void GrStringSize(void *text,int length,const GrTextOption *opt,int *w,int *h);

void GrDrawChar(int chr,int x,int y,const GrTextOption *opt);
void GrDrawString(void *text,int length,int x,int y,const GrTextOption *opt);
void GrTextXY(int x,int y,char *text,GrColor fg,GrColor bg);

void GrDumpChar(int chr,int col,int row,const GrTextRegion *r);
void GrDumpText(int col,int row,int wdt,int hgt,const GrTextRegion *r);
void GrDumpTextRegion(const GrTextRegion *r);
# 1179 "/home/arnold/scm/dostrs-build-extras/grx249/include/grx20.h"
typedef struct {
 GrColor lno_color;
 int lno_width;
 int lno_pattlen;
 unsigned char *lno_dashpat;
} GrLineOption;

void GrCustomLine(int x1,int y1,int x2,int y2,const GrLineOption *o);
void GrCustomBox(int x1,int y1,int x2,int y2,const GrLineOption *o);
void GrCustomCircle(int xc,int yc,int r,const GrLineOption *o);
void GrCustomEllipse(int xc,int yc,int xa,int ya,const GrLineOption *o);
void GrCustomCircleArc(int xc,int yc,int r,int start,int end,int style,const GrLineOption *o);
void GrCustomEllipseArc(int xc,int yc,int xa,int ya,int start,int end,int style,const GrLineOption *o);
void GrCustomPolyLine(int numpts,int points[][2],const GrLineOption *o);
void GrCustomPolygon(int numpts,int points[][2],const GrLineOption *o);
# 1204 "/home/arnold/scm/dostrs-build-extras/grx249/include/grx20.h"
typedef struct _GR_bitmap {
 int bmp_ispixmap;
 int bmp_height;
 char *bmp_data;
 GrColor bmp_fgcolor;
 GrColor bmp_bgcolor;
 int bmp_memflags;
} GrBitmap;







typedef struct _GR_pixmap {
 int pxp_ispixmap;
 int pxp_width;
 int pxp_height;
 GrColor pxp_oper;
 struct _GR_frame pxp_source;
} GrPixmap;




typedef union _GR_pattern {
 int gp_ispixmap;
 GrBitmap gp_bitmap;
 GrPixmap gp_pixmap;
} GrPattern;
# 1251 "/home/arnold/scm/dostrs-build-extras/grx249/include/grx20.h"
typedef struct {
 GrPattern *lnp_pattern;
 GrLineOption *lnp_option;
} GrLinePattern;

GrPattern *GrBuildPixmap(const char *pixels,int w,int h,const GrColorTableP colors);
GrPattern *GrBuildPixmapFromBits(const char *bits,int w,int h,GrColor fgc,GrColor bgc);
GrPattern *GrConvertToPixmap(GrContext *src);

void GrDestroyPattern(GrPattern *p);

void GrPatternedLine(int x1,int y1,int x2,int y2,GrLinePattern *lp);
void GrPatternedBox(int x1,int y1,int x2,int y2,GrLinePattern *lp);
void GrPatternedCircle(int xc,int yc,int r,GrLinePattern *lp);
void GrPatternedEllipse(int xc,int yc,int xa,int ya,GrLinePattern *lp);
void GrPatternedCircleArc(int xc,int yc,int r,int start,int end,int style,GrLinePattern *lp);
void GrPatternedEllipseArc(int xc,int yc,int xa,int ya,int start,int end,int style,GrLinePattern *lp);
void GrPatternedPolyLine(int numpts,int points[][2],GrLinePattern *lp);
void GrPatternedPolygon(int numpts,int points[][2],GrLinePattern *lp);

void GrPatternFilledPlot(int x,int y,GrPattern *p);
void GrPatternFilledLine(int x1,int y1,int x2,int y2,GrPattern *p);
void GrPatternFilledBox(int x1,int y1,int x2,int y2,GrPattern *p);
void GrPatternFilledCircle(int xc,int yc,int r,GrPattern *p);
void GrPatternFilledEllipse(int xc,int yc,int xa,int ya,GrPattern *p);
void GrPatternFilledCircleArc(int xc,int yc,int r,int start,int end,int style,GrPattern *p);
void GrPatternFilledEllipseArc(int xc,int yc,int xa,int ya,int start,int end,int style,GrPattern *p);
void GrPatternFilledConvexPolygon(int numpts,int points[][2],GrPattern *p);
void GrPatternFilledPolygon(int numpts,int points[][2],GrPattern *p);
void GrPatternFloodFill(int x, int y, GrColor border, GrPattern *p);

void GrPatternDrawChar(int chr,int x,int y,const GrTextOption *opt,GrPattern *p);
void GrPatternDrawString(void *text,int length,int x,int y,const GrTextOption *opt,GrPattern *p);
void GrPatternDrawStringExt(void *text,int length,int x,int y,const GrTextOption *opt,GrPattern *p);
# 1304 "/home/arnold/scm/dostrs-build-extras/grx249/include/grx20.h"
GrPixmap *GrImageBuild(const char *pixels,int w,int h,const GrColorTableP colors);
void GrImageDestroy(GrPixmap *i);
void GrImageDisplay(int x,int y, GrPixmap *i);
void GrImageDisplayExt(int x1,int y1,int x2,int y2, GrPixmap *i);
void GrImageFilledBoxAlign(int xo,int yo,int x1,int y1,int x2,int y2,GrPixmap *p);
void GrImageHLineAlign(int xo,int yo,int x,int y,int width,GrPixmap *p);
void GrImagePlotAlign(int xo,int yo,int x,int y,GrPixmap *p);

GrPixmap *GrImageInverse(GrPixmap *p,int flag);
GrPixmap *GrImageStretch(GrPixmap *p,int nwidth,int nheight);

GrPixmap *GrImageFromPattern(GrPattern *p);
GrPixmap *GrImageFromContext(GrContext *c);
GrPixmap *GrImageBuildUsedAsPattern(const char *pixels,int w,int h,const GrColorTableP colors);

GrPattern *GrPatternFromImage(GrPixmap *p);
# 1339 "/home/arnold/scm/dostrs-build-extras/grx249/include/grx20.h"
void GrSetUserWindow(int x1,int y1,int x2,int y2);
void GrGetUserWindow(int *x1,int *y1,int *x2,int *y2);
void GrGetScreenCoord(int *x,int *y);
void GrGetUserCoord(int *x,int *y);

void GrUsrPlot(int x,int y,GrColor c);
void GrUsrLine(int x1,int y1,int x2,int y2,GrColor c);
void GrUsrHLine(int x1,int x2,int y,GrColor c);
void GrUsrVLine(int x,int y1,int y2,GrColor c);
void GrUsrBox(int x1,int y1,int x2,int y2,GrColor c);
void GrUsrFilledBox(int x1,int y1,int x2,int y2,GrColor c);
void GrUsrFramedBox(int x1,int y1,int x2,int y2,int wdt,GrFBoxColors *c);
void GrUsrCircle(int xc,int yc,int r,GrColor c);
void GrUsrEllipse(int xc,int yc,int xa,int ya,GrColor c);
void GrUsrCircleArc(int xc,int yc,int r,int start,int end,int style,GrColor c);
void GrUsrEllipseArc(int xc,int yc,int xa,int ya,int start,int end,int style,GrColor c);
void GrUsrFilledCircle(int xc,int yc,int r,GrColor c);
void GrUsrFilledEllipse(int xc,int yc,int xa,int ya,GrColor c);
void GrUsrFilledCircleArc(int xc,int yc,int r,int start,int end,int style,GrColor c);
void GrUsrFilledEllipseArc(int xc,int yc,int xa,int ya,int start,int end,int style,GrColor c);
void GrUsrPolyLine(int numpts,int points[][2],GrColor c);
void GrUsrPolygon(int numpts,int points[][2],GrColor c);
void GrUsrFilledConvexPolygon(int numpts,int points[][2],GrColor c);
void GrUsrFilledPolygon(int numpts,int points[][2],GrColor c);
void GrUsrFloodFill(int x, int y, GrColor border, GrColor c);

GrColor GrUsrPixel(int x,int y);
GrColor GrUsrPixelC(GrContext *c,int x,int y);

void GrUsrCustomLine(int x1,int y1,int x2,int y2,const GrLineOption *o);
void GrUsrCustomBox(int x1,int y1,int x2,int y2,const GrLineOption *o);
void GrUsrCustomCircle(int xc,int yc,int r,const GrLineOption *o);
void GrUsrCustomEllipse(int xc,int yc,int xa,int ya,const GrLineOption *o);
void GrUsrCustomCircleArc(int xc,int yc,int r,int start,int end,int style,const GrLineOption *o);
void GrUsrCustomEllipseArc(int xc,int yc,int xa,int ya,int start,int end,int style,const GrLineOption *o);
void GrUsrCustomPolyLine(int numpts,int points[][2],const GrLineOption *o);
void GrUsrCustomPolygon(int numpts,int points[][2],const GrLineOption *o);

void GrUsrPatternedLine(int x1,int y1,int x2,int y2,GrLinePattern *lp);
void GrUsrPatternedBox(int x1,int y1,int x2,int y2,GrLinePattern *lp);
void GrUsrPatternedCircle(int xc,int yc,int r,GrLinePattern *lp);
void GrUsrPatternedEllipse(int xc,int yc,int xa,int ya,GrLinePattern *lp);
void GrUsrPatternedCircleArc(int xc,int yc,int r,int start,int end,int style,GrLinePattern *lp);
void GrUsrPatternedEllipseArc(int xc,int yc,int xa,int ya,int start,int end,int style,GrLinePattern *lp);
void GrUsrPatternedPolyLine(int numpts,int points[][2],GrLinePattern *lp);
void GrUsrPatternedPolygon(int numpts,int points[][2],GrLinePattern *lp);

void GrUsrPatternFilledPlot(int x,int y,GrPattern *p);
void GrUsrPatternFilledLine(int x1,int y1,int x2,int y2,GrPattern *p);
void GrUsrPatternFilledBox(int x1,int y1,int x2,int y2,GrPattern *p);
void GrUsrPatternFilledCircle(int xc,int yc,int r,GrPattern *p);
void GrUsrPatternFilledEllipse(int xc,int yc,int xa,int ya,GrPattern *p);
void GrUsrPatternFilledCircleArc(int xc,int yc,int r,int start,int end,int style,GrPattern *p);
void GrUsrPatternFilledEllipseArc(int xc,int yc,int xa,int ya,int start,int end,int style,GrPattern *p);
void GrUsrPatternFilledConvexPolygon(int numpts,int points[][2],GrPattern *p);
void GrUsrPatternFilledPolygon(int numpts,int points[][2],GrPattern *p);
void GrUsrPatternFloodFill(int x, int y, GrColor border, GrPattern *p);

void GrUsrDrawChar(int chr,int x,int y,const GrTextOption *opt);
void GrUsrDrawString(char *text,int length,int x,int y,const GrTextOption *opt);
void GrUsrTextXY(int x,int y,char *text,GrColor fg,GrColor bg);





typedef struct _GR_cursor {
 struct _GR_context work;
 int xcord,ycord;
 int xsize,ysize;
 int xoffs,yoffs;
 int xwork,ywork;
 int xwpos,ywpos;
 int displayed;
} GrCursor;

GrCursor *GrBuildCursor(char *pixels,int pitch,int w,int h,int xo,int yo,const GrColorTableP c);
void GrDestroyCursor(GrCursor *cursor);
void GrDisplayCursor(GrCursor *cursor);
void GrEraseCursor(GrCursor *cursor);
void GrMoveCursor(GrCursor *cursor,int x,int y);
# 1469 "/home/arnold/scm/dostrs-build-extras/grx249/include/grx20.h"
typedef struct _GR_mouseEvent {
 int flags;
 int x,y;
 int buttons;
 int key;
 int kbstat;
 long dtime;
} GrMouseEvent;




extern const struct _GR_mouseInfo {
 int (*block)(GrContext*,int,int,int,int);
 void (*unblock)(int flags);
 void (*uninit)(void);
 struct _GR_cursor *cursor;
 struct _GR_mouseEvent *queue;
 int msstatus;
 int displayed;
 int blockflag;
 int docheck;
 int cursmode;
 int x1,y1,x2,y2;
 GrColor curscolor;
 int owncursor;
 int xpos,ypos;
 int xmin,xmax;
 int ymin,ymax;
 int spmult,spdiv;
 int thresh,accel;
 int moved;
 int qsize;
 int qlength;
 int qread;
 int qwrite;
} * const GrMouseInfo;

int GrMouseDetect(void);
void GrMouseEventMode(int dummy);
void GrMouseInit(void);
void GrMouseInitN(int queue_size);
void GrMouseUnInit(void);
void GrMouseSetSpeed(int spmult,int spdiv);
void GrMouseSetAccel(int thresh,int accel);
void GrMouseSetLimits(int x1,int y1,int x2,int y2);
void GrMouseGetLimits(int *x1,int *y1,int *x2,int *y2);
void GrMouseWarp(int x,int y);
void GrMouseEventEnable(int enable_kb,int enable_ms);
void GrMouseGetEvent(int flags,GrMouseEvent *event);

void GrMouseGetEventT(int flags,GrMouseEvent *event,long timout_msecs);







int GrMousePendingEvent(void);

GrCursor *GrMouseGetCursor(void);
void GrMouseSetCursor(GrCursor *cursor);
void GrMouseSetColors(GrColor fg,GrColor bg);
void GrMouseSetCursorMode(int mode,...);
void GrMouseDisplayCursor(void);
void GrMouseEraseCursor(void);
void GrMouseUpdateCursor(void);
int GrMouseCursorIsDisplayed(void);

int GrMouseBlock(GrContext *c,int x1,int y1,int x2,int y2);
void GrMouseUnBlock(int return_value_from_GrMouseBlock);
# 1613 "/home/arnold/scm/dostrs-build-extras/grx249/include/grx20.h"
int GrSaveContextToPbm( GrContext *grc, char *pbmfn, char *docn );
int GrSaveContextToPgm( GrContext *grc, char *pgmfn, char *docn );
int GrSaveContextToPpm( GrContext *grc, char *ppmfn, char *docn );
int GrLoadContextFromPnm( GrContext *grc, char *pnmfn );
int GrQueryPnm( char *pnmfn, int *width, int *height, int *maxval );
int GrLoadContextFromPnmBuffer( GrContext *grc, const char *buffer );
int GrQueryPnmBuffer( const char *buffer, int *width, int *height, int *maxval );






int GrPngSupport( void );
int GrSaveContextToPng( GrContext *grc, char *pngfn );
int GrLoadContextFromPng( GrContext *grc, char *pngfn, int use_alpha );
int GrQueryPng( char *pngfn, int *width, int *height );






int GrJpegSupport( void );
int GrLoadContextFromJpeg( GrContext *grc, char *jpegfn, int scale );
int GrQueryJpeg( char *jpegfn, int *width, int *height );
int GrSaveContextToJpeg( GrContext *grc, char *jpegfn, int quality );
int GrSaveContextToGrayJpeg( GrContext *grc, char *jpegfn, int quality );





void GrResizeGrayMap(unsigned char *map,int pitch,int ow,int oh,int nw,int nh);
int GrMatchString(const char *pattern,const char *strg);
void GrSetWindowTitle(char *title);
void GrSleep(int msec);
void GrFlush(void);
long GrMsecTime(void);
# 1673 "/home/arnold/scm/dostrs-build-extras/grx249/include/grx20.h"
int SaveContextToTiff(GrContext *cxt, char *tiffn, unsigned compr, char *docn);
# 46 "/home/arnold/scm/xtrs/trs_djgpp.c" 2
# 1 "/home/arnold/scm/dostrs-build-extras/grx249/include/grxkeys.h" 1
# 35 "/home/arnold/scm/dostrs-build-extras/grx249/include/grxkeys.h"
typedef unsigned short GrKeyType;
# 352 "/home/arnold/scm/dostrs-build-extras/grx249/include/grxkeys.h"
extern int GrKeyPressed(void);
extern GrKeyType GrKeyRead(void);
extern int GrKeyStat(void);
# 47 "/home/arnold/scm/xtrs/trs_djgpp.c" 2


# 1 "/home/arnold/scm/xtrs/trs_iodefs.h" 1
# 26 "/home/arnold/scm/xtrs/trs_iodefs.h"
extern char trs_char_data[][256][12];
# 39 "/home/arnold/scm/xtrs/trs_iodefs.h"
typedef struct trs_pattern_table {
    char normal[256][12];
    char wideleft[256][12];
    char wideright[256][12];
} trs_pattern_table;


void init_pattern_table(trs_pattern_table *dest, const char **src_data, int model);
# 50 "/home/arnold/scm/xtrs/trs_djgpp.c" 2
# 1 "/home/arnold/scm/xtrs/trs.h" 1
# 26 "/home/arnold/scm/xtrs/trs.h"
# 1 "/home/arnold/scm/xtrs/z80.h" 1
# 24 "/home/arnold/scm/xtrs/z80.h"
# 1 "/home/arnold/scm/xtrs/config.h" 1
# 25 "/home/arnold/scm/xtrs/z80.h" 2

# 1 "/usr/i686-pc-msdosdjgpp/sys-include/ctype.h" 1 3 4
# 14 "/usr/i686-pc-msdosdjgpp/sys-include/ctype.h" 3 4

# 14 "/usr/i686-pc-msdosdjgpp/sys-include/ctype.h" 3 4
int isalnum(int c);
int isalpha(int c);
int iscntrl(int c);
int isdigit(int c);
int isgraph(int c);
int islower(int c);
int isprint(int c);
int ispunct(int c);
int isspace(int c);
int isupper(int c);
int isxdigit(int c);
int tolower(int c);
int toupper(int c);




int isblank(int c);




# 1 "/usr/i686-pc-msdosdjgpp/sys-include/inlines/ctype.ha" 1 3 4
# 18 "/usr/i686-pc-msdosdjgpp/sys-include/inlines/ctype.ha" 3 4
extern unsigned short __dj_ctype_flags[];
extern unsigned char __dj_ctype_toupper[];
extern unsigned char __dj_ctype_tolower[];
# 37 "/usr/i686-pc-msdosdjgpp/sys-include/ctype.h" 2 3 4
# 46 "/usr/i686-pc-msdosdjgpp/sys-include/ctype.h" 3 4
int _tolower(int c);
int _toupper(int c);


# 1 "/usr/i686-pc-msdosdjgpp/sys-include/inlines/ctype.hp" 1 3 4




extern unsigned char __dj_ctype_toupper[];
extern unsigned char __dj_ctype_tolower[];
# 51 "/usr/i686-pc-msdosdjgpp/sys-include/ctype.h" 2 3 4




int isascii(int c);
int toascii(int c);


# 1 "/usr/i686-pc-msdosdjgpp/sys-include/inlines/ctype.hd" 1 3 4
# 60 "/usr/i686-pc-msdosdjgpp/sys-include/ctype.h" 2 3 4
# 27 "/home/arnold/scm/xtrs/z80.h" 2








# 34 "/home/arnold/scm/xtrs/z80.h"
typedef unsigned int Uint;
typedef unsigned short Ushort;
typedef unsigned char Uchar;


typedef unsigned long long tstate_t;
# 52 "/home/arnold/scm/xtrs/z80.h"
struct twobyte
{



    Uchar low, high;

};

struct fourbyte
{



    Uchar byte0, byte1, byte2, byte3;

};


typedef union
{
    struct twobyte byte;
    Ushort word;
} wordregister;

struct z80_state_struct
{
    wordregister af;
    wordregister bc;
    wordregister de;
    wordregister hl;
    wordregister ix;
    wordregister iy;
    wordregister sp;
    wordregister pc;

    wordregister af_prime;
    wordregister bc_prime;
    wordregister de_prime;
    wordregister hl_prime;

    Uchar i;


    Uchar iff1, iff2;
    Uchar interrupt_mode;
# 111 "/home/arnold/scm/xtrs/z80.h"
    int irq;
# 120 "/home/arnold/scm/xtrs/z80.h"
    int nmi, nmi_seen;


    int delay;


    tstate_t t_count;


    float clockMHz;



    tstate_t sched;
};
# 229 "/home/arnold/scm/xtrs/z80.h"
extern struct z80_state_struct z80_state;

extern void z80_reset(void);
extern int z80_run(int continuous);
extern void mem_init(void);
extern int mem_read(int address);
extern void mem_write(int address, int value);
extern void mem_write_rom(int address, int value);
extern int mem_read_word(int address);
extern void mem_write_word(int address, int value);
Uchar *mem_pointer(int address, int writing);
extern int mem_block_transfer(Ushort dest, Ushort source, int direction,
         Ushort count);
extern int load_hex();
extern void debug(const char *fmt, ...);
extern void error(const char *fmt, ...);
extern void fatal(const char *fmt, ...);
extern void joshlog(const char *fmt, ...);
extern void joshlogv(const char *fmt, va_list args);
extern int joshlog_echo_to_stdout;

extern void z80_out(int port, int value);
extern int z80_in(int port);
extern int disassemble(unsigned short pc);
extern void debug_init(void);
extern void debug_shell(void);

extern volatile int josh_trace_enabled;
# 27 "/home/arnold/scm/xtrs/trs.h" 2






extern const char *emulator_printer_directory;
extern const char *emulator_base_directory;
extern const char *cassette_base_directory;



extern char *program_name;
extern int trs_model;
extern int trs_paused;
extern int trs_autodelay;
void trs_suspend_delay(void);
void trs_restore_delay(void);
extern int trs_continuous;


extern int trs_disk_debug_flags;
extern int trs_io_debug_flags;
extern int trs_emtsafe;

extern int trs_video_ram_7_bit;
extern int trs_model1_lowercase;
extern int trs_ram_end;
extern int trs_expansion_interface;

int trs_parse_command_line(int argc, char **argv, int *debug);

void trs_screen_init(void);
void trs_screen_write_char(int position, int char_index);
void trs_screen_expanded(int flag);
void trs_screen_alternate(int flag);
void trs_screen_80x24(int flag);
void trs_screen_inverse(int flag);
void trs_screen_scroll(void);
void trs_screen_refresh(void);

void trs_reset(int poweron);
void trs_exit(void);

void trs_kb_reset(void);
void trs_kb_bracket(int shifted);
int trs_kb_mem_read(int address);
int trs_next_key(int wait);
void trs_kb_heartbeat(void);
void trs_xlate_keysym(int keysym);
void queue_key(int key);
int dequeue_key(void);
void clear_key_queue(void);
void trs_skip_next_kbwait(void);
extern int stretch_amount;

void trs_get_event(int wait);

void trs_x_flush(void);

void trs_printer_write(int value);
int trs_printer_read(void);

void trs_cassette_motor(int value);
void trs_cassette_out(int value);
int trs_cassette_in(void);
void trs_cassette_select(int value);
void trs_sound_out(int value);

int trs_joystick_in(void);

extern int trs_rom_size;
extern int trs_rom1_size;
extern int trs_rom3_size;
extern int trs_rom4p_size;
extern unsigned char trs_rom1[];
extern unsigned char trs_rom3[];
extern unsigned char trs_rom4p[];

extern void trs_load_compiled_rom(int size, unsigned char rom[]);
extern void trs_load_rom(char *filename);

unsigned char trs_interrupt_latch_read(void);
unsigned char trs_nmi_latch_read(void);
void trs_interrupt_mask_write(unsigned char);
void trs_nmi_mask_write(unsigned char);
void trs_reset_button_interrupt(int state);
void trs_disk_intrq_interrupt(int state);
void trs_disk_drq_interrupt(int state);
void trs_disk_motoroff_interrupt(int state);
void trs_uart_err_interrupt(int state);
void trs_uart_rcv_interrupt(int state);
void trs_uart_snd_interrupt(int state);
tstate_t trs_timer_get_period();
void trs_timer_trigger_pulse();
void trs_timer_interrupt(int state);
void trs_timer_init(void);
void trs_timer_off(void);
void trs_timer_on(void);
void trs_timer_speed(int flag);
void trs_cassette_rise_interrupt(int dummy);
void trs_cassette_fall_interrupt(int dummy);
void trs_cassette_clear_interrupts(void);
int trs_cassette_interrupts_enabled(void);
void trs_cassette_update(int dummy);
extern int cassette_default_sample_rate;
void trs_orch90_out(int chan, int value);
void trs_cassette_reset(void);
int trs_cassette_is_motor_on(void);

const char *trs_disk_get_name(int drive);
int trs_disk_set_name(int drive, const char *newname);
int trs_disk_create(const char *newname);
void trs_disk_change_all(void);
void trs_disk_debug(void);
int trs_disk_motoroff(void);

void trs_change_all(void);

void trs_realtime_reset(void);

extern void (*trs_realtime_sync)(tstate_t threhsold);

void trs_realtime_disable();
void trs_realtime_enable();
void trs_realtime_log_status(char ctl);
void trs_realtime_force_enable();
int trs_is_realtime_enabled();

void mem_video_page(int which);
void mem_bank(int which);
void mem_map(int which);
void mem_romin(int state);

void trs_debug(void);

typedef void (*trs_event_func)(int arg);
void trs_schedule_event(trs_event_func f, int arg, int tstates);
void trs_schedule_event_us(trs_event_func f, int arg, int us);
void trs_do_event(void);
void trs_cancel_event(void);
trs_event_func trs_event_scheduled(void);

void grafyx_write_x(int value);
void grafyx_write_y(int value);
void grafyx_write_data(int value);
int grafyx_read_data(void);
void grafyx_write_mode(int value);
void grafyx_write_xoffset(int value);
void grafyx_write_yoffset(int value);
void grafyx_write_overlay(int value);
void grafyx_set_microlabs(int on_off);
int grafyx_get_microlabs(void);
void grafyx_m3_reset();
int grafyx_m3_active();
void grafyx_m3_write_mode(int value);
unsigned char grafyx_m3_read_byte(int position);
int grafyx_m3_write_byte(int position, int value);
void hrg_onoff(int enable);
void hrg_write_addr(int addr, int mask);
void hrg_write_data(int data);
int hrg_read_data(void);

void trs_get_mouse_pos(int *x, int *y, unsigned int *buttons);
void trs_set_mouse_pos(int x, int y);
void trs_get_mouse_max(int *x, int *y, unsigned int *sens);
void trs_set_mouse_max(int x, int y, unsigned int sens);
int trs_get_mouse_type(void);

void stringy_init(void);
const char *stringy_get_name(int unit);
int stringy_set_name(int unit, const char *name);
int stringy_create(const char *name);
int stringy_in(int unit);
void stringy_out(int unit, int value);
void stringy_reset(void);
void stringy_change_all(void);

int put_twobyte(Ushort n, FILE* f);
int put_fourbyte(Uint n, FILE* f);
int get_twobyte(Ushort *n, FILE* f);
int get_fourbyte(Uint *n, FILE* f);

void trs_wait_for_all_keys_up();


typedef struct joshem_modal_context {
   void * input;
   int result;
} joshem_modal_context;

typedef void (*joshem_modal_handler)(joshem_modal_context*);


int joshem_do_modal(joshem_modal_handler handler, void *input);

int joshem_modal_ask_yn(const char *pPrompt);
void joshem_modal_message(const char *pPrompt);


typedef char cassette_filename_buffer [1024];

typedef struct joshem_cassette_control_args {
   cassette_filename_buffer cassette_filename;
   int cassette_position;
   int cassette_format;
   int cassette_writable;
   int write_requested;
   int initial_selection;
   int view_current_status;
} joshem_cassette_control_args;



void joshem_cassette_control(joshem_cassette_control_args *pArgs);


void joshem_request_tapedialog();
void joshem_request_tapedialog_status();
# 254 "/home/arnold/scm/xtrs/trs.h"
int joshem_emulator_control();
void trs_ich_setup();
# 51 "/home/arnold/scm/xtrs/trs_djgpp.c" 2

# 1 "/home/arnold/scm/xtrs/trs_disk.h" 1
# 13 "/home/arnold/scm/xtrs/trs_disk.h"
void trs_disk_init(void);
void trs_disk_reset(void);
void trs_disk_select_write(unsigned char data);
unsigned char trs_disk_track_read(void);
void trs_disk_track_write(unsigned char data);
unsigned char trs_disk_sector_read(void);
void trs_disk_sector_write(unsigned char data);
unsigned char trs_disk_data_read(void);
void trs_disk_data_write(unsigned char data);
unsigned char trs_disk_status_read(void);
void trs_disk_command_write(unsigned char cmd);
unsigned char trs_disk_interrupt_read(void);
void trs_disk_interrupt_write(unsigned char mask);

void trs_disk_setstep(int unit, int value);
int trs_disk_getstep(int unit);
void trs_disk_setsize(int unit, int value);
int trs_disk_getsize(int unit);

extern int trs_disk_doubler;
extern char* trs_disk_dir;
extern unsigned short trs_changecount;
extern int trs_disk_truedam;
# 53 "/home/arnold/scm/xtrs/trs_djgpp.c" 2
# 1 "/home/arnold/scm/xtrs/trs_uart.h" 1
# 15 "/home/arnold/scm/xtrs/trs_uart.h"
# 1 "/home/arnold/scm/xtrs/trs_hard.h" 1
# 18 "/home/arnold/scm/xtrs/trs_hard.h"
void trs_hard_init(void);
void trs_hard_reset(void);
void trs_hard_change_all(void);
const char *trs_hard_get_name(int drive);
int trs_hard_set_name(int drive, const char *name);
int trs_hard_create(const char *name);
int trs_hard_in(int port);
void trs_hard_out(int port, int value);
extern char *trs_disk_dir;
# 16 "/home/arnold/scm/xtrs/trs_uart.h" 2

extern void trs_uart_init(int reset_button);
extern int trs_uart_check_avail();
extern int trs_uart_modem_in();
extern void trs_uart_reset_out(int value);
extern int trs_uart_switches_in();
extern void trs_uart_baud_out(int value);
extern int trs_uart_status_in();
extern void trs_uart_control_out(int value);
extern int trs_uart_data_in();
extern void trs_uart_data_out(int value);
extern char *trs_uart_name;
extern int trs_uart_switches;
# 54 "/home/arnold/scm/xtrs/trs_djgpp.c" 2
# 1 "/home/arnold/scm/xtrs/trs_imp_exp.h" 1
# 247 "/home/arnold/scm/xtrs/trs_imp_exp.h"
void do_emt_system();
void do_emt_getddir();
void do_emt_setddir();
void do_emt_mouse();
void do_emt_open();
void do_emt_close();
void do_emt_read();
void do_emt_write();
void do_emt_lseek();
void do_emt_strerror();
void do_emt_time();
void do_emt_opendir();
void do_emt_closedir();
void do_emt_readdir();
void do_emt_chdir();
void do_emt_getcwd();
void do_emt_misc();
void do_emt_ftruncate();
void do_emt_opendisk();
void do_emt_closedisk();
# 55 "/home/arnold/scm/xtrs/trs_djgpp.c" 2
# 1 "/home/arnold/scm/xtrs/keytrap/scanbuf.h" 1





struct scan_buffer {
    unsigned char suppress_flag;
    unsigned char filler0[7];
    unsigned char next_offset;
    unsigned char filler1[7];
    unsigned char key_ring[256];
    unsigned char key_states[128];
};
# 56 "/home/arnold/scm/xtrs/trs_djgpp.c" 2

# 1 "/usr/i686-pc-msdosdjgpp/sys-include/dpmi.h" 1 3 4
# 23 "/usr/i686-pc-msdosdjgpp/sys-include/dpmi.h" 3 4

# 23 "/usr/i686-pc-msdosdjgpp/sys-include/dpmi.h" 3 4
extern unsigned short __dpmi_error;

typedef struct {
  unsigned short offset16;
  unsigned short segment;
} __dpmi_raddr;

typedef struct {
  unsigned long offset32;
  unsigned short selector;
} __dpmi_paddr;

typedef struct {
  unsigned long handle;
  unsigned long size;
  unsigned long address;
} __dpmi_meminfo;

typedef union {
  struct {
    unsigned long edi;
    unsigned long esi;
    unsigned long ebp;
    unsigned long res;
    unsigned long ebx;
    unsigned long edx;
    unsigned long ecx;
    unsigned long eax;
  } d;
  struct {
    unsigned short di, di_hi;
    unsigned short si, si_hi;
    unsigned short bp, bp_hi;
    unsigned short res, res_hi;
    unsigned short bx, bx_hi;
    unsigned short dx, dx_hi;
    unsigned short cx, cx_hi;
    unsigned short ax, ax_hi;
    unsigned short flags;
    unsigned short es;
    unsigned short ds;
    unsigned short fs;
    unsigned short gs;
    unsigned short ip;
    unsigned short cs;
    unsigned short sp;
    unsigned short ss;
  } x;
  struct {
    unsigned char edi[4];
    unsigned char esi[4];
    unsigned char ebp[4];
    unsigned char res[4];
    unsigned char bl, bh, ebx_b2, ebx_b3;
    unsigned char dl, dh, edx_b2, edx_b3;
    unsigned char cl, ch, ecx_b2, ecx_b3;
    unsigned char al, ah, eax_b2, eax_b3;
  } h;
} __dpmi_regs;

typedef struct {
  unsigned char major;
  unsigned char minor;
  unsigned short flags;
  unsigned char cpu;
  unsigned char master_pic;
  unsigned char slave_pic;
} __dpmi_version_ret;

typedef struct {
  unsigned long largest_available_free_block_in_bytes;
  unsigned long maximum_unlocked_page_allocation_in_pages;
  unsigned long maximum_locked_page_allocation_in_pages;
  unsigned long linear_address_space_size_in_pages;
  unsigned long total_number_of_unlocked_pages;
  unsigned long total_number_of_free_pages;
  unsigned long total_number_of_physical_pages;
  unsigned long free_linear_address_space_in_pages;
  unsigned long size_of_paging_file_partition_in_pages;
  unsigned long reserved[3];
} __dpmi_free_mem_info;

typedef struct {
  unsigned long total_allocated_bytes_of_physical_memory_host;
  unsigned long total_allocated_bytes_of_virtual_memory_host;
  unsigned long total_available_bytes_of_virtual_memory_host;
  unsigned long total_allocated_bytes_of_virtual_memory_vcpu;
  unsigned long total_available_bytes_of_virtual_memory_vcpu;
  unsigned long total_allocated_bytes_of_virtual_memory_client;
  unsigned long total_available_bytes_of_virtual_memory_client;
  unsigned long total_locked_bytes_of_memory_client;
  unsigned long max_locked_bytes_of_memory_client;
  unsigned long highest_linear_address_available_to_client;
  unsigned long size_in_bytes_of_largest_free_memory_block;
  unsigned long size_of_minimum_allocation_unit_in_bytes;
  unsigned long size_of_allocation_alignment_unit_in_bytes;
  unsigned long reserved[19];
} __dpmi_memory_info;

typedef struct {
  unsigned long data16[2];
  unsigned long code16[2];
  unsigned short ip;
  unsigned short reserved;
  unsigned long data32[2];
  unsigned long code32[2];
  unsigned long eip;
} __dpmi_callback_info;

typedef struct {
  unsigned long size_requested;
  unsigned long size;
  unsigned long handle;
  unsigned long address;
  unsigned long name_offset;
  unsigned short name_selector;
  unsigned short reserved1;
  unsigned long reserved2;
} __dpmi_shminfo;



void __dpmi_yield(void);

int __dpmi_allocate_ldt_descriptors(int _count);
int __dpmi_free_ldt_descriptor(int _descriptor);
int __dpmi_segment_to_descriptor(int _segment);
int __dpmi_get_selector_increment_value(void);
int __dpmi_get_segment_base_address(int _selector, unsigned long *_addr);
int __dpmi_set_segment_base_address(int _selector, unsigned long _address);
unsigned long __dpmi_get_segment_limit(int _selector);
int __dpmi_set_segment_limit(int _selector, unsigned long _limit);
int __dpmi_get_descriptor_access_rights(int _selector);
int __dpmi_set_descriptor_access_rights(int _selector, int _rights);
int __dpmi_create_alias_descriptor(int _selector);
int __dpmi_get_descriptor(int _selector, void *_buffer);
int __dpmi_set_descriptor(int _selector, void *_buffer);
int __dpmi_allocate_specific_ldt_descriptor(int _selector);

int __dpmi_get_multiple_descriptors(int _count, void *_buffer);
int __dpmi_set_multiple_descriptors(int _count, void *_buffer);

int __dpmi_allocate_dos_memory(int _paragraphs, int *_ret_selector_or_max);
int __dpmi_free_dos_memory(int _selector);
int __dpmi_resize_dos_memory(int _selector, int _newpara, int *_ret_max);

int __dpmi_get_real_mode_interrupt_vector(int _vector, __dpmi_raddr *_address);
int __dpmi_set_real_mode_interrupt_vector(int _vector, __dpmi_raddr *_address);
int __dpmi_get_processor_exception_handler_vector(int _vector, __dpmi_paddr *_address);
int __dpmi_set_processor_exception_handler_vector(int _vector, __dpmi_paddr *_address);
int __dpmi_get_protected_mode_interrupt_vector(int _vector, __dpmi_paddr *_address);
int __dpmi_set_protected_mode_interrupt_vector(int _vector, __dpmi_paddr *_address);

int __dpmi_get_extended_exception_handler_vector_pm(int _vector, __dpmi_paddr *_address);
int __dpmi_get_extended_exception_handler_vector_rm(int _vector, __dpmi_paddr *_address);
int __dpmi_set_extended_exception_handler_vector_pm(int _vector, __dpmi_paddr *_address);
int __dpmi_set_extended_exception_handler_vector_rm(int _vector, __dpmi_paddr *_address);

int __dpmi_simulate_real_mode_interrupt(int _vector, __dpmi_regs *_regs);
int __dpmi_int(int _vector, __dpmi_regs *_regs);
extern short __dpmi_int_ss, __dpmi_int_sp, __dpmi_int_flags;
int __dpmi_simulate_real_mode_procedure_retf(__dpmi_regs *_regs);
int __dpmi_simulate_real_mode_procedure_retf_stack(__dpmi_regs *_regs, int stack_words_to_copy, const void *stack_data);
int __dpmi_simulate_real_mode_procedure_iret(__dpmi_regs *_regs);
int __dpmi_allocate_real_mode_callback(void (*_handler)(void), __dpmi_regs *_regs, __dpmi_raddr *_ret);
int __dpmi_free_real_mode_callback(__dpmi_raddr *_addr);
int __dpmi_get_state_save_restore_addr(__dpmi_raddr *_rm, __dpmi_paddr *_pm);
int __dpmi_get_raw_mode_switch_addr(__dpmi_raddr *_rm, __dpmi_paddr *_pm);

int __dpmi_get_version(__dpmi_version_ret *_ret);

int __dpmi_get_capabilities(int *_flags, char *vendor_info);

int __dpmi_get_free_memory_information(__dpmi_free_mem_info *_info);
int __dpmi_allocate_memory(__dpmi_meminfo *_info);
int __dpmi_free_memory(unsigned long _handle);
int __dpmi_resize_memory(__dpmi_meminfo *_info);

int __dpmi_allocate_linear_memory(__dpmi_meminfo *_info, int _commit);
int __dpmi_resize_linear_memory(__dpmi_meminfo *_info, int _commit);
int __dpmi_get_page_attributes(__dpmi_meminfo *_info, short *_buffer);
int __dpmi_set_page_attributes(__dpmi_meminfo *_info, short *_buffer);
int __dpmi_map_device_in_memory_block(__dpmi_meminfo *_info, unsigned long _physaddr);
int __dpmi_map_conventional_memory_in_memory_block(__dpmi_meminfo *_info, unsigned long _physaddr);
int __dpmi_get_memory_block_size_and_base(__dpmi_meminfo *_info);
int __dpmi_get_memory_information(__dpmi_memory_info *_buffer);

int __dpmi_lock_linear_region(__dpmi_meminfo *_info);
int __dpmi_unlock_linear_region(__dpmi_meminfo *_info);
int __dpmi_mark_real_mode_region_as_pageable(__dpmi_meminfo *_info);
int __dpmi_relock_real_mode_region(__dpmi_meminfo *_info);
int __dpmi_get_page_size(unsigned long *_size);

int __dpmi_mark_page_as_demand_paging_candidate(__dpmi_meminfo *_info);
int __dpmi_discard_page_contents(__dpmi_meminfo *_info);

int __dpmi_physical_address_mapping(__dpmi_meminfo *_info);
int __dpmi_free_physical_address_mapping(__dpmi_meminfo *_info);


int __dpmi_get_and_disable_virtual_interrupt_state(void);
int __dpmi_get_and_enable_virtual_interrupt_state(void);
int __dpmi_get_and_set_virtual_interrupt_state(int _old_state);
int __dpmi_get_virtual_interrupt_state(void);

int __dpmi_get_vendor_specific_api_entry_point(char *_id, __dpmi_paddr *_api);

int __dpmi_set_debug_watchpoint(__dpmi_meminfo *_info, int _type);
int __dpmi_clear_debug_watchpoint(unsigned long _handle);
int __dpmi_get_state_of_debug_watchpoint(unsigned long _handle, int *_status);
int __dpmi_reset_debug_watchpoint(unsigned long _handle);

int __dpmi_install_resident_service_provider_callback(__dpmi_callback_info *_info);
int __dpmi_terminate_and_stay_resident(int return_code, int paragraphs_to_keep);

int __dpmi_allocate_shared_memory(__dpmi_shminfo *_info);
int __dpmi_free_shared_memory(unsigned long _handle);
int __dpmi_serialize_on_shared_memory(unsigned long _handle, int _flags);
int __dpmi_free_serialization_on_shared_memory(unsigned long _handle, int _flags);

int __dpmi_get_coprocessor_status(void);
int __dpmi_set_coprocessor_emulation(int _flags);






typedef struct {
  unsigned long available_memory;
  unsigned long available_pages;
  unsigned long available_lockable_pages;
  unsigned long linear_space;
  unsigned long unlocked_pages;
  unsigned long available_physical_pages;
  unsigned long total_physical_pages;
  unsigned long free_linear_space;
  unsigned long max_pages_in_paging_file;
  unsigned long reserved[3];
} _go32_dpmi_meminfo;







typedef struct {
  unsigned long size;
  unsigned long pm_offset;
  unsigned short pm_selector;
  unsigned short rm_offset;
  unsigned short rm_segment;
} _go32_dpmi_seginfo;


int _go32_dpmi_allocate_dos_memory(_go32_dpmi_seginfo *info);



int _go32_dpmi_free_dos_memory(_go32_dpmi_seginfo *info);


int _go32_dpmi_resize_dos_memory(_go32_dpmi_seginfo *info);



int _go32_dpmi_get_real_mode_interrupt_vector(int vector, _go32_dpmi_seginfo *info);
int _go32_dpmi_set_real_mode_interrupt_vector(int vector, _go32_dpmi_seginfo *info);




int _go32_dpmi_get_protected_mode_interrupt_vector(int vector, _go32_dpmi_seginfo *info);

int _go32_dpmi_set_protected_mode_interrupt_vector(int vector, _go32_dpmi_seginfo *info);




int _go32_dpmi_chain_protected_mode_interrupt_vector(int vector, _go32_dpmi_seginfo *info);




int _go32_dpmi_allocate_iret_wrapper(_go32_dpmi_seginfo *info);

int _go32_dpmi_free_iret_wrapper(_go32_dpmi_seginfo *info);





int _go32_dpmi_allocate_real_mode_callback_retf(_go32_dpmi_seginfo *info, __dpmi_regs *regs);



int _go32_dpmi_allocate_real_mode_callback_iret(_go32_dpmi_seginfo *info, __dpmi_regs *regs);

int _go32_dpmi_free_real_mode_callback(_go32_dpmi_seginfo *info);






extern unsigned long _go32_interrupt_stack_size;
extern unsigned long _go32_rmcb_stack_size;


unsigned long _go32_dpmi_remaining_physical_memory(void);
unsigned long _go32_dpmi_remaining_virtual_memory(void);


int _go32_dpmi_lock_code( void *_lockaddr, unsigned long _locksize);
int _go32_dpmi_lock_data( void *_lockaddr, unsigned long _locksize);

int __djgpp_set_page_attributes(void *our_addr, unsigned long num_bytes,
           unsigned short attributes);
int __djgpp_map_physical_memory(void *our_addr, unsigned long num_bytes,
           unsigned long phys_addr);
# 58 "/home/arnold/scm/xtrs/trs_djgpp.c" 2

# 1 "/home/arnold/scm/xtrs/trs_djgpp.h" 1
# 10 "/home/arnold/scm/xtrs/trs_djgpp.h"

# 10 "/home/arnold/scm/xtrs/trs_djgpp.h"
extern GrColor COLOR_BORDER;
extern GrColor COLOR_PRIMARY;
extern GrColor COLOR_PRIMARY_DIM;
extern GrColor COLOR_SECONDARY;
extern GrColor COLOR_SECONDARY_BRIGHT;
extern GrColor COLOR_TERTIARY;
extern GrColor COLOR_DISABLED;
# 60 "/home/arnold/scm/xtrs/trs_djgpp.c" 2
# 1 "/home/arnold/scm/xtrs/trs_vga.h" 1





void vga_needs_reset();
void vga_screen_write_glyph_64_16(char *glyphRows, int position);
void vga_screen_scroll_64_16();
# 61 "/home/arnold/scm/xtrs/trs_djgpp.c" 2

GrColor COLOR_BORDER;
GrColor COLOR_PRIMARY;
GrColor COLOR_PRIMARY_DIM;
GrColor COLOR_SECONDARY;
GrColor COLOR_DISABLED;
GrColor COLOR_SECONDARY_BRIGHT;
GrColor COLOR_TERTIARY;

int trs_model1_lowercase = 0;

const char *emulator_base_directory = 0;
const char *cassette_base_directory = 0;
const char *emulator_printer_directory =0;





static GrPattern trs_screen_pattern;


static unsigned char trs_screen[2048];
static int screen_chars = 1024;
static int row_chars = 64;



static int scale_x = 1;
static int scale_y = 0;
static int resize = -1;
static int grafyx_microlabs = 0;
static int border_width = 2;
static int cur_char_height, cur_char_width;
static int trs_charset;

static struct scan_buffer *pScanBuffer = 0;
static unsigned char scanBufferCursor;






static trs_pattern_table *p_current_table;

static trs_pattern_table primary_pattern_table;

static int currentmode = 0;

static void trs_load_romfile();



static char reverse_bits(char c) {
    char r = 0;
    for (int i = 0; i < 8; i++) {
        r = (r << 1) | (c & 1);
        c >>= 1;
    }
    return r;
}

void reverse_bits_block(void *p, int len) {
    char *c = (char *) p;
    for (; len > 0; c++, len--) {
        *c = reverse_bits(*c);
    }
}


void rotate_left_block(void *p, int steps, int len) {
    char *c = (char *) p;





    for (; len > 0; c++, len--) {

        asm ( "rolb %%cl, (%0)"
                :
                : "r" (c), "c" (steps)
                : "cc"
                );
    }
}

static char expand_3to6bit(char c, int bitoffset) {
    char res;
    int i;
    int bt;
    res = 0;

    for (i = 0; i < 6; i++) {
        bt = bitoffset + (i / 2);
        if (c & (1 << bt)) {
            res |= (1 << i);
        }
    }

    return res;
}

static void expand_3to6bit_block(void *p, int bitoffset, int len) {
    char *c = (char *) p;
    for (; len > 0; c++, len--) {
        *c = expand_3to6bit(*c, bitoffset);
    }
}

static void not_implemented(const char *msg, int *counter) {
    int cnt;
    cnt = counter ? (*counter) : -1;
    if (cnt < 3) {
        joshlog("Not implemented: %s %d\n", msg, cnt);
        cnt++;
        if (counter) {
            *counter = cnt;
        }
    }


}



extern void trs_xlate_pc_scancode(unsigned char scan_code, int shifted);


void trs_get_event(int wait) {


    static int nest_count = 0;
# 204 "/home/arnold/scm/xtrs/trs_djgpp.c"
    while (pScanBuffer->next_offset != scanBufferCursor) {
        unsigned char keycode = pScanBuffer->key_ring[scanBufferCursor++];
        int ignoreKey = 0;
        if (nest_count == 0) {
            nest_count++;
            if (keycode == 0x3e) {
                ignoreKey = 1;
                if (joshem_modal_ask_yn("Exit Simulator?")) {
                    exit(0);
                }
            } else if (keycode == 0x3F) {

                 ignoreKey = 1;
# 226 "/home/arnold/scm/xtrs/trs_djgpp.c"
                joshem_modal_message("Tracing not supported in this build");
                josh_trace_enabled = 0;
            } else if (keycode == 0x40) {
                ignoreKey = 1;

            } else if (keycode == 0x41) {
                ignoreKey = 1;

            } else if (keycode == 0x42) {
                ignoreKey = 1;
                joshem_request_tapedialog_status();

            } else if (keycode == 0x43) {
                ignoreKey = 1;
                const char *p = trs_is_realtime_enabled() ? "Fast Mode is OFF.  Turn it on?"
                                                          : "Fast mode is ON.  Leave it on?";
                if (joshem_modal_ask_yn(p)) {
                    trs_realtime_disable();
                } else {
                    trs_realtime_force_enable();
                }
            } else if (keycode == 0x44) {
                ignoreKey = 1;
                int eec = joshem_emulator_control();
                if (eec == 1) {
                    exit(0);
                } else if (eec == 2) {
                    trs_reset(1);
                } else if (eec == 3) {
                    trs_reset(0);
                }
            }
            nest_count--;
        }
        if (ignoreKey) {
            continue;
        }

        int shifted = pScanBuffer->key_states[0x2A] || pScanBuffer->key_states[0x36];

        trs_xlate_pc_scancode(keycode, shifted);

    }


}

static void repaint_screen() {

    vga_needs_reset();




    for (int i = 0; i < screen_chars; i++) {
        trs_screen_write_char(i, trs_screen[i]);
    }
}

void trs_exit() {
    exit(0);
}

static void reload_grx_colors() {
    GrResetColors();
    COLOR_BORDER = GrAllocColor(0, 0, 192);
    COLOR_PRIMARY = GrAllocColor(0, 192, 0);
    COLOR_PRIMARY_DIM = GrAllocColor(0, 96, 0);
    COLOR_SECONDARY = GrAllocColor(192, 0, 0);
    COLOR_DISABLED = GrAllocColor(64,64,64);
    COLOR_SECONDARY_BRIGHT = GrAllocColor(255,255,255);
    COLOR_TERTIARY = GrAllocColor(127, 127, 0);

}



void trs_screen_init() {
    init_pattern_table(&primary_pattern_table, trs_char_data[1], 1);
    memset(trs_screen, 32, sizeof(trs_screen));



    GrSetDriver("stdvga");
# 362 "/home/arnold/scm/xtrs/trs_djgpp.c"
    GrSetMode(GR_width_height_graphics, 640, 200);
    joshlog("Video Driver is %s %d\n", ((const GrVideoDriver *)( GrDriverInfo->vdriver))->name, (int)(GrDriverInfo->vdriver ? GrDriverInfo->vdriver->adapter : GR_UNKNOWN));

    int x, y;
    x = (((GrContext *)(&GrContextInfo->current))->gc_xmax) / 2;
    y = (((GrContext *)(&GrContextInfo->current))->gc_ymax) / 2;
    joshlog("Midpoint: %d %d\n", x, y);


    trs_screen_pattern.gp_bitmap.bmp_ispixmap = 0;
    trs_screen_pattern.gp_bitmap.bmp_height = 12;
    trs_screen_pattern.gp_bitmap.bmp_data = 0;
    trs_screen_pattern.gp_bitmap.bmp_fgcolor = ( (GrColorInfo->white == (0x01000000UL | 0)) ? (GrWhite)() : GrColorInfo->white );
    trs_screen_pattern.gp_bitmap.bmp_bgcolor = ( (GrColorInfo->black == (0x01000000UL | 0)) ? (GrBlack)() : GrColorInfo->black );
    trs_screen_pattern.gp_bitmap.bmp_memflags = 0;
    reload_grx_colors();
    repaint_screen();
    trs_load_romfile();

    return;


}

void trs_screen_expanded(int flag) {
    int bit = flag ? 1 : 0;
    if ((currentmode ^ bit) & 1) {
        currentmode ^= 1;
        repaint_screen();
    }
}

void trs_screen_alternate(int flag) {
    static int nicounter = 0;
    not_implemented("trs_screen_alternate", &nicounter);
}

void trs_screen_80x24(int flag) {
    static int nicounter = 0;
    not_implemented("trs_screen_80x24", &nicounter);
}

void trs_screen_inverse(int flag) {
    static int nicounter = 0;
    not_implemented("trs_screen_inverse", &nicounter);
}

void trs_screen_scroll() {
# 418 "/home/arnold/scm/xtrs/trs_djgpp.c"
    trs_realtime_sync(5000);
    memmove(trs_screen, trs_screen + row_chars, screen_chars - row_chars);

    vga_screen_scroll_64_16();



}







static void trs_screen_write_glyph(char *glyphRows, int position) {

    char patData[12];
    memcpy(patData, glyphRows, 12);
    rotate_left_block(patData, (position & 3) << 1, 12);
# 448 "/home/arnold/scm/xtrs/trs_djgpp.c"
    trs_screen_pattern.gp_bitmap.bmp_data = patData;

    int x, y;

    x = (position & 63);
    y = position >> 6;
    int px, py;
    px = x * 6 + 120;
    py = y * 12 + 0;


    GrPatternFilledBox(px, py, px + 5, py + 12 - 1, &trs_screen_pattern);
    trs_screen_pattern.gp_bitmap.bmp_data = 0;
}

void trs_screen_write_char(int position, int char_index) {
# 482 "/home/arnold/scm/xtrs/trs_djgpp.c"
    char_index = char_index & 0xff;

    position = position & 1023;
    trs_screen[position] = (char) char_index;

    if (!(currentmode & 1)) {
        vga_screen_write_glyph_64_16(pattern_table_1[char_index], position);
    } else {
        if (position & 1) {
            return;
        }
        vga_screen_write_glyph_64_16(pattern_table_1_wideleft[char_index], position);
        vga_screen_write_glyph_64_16(pattern_table_1_wideright[char_index], position | 1);
    }
    return;
}


void trs_get_mouse_pos(int *x, int *y, unsigned int *buttons) {
    static int nicounter = 0;
    not_implemented("trs_get_mouse_pos", &nicounter);
}

void trs_set_mouse_pos(int x, int y) {
    static int nicounter = 0;
    not_implemented("trs_set_mouse_pos", &nicounter);
}

void trs_get_mouse_max(int *x, int *y, unsigned int *sens) {
    static int nicounter = 0;
    not_implemented("trs_get_mouse_max", &nicounter);
}

void trs_set_mouse_max(int x, int y, unsigned int sens) {
    static int nicounter = 0;
    not_implemented("trs_set_mouse_max", &nicounter);
}

int trs_get_mouse_type() {
    static int nicounter = 0;
    not_implemented("trs_get_mouse_type", &nicounter);
    return 0;
}


void grafyx_write_byte(int x, int y, char byte) {
    static int nicounter = 0;
    not_implemented("grafyx_write_byte", &nicounter); }

void grafyx_write_x(int value) {
    static int nicounter = 0;
    not_implemented("grafyx_write_x", &nicounter); }

void grafyx_write_y(int value) {
    static int nicounter = 0;
    not_implemented("grafyx_write_y", &nicounter); }

void grafyx_write_data(int value) {
    static int nicounter = 0;
    not_implemented("grafyx_write_data", &nicounter); }

int grafyx_read_data() {
    static int nicounter = 0;
    not_implemented("grafyx_read_data", &nicounter);
    return 0;
}

void grafyx_write_mode(int value) {
    static int nicounter = 0;
    not_implemented("grafyx_write_mode", &nicounter); }

void grafyx_write_xoffset(int value) {
    static int nicounter = 0;
    not_implemented("grafyx_write_xoffset", &nicounter); }

void grafyx_write_yoffset(int value) {
    static int nicounter = 0;
    not_implemented("grafyx_write_yoffset", &nicounter); }

void grafyx_write_overlay(int value) {
    static int nicounter = 0;
    not_implemented("grafyx_write_overlay", &nicounter); }

int grafyx_get_microlabs() {
    static int nicounter = 0;
    not_implemented("grafyx_get_microlabs", &nicounter);
    return 0;
}

void grafyx_set_microlabs(int on_off) {
    static int nicounter = 0;
    not_implemented("grafyx_set_microlabs", &nicounter);
}

void grafyx_m3_reset() {
    static int nicounter = 0;
    not_implemented("grafyx_m3_reset", &nicounter); }

void grafyx_m3_write_mode(int value) {
    static int nicounter = 0;
    not_implemented("grafyx_m3_write_mode", &nicounter); }

int grafyx_m3_write_byte(int position, int byte) {
    static int nicounter = 0;
    not_implemented("grafyx_m3_write_byte", &nicounter);
    return 0;
}

unsigned char grafyx_m3_read_byte(int position) {
    static int nicounter = 0;
    not_implemented("grafyx_m3_read_byte", &nicounter);
    return 0;
}

int grafyx_m3_active() {
    static int nicounter = 0;
    not_implemented("grafyx_m3_active", &nicounter);
    return 0;
}

int hrg_read_data() {
    static int nicounter = 0;
    not_implemented("hrg_read_data", &nicounter);
    return 0;
}

void hrg_write_addr(int addr, int mask) {
    static int nicounter = 0;
    not_implemented("hrg_write_addr", &nicounter); }

void hrg_write_data(int data) {
    static int nicounter = 0;
    not_implemented("hrg_write_data", &nicounter); }

void hrg_onoff(int enable) {
    static int nicounter = 0;
    not_implemented("hrg_onoff", &nicounter); }
# 627 "/home/arnold/scm/xtrs/trs_djgpp.c"
int opt_iconic = (0);
char *opt_background = 
# 628 "/home/arnold/scm/xtrs/trs_djgpp.c" 3 4
                      ((void*)0)
# 628 "/home/arnold/scm/xtrs/trs_djgpp.c"
                          ;
char *opt_foreground = 
# 629 "/home/arnold/scm/xtrs/trs_djgpp.c" 3 4
                      ((void*)0)
# 629 "/home/arnold/scm/xtrs/trs_djgpp.c"
                          ;
int opt_debug = (0);
char *opt_title;
int opt_shiftbracket = -1;
char *opt_charset = 
# 633 "/home/arnold/scm/xtrs/trs_djgpp.c" 3 4
                   ((void*)0)
# 633 "/home/arnold/scm/xtrs/trs_djgpp.c"
                       ;
char *opt_scale = 
# 634 "/home/arnold/scm/xtrs/trs_djgpp.c" 3 4
                 ((void*)0)
# 634 "/home/arnold/scm/xtrs/trs_djgpp.c"
                     ;
char *opt_romfile = 
# 635 "/home/arnold/scm/xtrs/trs_djgpp.c" 3 4
                   ((void*)0)
# 635 "/home/arnold/scm/xtrs/trs_djgpp.c"
                       ;
char *opt_romfile3 = 
# 636 "/home/arnold/scm/xtrs/trs_djgpp.c" 3 4
                    ((void*)0)
# 636 "/home/arnold/scm/xtrs/trs_djgpp.c"
                        ;
char *opt_romfile4p = 
# 637 "/home/arnold/scm/xtrs/trs_djgpp.c" 3 4
                     ((void*)0)
# 637 "/home/arnold/scm/xtrs/trs_djgpp.c"
                         ;
int opt_stepdefault = 1;
char *opt_stepmap = 
# 639 "/home/arnold/scm/xtrs/trs_djgpp.c" 3 4
                   ((void*)0)
# 639 "/home/arnold/scm/xtrs/trs_djgpp.c"
                       ;
char *opt_sizemap = 
# 640 "/home/arnold/scm/xtrs/trs_djgpp.c" 3 4
                   ((void*)0)
# 640 "/home/arnold/scm/xtrs/trs_djgpp.c"
                       ;

struct option {
    const char *name;
    int has_arg;
    int *flag;
    int val;
};

struct option options[] = {

        {"iconic", (0), &opt_iconic, (1)},
        {"noiconic", (0), &opt_iconic, (0)},
        {"background", (1), 
# 653 "/home/arnold/scm/xtrs/trs_djgpp.c" 3 4
                                ((void*)0)
# 653 "/home/arnold/scm/xtrs/trs_djgpp.c"
                                    , 0},
        {"bg", (1), 
# 654 "/home/arnold/scm/xtrs/trs_djgpp.c" 3 4
                                ((void*)0)
# 654 "/home/arnold/scm/xtrs/trs_djgpp.c"
                                    , 0},
        {"foreground", (1), 
# 655 "/home/arnold/scm/xtrs/trs_djgpp.c" 3 4
                                ((void*)0)
# 655 "/home/arnold/scm/xtrs/trs_djgpp.c"
                                    , 0},
        {"fg", (1), 
# 656 "/home/arnold/scm/xtrs/trs_djgpp.c" 3 4
                                ((void*)0)
# 656 "/home/arnold/scm/xtrs/trs_djgpp.c"
                                    , 0},
        {"title", (1), 
# 657 "/home/arnold/scm/xtrs/trs_djgpp.c" 3 4
                                ((void*)0)
# 657 "/home/arnold/scm/xtrs/trs_djgpp.c"
                                    , 0},
        {"borderwidth", (1), 
# 658 "/home/arnold/scm/xtrs/trs_djgpp.c" 3 4
                                ((void*)0)
# 658 "/home/arnold/scm/xtrs/trs_djgpp.c"
                                    , 0},
        {"scale", (1), 
# 659 "/home/arnold/scm/xtrs/trs_djgpp.c" 3 4
                                ((void*)0)
# 659 "/home/arnold/scm/xtrs/trs_djgpp.c"
                                    , 0},
        {"scale1", (0), &scale_x, 1},
        {"scale2", (0), &scale_x, 2},
        {"scale3", (0), &scale_x, 3},
        {"scale4", (0), &scale_x, 4},
        {"resize", (0), &resize, (1)},
        {"noresize", (0), &resize, (0)},
        {"charset", (1), 
# 666 "/home/arnold/scm/xtrs/trs_djgpp.c" 3 4
                                ((void*)0)
# 666 "/home/arnold/scm/xtrs/trs_djgpp.c"
                                    , 0},
        {"microlabs", (0), &grafyx_microlabs, (1)},
        {"nomicrolabs", (0), &grafyx_microlabs, (0)},
        {"debug", (0), &opt_debug, (1)},
        {"nodebug", (0), &opt_debug, (0)},
        {"romfile", (1), 
# 671 "/home/arnold/scm/xtrs/trs_djgpp.c" 3 4
                                ((void*)0)
# 671 "/home/arnold/scm/xtrs/trs_djgpp.c"
                                    , 0},
        {"romfile3", (1), 
# 672 "/home/arnold/scm/xtrs/trs_djgpp.c" 3 4
                                ((void*)0)
# 672 "/home/arnold/scm/xtrs/trs_djgpp.c"
                                    , 0},
        {"romfile4p", (1), 
# 673 "/home/arnold/scm/xtrs/trs_djgpp.c" 3 4
                                ((void*)0)
# 673 "/home/arnold/scm/xtrs/trs_djgpp.c"
                                    , 0},
        {"model", (1), 
# 674 "/home/arnold/scm/xtrs/trs_djgpp.c" 3 4
                                ((void*)0)
# 674 "/home/arnold/scm/xtrs/trs_djgpp.c"
                                    , 0},
        {"model1", (0), &trs_model, 1},
        {"model3", (0), &trs_model, 3},
        {"model4", (0), &trs_model, 4},
        {"model4p", (0), &trs_model, 5},
        {"delay", (1), 
# 679 "/home/arnold/scm/xtrs/trs_djgpp.c" 3 4
                                ((void*)0)
# 679 "/home/arnold/scm/xtrs/trs_djgpp.c"
                                    , 0},
        {"autodelay", (0), &trs_autodelay, (1)},
        {"noautodelay", (0), &trs_autodelay, (0)},
        {"keystretch", (1), 
# 682 "/home/arnold/scm/xtrs/trs_djgpp.c" 3 4
                                ((void*)0)
# 682 "/home/arnold/scm/xtrs/trs_djgpp.c"
                                    , 0},
        {"shiftbracket", (0), &opt_shiftbracket, (1)},
        {"noshiftbracket", (0), &opt_shiftbracket, (0)},
        {"diskdir", (1), 
# 685 "/home/arnold/scm/xtrs/trs_djgpp.c" 3 4
                                ((void*)0)
# 685 "/home/arnold/scm/xtrs/trs_djgpp.c"
                                    , 0},
        {"doubler", (1), 
# 686 "/home/arnold/scm/xtrs/trs_djgpp.c" 3 4
                                ((void*)0)
# 686 "/home/arnold/scm/xtrs/trs_djgpp.c"
                                    , 0},
        {"doublestep", (0), &opt_stepdefault, 2},
        {"nodoublestep", (0), &opt_stepdefault, 1},
        {"stepmap", (1), 
# 689 "/home/arnold/scm/xtrs/trs_djgpp.c" 3 4
                                ((void*)0)
# 689 "/home/arnold/scm/xtrs/trs_djgpp.c"
                                    , 0},
        {"sizemap", (1), 
# 690 "/home/arnold/scm/xtrs/trs_djgpp.c" 3 4
                                ((void*)0)
# 690 "/home/arnold/scm/xtrs/trs_djgpp.c"
                                    , 0},
        {"truedam", (0), &trs_disk_truedam, (1)},
        {"notruedam", (0), &trs_disk_truedam, (0)},
        {"samplerate", (1), 
# 693 "/home/arnold/scm/xtrs/trs_djgpp.c" 3 4
                                ((void*)0)
# 693 "/home/arnold/scm/xtrs/trs_djgpp.c"
                                    , 0},
        {"serial", (1), 
# 694 "/home/arnold/scm/xtrs/trs_djgpp.c" 3 4
                                ((void*)0)
# 694 "/home/arnold/scm/xtrs/trs_djgpp.c"
                                    , 0},
        {"switches", (1), 
# 695 "/home/arnold/scm/xtrs/trs_djgpp.c" 3 4
                                ((void*)0)
# 695 "/home/arnold/scm/xtrs/trs_djgpp.c"
                                    , 0},
        {"emtsafe", (0), &trs_emtsafe, (1)},
        {"noemtsafe", (0), &trs_emtsafe, (0)},
        {"m1lc", (0), &trs_model1_lowercase, (1)},
        {"nom1lc", (0), &trs_model1_lowercase, (0)},
        {"expintf", (0), &trs_expansion_interface, (1)},
        {"noexpintf", (0), &trs_expansion_interface, (0)},
        {
# 702 "/home/arnold/scm/xtrs/trs_djgpp.c" 3 4
        ((void*)0)
# 702 "/home/arnold/scm/xtrs/trs_djgpp.c"
            , 0, 0, 0}
};

static int find_opt_match(const char *arg, const struct option *longopts) {
    int i;
    for (i = 0; longopts && longopts->name; longopts++, i++) {
        if (strcmp(arg, longopts->name) == 0) {
            return i;
        }
    }
    return -1;
}

static int getopt_long_only(int argc, char *const argv[],
                            const char *optstring,
                            const struct option *longopts, int *longindex) {

    const char *cur;
    int match_index;
    static const struct option *match;
    if (optind <= 1) {
        optind = 1;
    }
    for (; optind < argc;) {
        cur = argv[optind++];
        joshlog("on %s\n", cur);
        if (cur[0] != '-') {
            fatal("Bad option: %s", cur);
        }
        cur += 1;
        match_index = find_opt_match(cur, longopts);
        if (match_index < 0) {
            fatal("Bad option: -%s", cur);
            return -1;
        }
        match = longopts + match_index;
        *longindex = match_index;

        if (match->has_arg) {
            if (optind >= argc) {
                fatal("Missing argument to -%s", cur);
                return -1;
            }
            optarg = argv[optind++];
            joshlog("getopt : -%s with %s\n", cur, optarg);
        } else {
            joshlog("getopt : -%s\n", cur);
        }
        if (match->flag) {
            *(match->flag) = match->val;
            return 0;
        }
        return match->val;
    }
    return -1;


}


int
trs_parse_command_line(int argc, char **argv, int *debug) {
    int i;
    int s[8];
    char *charpeek;

    charpeek = getenv("CHARPEEK");
    if (!charpeek) {
        fatal("Unable to read CHARPEEK environment variable");
    } else {
        unsigned short cps, cpo;
        if (sscanf(charpeek, "%hx:%hx", &cps, &cpo) != 2) {
            fatal("Cannot parse CHARPEEK");
        }
        unsigned int real_addr = ((unsigned long) cps) * 16 + cpo;
        unsigned int real_page = real_addr & (~4095);
        unsigned int real_offset = real_addr - real_page;
        joshlog("CHARPEEK REAL MODE ADDRESS RADDR=%x RPAGE=%x ROFF=%x\n", real_addr, real_page, real_offset);


        char *p;
        p = malloc(3 * 4096);
        p += 4096 - (((unsigned int) p) & 4095);


        int x = -1;

        x = __djgpp_map_physical_memory(p, 8192, real_page);
        joshlog("CHARPEAK MAPPED RPAGE=%x PPAGE=%p MAPRES=%x ERRNO=%d\n", real_page, p, x, errno);


        pScanBuffer = (struct scan_buffer *) (p + real_offset);
        joshlog("CHARPEAK MAPPED BUFFER=%p\n", pScanBuffer);

        pScanBuffer->suppress_flag = 1;

        scanBufferCursor = pScanBuffer->next_offset;
    }


    emulator_base_directory = getcwd(0, 1024);
    if (!emulator_base_directory) {
        joshlog("Unable to get emulator base dir\n");
        emulator_base_directory = ".";
    }
    joshlog("Emulator base dir = %s\n", emulator_base_directory);

    cassette_base_directory = malloc(32 + strlen(emulator_base_directory));
    if (!cassette_base_directory) {
        joshlog("Unable to allocate cassette base directory");
        cassette_base_directory = "./CAS";
    } else {
        sprintf((char *) cassette_base_directory, "%s/CAS", emulator_base_directory);
    }
    mkdir(cassette_base_directory, 
# 816 "/home/arnold/scm/xtrs/trs_djgpp.c" 3 4
                                  00200
# 816 "/home/arnold/scm/xtrs/trs_djgpp.c"
                                         );

    emulator_printer_directory = malloc(32 + strlen(emulator_base_directory));
    if (!emulator_printer_directory) {
        joshlog("Unable to allocate printer base directory");
        emulator_printer_directory = "./PRINT";
    } else {
        sprintf((char *) emulator_printer_directory, "%s/PRINT", emulator_base_directory);
    }
    mkdir(emulator_printer_directory, 
# 825 "/home/arnold/scm/xtrs/trs_djgpp.c" 3 4
                                     00200
# 825 "/home/arnold/scm/xtrs/trs_djgpp.c"
                                            );

    trs_model = 1;
    trs_model1_lowercase = (0);


    optind = 1;
    opterr = 0;
    for (;;) {
        int c;
        int option_index = 0;
        const char *name;

        c = getopt_long_only(argc, argv, "", options, &option_index);
        if (c == -1) break;
        if (c == '?') {
            fatal("unrecognized option %s", argv[optind - 1]);
        }
        name = options[option_index].name;
        if (strcmp(name, "background") == 0 ||
            strcmp(name, "bg") == 0) {
            opt_background = optarg;
        } else if (strcmp(name, "foreground") == 0 ||
                   strcmp(name, "fg") == 0) {
            opt_foreground = optarg;
        } else if (strcmp(name, "title") == 0) {
            opt_title = optarg;
        } else if (strcmp(name, "borderwidth") == 0) {
            border_width = strtoul(optarg, 
# 853 "/home/arnold/scm/xtrs/trs_djgpp.c" 3 4
                                          ((void*)0)
# 853 "/home/arnold/scm/xtrs/trs_djgpp.c"
                                              , 0);
        } else if (strcmp(name, "scale") == 0) {
            sscanf(optarg, "%u,%u", &scale_x, &scale_y);
        } else if (strcmp(name, "charset") == 0) {
            opt_charset = optarg;
        } else if (strcmp(name, "romfile") == 0) {
            opt_romfile = optarg;
        } else if (strcmp(name, "romfile3") == 0) {
            opt_romfile3 = optarg;
        } else if (strcmp(name, "romfile4p") == 0) {
            opt_romfile4p = optarg;
        } else if (strcmp(name, "model") == 0) {
            if (strcmp(optarg, "1") == 0 ||
                strcasecmp(optarg, "I") == 0) {
                trs_model = 1;
            } else if (strcmp(optarg, "3") == 0 ||
                       strcasecmp(optarg, "III") == 0) {
                trs_model = 3;
            } else if (strcmp(optarg, "4") == 0 ||
                       strcasecmp(optarg, "IV") == 0) {
                trs_model = 4;
            } else if (strcasecmp(optarg, "4P") == 0 ||
                       strcasecmp(optarg, "IVp") == 0) {
                trs_model = 5;
            } else {
                fatal("TRS-80 Model %s not supported", optarg);
            }
        } else if (strcmp(name, "delay") == 0) {
            z80_state.delay = strtol(optarg, 
# 881 "/home/arnold/scm/xtrs/trs_djgpp.c" 3 4
                                            ((void*)0)
# 881 "/home/arnold/scm/xtrs/trs_djgpp.c"
                                                , 0);
        } else if (strcmp(name, "keystretch") == 0) {
            stretch_amount = strtol(optarg, 
# 883 "/home/arnold/scm/xtrs/trs_djgpp.c" 3 4
                                           ((void*)0)
# 883 "/home/arnold/scm/xtrs/trs_djgpp.c"
                                               , 0);
        } else if (strcmp(name, "diskdir") == 0) {
            trs_disk_dir = strdup(optarg);
            if (trs_disk_dir[0] == '~' &&
                (trs_disk_dir[1] == '/' || trs_disk_dir[1] == '\0')) {
                char *home = getenv("HOME");
                if (home) {
                    char *p = (char *) malloc(strlen(home) + strlen(trs_disk_dir) + 1);
                    sprintf(p, "%s/%s", home, trs_disk_dir + 1);
                    trs_disk_dir = p;
                }
            }
        } else if (strcmp(name, "doubler") == 0) {
            switch (optarg[0]) {
                case 'p':
                case 'P':
                    trs_disk_doubler = 1;
                    break;
                case 'r':
                case 'R':
                case 't':
                case 'T':
                    trs_disk_doubler = 2;
                    break;
                case 'b':
                case 'B':
                    trs_disk_doubler = 3;
                    break;
                case 'n':
                case 'N':
                    trs_disk_doubler = 0;
                    break;
                default:
                    fatal("unrecognized doubler type %s\n", optarg);
            }
        } else if (strcmp(name, "stepmap") == 0) {
            opt_stepmap = optarg;
        } else if (strcmp(name, "sizemap") == 0) {
            opt_sizemap = optarg;
        } else if (strcmp(name, "samplerate") == 0) {
            cassette_default_sample_rate = strtol(optarg, 
# 923 "/home/arnold/scm/xtrs/trs_djgpp.c" 3 4
                                                         ((void*)0)
# 923 "/home/arnold/scm/xtrs/trs_djgpp.c"
                                                             , 0);
        } else if (strcmp(name, "serial") == 0) {
            trs_uart_name = strdup(optarg);
        } else if (strcmp(name, "switches") == 0) {
            trs_uart_switches = strtol(optarg, 
# 927 "/home/arnold/scm/xtrs/trs_djgpp.c" 3 4
                                              ((void*)0)
# 927 "/home/arnold/scm/xtrs/trs_djgpp.c"
                                                  , 0);
        }
    }
    if (optind != argc) {
        fatal("unrecognized argument %s", argv[optind]);
    }

    trs_video_ram_7_bit = trs_model == 1 && !trs_model1_lowercase;

    if (trs_video_ram_7_bit) {
        joshlog("Video RAM is 7 bits\n");
    } else {
        joshlog("Video RAM is 8 bits\n");
    }
    if (trs_model == 1 && !trs_expansion_interface) {
        trs_ram_end = 0x8000;
    }





    *debug = opt_debug;

    if (resize == -1) {
        resize = (trs_model == 3);
    }

    if (opt_shiftbracket == -1) {
        opt_shiftbracket = trs_model >= 4;
    }
    trs_kb_bracket(opt_shiftbracket);

    if (scale_y == 0) scale_y = 2 * scale_x;


    if (trs_model == 1) {
        if (opt_charset == 
# 964 "/home/arnold/scm/xtrs/trs_djgpp.c" 3 4
                          ((void*)0)
# 964 "/home/arnold/scm/xtrs/trs_djgpp.c"
                              ) {
            opt_charset = "wider";
        }
        if (
# 967 "/home/arnold/scm/xtrs/trs_djgpp.c" 3 4
           (__dj_ctype_flags[(int)(
# 967 "/home/arnold/scm/xtrs/trs_djgpp.c"
           *opt_charset
# 967 "/home/arnold/scm/xtrs/trs_djgpp.c" 3 4
           )+1] & 0x0008)
# 967 "/home/arnold/scm/xtrs/trs_djgpp.c"
                                ) {
            trs_charset = strtol(opt_charset, 
# 968 "/home/arnold/scm/xtrs/trs_djgpp.c" 3 4
                                             ((void*)0)
# 968 "/home/arnold/scm/xtrs/trs_djgpp.c"
                                                 , 0);
            cur_char_width = 8 * scale_x;
        } else {
            if (opt_charset[0] == 'e' ) {
                trs_charset = 0;
                cur_char_width = 6 * scale_x;
            } else if (opt_charset[0] == 's' ) {
                trs_charset = 1;
                cur_char_width = 6 * scale_x;
            } else if (opt_charset[0] == 'l' ) {
                trs_charset = 2;
                cur_char_width = 6 * scale_x;
            } else if (opt_charset[0] == 'w' ) {
                trs_charset = 3;
                cur_char_width = 8 * scale_x;
            } else if (opt_charset[0] == 'g' ) {
                trs_charset = 10;
                cur_char_width = 8 * scale_x;
            } else {
                fatal("unknown charset name %s", opt_charset);
            }
        }
        cur_char_height = 12 * scale_y;
    } else {
        if (opt_charset == 
# 992 "/home/arnold/scm/xtrs/trs_djgpp.c" 3 4
                          ((void*)0)
# 992 "/home/arnold/scm/xtrs/trs_djgpp.c"
                              ) {

            opt_charset = (trs_model == 3) ? "katakana" : "international";
        }
        if (
# 996 "/home/arnold/scm/xtrs/trs_djgpp.c" 3 4
           (__dj_ctype_flags[(int)(
# 996 "/home/arnold/scm/xtrs/trs_djgpp.c"
           *opt_charset
# 996 "/home/arnold/scm/xtrs/trs_djgpp.c" 3 4
           )+1] & 0x0008)
# 996 "/home/arnold/scm/xtrs/trs_djgpp.c"
                                ) {
            trs_charset = strtol(opt_charset, 
# 997 "/home/arnold/scm/xtrs/trs_djgpp.c" 3 4
                                             ((void*)0)
# 997 "/home/arnold/scm/xtrs/trs_djgpp.c"
                                                 , 0);
        } else {
            if (opt_charset[0] == 'k' ) {
                trs_charset = 4 + 3 * (trs_model > 3);
            } else if (opt_charset[0] == 'i' ) {
                trs_charset = 5 + 3 * (trs_model > 3);
            } else if (opt_charset[0] == 'b' ) {
                trs_charset = 6 + 3 * (trs_model > 3);
            } else {
                fatal("unknown charset name %s", opt_charset);
            }
        }
        cur_char_width = 8 * scale_x;
        cur_char_height = 12 * scale_y;
    }

    for (i = 0; i <= 7; i++) {
        s[i] = opt_stepdefault;
    }
    if (opt_stepmap) {
        sscanf(opt_stepmap, "%d,%d,%d,%d,%d,%d,%d,%d",
               &s[0], &s[1], &s[2], &s[3], &s[4], &s[5], &s[6], &s[7]);
    }
    for (i = 0; i <= 7; i++) {
        if (s[i] != 1 && s[i] != 2) {
            fatal("bad value %d for disk %d single/double step\n", s[i], i);
        } else {
            trs_disk_setstep(i, s[i]);
        }
    }


    s[0] = 5;
    s[1] = 5;
    s[2] = 5;
    s[3] = 5;
    s[4] = 8;
    s[5] = 8;
    s[6] = 8;
    s[7] = 8;
    if (opt_sizemap) {
        sscanf(opt_sizemap, "%d,%d,%d,%d,%d,%d,%d,%d",
               &s[0], &s[1], &s[2], &s[3], &s[4], &s[5], &s[6], &s[7]);
    }
    for (i = 0; i <= 7; i++) {
        if (s[i] != 5 && s[i] != 8) {
            fatal("bad value %d for disk %d size", s[i], i);
        } else {
            trs_disk_setsize(i, s[i]);
        }
    }

    return 1;
}






void
trs_load_romfile() {
    char *romfile = 
# 1059 "/home/arnold/scm/xtrs/trs_djgpp.c" 3 4
                   ((void*)0)
# 1059 "/home/arnold/scm/xtrs/trs_djgpp.c"
                       ;
    struct stat statbuf;

    switch (trs_model) {
        case 1:
            if (opt_romfile) {
                romfile = opt_romfile;

            } else if (stat("/home/arnold/local/xtrs/share/xtrs/level2rom.hex", &statbuf) == 0) {
                romfile = "/home/arnold/local/xtrs/share/xtrs/level2rom.hex";

            }
            if (romfile != 
# 1071 "/home/arnold/scm/xtrs/trs_djgpp.c" 3 4
                          ((void*)0)
# 1071 "/home/arnold/scm/xtrs/trs_djgpp.c"
                              ) {
                joshlog("Loading rom %s\n", romfile);
                trs_load_rom(romfile);
                joshlog("Loaded rom %s\n", romfile);
            } else if (trs_rom1_size > 0) {
                trs_load_compiled_rom(trs_rom1_size, trs_rom1);
            } else {
                fatal("ROM file not specified!");
            }
            break;

        case 3:
        case 4:
            if (opt_romfile3) {
                romfile = opt_romfile3;

            } else if (stat("/home/arnold/local/xtrs/share/xtrs/romimage.m3", &statbuf) == 0) {
                romfile = "/home/arnold/local/xtrs/share/xtrs/romimage.m3";

            }
            if (romfile != 
# 1091 "/home/arnold/scm/xtrs/trs_djgpp.c" 3 4
                          ((void*)0)
# 1091 "/home/arnold/scm/xtrs/trs_djgpp.c"
                              ) {
                joshlog("Loading rom %s\n", romfile);
                trs_load_rom(romfile);
                joshlog("Loaded rom %s\n", romfile);
            } else if (trs_rom3_size > 0) {
                trs_load_compiled_rom(trs_rom3_size, trs_rom3);
            } else {
                fatal("ROM file not specified!");
            }
            break;

        default:
            if (opt_romfile4p) {
                romfile = opt_romfile4p;

            } else if (stat("/home/arnold/local/xtrs/share/xtrs/romimage.m4p", &statbuf) == 0) {
                romfile = "/home/arnold/local/xtrs/share/xtrs/romimage.m4p";

            }
            if (romfile != 
# 1110 "/home/arnold/scm/xtrs/trs_djgpp.c" 3 4
                          ((void*)0)
# 1110 "/home/arnold/scm/xtrs/trs_djgpp.c"
                              ) {
                trs_load_rom(romfile);
            } else if (trs_rom4p_size > 0) {
                trs_load_compiled_rom(trs_rom4p_size, trs_rom4p);
            } else {
                fatal("ROM file not specified!");
            }
            break;
    }
}


int joshem_do_modal(joshem_modal_handler handler, void *input) {

    GrSetMode(GR_width_height_graphics, 640, 200);
    reload_grx_colors();

    joshem_modal_context context;

    memset(&context, 0, sizeof(context));

    context.input = input;

    trs_wait_for_all_keys_up();
    pScanBuffer->suppress_flag = 0;

    handler(&context);
    repaint_screen();
    pScanBuffer->suppress_flag = 1;

    scanBufferCursor = pScanBuffer->next_offset;
    trs_realtime_reset();
    return context.result;

}
