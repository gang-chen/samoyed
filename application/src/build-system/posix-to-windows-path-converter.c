// POSIX to Windows path converter for MSYS2 (Cygwin).
// Copyright (C) 2015 Gang Chen.

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <sys/cygwin.h>

int main(int argc, char *argv[])
{
    FILE *fp;
    int i;
    ssize_t size;
    wchar_t *win;
    if (argc < 2)
    {
        printf("Usage: %s output-file path-1 path-2 ...\n", argv[0]);
        return 1;
    }
    fp = fopen(argv[1], "w");
    if (!fp)
        return 2;
    for (i = 2; i < argc; i++)
    {
        size = cygwin_conv_path(CCP_POSIX_TO_WIN_W | CCP_RELATIVE,
                                argv[i],
                                NULL,
                                0);
        if (size < 0)
        {
            if (fwrite(L"", sizeof(wchar_t), 1, fp) < 1)
            {
                fclose(fp);
                return 3;
            }
        }
        else
        {
            win = (wchar_t *) malloc(size);
            if (cygwin_conv_path(CCP_POSIX_TO_WIN_W | CCP_RELATIVE,
                                 argv[i],
                                 win,
                                 size))
            {
                if (fwrite(L"", sizeof(wchar_t), 1, fp) < 1)
                {
                    fclose(fp);
                    return 3;
                }
            }
            else
            {
                if (fwrite(win, 1, size, fp) < size)
                {
                    fclose(fp);
                    return 3;
                }
            }
            free(win);
        }
    }
    fclose(fp);
    return 0;
}
