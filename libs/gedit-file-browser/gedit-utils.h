/*
 * gedit-utils.h
 * This file is part of gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
 * Copyright (C) 2000, 2001 Chema Celorio, Paolo Maggi
 * Copyright (C) 2002 - 2005 Paolo Maggi
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GEDIT_UTILS_H__
#define __GEDIT_UTILS_H__

#include <glib.h>
#include <gtk/gtk.h>
#include <atk/atk.h>

G_BEGIN_DECLS

/* useful macro */
#define GBOOLEAN_TO_POINTER(i) (GINT_TO_POINTER ((i) ? 2 : 1))
#define GPOINTER_TO_BOOLEAN(i) ((gboolean) ((GPOINTER_TO_INT(i) == 2) ? TRUE : FALSE))

#define IS_VALID_BOOLEAN(v) (((v == TRUE) || (v == FALSE)) ? TRUE : FALSE)

enum { GEDIT_ALL_WORKSPACES = 0xffffffff };

void		 gedit_utils_menu_position_under_widget (GtkMenu          *menu,
							 gint             *x,
							 gint             *y,
							 gboolean         *push_in,
							 gpointer          user_data);

void		 gedit_utils_menu_position_under_tree_view
							(GtkMenu          *menu,
							 gint             *x,
							 gint             *y,
							 gboolean         *push_in,
							 gpointer          user_data);

gchar		*gedit_utils_escape_underscores		(const gchar      *text,
							 gssize            length);

gchar		*gedit_utils_str_middle_truncate	(const gchar      *string,
							 guint             truncate_length);

gchar		*gedit_utils_str_end_truncate		(const gchar      *string,
							 guint             truncate_length);

void		 gedit_utils_set_atk_name_description	(GtkWidget        *widget,
							 const gchar      *name,
							 const gchar      *description);

void		 gedit_utils_set_atk_relation		(GtkWidget        *obj1,
							 GtkWidget        *obj2,
							 AtkRelationType   rel_type);

void		 gedit_warning				(GtkWindow        *parent,
							 const gchar      *format,
							 ...) G_GNUC_PRINTF(2, 3);

gchar		*gedit_utils_make_valid_utf8		(const char       *name);

/* Note that this function replace home dir with ~ */
gchar		*gedit_utils_uri_get_dirname		(const char       *uri);

gchar		*gedit_utils_location_get_dirname_for_display
							(GFile            *location);

gchar		*gedit_utils_replace_home_dir_with_tilde(const gchar      *uri);

guint		 gedit_utils_get_current_workspace	(GdkScreen        *screen);

guint		 gedit_utils_get_window_workspace	(GtkWindow        *gtkwindow);

void		 gedit_utils_get_current_viewport	(GdkScreen        *screen,
							 gint             *x,
							 gint             *y);

gboolean	 gedit_utils_is_valid_location		(GFile            *location);

gboolean	 gedit_utils_get_ui_objects		(const gchar      *filename,
							 gchar           **root_objects,
							 GtkWidget       **error_widget,
							 const gchar      *object_name,
							 ...) G_GNUC_NULL_TERMINATED;

gboolean         gedit_utils_get_ui_objects_with_translation_domain
                                                        (const gchar  *filename,
                                                         const gchar  *translation_domain,
                                                         gchar       **root_objects,
                                                         GtkWidget   **error_widget,
                                                         const gchar  *object_name,
                                                         ...) G_GNUC_NULL_TERMINATED;

/* Return NULL if str is not a valid URI and/or filename */
gchar		*gedit_utils_make_canonical_uri_from_shell_arg
							(const gchar      *str);

gchar		*gedit_utils_basename_for_display	(GFile            *location);
gboolean	 gedit_utils_decode_uri 		(const gchar      *uri,
							 gchar           **scheme,
							 gchar           **user,
							 gchar           **port,
							 gchar           **host,
							 gchar           **path);


/* Turns data from a drop into a list of well formatted uris */
gchar		**gedit_utils_drop_get_uris		(GtkSelectionData *selection_data);

gchar           *gedit_utils_set_direct_save_filename	(GdkDragContext *context);

G_END_DECLS

#endif /* __GEDIT_UTILS_H__ */

/* ex:set ts=8 noet: */
