// IO channels based on Windows bidirectional pipes.

/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * giowin32.c: IO Channels for Win32.
 * Copyright 1998 Owen Taylor and Tor Lillqvist
 * Copyright 1999-2000 Tor Lillqvist and Craig Setera
 * Copyright 2001-2003 Andrew Lanoix
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/.
 */

/*
 * Bugs that are related to the code in this file:
 *
 * Bug 137968 - Sometimes a GIOFunc on Win32 is called with zero condition
 * http://bugzilla.gnome.org/show_bug.cgi?id=137968
 *
 * Bug 324234 - Using g_io_add_watch_full() to wait for connect() to return on a non-blocking socket returns prematurely
 * http://bugzilla.gnome.org/show_bug.cgi?id=324234
 *
 * Bug 331214 - g_io_channel async socket io stalls
 * http://bugzilla.gnome.org/show_bug.cgi?id=331214
 *
 * Bug 338943 - Multiple watches on the same socket
 * http://bugzilla.gnome.org/show_bug.cgi?id=338943
 *
 * Bug 357674 - 2 serious bugs in giowin32.c making glib iochannels useless
 * http://bugzilla.gnome.org/show_bug.cgi?id=357674
 *
 * Bug 425156 - GIOChannel deadlocks on a win32 socket
 * http://bugzilla.gnome.org/show_bug.cgi?id=425156
 *
 * Bug 468910 - giofunc condition=0
 * http://bugzilla.gnome.org/show_bug.cgi?id=468910
 *
 * Bug 500246 - Bug fixes for giowin32
 * http://bugzilla.gnome.org/show_bug.cgi?id=500246
 *
 * Bug 548278 - Async GETs connections are always terminated unexpectedly on windows
 * http://bugzilla.gnome.org/show_bug.cgi?id=548278
 *
 * Bug 548536 - giowin32 problem when adding and removing watches
 * http://bugzilla.gnome.org/show_bug.cgi?id=548536
 *
 * When fixing bugs related to the code in this file, either the above
 * bugs or others, make sure that the test programs attached to the
 * above bugs continue to work.
 */

#include <config.h>

#include <glib.h>

#include <stdlib.h>
#include <windows.h>
#include <process.h>
#include <errno.h>

#include <glib/gstdio.h>


typedef struct _GIOWin32Channel GIOWin32Channel;
typedef struct _GIOWin32Watch GIOWin32Watch;

#define BUFFER_SIZE 4096

typedef enum {
  G_IO_WIN32_PIPE		/* Bidirectional pipes. */
} GIOWin32ChannelType;

struct _GIOWin32Channel {
  GIOChannel channel;
  GIOWin32ChannelType type;
  
  gboolean debug;

  HANDLE pipe;			/* Handle of pipe */

  CRITICAL_SECTION mutex;

  int direction;		/* 0 means we read from it,
				 * 1 means we write to it.
				 */

  gboolean running;		/* Is reader or writer thread
				 * running. FALSE if EOF has been
				 * reached by the reader thread.
				 */

  gboolean needs_close;		/* If the channel has been closed while
				 * the reader thread was still running.
				 */

  guint thread_id;		/* If non-NULL the channel has or has
				 * had a reader or writer thread.
				 */
  HANDLE data_avail_event;

  gushort revents;

  /* Data is kept in a circular buffer. To be able to distinguish between
   * empty and full buffers, we cannot fill it completely, but have to
   * leave a one character gap.
   *
   * Data available is between indexes rdp and wrp-1 (modulo BUFFER_SIZE).
   *
   * Empty:    wrp == rdp
   * Full:     (wrp + 1) % BUFFER_SIZE == rdp
   * Partial:  otherwise
   */
  guchar *buffer;		/* (Circular) buffer */
  gint wrp, rdp;		/* Buffer indices for writing and reading */
  HANDLE space_avail_event;
};

struct _GIOWin32Watch {
  GSource       source;
  GPollFD       pollfd;
  GIOChannel   *channel;
  GIOCondition  condition;
};

static void
g_win32_print_gioflags (GIOFlags flags)
{
  char *bar = "";

  if (flags & G_IO_FLAG_APPEND)
    bar = "|", g_print ("APPEND");
  if (flags & G_IO_FLAG_NONBLOCK)
    g_print ("%sNONBLOCK", bar), bar = "|";
  if (flags & G_IO_FLAG_IS_READABLE)
    g_print ("%sREADABLE", bar), bar = "|";
  if (flags & G_IO_FLAG_IS_WRITABLE)
    g_print ("%sWRITABLE", bar), bar = "|";
  if (flags & G_IO_FLAG_IS_SEEKABLE)
    g_print ("%sSEEKABLE", bar), bar = "|";
}

