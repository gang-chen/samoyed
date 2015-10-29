// Compiler invocation interceptor.
// Copyright (C) 2015 Gang Chen.

#define _GNU_SOURCE
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#ifdef OS_WINDOWS
# define FWRITES(s, fp) fwrite(L##s, sizeof(wchar_t), wcslen(L##s) + 1, fp)
# include <wchar.h>
# include <sys/cygwin.h>
# include <windows.h>
#else
# define FWRITES(s, fp) fwrite(s, sizeof(char), strlen(s) + 1, fp)
#endif

#ifdef OS_WINDOWS

static int strip_exe(const char *a)
{
    int l;
    l = strlen(a);
    if (l >= 4 &&
        a[l - 4] == '.' &&
        tolower(a[l - 3]) == 'e' &&
        tolower(a[l - 2]) == 'x' &&
        tolower(a[l - 1]) == 'e')
        l -= 4;
    return l;
}

static int match_exe(const char *a, const char *b)
{
    int al, bl;
    if (!a || !b)
        return 0;
    al = strip_exe(a);
    bl = strip_exe(b);
    if (!al || !bl || al != bl)
        return 0;
    return strncasecmp(a, b, al) == 0;
}

#else

static int match_exe(const char *a, const char *b)
{
    if (!a || !b)
        return 0;
    return strcmp(a, b) == 0;
}

#endif

static void print(FILE *fp, const char *s)
{
#if OS_WINDOWS
    // Convert the word into a wide string.
    mbstate_t state;
    memset(&state, 0, sizeof(state));
    while (s)
    {
        wchar_t buf[BUFSIZ];
        size_t n;
        n = mbsrtowcs(buf, &s, BUFSIZ, &state);
        fwrite(buf, sizeof(wchar_t), n, fp);
    }
#else
    fwrite(s, sizeof(char), strlen(s), fp);
#endif
}

#ifdef OS_WINDOWS

static void conv_print_path(FILE *fp, const char *path)
{
    // Convert the path into the Windows format.
    ssize_t size;
    wchar_t *win;
    size = cygwin_conv_path(CCP_POSIX_TO_WIN_W | CCP_RELATIVE,
                            path,
                            NULL,
                            0);
    if (size >= 0)
    {
        win = (wchar_t *) malloc(size);
        if (cygwin_conv_path(CCP_POSIX_TO_WIN_W | CCP_RELATIVE,
                             path,
                             win,
                             size) == 0)
            fwrite(win, 1, size, fp);
        else
        {
            print(fp, path);
            FWRITES("", fp);
        }
        free(win);
    }
    else
    {
        print(fp, path);
        FWRITES("", fp);
    }
}

#endif

// On MSYS2 (Cygwin), the paths are probably in the POSIX format that need to
// convert to the Windows format.  The arguments directly following the "-I" or
// "-L" options without any space are paths.  We assume that all command-line
// arguments except compiler options are paths and try to convert, if possible.
const char *DIR_OPTIONS[] = { "-I", "-L", NULL };

static void print_arg(FILE *fp, const char *arg)
{
#ifdef OS_WINDOWS

    const char **opt;

    if (*arg != '-')
    {
        conv_print_path(fp, arg);
        return;
    }

    for (opt = DIR_OPTIONS; *opt; opt++)
    {
        int l = strlen(*opt);
        if (strncmp(*opt, arg, l) == 0)
        {
            print(fp, *opt);
            arg += l;
            conv_print_path(fp, arg);
            return;
        }
    }

#endif
    print(fp, arg);
    FWRITES("", fp);
}

static void print_argv(FILE *fp, char *const argv[])
{
    for (; *argv; argv++)
        print_arg(fp, *argv);
}

int
#ifdef OS_WINDOWS
intercepted_execve(
#else
execve(
#endif
    const char *filename,
    char *const argv[],
    char *const envp[])
{
    char *cc, *cxx, *output_file, *cwd;
    int is_cc, is_cxx, i;
    FILE *fp;
#ifndef OS_WINDOWS
    int (*real_execve)(const char *, char *const[], char *const[]));
#endif

    cc = getenv("SAMOYED_BUILDER_CC");
    if (match_exe(filename, cc) || match_exe(argv[0], cc))
        is_cc = 1;
    else
        is_cc = 0;
    cxx = getenv("SAMOYED_BUILDER_CXX");
    if (match_exe(filename, cxx) || match_exe(argv[0], cxx))
        is_cxx = 1;
    else
        is_cxx = 0;
    if (!is_cc && !is_cxx)
        goto EXECVE;

    output_file = getenv("SAMOYED_BUILDER_OUTPUT_FILE");
    if (!output_file)
        goto EXECVE;
    fp = fopen(output_file, "a");
    if (!fp)
        goto EXECVE;

    FWRITES("EXE:", fp);
    print(fp, filename);
    FWRITES("", fp);
    FWRITES("", fp);

    FWRITES("CWD:", fp);
    cwd = get_current_dir_name();
    conv_print_path(fp, cwd);
    free(cwd);
    FWRITES("", fp);

    if (is_cc)
    {
        FWRITES("CC ARGV:", fp);
        print(fp, argv[0]);
        FWRITES("", fp);
        print_argv(fp, argv + 1);
        FWRITES("", fp);
    }
    else if (is_cxx)
    {
        FWRITES("CXX ARGV:", fp);
        print(fp, argv[0]);
        FWRITES("", fp);
        print_argv(fp, argv + 1);
        FWRITES("", fp);
    }

    FWRITES("", fp);
    fclose(fp);

EXECVE:
#ifdef OS_WINDOWS
    return execve(filename, argv, envp);
#else
    real_execve = (int (*)(const char *, char *const[], char *const[]))
        dlsym(RTLD_NEXT, "execve");
    return real_execve(filename, argv, envp);
#endif
}

#ifdef OS_WINDOWS
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID p)
{
    if (reason == DLL_PROCESS_ATTACH)
        cygwin_internal(CW_HOOK, "execve", intercepted_execve);
    return TRUE;
}
#endif
