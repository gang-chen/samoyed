/*
 * gedit-file-browser-error.h - Gedit plugin providing easy file access
 * from the sidepanel
 *
 * Copyright (C) 2006 - Jesse van den Kieboom <jesse@icecrew.nl>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GEDIT_FILE_BROWSER_ERROR_H__
#define __GEDIT_FILE_BROWSER_ERROR_H__

G_BEGIN_DECLS

typedef enum {
	GEDIT_FILE_BROWSER_ERROR_NONE,
	GEDIT_FILE_BROWSER_ERROR_RENAME,
	GEDIT_FILE_BROWSER_ERROR_DELETE,
	GEDIT_FILE_BROWSER_ERROR_NEW_FILE,
	GEDIT_FILE_BROWSER_ERROR_NEW_DIRECTORY,
	GEDIT_FILE_BROWSER_ERROR_OPEN_DIRECTORY,
	GEDIT_FILE_BROWSER_ERROR_SET_ROOT,
	GEDIT_FILE_BROWSER_ERROR_LOAD_DIRECTORY,
	GEDIT_FILE_BROWSER_ERROR_NUM
} GeditFileBrowserError;

G_END_DECLS

#endif /* __GEDIT_FILE_BROWSER_ERROR_H__ */
/* ex:set ts=8 noet: */