static const char *
condition_to_string (GIOCondition condition)
{
  char buf[100];
  int checked_bits = 0;
  char *bufp = buf;

  if (condition == 0)
    return "";

#define BIT(n) checked_bits |= G_IO_##n; if (condition & G_IO_##n) bufp += sprintf (bufp, "%s" #n, (bufp>buf ? "|" : ""))

  BIT (IN);
  BIT (OUT);
  BIT (PRI);
  BIT (ERR);
  BIT (HUP);
  BIT (NVAL);
  
#undef BIT

  if ((condition & ~checked_bits) != 0)
	  bufp += sprintf (bufp, "|%#x", condition & ~checked_bits);
  
  return g_quark_to_string (g_quark_from_string (buf));
}

static gboolean
g_io_win32_get_debug_flag (void)
{
  return (getenv ("G_IO_WIN32_DEBUG") != NULL);
}

static void
g_io_channel_win32_init (GIOWin32Channel *channel)
{
  channel->debug = g_io_win32_get_debug_flag ();

  InitializeCriticalSection (&channel->mutex);
  channel->running = FALSE;
  channel->needs_close = FALSE;
  channel->thread_id = 0;
  channel->data_avail_event = NULL;
  channel->revents = 0;
  channel->buffer = NULL;
  channel->space_avail_event = NULL;
}

static void
create_events (GIOWin32Channel *channel)
{
  SECURITY_ATTRIBUTES sec_attrs;
  
  sec_attrs.nLength = sizeof (SECURITY_ATTRIBUTES);
  sec_attrs.lpSecurityDescriptor = NULL;
  sec_attrs.bInheritHandle = FALSE;

  /* The data available event is manual reset, the space available event
   * is automatic reset.
   */
  if (!(channel->data_avail_event = CreateEvent (&sec_attrs, TRUE, FALSE, NULL))
      || !(channel->space_avail_event = CreateEvent (&sec_attrs, FALSE, FALSE, NULL)))
    {
      gchar *emsg = g_win32_error_message (GetLastError ());

      g_error ("Error creating event: %s", emsg);
      g_free (emsg);
    }
}

static unsigned __stdcall
read_thread (void *parameter)
{
  GIOWin32Channel *channel = parameter;
  guchar *buffer;
  gint nbytes;
  HANDLE event;
  OVERLAPPED over;
  DWORD amount;
  BOOL ret;

  g_io_channel_ref ((GIOChannel *)channel);

  if (channel->debug)
    g_print ("read_thread %#x: start pipe=%p, data_avail=%p space_avail=%p\n",
	     channel->thread_id,
	     channel->pipe,
	     channel->data_avail_event,
	     channel->space_avail_event);

  channel->direction = 0;
  channel->buffer = g_malloc (BUFFER_SIZE);
  channel->rdp = channel->wrp = 0;
  channel->running = TRUE;

  SetEvent (channel->space_avail_event);
  
  event = CreateEvent (NULL, TRUE, FALSE, NULL);
  EnterCriticalSection (&channel->mutex);
  while (channel->running)
    {
      if (channel->debug)
	g_print ("read_thread %#x: rdp=%d, wrp=%d\n",
		 channel->thread_id, channel->rdp, channel->wrp);
      if ((channel->wrp + 1) % BUFFER_SIZE == channel->rdp)
	{
	  /* Buffer is full */
	  if (channel->debug)
	    g_print ("read_thread %#x: resetting space_avail\n",
		     channel->thread_id);
	  ResetEvent (channel->space_avail_event);
	  if (channel->debug)
	    g_print ("read_thread %#x: waiting for space\n",
		     channel->thread_id);
	  LeaveCriticalSection (&channel->mutex);
	  WaitForSingleObject (channel->space_avail_event, INFINITE);
	  EnterCriticalSection (&channel->mutex);
	  if (channel->debug)
	    g_print ("read_thread %#x: rdp=%d, wrp=%d\n",
		     channel->thread_id, channel->rdp, channel->wrp);
	}
      
      buffer = channel->buffer + channel->wrp;
      
      /* Always leave at least one byte unused gap to be able to
       * distinguish between the full and empty condition...
       */
      nbytes = MIN ((channel->rdp + BUFFER_SIZE - channel->wrp - 1) % BUFFER_SIZE,
		    BUFFER_SIZE - channel->wrp);

      if (channel->debug)
	g_print ("read_thread %#x: calling read() for %d bytes\n",
		 channel->thread_id, nbytes);

      LeaveCriticalSection (&channel->mutex);

      memset (&over, 0, sizeof (over));
      over.hEvent = event;
      ret = ReadFile (channel->pipe, buffer, nbytes, &amount, &over);
      if (!ret && GetLastError () == ERROR_IO_PENDING)
        ret = GetOverlappedResult (channel->pipe, &over, &amount, TRUE);
      nbytes = amount;
      
      EnterCriticalSection (&channel->mutex);

      channel->revents = G_IO_IN;
      if (!ret)
        {
          if (GetLastError () == ERROR_BROKEN_PIPE)
            channel->revents |= G_IO_HUP;
          else
            channel->revents |= G_IO_ERR;
        }

      if (channel->debug)
	g_print ("read_thread %#x: read() returned %d, rdp=%d, wrp=%d\n",
		 channel->thread_id, nbytes, channel->rdp, channel->wrp);

      if (nbytes <= 0)
	break;

      channel->wrp = (channel->wrp + nbytes) % BUFFER_SIZE;
      if (channel->debug)
	g_print ("read_thread %#x: rdp=%d, wrp=%d, setting data_avail\n",
		 channel->thread_id, channel->rdp, channel->wrp);
      SetEvent (channel->data_avail_event);
    }
  CloseHandle (event);
  
  channel->running = FALSE;
  if (channel->needs_close)
    {
      if (channel->debug)
	g_print ("read_thread %#x: channel pipe %p needs closing\n",
		 channel->thread_id, channel->pipe);
      CloseHandle (channel->pipe);
      channel->pipe = INVALID_HANDLE_VALUE;
    }

  if (channel->debug)
    g_print ("read_thread %#x: EOF, rdp=%d, wrp=%d, setting data_avail\n",
	     channel->thread_id, channel->rdp, channel->wrp);
  SetEvent (channel->data_avail_event);
  LeaveCriticalSection (&channel->mutex);
  
  g_io_channel_unref ((GIOChannel *)channel);
  
  /* No need to call _endthreadex(), the actual thread starter routine
   * in MSVCRT (see crt/src/threadex.c:_threadstartex) calls
   * _endthreadex() for us.
   */

  return 0;
}

