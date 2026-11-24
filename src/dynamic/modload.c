/*
 * pongoOS - https://checkra.in
 *
 * Copyright (C) 2019-2023 checkra1n team
 *
 * This file is part of pongoOS.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */
#include <errno.h>
#include <stdarg.h>
#include <mach-o/loader.h>
#include <mach-o/nlist.h>
#include <mach-o/reloc.h>
#include <lzma/lzmadec.h>
#include <pongo.h>
#include <aes/aes.h>

_Noreturn void abort(void) { panic("abort()"); }
_Noreturn void __chk_fail(void) { panic("__chk_fail()"); }
_Noreturn void __stack_chk_fail(void) { panic("__stack_chk_fail()"); }
uint64_t __stack_chk_guard = 0x4141414141414141;

// A8: keep module formatting on newlib's integer-only implementations.
int printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int rv = vfiprintf(stdout, fmt, ap);
    va_end(ap);
    return rv;
}

int sprintf(char *str, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int rv = vsniprintf(str, (size_t)-1, fmt, ap);
    va_end(ap);
    return rv;
}

#if 0
void *__memset_chk (void *dest, int c, size_t n, size_t dest_len) {
     if (n > dest_len) {
         panic("__memset_chk: overflow detected");
     }
     return memset(dest,c,n);
}
void *__memcpy_chk (void *dest, const void * src, size_t n, size_t dest_len) {
     if (n > dest_len) {
         panic("__memcpy_chk: overflow detected");
     }
     return memcpy(dest,src,n);
}
void __muldc3() { panic("__muldc3"); }
void __mulsc3() { panic("__mulsc3"); }
void __muloti4() { panic("__muloti4"); }
int regcomp() { panic("regcomp"); }
int regexec() { panic("regexec"); }
int regfree() { panic("regfree"); }
int getentropy()    { errno = ENOSYS; return -1; }
int _gettimeofday() { errno = ENOSYS; return -1; }
int _times()        { errno = ENOSYS; return -1; }
int _fcntl()        { errno = ENOSYS; return -1; }
int _stat()         { errno = ENOSYS; return -1; }
int _link()         { errno = ENOSYS; return -1; }
int _unlink()       { errno = ENOSYS; return -1; }
int _mkdir()        { errno = ENOSYS; return -1; }
int _fork()         { errno = ENOSYS; return -1; }
int _execve()       { errno = ENOSYS; return -1; }
int _wait()         { errno = ENOSYS; return -1; }
int sigprocmask()   { errno = ENOSYS; return -1; }
#endif


__asm__
(
    ".section __DATA, __pongo_exports\n"
    ".no_dead_strip pongo$exports\n"
    "pongo$exports:\n"
);
#define STR_(x) #x
#define STR(x) STR_(x)
#define PONGO_EXPORT_RAW(name, value)       \
__asm__                                     \
(                                           \
    ".section __TEXT, __cstring\n"          \
    "1:\n"                                  \
    "    .asciz \"_" #name "\"\n"           \
    ".section __DATA, __pongo_exports\n"    \
    ".align 4\n"                            \
    "    .8byte 1b\n"                       \
    "    .8byte " #value "\n"               \
)
#define PONGO_EXPORT_RENAME(name, orig) PONGO_EXPORT_RAW(name, _ ## orig)
#define PONGO_EXPORT(name) PONGO_EXPORT_RENAME(name, name)

