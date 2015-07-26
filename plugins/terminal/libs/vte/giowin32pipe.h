// IO channels based on Windows bidirectional pipes.

#ifndef GIOWIN32PIPE_H
#define GIOWIN32PIPE_H

#include <glib.h>

#ifdef G_OS_WIN32

# include <windows.h>

extern GIOChannel *g_io_channel_win32_new_pipe (HANDLE pipe);

#endif

#endif