static unsigned __stdcall
write_thread (void *parameter)
{
  GIOWin32Channel *channel = parameter;
  guchar *buffer;
  gint nbytes;
  HANDLE event;
  OVERLAPPED over;
  DWORD written;
  BOOL ret;

  g_io_channel_ref ((GIOChannel *)channel);

  if (channel->debug)
    g_print ("write_thread %#x: start pipe=%p, data_avail=%p space_avail=%p\n",
	     channel->thread_id,
	     channel->pipe,
	     channel->data_avail_event,
	     channel->space_avail_event);
  
  channel->direction = 1;
  channel->buffer = g_malloc (BUFFER_SIZE);
  channel->rdp = channel->wrp = 0;
  channel->running = TRUE;

  SetEvent (channel->space_avail_event);

  /* We use the same event objects as for a reader thread, but with
   * reversed meaning. So, space_avail is used if data is available
   * for writing, and data_avail is used if space is available in the
   * write buffer.
   */

  event = CreateEvent (NULL, TRUE, FALSE, NULL);
  EnterCriticalSection (&channel->mutex);
  while (channel->running || channel->rdp != channel->wrp)
    {
      if (channel->debug)
	g_print ("write_thread %#x: rdp=%d, wrp=%d\n",
		 channel->thread_id, channel->rdp, channel->wrp);
      if (channel->wrp == channel->rdp)
	{
	  /* Buffer is empty. */
	  if (channel->debug)
	    g_print ("write_thread %#x: resetting space_avail\n",
		     channel->thread_id);
	  ResetEvent (channel->space_avail_event);
	  if (channel->debug)
	    g_print ("write_thread %#x: waiting for data\n",
		     channel->thread_id);
	  channel->revents = G_IO_OUT;
	  SetEvent (channel->data_avail_event);
	  LeaveCriticalSection (&channel->mutex);
	  WaitForSingleObject (channel->space_avail_event, INFINITE);

	  EnterCriticalSection (&channel->mutex);
	  if (channel->rdp == channel->wrp)
	    break;

	  if (channel->debug)
	    g_print ("write_thread %#x: rdp=%d, wrp=%d\n",
		     channel->thread_id, channel->rdp, channel->wrp);
	}
      
      buffer = channel->buffer + channel->rdp;
      if (channel->rdp < channel->wrp)
	nbytes = channel->wrp - channel->rdp;
      else
	nbytes = BUFFER_SIZE - channel->rdp;

      if (channel->debug)
	g_print ("write_thread %#x: calling write() for %d bytes\n",
		 channel->thread_id, nbytes);

      LeaveCriticalSection (&channel->mutex);

      memset (&over, 0, sizeof (over));
      over.hEvent = event;
      ret = WriteFile (channel->pipe, buffer, nbytes, &written, &over);
      if (!ret && GetLastError () == ERROR_IO_PENDING)
        ret = GetOverlappedResult (channel->pipe, &over, &written, TRUE);
      nbytes = written;

      EnterCriticalSection (&channel->mutex);

      if (channel->debug)
	g_print ("write_thread %#x: write(%p) returned %d, rdp=%d, wrp=%d\n",
		 channel->thread_id, channel->pipe, nbytes, channel->rdp, channel->wrp);

      channel->revents = 0;
      if (nbytes > 0)
	channel->revents |= G_IO_OUT;
      if (!ret)
        {
          if (GetLastError () == ERROR_NO_DATA)
            channel->revents |= G_IO_HUP;
          else
            channel->revents |= G_IO_ERR;
        }

      channel->rdp = (channel->rdp + nbytes) % BUFFER_SIZE;

      if (nbytes <= 0)
	break;

      if (channel->debug)
	g_print ("write_thread: setting data_avail for thread %#x\n",
		 channel->thread_id);
      SetEvent (channel->data_avail_event);
    }
  CloseHandle (event);
  
  channel->running = FALSE;
  if (channel->needs_close)
    {
      if (channel->debug)
	g_print ("write_thread %#x: channel pipe %p needs closing\n",
		 channel->thread_id, channel->pipe);
      CloseHandle (channel->pipe);
      channel->pipe = INVALID_HANDLE_VALUE;
    }

  LeaveCriticalSection (&channel->mutex);
  
  g_io_channel_unref ((GIOChannel *)channel);
  
  return 0;
}