// ========== ========== ========== ========== Newlib ========== ========== ========== ==========
// TRIM-A8: retain the module import set from PongoOS-a8-build.patch.
// Export names include the Darwin ABI underscore: '_' + the C identifier.
// This list has been manually curated, since we want a lot of newlib to be removed with LTO.
// Removed stuff includes:  all float/double math, wide/multibyte char support, most file operations,
//                          arc4, time functions, ctype/locale/timezone, atexit, env variables
// TRIM-A8: PONGO_EXPORT(__bsd_qsort_r);
// TRIM-A8: PONGO_EXPORT(__dprintf);
// TRIM-A8: PONGO_EXPORT(__eprintf);
PONGO_EXPORT(__errno);
// TRIM-A8: PONGO_EXPORT(__fbufsize);
// TRIM-A8: PONGO_EXPORT(__flbf);
// TRIM-A8: PONGO_EXPORT(__fpending);
// TRIM-A8: PONGO_EXPORT(__fpurge);
// TRIM-A8: PONGO_EXPORT(__freadable);
// TRIM-A8: PONGO_EXPORT(__freading);
// TRIM-A8: PONGO_EXPORT(__fsetlocking);
// TRIM-A8: PONGO_EXPORT(__fwritable);
// TRIM-A8: PONGO_EXPORT(__fwriting);
// TRIM-A8: PONGO_EXPORT(__getdelim);
// TRIM-A8: PONGO_EXPORT(__getline);
PONGO_EXPORT(__getreent);
// TRIM-A8: PONGO_EXPORT(__itoa);
// TRIM-A8: PONGO_EXPORT(__malloc_lock);
// TRIM-A8: PONGO_EXPORT(__malloc_unlock);
// TRIM-A8: PONGO_EXPORT(__memcpy_chk);
// TRIM-A8: PONGO_EXPORT(__memmove_chk);
// TRIM-A8: PONGO_EXPORT(__mempcpy_chk);
// TRIM-A8: PONGO_EXPORT(__memset_chk);
// TRIM-A8: PONGO_EXPORT(__sinit);
// TRIM-A8: PONGO_EXPORT(__snprintf_chk);
// TRIM-A8: PONGO_EXPORT(__sprintf_chk);
// TRIM-A8: PONGO_EXPORT(__stpcpy_chk);
// TRIM-A8: PONGO_EXPORT(__stpncpy_chk);
// TRIM-A8: PONGO_EXPORT(__strcat_chk);
// TRIM-A8: PONGO_EXPORT(__strcpy_chk);
// TRIM-A8: PONGO_EXPORT(__strncat_chk);
// TRIM-A8: PONGO_EXPORT(__strncpy_chk);
// TRIM-A8: PONGO_EXPORT(__utoa);
// TRIM-A8: PONGO_EXPORT(__vsnprintf_chk);
// TRIM-A8: PONGO_EXPORT(__vsprintf_chk);
// TRIM-A8: PONGO_EXPORT(__xpg_strerror_r);
// TRIM-A8: PONGO_EXPORT(_asiprintf_r);
// TRIM-A8: PONGO_EXPORT(_asniprintf_r);
// TRIM-A8: PONGO_EXPORT(_asnprintf_r);
// TRIM-A8: PONGO_EXPORT(_asprintf_r);
// TRIM-A8: PONGO_EXPORT(_atoi_r);
// TRIM-A8: PONGO_EXPORT(_atol_r);
// TRIM-A8: PONGO_EXPORT(_atoll_r);
// TRIM-A8: PONGO_EXPORT(_calloc_r);
// TRIM-A8: PONGO_EXPORT(_close_r);
// TRIM-A8: PONGO_EXPORT(_diprintf_r);
// TRIM-A8: PONGO_EXPORT(_dprintf_r);
// TRIM-A8: PONGO_EXPORT(_fclose_r);
// TRIM-A8: PONGO_EXPORT(_fcloseall_r);
// TRIM-A8: PONGO_EXPORT(_fdopen_r);
// TRIM-A8: PONGO_EXPORT(_fflush_r);
// TRIM-A8: PONGO_EXPORT(_fgetc_r);
// TRIM-A8: PONGO_EXPORT(_fgetc_unlocked_r);
// TRIM-A8: PONGO_EXPORT(_fgetpos_r);
// TRIM-A8: PONGO_EXPORT(_fgets_r);
// TRIM-A8: PONGO_EXPORT(_fgets_unlocked_r);
// TRIM-A8: PONGO_EXPORT(_fiprintf_r);
// TRIM-A8: PONGO_EXPORT(_fiscanf_r);
// TRIM-A8: PONGO_EXPORT(_fmemopen_r);
// TRIM-A8: PONGO_EXPORT(_fprintf_r);
// TRIM-A8: PONGO_EXPORT(_fpurge_r);
// TRIM-A8: PONGO_EXPORT(_fputc_r);
// TRIM-A8: PONGO_EXPORT(_fputc_unlocked_r);
// TRIM-A8: PONGO_EXPORT(_fputs_r);
// TRIM-A8: PONGO_EXPORT(_fputs_unlocked_r);
// TRIM-A8: PONGO_EXPORT(_fread_r);
// TRIM-A8: PONGO_EXPORT(_fread_unlocked_r);
// TRIM-A8: PONGO_EXPORT(_free_r);
// TRIM-A8: PONGO_EXPORT(_fscanf_r);
// TRIM-A8: PONGO_EXPORT(_fseek_r);
// TRIM-A8: PONGO_EXPORT(_fseeko_r);
// TRIM-A8: PONGO_EXPORT(_fsetpos_r);
// TRIM-A8: PONGO_EXPORT(_fstat_r);
// TRIM-A8: PONGO_EXPORT(_ftell_r);
// TRIM-A8: PONGO_EXPORT(_ftello_r);
// TRIM-A8: PONGO_EXPORT(_funopen_r);
// TRIM-A8: PONGO_EXPORT(_fwrite_r);
// TRIM-A8: PONGO_EXPORT(_fwrite_unlocked_r);
// TRIM-A8: PONGO_EXPORT(_getc_r);
// TRIM-A8: PONGO_EXPORT(_getc_unlocked_r);
// TRIM-A8: PONGO_EXPORT(_getchar_r);
// TRIM-A8: PONGO_EXPORT(_getchar_unlocked_r);
// TRIM-A8: PONGO_EXPORT(_getpid_r);
// TRIM-A8: PONGO_EXPORT(_iprintf_r);
// TRIM-A8: PONGO_EXPORT(_isatty_r);
// TRIM-A8: PONGO_EXPORT(_iscanf_r);
// TRIM-A8: PONGO_EXPORT(_l64a_r);
// TRIM-A8: PONGO_EXPORT(_lseek_r);
//PONGO_EXPORT(_mallinfo_r);
// TRIM-A8: PONGO_EXPORT(_malloc_r);
//PONGO_EXPORT(_malloc_stats_r);
//PONGO_EXPORT(_malloc_trim_r);
//PONGO_EXPORT(_malloc_usable_size_r);
// TRIM-A8: PONGO_EXPORT(_mallopt_r);
// TRIM-A8: PONGO_EXPORT(_memalign_r);
//PONGO_EXPORT(_mstats_r);
// TRIM-A8: PONGO_EXPORT(_open_memstream_r);
// TRIM-A8: PONGO_EXPORT(_perror_r);
// TRIM-A8: PONGO_EXPORT(_printf_r);
// TRIM-A8: PONGO_EXPORT(_putc_r);
// TRIM-A8: PONGO_EXPORT(_putc_unlocked_r);
// TRIM-A8: PONGO_EXPORT(_putchar_r);
// TRIM-A8: PONGO_EXPORT(_putchar_unlocked_r);
// TRIM-A8: PONGO_EXPORT(_puts_r);
// TRIM-A8: PONGO_EXPORT(_pvalloc_r);
// TRIM-A8: PONGO_EXPORT(_read_r);
// TRIM-A8: PONGO_EXPORT(_realloc_r);
// TRIM-A8: PONGO_EXPORT(_reallocf_r);
//PONGO_EXPORT(_reclaim_reent);
// TRIM-A8: PONGO_EXPORT(_rewind_r);
// TRIM-A8: PONGO_EXPORT(_sbrk_r);
// TRIM-A8: PONGO_EXPORT(_scanf_r);
// TRIM-A8: PONGO_EXPORT(_siprintf_r);
// TRIM-A8: PONGO_EXPORT(_siscanf_r);
// TRIM-A8: PONGO_EXPORT(_sniprintf_r);
// TRIM-A8: PONGO_EXPORT(_snprintf_r);
// TRIM-A8: PONGO_EXPORT(_sprintf_r);
// TRIM-A8: PONGO_EXPORT(_sscanf_r);
// TRIM-A8: PONGO_EXPORT(_strdup_r);
// TRIM-A8: PONGO_EXPORT(_strerror_r);
// TRIM-A8: PONGO_EXPORT(_strndup_r);
// TRIM-A8: PONGO_EXPORT(_strtoimax_r);
// TRIM-A8: PONGO_EXPORT(_strtol_r);
// TRIM-A8: PONGO_EXPORT(_strtoll_r);
// TRIM-A8: PONGO_EXPORT(_strtoul_r);
// TRIM-A8: PONGO_EXPORT(_strtoull_r);
// TRIM-A8: PONGO_EXPORT(_strtoumax_r);
// TRIM-A8: PONGO_EXPORT(_ungetc_r);
// TRIM-A8: PONGO_EXPORT(_valloc_r);
// TRIM-A8: PONGO_EXPORT(_vasiprintf_r);
// TRIM-A8: PONGO_EXPORT(_vasniprintf_r);
// TRIM-A8: PONGO_EXPORT(_vasnprintf_r);
// TRIM-A8: PONGO_EXPORT(_vasprintf_r);
// TRIM-A8: PONGO_EXPORT(_vdiprintf_r);
// TRIM-A8: PONGO_EXPORT(_vdprintf_r);
// TRIM-A8: PONGO_EXPORT(_vfiprintf_r);
// TRIM-A8: PONGO_EXPORT(_vfiscanf_r);
// TRIM-A8: PONGO_EXPORT(_vfprintf_r);
// TRIM-A8: PONGO_EXPORT(_vfscanf_r);
// TRIM-A8: PONGO_EXPORT(_viprintf_r);
// TRIM-A8: PONGO_EXPORT(_viscanf_r);
// TRIM-A8: PONGO_EXPORT(_vprintf_r);
// TRIM-A8: PONGO_EXPORT(_vscanf_r);
// TRIM-A8: PONGO_EXPORT(_vsiprintf_r);
// TRIM-A8: PONGO_EXPORT(_vsiscanf_r);
// TRIM-A8: PONGO_EXPORT(_vsniprintf_r);
// TRIM-A8: PONGO_EXPORT(_vsnprintf_r);
// TRIM-A8: PONGO_EXPORT(_vsprintf_r);
// TRIM-A8: PONGO_EXPORT(_vsscanf_r);
// TRIM-A8: PONGO_EXPORT(_write_r);
// TRIM-A8: PONGO_EXPORT(a64l);
// TRIM-A8: PONGO_EXPORT(abs);
// TRIM-A8: PONGO_EXPORT(aligned_alloc);
// TRIM-A8: PONGO_EXPORT(asiprintf);
// TRIM-A8: PONGO_EXPORT(asniprintf);
// TRIM-A8: PONGO_EXPORT(asnprintf);
// TRIM-A8: PONGO_EXPORT(asprintf);
// TRIM-A8: PONGO_EXPORT(atoi);
// TRIM-A8: PONGO_EXPORT(atol);
// TRIM-A8: PONGO_EXPORT(atoll);
// TRIM-A8: PONGO_EXPORT(bcmp);
// TRIM-A8: PONGO_EXPORT(bcopy);
// TRIM-A8: PONGO_EXPORT(bsearch);
PONGO_EXPORT(bzero);
// TRIM-A8: PONGO_EXPORT(calloc);
// TRIM-A8: PONGO_EXPORT(category);
// TRIM-A8: PONGO_EXPORT(cfree);
//PONGO_EXPORT(cleanup_glue);
// TRIM-A8: PONGO_EXPORT(clearerr);
// TRIM-A8: PONGO_EXPORT(clearerr_unlocked);
// TRIM-A8: PONGO_EXPORT(close);
// TRIM-A8: PONGO_EXPORT(diprintf);
// TRIM-A8: PONGO_EXPORT(div);
// TRIM-A8: PONGO_EXPORT(dprintf);
//PONGO_EXPORT(explicit_bzero);
// TRIM-A8: PONGO_EXPORT(fclose);
// TRIM-A8: PONGO_EXPORT(fcloseall);
// TRIM-A8: PONGO_EXPORT(fdopen);
// TRIM-A8: PONGO_EXPORT(feof);
// TRIM-A8: PONGO_EXPORT(feof_unlocked);
// TRIM-A8: PONGO_EXPORT(ferror);
// TRIM-A8: PONGO_EXPORT(ferror_unlocked);
// TRIM-A8: PONGO_EXPORT(fflush);
// TRIM-A8: PONGO_EXPORT(fflush_unlocked);
// TRIM-A8: PONGO_EXPORT(ffs);
// TRIM-A8: PONGO_EXPORT(ffsl);
// TRIM-A8: PONGO_EXPORT(ffsll);
// TRIM-A8: PONGO_EXPORT(fgetc);
// TRIM-A8: PONGO_EXPORT(fgetc_unlocked);
// TRIM-A8: PONGO_EXPORT(fgetpos);
// TRIM-A8: PONGO_EXPORT(fgets);
// TRIM-A8: PONGO_EXPORT(fgets_unlocked);
// TRIM-A8: PONGO_EXPORT(fileno);
// TRIM-A8: PONGO_EXPORT(fileno_unlocked);
PONGO_EXPORT(fiprintf);
// TRIM-A8: PONGO_EXPORT(fiscanf);
// TRIM-A8: PONGO_EXPORT(fls);
// TRIM-A8: PONGO_EXPORT(flsl);
// TRIM-A8: PONGO_EXPORT(flsll);
// TRIM-A8: PONGO_EXPORT(fmemopen);
//PONGO_EXPORT(fprintf);
// TRIM-A8: PONGO_EXPORT(fpurge);
// TRIM-A8: PONGO_EXPORT(fputc);
// TRIM-A8: PONGO_EXPORT(fputc_unlocked);
// TRIM-A8: PONGO_EXPORT(fputs);
// TRIM-A8: PONGO_EXPORT(fputs_unlocked);
// TRIM-A8: PONGO_EXPORT(fread);
// TRIM-A8: PONGO_EXPORT(fread_unlocked);
PONGO_EXPORT(free);
// TRIM-A8: PONGO_EXPORT(fscanf);
// TRIM-A8: PONGO_EXPORT(fseek);
// TRIM-A8: PONGO_EXPORT(fseeko);
// TRIM-A8: PONGO_EXPORT(fsetpos);
// TRIM-A8: PONGO_EXPORT(fstat);
// TRIM-A8: PONGO_EXPORT(ftell);
// TRIM-A8: PONGO_EXPORT(ftello);
// TRIM-A8: PONGO_EXPORT(funopen);
// TRIM-A8: PONGO_EXPORT(fwrite);
// TRIM-A8: PONGO_EXPORT(fwrite_unlocked);
// TRIM-A8: PONGO_EXPORT(getc);
// TRIM-A8: PONGO_EXPORT(getc_unlocked);
// TRIM-A8: PONGO_EXPORT(getchar);
// TRIM-A8: PONGO_EXPORT(getchar_unlocked);
// TRIM-A8: PONGO_EXPORT(getpid);
// TRIM-A8: PONGO_EXPORT(getw);
// TRIM-A8: PONGO_EXPORT(imaxabs);
// TRIM-A8: PONGO_EXPORT(imaxdiv);
// TRIM-A8: PONGO_EXPORT(index);
PONGO_EXPORT(iprintf);
// TRIM-A8: PONGO_EXPORT(isalnum);
// TRIM-A8: PONGO_EXPORT(isalpha);
// TRIM-A8: PONGO_EXPORT(isascii);
// TRIM-A8: PONGO_EXPORT(isatty);
// TRIM-A8: PONGO_EXPORT(isblank);
// TRIM-A8: PONGO_EXPORT(iscanf);
// TRIM-A8: PONGO_EXPORT(iscntrl);
// TRIM-A8: PONGO_EXPORT(isdigit);
// TRIM-A8: PONGO_EXPORT(isgraph);
// TRIM-A8: PONGO_EXPORT(islower);
// TRIM-A8: PONGO_EXPORT(isprint);
// TRIM-A8: PONGO_EXPORT(ispunct);
// TRIM-A8: PONGO_EXPORT(isspace);
// TRIM-A8: PONGO_EXPORT(isupper);
// TRIM-A8: PONGO_EXPORT(isxdigit);
// TRIM-A8: PONGO_EXPORT(itoa);
// TRIM-A8: PONGO_EXPORT(l64a);
// TRIM-A8: PONGO_EXPORT(labs);
// TRIM-A8: PONGO_EXPORT(ldiv);
// TRIM-A8: PONGO_EXPORT(llabs);
// TRIM-A8: PONGO_EXPORT(lldiv);
// TRIM-A8: PONGO_EXPORT(longjmp);
// TRIM-A8: PONGO_EXPORT(lseek);
//PONGO_EXPORT(mallinfo);
PONGO_EXPORT(malloc);
//PONGO_EXPORT(malloc_stats);
//PONGO_EXPORT(malloc_trim);
//PONGO_EXPORT(malloc_usable_size);
// TRIM-A8: PONGO_EXPORT(mallopt);
// TRIM-A8: PONGO_EXPORT(memalign);
// TRIM-A8: PONGO_EXPORT(memccpy);
// TRIM-A8: PONGO_EXPORT(memchr);
PONGO_EXPORT(memcmp);
PONGO_EXPORT(memcpy);
PONGO_EXPORT(memmem);
// TRIM-A8: PONGO_EXPORT(memmove);
// TRIM-A8: PONGO_EXPORT(mempcpy);
// TRIM-A8: PONGO_EXPORT(memrchr);
PONGO_EXPORT(memset);
//PONGO_EXPORT(mstats);
// TRIM-A8: PONGO_EXPORT(open_memstream);
// TRIM-A8: PONGO_EXPORT(perror);
//PONGO_EXPORT(printf);
// TRIM-A8: PONGO_EXPORT(putc);
// TRIM-A8: PONGO_EXPORT(putc_unlocked);
// TRIM-A8: PONGO_EXPORT(putchar);
// TRIM-A8: PONGO_EXPORT(putchar_unlocked);
PONGO_EXPORT(puts);
// TRIM-A8: PONGO_EXPORT(putw);
// TRIM-A8: PONGO_EXPORT(pvalloc);
PONGO_EXPORT(qsort); // Required by the current KPF module.
// TRIM-A8: PONGO_EXPORT(qsort_r);
// TRIM-A8: PONGO_EXPORT(rand);
// TRIM-A8: PONGO_EXPORT(rand_r);
// TRIM-A8: PONGO_EXPORT(random);
// TRIM-A8: PONGO_EXPORT(rawmemchr);
// TRIM-A8: PONGO_EXPORT(read);
PONGO_EXPORT(realloc); // Required by the current KPF module.
// TRIM-A8: PONGO_EXPORT(reallocarray);
// TRIM-A8: PONGO_EXPORT(reallocf);
// TRIM-A8: PONGO_EXPORT(rewind);
// TRIM-A8: PONGO_EXPORT(rindex);
// TRIM-A8: PONGO_EXPORT(sbrk);
// TRIM-A8: PONGO_EXPORT(scanf);
// TRIM-A8: PONGO_EXPORT(setbuf);
// TRIM-A8: PONGO_EXPORT(setbuffer);
// TRIM-A8: PONGO_EXPORT(setjmp);
// TRIM-A8: PONGO_EXPORT(setlinebuf);
// TRIM-A8: PONGO_EXPORT(setvbuf);
// TRIM-A8: PONGO_EXPORT(siprintf);
// TRIM-A8: PONGO_EXPORT(siscanf);
// TRIM-A8: PONGO_EXPORT(sniprintf);
//PONGO_EXPORT(snprintf);
// TRIM-A8: PONGO_EXPORT(sprintf);
// TRIM-A8: PONGO_EXPORT(srand);
// TRIM-A8: PONGO_EXPORT(srandom);
// TRIM-A8: PONGO_EXPORT(sscanf);
// TRIM-A8: PONGO_EXPORT(stpcpy);
// TRIM-A8: PONGO_EXPORT(stpncpy);
// TRIM-A8: PONGO_EXPORT(strcasecmp);
// TRIM-A8: PONGO_EXPORT(strcasestr);
// TRIM-A8: PONGO_EXPORT(strcat);
PONGO_EXPORT(strchr); // Required by the current KPF module.
// TRIM-A8: PONGO_EXPORT(strchrnul);
PONGO_EXPORT(strcmp);
PONGO_EXPORT(strcpy);
// TRIM-A8: PONGO_EXPORT(strcspn);
// TRIM-A8: PONGO_EXPORT(strdup);
// TRIM-A8: PONGO_EXPORT(strerror);
// TRIM-A8: PONGO_EXPORT(strerror_r);
PONGO_EXPORT(strlcat); // Required by the current KPF module.
// TRIM-A8: PONGO_EXPORT(strlcpy);
PONGO_EXPORT(strlen);
// TRIM-A8: PONGO_EXPORT(strlwr);
// TRIM-A8: PONGO_EXPORT(strncasecmp);
// TRIM-A8: PONGO_EXPORT(strncat);
PONGO_EXPORT(strncmp); // Required by the current KPF module.
// TRIM-A8: PONGO_EXPORT(strncpy);
// TRIM-A8: PONGO_EXPORT(strndup);
// TRIM-A8: PONGO_EXPORT(strnlen);
// TRIM-A8: PONGO_EXPORT(strnstr);
// TRIM-A8: PONGO_EXPORT(strpbrk);
PONGO_EXPORT(strrchr); // Required by the current KPF module.
// TRIM-A8: PONGO_EXPORT(strsep);
// TRIM-A8: PONGO_EXPORT(strspn);
PONGO_EXPORT(strstr); // Required by the current KPF module.
PONGO_EXPORT(strtoimax);
// TRIM-A8: PONGO_EXPORT(strtok);
// TRIM-A8: PONGO_EXPORT(strtok_r);
// TRIM-A8: PONGO_EXPORT(strtol);
// TRIM-A8: PONGO_EXPORT(strtoll);
PONGO_EXPORT(strtoul);
PONGO_EXPORT(strtoull); // Required by the current KPF module.
// TRIM-A8: PONGO_EXPORT(strtoumax);
// TRIM-A8: PONGO_EXPORT(strupr);
// TRIM-A8: PONGO_EXPORT(strverscmp);
// TRIM-A8: PONGO_EXPORT(swab);
// TRIM-A8: PONGO_EXPORT(tdelete);
// TRIM-A8: PONGO_EXPORT(tdestroy);
// TRIM-A8: PONGO_EXPORT(tfind);
//PONGO_EXPORT(timingsafe_bcmp);
//PONGO_EXPORT(timingsafe_memcmp);
// TRIM-A8: PONGO_EXPORT(toascii);
// TRIM-A8: PONGO_EXPORT(tolower);
// TRIM-A8: PONGO_EXPORT(toupper);
// TRIM-A8: PONGO_EXPORT(tsearch);
// TRIM-A8: PONGO_EXPORT(twalk);
// TRIM-A8: PONGO_EXPORT(ungetc);
// TRIM-A8: PONGO_EXPORT(utoa);
// TRIM-A8: PONGO_EXPORT(valloc);
// TRIM-A8: PONGO_EXPORT(vasiprintf);
// TRIM-A8: PONGO_EXPORT(vasniprintf);
// TRIM-A8: PONGO_EXPORT(vasnprintf);
// TRIM-A8: PONGO_EXPORT(vasprintf);
// TRIM-A8: PONGO_EXPORT(vdiprintf);
// TRIM-A8: PONGO_EXPORT(vdprintf);
// TRIM-A8: PONGO_EXPORT(vfiprintf);
// TRIM-A8: PONGO_EXPORT(vfiscanf);
//PONGO_EXPORT(vfprintf);
// TRIM-A8: PONGO_EXPORT(vfscanf);
// TRIM-A8: PONGO_EXPORT(viprintf);
// TRIM-A8: PONGO_EXPORT(viscanf);
//PONGO_EXPORT(vprintf);
// TRIM-A8: PONGO_EXPORT(vscanf);
// TRIM-A8: PONGO_EXPORT(vsiprintf);
// TRIM-A8: PONGO_EXPORT(vsiscanf);
// TRIM-A8: PONGO_EXPORT(vsniprintf);
//PONGO_EXPORT(vsnprintf);
// TRIM-A8: PONGO_EXPORT(vsprintf);
// TRIM-A8: PONGO_EXPORT(vsscanf);
// TRIM-A8: PONGO_EXPORT(write);

