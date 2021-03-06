// Action extension: find text.
// Copyright (C) 2014 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "find-text-action-extension.hpp"
#include "text-finder-bar.hpp"
#include "finder-plugin.hpp"
#include "editors/editor.hpp"
#include "editors/file.hpp"
#include "editors/text-editor.hpp"
#include "window/window.hpp"
#include "widget/notebook.hpp"
#include "widget/widget-with-bars.hpp"
#include <boost/bind.hpp>
#include <boost/ref.hpp>
#include <gio/gio.h>

namespace Samoyed
{

namespace Finder
{

void FindTextActionExtension::activateAction(Window &window, GtkAction *action)
{
    if (!isActionSensitive(window, action))
        return;
    TextEditor &editor =
        static_cast<Samoyed::TextEditor &>(window.currentEditorGroup().
                                           currentChild());
    TextFinderBar *bar = TextFinderBar::create(editor, m_next);
    window.mainArea().addBar(*bar, true);
    bar->setCurrent();
    FinderPlugin::instance().onTextFinderBarCreated(*bar);
    bar->addClosedCallback(boost::bind(&FinderPlugin::onTextFinderBarClosed,
                                       boost::ref(FinderPlugin::instance()),
                                       _1));
}

bool FindTextActionExtension::isActionSensitive(Window &window,
                                                GtkAction *action)
{
    Notebook &editorGroup = window.currentEditorGroup();
    if (editorGroup.childCount() == 0)
        return false;
    Editor &editor = static_cast<Samoyed::Editor &>(editorGroup.currentChild());
    const char *mimeType = editor.file().mimeType();
    char *type = g_content_type_from_mime_type(mimeType);
    char *textType = g_content_type_from_mime_type("text/plain");
    bool supported = g_content_type_is_a(type, textType);
    g_free(type);
    g_free(textType);
    return supported;
}

}

}