static void
create_thread (GIOWin32Channel     *channel,
	       GIOCondition         condition,
	       unsigned (__stdcall *thread) (void *parameter))
{
  HANDLE thread_handle;

  thread_handle = (HANDLE) _beginthreadex (NULL, 0, thread, channel, 0,
					   &channel->thread_id);
  if (thread_handle == 0)
    g_warning ("Error creating thread: %s.",
	       g_strerror (errno));
  else if (!CloseHandle (thread_handle))
    {
      gchar *emsg = g_win32_error_message (GetLastError ());

      g_warning ("Error closing thread handle: %s.", emsg);
      g_free (emsg);
    }

  WaitForSingleObject (channel->space_avail_event, INFINITE);
}

static GIOStatus
buffer_read (GIOWin32Channel *channel,
	     gchar           *dest,
	     gsize            count,
	     gsize           *bytes_read,
	     GError         **err)
{
  guint nbytes;
  guint left = count;
  
  EnterCriticalSection (&channel->mutex);
  if (channel->debug)
    g_print ("reading from thread %#x %" G_GSIZE_FORMAT " bytes, rdp=%d, wrp=%d\n",
	     channel->thread_id, count, channel->rdp, channel->wrp);
  
  if (channel->wrp == channel->rdp)
    {
      LeaveCriticalSection (&channel->mutex);
      if (channel->debug)
	g_print ("waiting for data from thread %#x\n", channel->thread_id);
      WaitForSingleObject (channel->data_avail_event, INFINITE);
      if (channel->debug)
	g_print ("done waiting for data from thread %#x\n", channel->thread_id);
      EnterCriticalSection (&channel->mutex);
      if (channel->wrp == channel->rdp && !channel->running)
	{
	  if (channel->debug)
	    g_print ("wrp==rdp, !running\n");
	  LeaveCriticalSection (&channel->mutex);
          *bytes_read = 0;
	  return G_IO_STATUS_EOF;
	}
    }
  
  if (channel->rdp < channel->wrp)
    nbytes = channel->wrp - channel->rdp;
  else
    nbytes = BUFFER_SIZE - channel->rdp;
  LeaveCriticalSection (&channel->mutex);
  nbytes = MIN (left, nbytes);
  if (channel->debug)
    g_print ("moving %d bytes from thread %#x\n",
	     nbytes, channel->thread_id);
  memcpy (dest, channel->buffer + channel->rdp, nbytes);
  dest += nbytes;
  left -= nbytes;
  EnterCriticalSection (&channel->mutex);
  channel->rdp = (channel->rdp + nbytes) % BUFFER_SIZE;
  if (channel->debug)
    g_print ("setting space_avail for thread %#x\n", channel->thread_id);
  SetEvent (channel->space_avail_event);
  if (channel->debug)
    g_print ("for thread %#x: rdp=%d, wrp=%d\n",
	     channel->thread_id, channel->rdp, channel->wrp);
  if (channel->running && channel->wrp == channel->rdp)
    {
      if (channel->debug)
	g_print ("resetting data_avail of thread %#x\n",
		 channel->thread_id);
      ResetEvent (channel->data_avail_event);
    };
  LeaveCriticalSection (&channel->mutex);
  
  /* We have no way to indicate any errors form the actual
   * read() or recv() call in the reader thread. Should we have?
   */
  *bytes_read = count - left;
  return (*bytes_read > 0) ? G_IO_STATUS_NORMAL : G_IO_STATUS_EOF;
}


