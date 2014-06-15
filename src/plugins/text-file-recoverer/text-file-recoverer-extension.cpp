// File recoverer extension: text file recoverer.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "text-file-recoverer-extension.hpp"
#include "text-file-recoverer-plugin.hpp"
#include "utilities/scheduler.hpp"
#include "utilities/miscellaneous.hpp"
#include "ui/text-file.hpp"
#include "ui/window.hpp"
#include "application.hpp"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>

namespace
{

const char *TEXT_REPLAY_FILE_PREFIX = ".smyd.txt.rep.";

}

namespace Samoyed
{

namespace TextFileRecoverer
{

TextFileRecovererExtension::ReplayFileReader::ReplayFileReader(
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

TextFileRecovererExtension::ReplayFileReader::~ReplayFileReader()
{
    g_free(m_byteCode);
}

bool TextFileRecovererExtension::ReplayFileReader::step()
{
    char *cp;

    cp = g_filename_from_uri(m_recoverer.m_file->uri(), NULL, NULL);
    std::string fileName(TEXT_REPLAY_FILE_PREFIX);
    fileName += cp;
    g_free(cp);

    GError *error = NULL;
    g_file_get_contents(fileName.c_str(),
                        &m_byteCode,
                        &m_byteCodeLength,
                        &error);
    if (error)
    {
        cp = g_strdup_printf(
            _("Samoyed failed to read text edit replay file \"%s\". %s."),
            m_recoverer.m_file->uri(), error->message);
        m_error = cp;
        g_free(cp);
        g_error_free(error);
        return true;
    }
    return true;
}

TextFileRecoverer::~TextFileRecoverer()
{
    assert(m_destroy);
}

void TextFileRecoverer::recoverFile(File &file)
{
    m_file = static_cast<TextFile *>(&file);
    m_closeFileConnection = file.addCloseCallback(boost::bind(
        &TextFileRecoverer::onCloseFile, this, _1));
    if (file.loading())
        m_fileLoadedConnection = file.addLoadedCallback(boost::bind(
            &TextFileRecoverer::onFileLoaded, this, _1));
    m_reader = new TextReplayFileReader(
        Application::instance().scheduler(),
        Worker::PRIORITY_INTERACTIVE,
        boost::bind(&TextFileRecoverer::onTextReplayFileRead, this, _1));
    Application::instance().scheduler().schedule(*m_reader);
}

void TextFileRecoverer::destroy()
{
    m_closeFileConnection.disconnect();
    m_fileLoadedConnection.disconnect();
    m_destroy = true;
    if (m_reader)
        m_reader->cancel();
    else
        m_plugin.destroyRecoverer(*this);
}

void TextFileRecoverer::onCloseFile()
{
    destroy();
}

void TextFileRecoverer::replayEdits()
{
    for (TextEdit *edit = m_reader->m_edits; edit; edit = edit->next)
    {
        if (edit->type == TextEdit::TYPE_INSERTION)
        {
            TextInsertion *ins = static_cast<TextInsertion *>(edit);
            m_file.insert(ins->m_line, ins->m_column,
                          ins->m_text, ins->m_length);
        }
        else if (edit->type == TextEdit::TYPE_REMOVAL)
        {
            TextRemoval *rem = static_cast<TextRemoval *>(edit);
            m_file.remove(rem->m_beginLine, rem->m_beginColumn,
                          rem->m_endLine, rem->m_endColumn);
        }
    }
}

void TextFileRecoverer::recoverFromReplayFile()
{
    // Check to see the current contents of the file are equal to the initial
    // text.
    char *text = m_file.text(0, 0, -1, -1);
    if (*(text + m_reader->m_initialLength) != '\0' ||
        memcmp(text, m_reader->m_initialText))
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            Application::instance().currentWindow().gtkWidget(),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_QUESTION,
            GTK_BUTTONS_YES_NO,
            _("The current contents of file \"%s\" are changed since the last "
              "edit. The changes will be discarded if you recover the file. "
              "Continue recovering it?"),
            m_file.uri());
        gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_NO);
        int response = gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);

        if (response == GTK_RESPONSE_YES)
        {
            m_plugin.onFileRecoveringBegun(m_file);
            m_file.remove(0, 0, -1, -1);
            m_file.insert(0, 0,
                          m_reader->m_initialText, m_reader->m_initialLength);
            replayEdits();
            m_plugin.onFileRecoveringEnded(m_file);
        }
    }
    else
    {
        m_plugin.onFileRecoveringBegun(m_file);
        replayEdits();
        m_plugin.onFileRecoveringEnded(m_file);
    }

    g_free(text);

    delete m_reader;
    m_reader = NULL;

    // Automatically destroy the recoverer when done.
    destroy();
}

gboolean TextFileRecoverer::recoverOnFileLoaded(gpointer recoverer)
{
    TextFileRecoverer *rec = static_cast<TextFileRecoverer *>(recoverer);

    if (rec->m_destroy)
    {
        delete rec->m_reader;
        rec->m_reader = NULL;
        m_plugin.destroyRecoverer(*rec);
    }

    rec->recoverFromReplayFile();
    return FALSE;
}

void TextFileRecoverer::onFileLoaded()
{
    if (m_read)
        g_idle_add_full(G_PRIORITY_HIGH_IDLE,
                        recoverOnFileLoaded,
                        this,
                        NULL);
}

gboolean TextFileRecoverer::onTextReplayFileReadInMainThread(gpointer recoverer)
{
    TextFileRecoverer *rec = static_cast<TextFileRecoverer *>(recoverer);

    if (rec->m_destroy)
    {
        delete rec->m_reader;
        rec->m_reader = NULL;
        m_plugin.destroyRecoverer(*rec);
    }

    if (!rec->m_reader->m_error.empty())
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            Application::instance().currentWindow().gtkWidget(),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to recover file \"%s\"."),
            rec->m_file.uri());
        gtkMessageDialogAddDetails(
            _("Samoyed failed to read the automatically saved text edit replay "
              "file \"%s\" to recover file \"%s\". %s"),
            rec->m_reader->m_uri.c_str(), rec->m_file.uri(),
            rec->m_reader->m_error.c_str());
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        delete rec->m_reader;
        rec->m_reader = NULL;
        destroy();
    }

    rec->m_read = true;
    if (rec->m_file.loaded())
        rec->recoverFromReplayFile();
    return FALSE;
}

void TextFileRecoverer::onTextReplayFileRead(Worker &worker)
{
    g_idle_add_full(G_PRIORITY_HIGH_IDLE,
                    onTextReplayFileReadInMainThread,
                    this,
                    NULL);
}

}

}
