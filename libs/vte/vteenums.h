/*
 * Copyright (C) 2001,2002,2003,2009,2010 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef __VTE_VTE_ENUMS_H__
#define __VTE_VTE_ENUMS_H__

#if !defined (__VTE_VTE_H_INSIDE__) && !defined (VTE_COMPILATION)
#error "Only <vte/vte.h> can be included directly."
#endif

#include <glib.h>

G_BEGIN_DECLS

/**
 * VteCursorBlinkMode:
 * @VTE_CURSOR_BLINK_SYSTEM: Follow GTK+ settings for cursor blinking.
 * @VTE_CURSOR_BLINK_ON: Cursor blinks.
 * @VTE_CURSOR_BLINK_OFF: Cursor does not blink.
 *
 * An enumerated type which can be used to indicate the cursor blink mode
 * for the terminal.
 */
typedef enum {
        VTE_CURSOR_BLINK_SYSTEM,
        VTE_CURSOR_BLINK_ON,
        VTE_CURSOR_BLINK_OFF
} VteCursorBlinkMode;

/**
 * VteCursorShape:
 * @VTE_CURSOR_SHAPE_BLOCK: Draw a block cursor.  This is the default.
 * @VTE_CURSOR_SHAPE_IBEAM: Draw a vertical bar on the left side of character.
 * This is similar to the default cursor for other GTK+ widgets.
 * @VTE_CURSOR_SHAPE_UNDERLINE: Draw a horizontal bar below the character.
 *
 * An enumerated type which can be used to indicate what should the terminal
 * draw at the cursor position.
 */
typedef enum {
        VTE_CURSOR_SHAPE_BLOCK,
        VTE_CURSOR_SHAPE_IBEAM,
        VTE_CURSOR_SHAPE_UNDERLINE
} VteCursorShape;

/**
 * VteEraseBinding:
 * @VTE_ERASE_AUTO: For backspace, attempt to determine the right value from the terminal's IO settings.  For delete, use the control sequence.
 * @VTE_ERASE_ASCII_BACKSPACE: Send an ASCII backspace character (0x08).
 * @VTE_ERASE_ASCII_DELETE: Send an ASCII delete character (0x7F).
 * @VTE_ERASE_DELETE_SEQUENCE: Send the "@@7" control sequence.
 * @VTE_ERASE_TTY: Send terminal's "erase" setting.
 *
 * An enumerated type which can be used to indicate which string the terminal
 * should send to an application when the user presses the Delete or Backspace
 * keys.
 */
typedef enum {
	VTE_ERASE_AUTO,
	VTE_ERASE_ASCII_BACKSPACE,
	VTE_ERASE_ASCII_DELETE,
	VTE_ERASE_DELETE_SEQUENCE,
	VTE_ERASE_TTY
} VteEraseBinding;

/**
 * VtePtyError:
 * @VTE_PTY_ERROR_PTY_HELPER_FAILED: failure when using the GNOME PTY helper to
 *   allocate the PTY
 * @VTE_PTY_ERROR_PTY98_FAILED: failure when using PTY98 to allocate the PTY
 *
 * Since: 0.26
 */
typedef enum {
  VTE_PTY_ERROR_PTY_HELPER_FAILED = 0,
  VTE_PTY_ERROR_PTY98_FAILED
} VtePtyError;

/**
 * VtePtyFlags:
 * @VTE_PTY_NO_LASTLOG: don't record the session in lastlog. Deprecated: 0.38
 * @VTE_PTY_NO_UTMP: don't record the session in utmp. Deprecated: 0.38
 * @VTE_PTY_NO_WTMP: don't record the session in wtmp. Deprecated: 0.38
 * @VTE_PTY_NO_HELPER: don't use the GNOME PTY helper to allocate the PTY. Deprecated: 0.38
 * @VTE_PTY_NO_FALLBACK: when allocating the PTY with the PTY helper fails,
 *   don't fall back to try using PTY98. Deprecated: 0.38
 * @VTE_PTY_DEFAULT: the default flags
 *
 * Since: 0.26
 */
typedef enum {
  VTE_PTY_NO_LASTLOG  = 1 << 0,
  VTE_PTY_NO_UTMP     = 1 << 1,
  VTE_PTY_NO_WTMP     = 1 << 2,
  VTE_PTY_NO_HELPER   = 1 << 3,
  VTE_PTY_NO_FALLBACK = 1 << 4,
  VTE_PTY_DEFAULT     = 0
} VtePtyFlags;

/**
 * VteWriteFlags:
 * @VTE_WRITE_DEFAULT: Write contents as UTF-8 text.  This is the default.
 *
 * A flag type to determine how terminal contents should be written
 * to an output stream.
 */
typedef enum {
  VTE_WRITE_DEFAULT = 0
} VteWriteFlags;

G_END_DECLS

#endif /* __VTE_VTE_ENUMS_H__ */