static GIOStatus
buffer_write (GIOWin32Channel *channel,
	      const gchar     *dest,
	      gsize            count,
	      gsize           *bytes_written,
	      GError         **err)
{
  guint nbytes;
  guint left = count;
  
  EnterCriticalSection (&channel->mutex);
  if (channel->debug)
    g_print ("buffer_write: writing to thread %#x %" G_GSIZE_FORMAT " bytes, rdp=%d, wrp=%d\n",
	     channel->thread_id, count, channel->rdp, channel->wrp);
  
  if ((channel->wrp + 1) % BUFFER_SIZE == channel->rdp)
    {
      /* Buffer is full */
      if (channel->debug)
	g_print ("buffer_write: tid %#x: resetting data_avail\n",
		 channel->thread_id);
      ResetEvent (channel->data_avail_event);
      if (channel->debug)
	g_print ("buffer_write: tid %#x: waiting for space\n",
		 channel->thread_id);
      LeaveCriticalSection (&channel->mutex);
      WaitForSingleObject (channel->data_avail_event, INFINITE);
      EnterCriticalSection (&channel->mutex);
      if (channel->debug)
	g_print ("buffer_write: tid %#x: rdp=%d, wrp=%d\n",
		 channel->thread_id, channel->rdp, channel->wrp);
    }
   
  nbytes = MIN ((channel->rdp + BUFFER_SIZE - channel->wrp - 1) % BUFFER_SIZE,
		BUFFER_SIZE - channel->wrp);

  LeaveCriticalSection (&channel->mutex);
  nbytes = MIN (left, nbytes);
  if (channel->debug)
    g_print ("buffer_write: tid %#x: writing %d bytes\n",
	     channel->thread_id, nbytes);
  memcpy (channel->buffer + channel->wrp, dest, nbytes);
  dest += nbytes;
  left -= nbytes;
  EnterCriticalSection (&channel->mutex);

  channel->wrp = (channel->wrp + nbytes) % BUFFER_SIZE;
  if (channel->debug)
    g_print ("buffer_write: tid %#x: rdp=%d, wrp=%d, setting space_avail\n",
	     channel->thread_id, channel->rdp, channel->wrp);
  SetEvent (channel->space_avail_event);

  if ((channel->wrp + 1) % BUFFER_SIZE == channel->rdp)
    {
      /* Buffer is full */
      if (channel->debug)
	g_print ("buffer_write: tid %#x: resetting data_avail\n",
		 channel->thread_id);
      ResetEvent (channel->data_avail_event);
    }

  LeaveCriticalSection (&channel->mutex);
  
  /* We have no way to indicate any errors form the actual
   * write() call in the writer thread. Should we have?
   */
  *bytes_written = count - left;
  return (*bytes_written > 0) ? G_IO_STATUS_NORMAL : G_IO_STATUS_EOF;
}


static gboolean
g_io_win32_prepare (GSource *source,
		    gint    *timeout)
{
  GIOWin32Watch *watch = (GIOWin32Watch *)source;
  GIOCondition buffer_condition = g_io_channel_get_buffer_condition (watch->channel);
  GIOWin32Channel *channel = (GIOWin32Channel *)watch->channel;
  int event_mask;
  
  *timeout = -1;
  
  if (channel->debug)
    g_print ("g_io_win32_prepare: source=%p channel=%p", source, channel);

  switch (channel->type)
    {
    case G_IO_WIN32_PIPE:
      if (channel->debug)
	g_print (" FD thread=%#x buffer_condition:{%s}"
		 "\n  watch->pollfd.events:{%s} watch->pollfd.revents:{%s} channel->revents:{%s}",
		 channel->thread_id, condition_to_string (buffer_condition),
		 condition_to_string (watch->pollfd.events),
		 condition_to_string (watch->pollfd.revents),
		 condition_to_string (channel->revents));
      
      EnterCriticalSection (&channel->mutex);
      if (channel->running)
	{
	  if (channel->direction == 0 && channel->wrp == channel->rdp)
	    {
	      if (channel->debug)
		g_print ("\n  setting revents=0");
	      channel->revents = 0;
	    }
	}
      else
	{
	  if (channel->direction == 1
	      && (channel->wrp + 1) % BUFFER_SIZE == channel->rdp)
	    {
	      if (channel->debug)
		g_print ("\n setting revents=0");
	      channel->revents = 0;
	    }
	}	  
      LeaveCriticalSection (&channel->mutex);
      break;

    default:
      g_assert_not_reached ();
      abort ();
    }
  if (channel->debug)
    g_print ("\n");

  return ((watch->condition & buffer_condition) == watch->condition);
}

