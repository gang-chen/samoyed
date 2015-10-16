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
 #include <sys/cygwin.h>
 #include <windows.h>
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

static void print_word(FILE *fp, const char *word)
{
    fputc('"', fp);
    while (*word)
    {
        if (*word == '"')
            fputs("\\\"", fp);
        else if (*word == '\\')
            fputs("\\\\", fp);
        else
            fputc(*word, fp);
        word++;
    }
    fputc('"', fp);
}

static void print_argv(FILE *fp, char *const argv[])
{
    char *const *p;
    for (p = argv; *p; p++)
    {
        fputc(' ', fp);
        print_word(fp, *p);
    }
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

    fputs("EXE: ", fp);
    print_word(fp, filename);
    fputc('\n', fp);

    cwd = get_current_dir_name();
    fputs("CWD: ", fp);
    print_word(fp, cwd);
    fputc('\n', fp);
    free(cwd);

    if (is_cc)
    {
        fprintf(fp, "CC ARGV:");
        print_argv(fp, argv);
        fputc('\n', fp);
    }
    if (is_cxx)
    {
        fprintf(fp, "CXX ARGV:");
        print_argv(fp, argv);
        fputc('\n', fp);
    }

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
