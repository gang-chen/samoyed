// Text file recoverer.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "text-file-recoverer.hpp"
#include "text-edit.hpp"
#include "text-file-recoverer-plugin.hpp"
#include "application.hpp"
#include "utilities/scheduler.hpp"
#include "utilities/miscellaneous.hpp"
#include "ui/text-file.hpp"
#include "ui/window.hpp"
#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>

namespace Samoyed
{

namespace TextFileRecoverer
{

TextFileRecoverer::ReplayFileReader::ReplayFileReader(
    Scheduler &scheduler,
    unsigned int priority,
    const Callback &callback,
    TextFileRecoverer &recoverer):
    Worker(scheduler,
           priority,
           callback),
    m_recoverer(recoverer),
    m_byteCode(NULL),
    m_byteCodeLength(0)
{
}

TextFileRecoverer::ReplayFileReader::~ReplayFileReader()
{
    g_free(m_byteCode);
}

bool TextFileRecoverer::ReplayFileReader::step()
{
    GError *error = NULL;
    char *fileName = TextFileRecovererPlugin::getTextReplayFileName(
        m_recoverer.m_file.uri());
    g_file_get_contents(fileName,
                        &m_byteCode,
                        &m_byteCodeLength,
                        &error);
    g_free(fileName);
    if (error)
    {
        m_error = error->message;
        g_error_free(error);
        return true;
    }
    return true;
}

TextFileRecoverer::TextFileRecoverer(TextFile &file,
                                     TextFileRecovererPlugin &plugin):
    m_file(file),
    m_plugin(plugin),
    m_destroy(false),
    m_reader(NULL),
    m_read(false)
{
    m_plugin.onTextFileRecoveringBegun(*this);
    m_file.reserve(_("Samoyed is recovering it"));
    if (file.loading())
        m_fileLoadedConnection = file.addLoadedCallback(boost::bind(
            &TextFileRecoverer::onFileLoaded, this, _1));
    m_reader = new ReplayFileReader(
        Application::instance().scheduler(),
        Worker::PRIORITY_INTERACTIVE,
        boost::bind(&TextFileRecoverer::onReplayFileRead, this, _1),
        *this);
    Application::instance().scheduler().schedule(*m_reader);
}

TextFileRecoverer::~TextFileRecoverer()
{
    delete m_reader;
    m_file.release(_("Samoyed is recovering it"));
    m_plugin.onTextFileRecoveringEnded(*this);
}

void TextFileRecoverer::deactivate()
{
    m_fileLoadedConnection.disconnect();
    m_destroy = true;
    if (m_reader && !m_read)
        m_reader->cancel();
    else
        delete this;
}

void TextFileRecoverer::recoverFromReplayFile()
{
    const char *byteCode = m_reader->m_byteCode;
    int length = m_reader->m_byteCodeLength;
    TextEdit::replay(m_file, byteCode, length);

    // Automatically deactivate the recoverer when done.
    deactivate();
}

void TextFileRecoverer::onFileLoaded(File &file)
{
    if (m_read)
        recoverFromReplayFile();
}

gboolean TextFileRecoverer::onReplayFileReadInMainThread(gpointer recoverer)
{
    TextFileRecoverer *rec = static_cast<TextFileRecoverer *>(recoverer);
    rec->m_read = true;

    if (rec->m_destroy)
        delete rec;
    else if (!rec->m_reader->m_error.empty())
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            GTK_WINDOW(Application::instance().currentWindow().gtkWidget()),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to recover file \"%s\"."),
            rec->m_file.uri());
        gtkMessageDialogAddDetails(
            dialog,
            _("Samoyed failed to read the automatically saved text edit replay "
              "file to recover file \"%s\". %s"),
            rec->m_file.uri(), rec->m_reader->m_error.c_str());
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        rec->deactivate();
    }
    else
    {
        if (!rec->m_file.loading())
            rec->recoverFromReplayFile();
    }
    return FALSE;
}

void TextFileRecoverer::onReplayFileRead(Worker &worker)
{
    g_idle_add_full(G_PRIORITY_HIGH_IDLE,
                    onReplayFileReadInMainThread,
                    this,
                    NULL);
}

}

}