static gboolean
g_io_win32_check (GSource *source)
{
  MSG msg;
  GIOWin32Watch *watch = (GIOWin32Watch *)source;
  GIOWin32Channel *channel = (GIOWin32Channel *)watch->channel;
  GIOCondition buffer_condition = g_io_channel_get_buffer_condition (watch->channel);

  if (channel->debug)
    g_print ("g_io_win32_check: source=%p channel=%p", source, channel);

  switch (channel->type)
    {
    case G_IO_WIN32_PIPE:
      if (channel->debug)
	g_print (" FD thread=%#x buffer_condition=%s\n"
		 "  watch->pollfd.events={%s} watch->pollfd.revents={%s} channel->revents={%s}\n",
		 channel->thread_id, condition_to_string (buffer_condition),
		 condition_to_string (watch->pollfd.events),
		 condition_to_string (watch->pollfd.revents),
		 condition_to_string (channel->revents));
      
      watch->pollfd.revents = (watch->pollfd.events & channel->revents);

      return ((watch->pollfd.revents | buffer_condition) & watch->condition);

    default:
      g_assert_not_reached ();
      abort ();
    }
}

static gboolean
g_io_win32_dispatch (GSource     *source,
		     GSourceFunc  callback,
		     gpointer     user_data)
{
  GIOFunc func = (GIOFunc)callback;
  GIOWin32Watch *watch = (GIOWin32Watch *)source;
  GIOWin32Channel *channel = (GIOWin32Channel *)watch->channel;
  GIOCondition buffer_condition = g_io_channel_get_buffer_condition (watch->channel);
  
  if (!func)
    {
      g_warning ("IO Watch dispatched without callback\n"
		 "You must call g_source_connect().");
      return FALSE;
    }
  
  if (channel->debug)
    g_print ("g_io_win32_dispatch: pollfd.revents=%s condition=%s result=%s\n",
	     condition_to_string (watch->pollfd.revents),
	     condition_to_string (watch->condition),
	     condition_to_string ((watch->pollfd.revents | buffer_condition) & watch->condition));

  return (*func) (watch->channel,
		  (watch->pollfd.revents | buffer_condition) & watch->condition,
		  user_data);
}

static void
g_io_win32_finalize (GSource *source)
{
  GIOWin32Watch *watch = (GIOWin32Watch *)source;
  GIOWin32Channel *channel = (GIOWin32Channel *)watch->channel;
  
  if (channel->debug)
    g_print ("g_io_win32_finalize: source=%p channel=%p", source, channel);

  switch (channel->type)
    {
    case G_IO_WIN32_PIPE:
      if (channel->debug)
	g_print (" FD thread=%#x", channel->thread_id);
      break;

    default:
      g_assert_not_reached ();
      abort ();
    }
  if (channel->debug)
    g_print ("\n");
  g_io_channel_unref (watch->channel);
}

GSourceFuncs g_io_watch_funcs = {
  g_io_win32_prepare,
  g_io_win32_check,
  g_io_win32_dispatch,
  g_io_win32_finalize
};

static void
g_io_win32_free (GIOChannel *channel)
{
  GIOWin32Channel *win32_channel = (GIOWin32Channel *)channel;
  
  if (win32_channel->debug)
    g_print ("g_io_win32_free channel=%p pipe=%p\n", channel, win32_channel->pipe);

  DeleteCriticalSection (&win32_channel->mutex);

  if (win32_channel->data_avail_event)
    if (!CloseHandle (win32_channel->data_avail_event))
      if (win32_channel->debug)
	{
	  gchar *emsg = g_win32_error_message (GetLastError ());

	  g_print ("  CloseHandle(%p) failed: %s\n",
		   win32_channel->data_avail_event, emsg);
	  g_free (emsg);
	}

  g_free (win32_channel->buffer);

  if (win32_channel->space_avail_event)
    if (!CloseHandle (win32_channel->space_avail_event))
      if (win32_channel->debug)
	{
	  gchar *emsg = g_win32_error_message (GetLastError ());

	  g_print ("  CloseHandle(%p) failed: %s\n",
		   win32_channel->space_avail_event, emsg);
	  g_free (emsg);
	}

  g_free (win32_channel);
}