// ========== ========== ========== ========== Pongo ========== ========== ========== ==========

// DeviceTree
PONGO_EXPORT(gDeviceTree);
PONGO_EXPORT(dt_check$64);
PONGO_EXPORT(dt_parse$64);
PONGO_EXPORT(dt_find);
PONGO_EXPORT(dt_prop$64);
PONGO_EXPORT(dt_print);
PONGO_EXPORT(dt_node);
PONGO_EXPORT(dt_get);
PONGO_EXPORT(dt_node_prop);
PONGO_EXPORT(dt_get_prop$64);
PONGO_EXPORT(dt_node_u32);
PONGO_EXPORT(dt_get_u32);
PONGO_EXPORT(dt_node_u64);
PONGO_EXPORT(dt_get_u64);
PONGO_EXPORT(dt_alloc_memmap);
// DeviceTree legacy compat
PONGO_EXPORT_RENAME(dt_check, dt_check$32);
PONGO_EXPORT_RENAME(dt_parse, dt_parse$32);
PONGO_EXPORT_RENAME(dt_prop, dt_prop$32);
PONGO_EXPORT_RENAME(dt_get_prop, dt_get_prop$32);
PONGO_EXPORT(dt_get_u32_prop);
PONGO_EXPORT(dt_get_u64_prop);
PONGO_EXPORT(dt_get_u64_prop_i);

