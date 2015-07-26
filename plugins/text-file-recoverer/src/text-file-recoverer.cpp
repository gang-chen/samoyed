// Text file recoverer.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "text-file-recoverer.hpp"
#include "text-edit.hpp"
#include "text-file-recoverer-plugin.hpp"
#include "editors/text-file.hpp"
#include "window/window.hpp"
#include "application.hpp"
#include "utilities/scheduler.hpp"
#include "utilities/miscellaneous.hpp"
#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>

namespace Samoyed
{

namespace TextFileRecoverer
{

TextFileRecoverer::ReplayFileReader::ReplayFileReader(Scheduler &scheduler,
                                                      unsigned int priority,
                                                      const char *uri):
    RawFileLoader(scheduler, priority, uri)
{
    char *desc = g_strdup_printf("Reading replay file \"%s\".", uri);
    setDescription(desc);
    g_free(desc);
}

bool TextFileRecoverer::ReplayFileReader::step()
{
    RawFileLoader::step();

    // After reading the file, remove it.
    char *fileName = g_filename_from_uri(uri(), NULL, NULL);
    g_unlink(fileName);
    g_free(fileName);
    return true;
}

TextFileRecoverer::TextFileRecoverer(TextFile &file,
                                     long timeStamp):
    m_file(file),
    m_timeStamp(timeStamp),
    m_replayFileRead(false)
{
    TextFileRecovererPlugin::instance().onTextFileRecoveringBegun(*this);
    m_file.pin(_("Samoyed is recovering it"));
    if (file.loading())
        m_fileLoadedConnection = file.addLoadedCallback(boost::bind(
            &TextFileRecoverer::onFileLoaded, this, _1));
    char *fileName = TextFileRecovererPlugin::getTextReplayFileName(
        m_file.uri(), m_timeStamp);
    char *uri = g_filename_to_uri(fileName, NULL, NULL);
    m_replayFileReader.reset(new ReplayFileReader(
        Application::instance().scheduler(),
        Worker::PRIORITY_INTERACTIVE,
        uri));
    m_replayFileReaderFinishedConn =
        m_replayFileReader->addFinishedCallbackInMainThread(
            boost::bind(&TextFileRecoverer::onReplayFileReaderFinished,
                        this, _1));
    m_replayFileReaderCanceledConn =
        m_replayFileReader->addCanceledCallbackInMainThread(
            boost::bind(&TextFileRecoverer::onReplayFileReaderCanceled,
                        this, _1));
    m_replayFileReader->submit(m_replayFileReader);
    g_free(uri);
    g_free(fileName);
}

TextFileRecoverer::~TextFileRecoverer()
{
    m_file.unpin(_("Samoyed is recovering it"));
    TextFileRecovererPlugin::instance().onTextFileRecoveringEnded(*this);
}

void TextFileRecoverer::deactivate()
{
    m_fileLoadedConnection.disconnect();
    m_replayFileReaderFinishedConn.disconnect();
    m_replayFileReaderCanceledConn.disconnect();
    m_replayFileReader->cancel(m_replayFileReader);
    delete this;
}

void TextFileRecoverer::recoverFromReplayFile()
{
    const char *byteCode = m_byteCode.get();
    TextEdit::replay(m_file, byteCode, m_byteCodeLength);

    // Automatically deactivate the recoverer when done.
    deactivate();
}

void TextFileRecoverer::onFileLoaded(File &file)
{
    if (m_replayFileRead)
        recoverFromReplayFile();
}

void TextFileRecoverer::onReplayFileReaderFinished(
    const boost::shared_ptr<Worker> &worker)
{
    assert(m_replayFileReader == worker);
    if (m_replayFileReader->error())
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            GTK_WINDOW(Application::instance().currentWindow().gtkWidget()),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to recover file \"%s\"."),
            m_file.uri());
        gtkMessageDialogAddDetails(
            dialog,
            _("Samoyed failed to read the automatically saved text edit replay "
              "file to recover file \"%s\". %s"),
            m_file.uri(), m_replayFileReader->error()->message);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        deactivate();
    }
    else
    {
        m_replayFileRead = true;
        m_byteCode = m_replayFileReader->contents();
        m_byteCodeLength = m_replayFileReader->length();
        if (!m_file.loading())
            recoverFromReplayFile();
    }
}

void TextFileRecoverer::onReplayFileReaderCanceled(
    const boost::shared_ptr<Worker> &worker)
{
    assert(0);
}

}

}