static GIOStatus
g_io_win32_pipe_read (GIOChannel *channel,
		      gchar      *buf,
		      gsize       count,
		      gsize      *bytes_read,
		      GError    **err)
{
  GIOWin32Channel *win32_channel = (GIOWin32Channel *)channel;
  gint result;
  
  if (win32_channel->debug)
    g_print ("g_io_win32_fd_read: pipe=%p count=%" G_GSIZE_FORMAT "\n",
	     win32_channel->pipe, count);
  
  if (win32_channel->thread_id)
    {
      return buffer_read (win32_channel, buf, count, bytes_read, err);
    }

#if 0 // TODO
  result = read (win32_channel->fd, buf, count);

  if (win32_channel->debug)
    g_print ("g_io_win32_fd_read: read() => %d\n", result);

  if (result < 0)
    {
      *bytes_read = 0;

      switch (errno)
        {
#ifdef EAGAIN
	case EAGAIN:
	  return G_IO_STATUS_AGAIN;
#endif
	default:
	  g_set_error_literal (err, G_IO_CHANNEL_ERROR,
                               g_io_channel_error_from_errno (errno),
                               g_strerror (errno));
	  return G_IO_STATUS_ERROR;
        }
    }

  *bytes_read = result;

  return (result > 0) ? G_IO_STATUS_NORMAL : G_IO_STATUS_EOF;
#else
  g_assert_not_reached ();
  return G_IO_STATUS_ERROR;
#endif
}

static GIOStatus
g_io_win32_pipe_write (GIOChannel  *channel,
		       const gchar *buf,
		       gsize        count,
		       gsize       *bytes_written,
		       GError     **err)
{
  GIOWin32Channel *win32_channel = (GIOWin32Channel *)channel;
  gint result;

  if (win32_channel->thread_id)
    {
      return buffer_write (win32_channel, buf, count, bytes_written, err);
    }
  
#if 0 // TODO
  result = write (win32_channel->fd, buf, count);
  if (win32_channel->debug)
    g_print ("g_io_win32_fd_write: fd=%d count=%" G_GSIZE_FORMAT " => %d\n",
	     win32_channel->fd, count, result);

  if (result < 0)
    {
      *bytes_written = 0;

      switch (errno)
        {
#ifdef EAGAIN
	case EAGAIN:
	  return G_IO_STATUS_AGAIN;
#endif
	default:
	  g_set_error_literal (err, G_IO_CHANNEL_ERROR,
                               g_io_channel_error_from_errno (errno),
                               g_strerror (errno));
	  return G_IO_STATUS_ERROR;
        }
    }

  *bytes_written = result;

  return G_IO_STATUS_NORMAL;
#else
  g_assert_not_reached ();
  return G_IO_STATUS_ERROR;
#endif
}

static GIOStatus
g_io_win32_pipe_close (GIOChannel *channel,
		       GError    **err)
{
  GIOWin32Channel *win32_channel = (GIOWin32Channel *)channel;
  
  if (win32_channel->debug)
    g_print ("g_io_win32_fd_close: thread=%#x: pipe=%p\n",
	     win32_channel->thread_id,
	     win32_channel->pipe);
  EnterCriticalSection (&win32_channel->mutex);
  if (win32_channel->running)
    {
      if (win32_channel->debug)
	g_print ("thread %#x: running, marking pipe %p for later close\n",
		 win32_channel->thread_id, win32_channel->pipe);
      win32_channel->running = FALSE;
      win32_channel->needs_close = TRUE;
      if (win32_channel->direction == 0)
	SetEvent (win32_channel->data_avail_event);
      else
	SetEvent (win32_channel->space_avail_event);
    }
  else
    {
      if (win32_channel->debug)
	g_print ("closing pipe %p\n", win32_channel->pipe);
      CloseHandle (win32_channel->pipe);
      if (win32_channel->debug)
	g_print ("closed pipe %p, setting to -1\n",
		 win32_channel->pipe);
      win32_channel->pipe = INVALID_HANDLE_VALUE;
    }
  LeaveCriticalSection (&win32_channel->mutex);

  /* FIXME error detection? */

  return G_IO_STATUS_NORMAL;
}

static GSource *
g_io_win32_pipe_create_watch (GIOChannel    *channel,
			      GIOCondition   condition)
{
  GIOWin32Channel *win32_channel = (GIOWin32Channel *)channel;
  GSource *source = g_source_new (&g_io_watch_funcs, sizeof (GIOWin32Watch));
  GIOWin32Watch *watch = (GIOWin32Watch *)source;

  watch->channel = channel;
  g_io_channel_ref (channel);
  
  watch->condition = condition;
  
  if (win32_channel->data_avail_event == NULL)
    create_events (win32_channel);

  watch->pollfd.fd = (gintptr) win32_channel->data_avail_event;
  watch->pollfd.events = condition;
  
  if (win32_channel->debug)
    g_print ("g_io_win32_fd_create_watch: channel=%p pipe=%p condition={%s} event=%p\n",
	     channel, win32_channel->pipe,
	     condition_to_string (condition), (HANDLE) watch->pollfd.fd);

  EnterCriticalSection (&win32_channel->mutex);
  if (win32_channel->thread_id == 0)
    {
      if (condition & G_IO_IN)
	create_thread (win32_channel, condition, read_thread);
      else if (condition & G_IO_OUT)
	create_thread (win32_channel, condition, write_thread);
    }

  g_source_add_poll (source, &watch->pollfd);
  LeaveCriticalSection (&win32_channel->mutex);

  return source;
}

