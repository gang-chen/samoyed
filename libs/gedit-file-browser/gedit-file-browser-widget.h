/*
 * gedit-file-browser-widget.h - Gedit plugin providing easy file access
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

#ifndef __GEDIT_FILE_BROWSER_WIDGET_H__
#define __GEDIT_FILE_BROWSER_WIDGET_H__

#include <gtk/gtk.h>
#include "gedit-file-browser-store.h"
#include "gedit-file-bookmarks-store.h"
#include "gedit-file-browser-view.h"

G_BEGIN_DECLS
#define GEDIT_TYPE_FILE_BROWSER_WIDGET			(gedit_file_browser_widget_get_type ())
#define GEDIT_FILE_BROWSER_WIDGET(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_FILE_BROWSER_WIDGET, GeditFileBrowserWidget))
#define GEDIT_FILE_BROWSER_WIDGET_CONST(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_FILE_BROWSER_WIDGET, GeditFileBrowserWidget const))
#define GEDIT_FILE_BROWSER_WIDGET_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_FILE_BROWSER_WIDGET, GeditFileBrowserWidgetClass))
#define GEDIT_IS_FILE_BROWSER_WIDGET(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_FILE_BROWSER_WIDGET))
#define GEDIT_IS_FILE_BROWSER_WIDGET_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_FILE_BROWSER_WIDGET))
#define GEDIT_FILE_BROWSER_WIDGET_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEDIT_TYPE_FILE_BROWSER_WIDGET, GeditFileBrowserWidgetClass))

typedef struct _GeditFileBrowserWidget        GeditFileBrowserWidget;
typedef struct _GeditFileBrowserWidgetClass   GeditFileBrowserWidgetClass;
typedef struct _GeditFileBrowserWidgetPrivate GeditFileBrowserWidgetPrivate;

typedef
gboolean (*GeditFileBrowserWidgetFilterFunc) (GeditFileBrowserWidget *obj,
					      GeditFileBrowserStore  *model,
					      GtkTreeIter            *iter,
					      gpointer                user_data);

struct _GeditFileBrowserWidget
{
	GtkBox parent;

	GeditFileBrowserWidgetPrivate *priv;
};

struct _GeditFileBrowserWidgetClass
{
	GtkBoxClass parent_class;

	/* Signals */
	void (* location_activated)	(GeditFileBrowserWidget *widget,
					 GFile                  *location);
	void (* error)			(GeditFileBrowserWidget *widget,
					 guint                   code,
					 gchar const            *message);
	gboolean (* confirm_delete)	(GeditFileBrowserWidget *widget,
					 GeditFileBrowserStore  *model,
					 GList                  *list);
	gboolean (* confirm_no_trash)	(GeditFileBrowserWidget *widget,
					 GList                  *list);
	void (* open_in_terminal)       (GeditFileBrowserWidget *widget,
	                                 GFile                  *location);
	void (* set_active_root)        (GeditFileBrowserWidget *widget);
};

GType		 gedit_file_browser_widget_get_type            (void) G_GNUC_CONST;

GtkWidget	*gedit_file_browser_widget_new            	(void);

void		 gedit_file_browser_widget_show_bookmarks       (GeditFileBrowserWidget *obj);
void		 gedit_file_browser_widget_show_files           (GeditFileBrowserWidget *obj);

void		 gedit_file_browser_widget_set_root		(GeditFileBrowserWidget *obj,
								 GFile                  *root,
								 gboolean                virtual_root);
void		gedit_file_browser_widget_set_root_and_virtual_root
								(GeditFileBrowserWidget *obj,
								 GFile                  *root,
								 GFile                  *virtual_root);

gboolean	 gedit_file_browser_widget_get_selected_directory
								(GeditFileBrowserWidget *obj,
								 GtkTreeIter            *iter);

void             gedit_file_browser_widget_set_active_root_enabled (GeditFileBrowserWidget *widget,
                                                                    gboolean                enabled);

GeditFileBrowserStore *
gedit_file_browser_widget_get_browser_store         		(GeditFileBrowserWidget *obj);
GeditFileBookmarksStore *
gedit_file_browser_widget_get_bookmarks_store			(GeditFileBrowserWidget *obj);
GeditFileBrowserView *
gedit_file_browser_widget_get_browser_view			(GeditFileBrowserWidget *obj);
GtkWidget *
gedit_file_browser_widget_get_filter_entry			(GeditFileBrowserWidget *obj);

gulong gedit_file_browser_widget_add_filter			(GeditFileBrowserWidget *obj,
								 GeditFileBrowserWidgetFilterFunc func,
								 gpointer                user_data,
								 GDestroyNotify          notify);
void		 gedit_file_browser_widget_remove_filter	(GeditFileBrowserWidget *obj,
								 gulong                  id);
void		 gedit_file_browser_widget_set_filter_pattern	(GeditFileBrowserWidget *obj,
								 gchar const            *pattern);
GMenuModel *
		 gedit_file_browser_widget_extend_context_menu	(GeditFileBrowserWidget *obj);
void		 gedit_file_browser_widget_refresh		(GeditFileBrowserWidget *obj);
void		 gedit_file_browser_widget_history_back		(GeditFileBrowserWidget *obj);
void		 gedit_file_browser_widget_history_forward	(GeditFileBrowserWidget *obj);

void		 _gedit_file_browser_widget_register_type      (GTypeModule            *type_module);

G_END_DECLS

#endif /* __GEDIT_FILE_BROWSER_WIDGET_H__ */
/* ex:set ts=8 noet: */