// TODO: sort & clean up, match headers
PONGO_EXPORT(xnu_pf_apply_each_kext);
PONGO_EXPORT(xnu_pf_get_first_kext);
PONGO_EXPORT(xnu_pf_get_kext_header);
PONGO_EXPORT(xnu_pf_disable_patch);
PONGO_EXPORT(xnu_pf_enable_patch);
PONGO_EXPORT(xnu_pf_ptr_to_data);
PONGO_EXPORT(xnu_pf_patchset_destroy);
PONGO_EXPORT(xnu_pf_patchset_create);
PONGO_EXPORT(print_register);
PONGO_EXPORT(alloc_static);
PONGO_EXPORT(xnu_slide_hdr_va);
PONGO_EXPORT(xnu_slide_value);
PONGO_EXPORT(xnu_header);
PONGO_EXPORT(xnu_platform);
PONGO_EXPORT(xnu_va_to_ptr);
PONGO_EXPORT(xnu_ptr_to_va);
PONGO_EXPORT(xnu_rebase_va);
PONGO_EXPORT(kext_rebase_va);
PONGO_EXPORT(xnu_pf_range_from_va);
PONGO_EXPORT(xnu_pf_segment);
PONGO_EXPORT(xnu_pf_section);
PONGO_EXPORT(xnu_pf_all);
PONGO_EXPORT(xnu_pf_all_x);
PONGO_EXPORT(xnu_pf_maskmatch);
PONGO_EXPORT(xnu_pf_emit);
PONGO_EXPORT(xnu_pf_apply);
PONGO_EXPORT(macho_get_segment);
PONGO_EXPORT(macho_get_section);
PONGO_EXPORT(event_fire);
PONGO_EXPORT(event_wait);
PONGO_EXPORT(event_wait_asserted);
//PONGO_EXPORT(bzero);
//PONGO_EXPORT(memset);
PONGO_EXPORT(memcpy_trap);
PONGO_EXPORT(free_contig);
PONGO_EXPORT(phys_reference);
PONGO_EXPORT(phys_dereference);
PONGO_EXPORT(phys_force_free);
PONGO_EXPORT(mark_phys_wired);
PONGO_EXPORT(phys_get_entry);
PONGO_EXPORT(phys_set_entry);
PONGO_EXPORT(vm_flush);
PONGO_EXPORT(vm_flush_by_addr);
PONGO_EXPORT(free_phys);
PONGO_EXPORT(vm_space_map_page_physical_prot);
PONGO_EXPORT(proc_reference);
PONGO_EXPORT(proc_release);
PONGO_EXPORT(proc_create_task);
PONGO_EXPORT(vm_deallocate);
PONGO_EXPORT(vm_allocate);
//PONGO_EXPORT(strcmp);
PONGO_EXPORT(queue_rx_string);
//PONGO_EXPORT(strlen);
//PONGO_EXPORT(strcpy);
PONGO_EXPORT(task_create);
PONGO_EXPORT(task_create_extended);
PONGO_EXPORT(task_restart_and_link);
PONGO_EXPORT(task_critical_enter);
PONGO_EXPORT(task_critical_exit);
PONGO_EXPORT(task_bind_to_irq);
PONGO_EXPORT(task_release);
PONGO_EXPORT(task_reference);
PONGO_EXPORT(tz_get);
PONGO_EXPORT(tz_set);
PONGO_EXPORT(tz_locked);
PONGO_EXPORT(tz_lock);
PONGO_EXPORT(tz_lockdown);
PONGO_EXPORT(tz_blackbird);
PONGO_EXPORT(tz0_calculate_encrypted_block_offset);
PONGO_EXPORT(vatophys);
PONGO_EXPORT(vatophys_static);
PONGO_EXPORT(lock_take);
PONGO_EXPORT(lock_take_spin);
PONGO_EXPORT(lock_release);
//PONGO_EXPORT(memmove);
//PONGO_EXPORT(memmem);
PONGO_EXPORT(memstr);
PONGO_EXPORT(memstr_partial);
//PONGO_EXPORT(memcpy);
//PONGO_EXPORT(putc);
//PONGO_EXPORT(putchar);
//PONGO_EXPORT(puts);
//PONGO_EXPORT(strtoul);
PONGO_EXPORT(invalidate_icache);
PONGO_EXPORT(task_current);
PONGO_EXPORT(task_register_irq);
PONGO_EXPORT(task_register);
PONGO_EXPORT(task_yield);
PONGO_EXPORT(task_wait);
PONGO_EXPORT(task_exit);
PONGO_EXPORT(task_switch_irq);
PONGO_EXPORT(task_exit_irq);
PONGO_EXPORT(task_switch);
PONGO_EXPORT(task_link);
PONGO_EXPORT(task_unlink);
PONGO_EXPORT(task_irq_dispatch);
PONGO_EXPORT(task_yield_asserted);
PONGO_EXPORT(task_register_unlinked);
PONGO_EXPORT(panic);
PONGO_EXPORT(abort);
PONGO_EXPORT(spin);
PONGO_EXPORT(get_ticks);
PONGO_EXPORT(usleep);
PONGO_EXPORT(sleep);
PONGO_EXPORT(wdt_reset);
PONGO_EXPORT(wdt_enable);
PONGO_EXPORT(wdt_disable);
PONGO_EXPORT(command_putc);
PONGO_EXPORT(command_puts);
PONGO_EXPORT(command_register);
PONGO_EXPORT(command_tokenize);
PONGO_EXPORT(hexparse);
PONGO_EXPORT(hexprint);
PONGO_EXPORT(get_el);
PONGO_EXPORT(cache_invalidate);
PONGO_EXPORT(cache_clean_and_invalidate);
PONGO_EXPORT(cache_clean_and_invalidate_all);
PONGO_EXPORT(register_irq_handler);
PONGO_EXPORT(device_clock_by_id);
PONGO_EXPORT(device_clock_by_name);
PONGO_EXPORT(clock_gate);
PONGO_EXPORT(disable_interrupts);
PONGO_EXPORT(enable_interrupts);
PONGO_EXPORT(alloc_contig);
PONGO_EXPORT(alloc_phys);
PONGO_EXPORT(map_physical_range);
PONGO_EXPORT(task_vm_space);
PONGO_EXPORT(usbloader_init);
PONGO_EXPORT(task_irq_teardown);
//PONGO_EXPORT(realloc);
//PONGO_EXPORT(malloc);
PONGO_EXPORT(phystokv);
//PONGO_EXPORT(free);
PONGO_EXPORT(hexdump);
//PONGO_EXPORT(memcmp);
PONGO_EXPORT(map_range);
PONGO_EXPORT(linear_kvm_alloc);
PONGO_EXPORT(vm_flush_by_addr_all_asid);
PONGO_EXPORT(vatophys_force);
PONGO_EXPORT(serial_disable_rx);
PONGO_EXPORT(serial_enable_rx);
//PONGO_EXPORT(__memset_chk);
//PONGO_EXPORT(__memcpy_chk);
PONGO_EXPORT(resize_loader_xfer_data);
PONGO_EXPORT(gBootArgs);
PONGO_EXPORT(gTopOfKernelData);
PONGO_EXPORT(gEntryPoint);
PONGO_EXPORT(gIOBase);
PONGO_EXPORT(gPMGRBase);
PONGO_EXPORT(unlzma_decompress);
PONGO_EXPORT(gDevType);
PONGO_EXPORT(soc_name);
PONGO_EXPORT(socnum);
PONGO_EXPORT(loader_xfer_recv_data);
PONGO_EXPORT(loader_xfer_recv_count);
PONGO_EXPORT(preboot_hook);
PONGO_EXPORT(ramdisk_buf);
PONGO_EXPORT(ramdisk_size);
PONGO_EXPORT(sep_boot_hook);
PONGO_EXPORT(aes);
// TRIM-A8: PONGO_EXPORT(_impure_ptr);
PONGO_EXPORT(loader_xfer_recv_size);
PONGO_EXPORT(overflow_mode);
PONGO_EXPORT(gFramebuffer);
PONGO_EXPORT(gWidth);
PONGO_EXPORT(gHeight);
PONGO_EXPORT(gRowPixels);
PONGO_EXPORT(y_cursor);
PONGO_EXPORT(x_cursor);
PONGO_EXPORT(scale_factor);
PONGO_EXPORT(__chk_fail);
PONGO_EXPORT(__stack_chk_fail);
PONGO_EXPORT(__stack_chk_guard);
//PONGO_EXPORT_RENAME(iprintf, iprintf);
PONGO_EXPORT(printf);
PONGO_EXPORT(sprintf);
//PONGO_EXPORT_RENAME(fiprintf, fiprintf);
PONGO_EXPORT_RENAME(fprintf, fiprintf);
//PONGO_EXPORT_RENAME(viprintf, viprintf);
PONGO_EXPORT_RENAME(vprintf, viprintf);
//PONGO_EXPORT_RENAME(vfiprintf, vfiprintf);
PONGO_EXPORT_RENAME(vfprintf, vfiprintf);
//PONGO_EXPORT_RENAME(sniprintf, sniprintf);
PONGO_EXPORT_RENAME(snprintf, sniprintf);
//PONGO_EXPORT_RENAME(vsniprintf, vsniprintf);
PONGO_EXPORT_RENAME(vsnprintf, vsniprintf);
__asm__
(
    ".section __DATA, __pongo_exports\n"
    ".align 4\n"
    "    .8byte 0x0\n"
    "    .8byte 0x0\n"
);