static GIOStatus
g_io_win32_unimpl_set_flags (GIOChannel *channel,
			     GIOFlags    flags,
			     GError    **err)
{
  GIOWin32Channel *win32_channel = (GIOWin32Channel *)channel;

  if (win32_channel->debug)
    {
      g_print ("g_io_win32_unimpl_set_flags: ");
      g_win32_print_gioflags (flags);
      g_print ("\n");
    }

  g_set_error_literal (err, G_IO_CHANNEL_ERROR,
                       G_IO_CHANNEL_ERROR_FAILED,
                       "Not implemented on Win32");

  return G_IO_STATUS_ERROR;
}

static GIOFlags
g_io_win32_pipe_get_flags_internal (GIOChannel *channel)
{
  channel->is_readable = TRUE;
  channel->is_writeable = TRUE;
  channel->is_seekable = FALSE;

  /* XXX: G_IO_FLAG_APPEND */
  /* XXX: G_IO_FLAG_NONBLOCK */

  return 0;
}

static GIOFlags
g_io_win32_pipe_get_flags (GIOChannel *channel)
{
  GIOWin32Channel *win32_channel = (GIOWin32Channel *)channel;

  g_return_val_if_fail (win32_channel != NULL, 0);
  g_return_val_if_fail (win32_channel->type == G_IO_WIN32_PIPE, 0);

  return g_io_win32_pipe_get_flags_internal (channel);
}

static GIOFuncs win32_channel_pipe_funcs = {
  g_io_win32_pipe_read,
  g_io_win32_pipe_write,
  NULL,
  g_io_win32_pipe_close,
  g_io_win32_pipe_create_watch,
  g_io_win32_free,
  g_io_win32_unimpl_set_flags,
  g_io_win32_pipe_get_flags,
};

void
g_io_channel_win32_set_debug (GIOChannel *channel,
			      gboolean    flag)
{
  GIOWin32Channel *win32_channel = (GIOWin32Channel *)channel;

  win32_channel->debug = flag;
}

gint
g_io_channel_win32_poll (GPollFD *fds,
			 gint     n_fds,
			 gint     timeout)
{
  g_return_val_if_fail (n_fds >= 0, 0);

  return g_poll (fds, n_fds, timeout);
}

void
g_io_channel_win32_make_pollfd (GIOChannel   *channel,
				GIOCondition  condition,
				GPollFD      *fd)
{
  GIOWin32Channel *win32_channel = (GIOWin32Channel *)channel;

  switch (win32_channel->type)
    {
    case G_IO_WIN32_PIPE:
      if (win32_channel->data_avail_event == NULL)
	create_events (win32_channel);

      fd->fd = (gintptr) win32_channel->data_avail_event;

      if (win32_channel->thread_id == 0)
	{
	  /* Is it meaningful for a file descriptor to be polled for
	   * both IN and OUT? For what kind of file descriptor would
	   * that be? Doesn't seem to make sense, in practise the file
	   * descriptors handled here are always read or write ends of
	   * pipes surely, and thus unidirectional.
	   */
	  if (condition & G_IO_IN)
	    create_thread (win32_channel, condition, read_thread);
	  else if (condition & G_IO_OUT)
	    create_thread (win32_channel, condition, write_thread);
	}
      break;

    default:
      g_assert_not_reached ();
      abort ();
    }
  
  fd->events = condition;
}

GIOChannel *
g_io_channel_win32_new_pipe (HANDLE pipe)
{
  GIOWin32Channel *win32_channel;
  GIOChannel *channel;

  win32_channel = g_new (GIOWin32Channel, 1);
  channel = (GIOChannel *)win32_channel;

  g_io_channel_init (channel);
  g_io_channel_win32_init (win32_channel);

  win32_channel->pipe = pipe;

  if (win32_channel->debug)
    g_print ("g_io_channel_win32_new_fd: channel=%p pipe=%p\n",
	     channel, pipe);

  channel->funcs = &win32_channel_pipe_funcs;
  win32_channel->type = G_IO_WIN32_PIPE;
  g_io_win32_pipe_get_flags_internal (channel);
  
  return channel;
}