extern struct pongo_exports public_api[] __asm__("section$start$__DATA$__pongo_exports");

void link_exports(struct pongo_exports* export) {
    struct pongo_exports* api = &public_api[0];
    while (1) {
        while (api->name) {
            api++;
        }
        if (!api->name && api->value) {
            api = (struct pongo_exports*)api->value;
            continue;
        } else if (!api->name && !api->value) {
            api->value = (void*)export;
            return;
        }
    }
}
void* resolve_symbol(const char* name) {
    struct pongo_exports* api = &public_api[0];
    while (1) {
        while (api->name) {
            if (strcmp(api->name, name) == 0) {
                return api->value;
            }
            api++;
        }
        if (!api->name && api->value) {
            // we have another set of exported symbols we can walk
            api = (struct pongo_exports*)api->value;
            continue;
        }
        break;
    }
    iprintf("resolve_symbol: missing symbol: %s\n", name);
    iprintf("usbloader-linker could not load this module!!\n");
    return 0;
}
static struct pongo_module_info* head;
struct pongo_module_info* pongo_module_create(uint32_t segmentCount) {
    struct pongo_module_info* mod = calloc(sizeof (struct pongo_module_info) + sizeof(struct pongo_module_segment_info) * segmentCount, 1);
    disable_interrupts();
    mod->next = head;
    mod->segcount = segmentCount;
    head = mod;
    enable_interrupts();
    return mod;
}
void pongo_module_print_list(void) {
    struct pongo_module_info* cur = head;
    while (cur) {
        iprintf(" | %26s @ 0x%llx->0x%llx\n", cur->name, cur->vm_base, cur->vm_end);
        for (uint32_t i = 0; i < cur->segcount; i++) {
            iprintf(" |---> %22s @ 0x%08llx, size 0x%06llx (%s%s%s)\n", cur->segments[i].name, cur->vm_base + cur->segments[i].vm_addr, cur->segments[i].vm_size, cur->segments[i].prot & PROT_READ ? "r" : "-", cur->segments[i].prot & PROT_WRITE ? "w" : "-", cur->segments[i].prot & PROT_EXEC ? "x" : "-");
        }
        cur = cur->next;
    }
}
