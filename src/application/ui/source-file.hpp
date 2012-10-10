// Opened source file.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_SOURCE_FILE_HPP
#define SMYD_SOURCE_FILE_HPP

#include "../utilities/revision.hpp"
#include "../utilities/manager.hpp"
#include <string>
#include <vector>
#include <stack>
#include <boost/signals2/signal.hpp>
#include <glib.h>
#include <gtk/gtk.h>

namespace Samoyed
{

class SourceFileEditor;
class TextFileReader;
class FileSource;
class FileAst;

/**
 * A document is the in-memory buffer for a file being edited by the user.  It
 * can be opened, loaded, edited, saved and closed by the user.  It is a user
 * interface object that can be accessed in the main thread only.
 *
 * For a file, the document is the model while the editors are the views.
 * Basically, a document is a buffer implemented a GTK+ text buffer, and editors
 * are views implemented by GTK+ text views.  For a source file, we extend their
 * functionality further.  The document also models the abstract syntax tree
 * representation of the source file, and the editors not only display the text
 * but also performs syntax highlighting, code folding, etc. based on the
 * abstract syntax tree.
 *
 * A document saves the editing history, supporting undo and redo.
 *
 * When a document is opened, it keeps a reference to the file source, and then
 * pushes user edits so as to let the file source update it and, if the file is
 * a source file, the related abstract syntax trees.
 */
class Document
{
private:
    typedef boost::signals2::signal<void (const Document &doc)> Closed;
    typedef boost::signals2::signal<void (const Document &doc)> Loaded;
    typedef boost::signals2::signal<void (const Document &doc)> Saved;
    typedef boost::signals2::signal<void (const Document &doc,
                                          int line,
                                          int column,
                                          const char *text,
                                          int length)> TextInserted;
    typedef boost::signals2::signal<void (const Document &doc,
                                          int beginLine,
                                          int beginColumn,
                                          int endLine,
                                          int endColumn)> TextRemoved;

public:
    class Insertion;
    class Removal;

    class Edit
    {
    public:
        virtual ~Edit() {}

        /**
         * @return True iff successful.
         */
        virtual bool execute(Document &document) const = 0;

        /**
         * Merge this text edit with the edit that will be executed immediately
         * before this edit.
         */
        virtual bool merge(const Insertion &ins) { return false; }
        virtual bool merge(const Removal &rem) { return false; }
    };

    class Insertion: public Edit
    {
    public:
        /**
         * @param line The line number of the insertion position, starting from
         * 0.
         * @param column The column number of the insertion position, the
         * character index, starting from 0.
         * @param text The text to be inserted.
         * @param length The number of the bytes to be inserted, or -1 if
         * inserting the text until '\0'.
         */
        Insertion(int line, int column, const char *text, int length):
            m_line(line),
            m_column(column),
            m_text(text, length)
        {}

        virtual bool execute(Document &document) const
        {
            return document.insert(m_line, m_column, m_text.c_str(), -1);
        }

        virtual bool merge(const Insertion &ins);

    private:
        int m_line;
        int m_column;
        std::string m_text;
    };

    class Removal: public Edit
    {
    public:
        /**
         * @param beginLine The line number of the first character to be
         * removed, starting from 0.
         * @param beginColumn The column number of the first character to be
         * removed, the character index, starting from 0.
         * @param endLine The line number of the exclusive last character to be
         * removed, starting from 0.
         * @param endColumn The column number of the exclusive last character to
         * be removed, the character index, starting from 0.
         */
        Removal(int beginLine, int beginColumn, int endLine, int endColumn):
            m_beginLine(beginLine),
            m_beginColumn(beginColumn),
            m_endLine(endLine),
            m_endColumn(endColumn)
        {}

        virtual bool execute(Document &document) const
        {
            return document.remove(m_beginLine, m_beginColumn,
                                   m_endLine, m_endColumn);
        }

        virtual bool merge(const Removal &rem);

    private:
        int m_beginLine;
        int m_beginColumn;
        int m_endLine;
        int m_endColumn;
    };

    /**
     * A group of text edits.  It is used to group a sequence of edits that will
     * be executed together as one edit from the user's view.
     */
    class EditGroup: public Edit
    {
    public:
        virtual ~EditGroup();

        virtual bool execute(Document &document) const;

        void add(Edit *edit) { m_edits.push_back(edit); }

        bool empty() const { return m_edits.empty(); }

    private:
        std::vector<Edit *> m_edits;
    };

    /**
     * Create the data that will be shared by all the documents.  It is required
     * that this function be called before any document is created.  This
     * function creates a tag group.
     */
    static void createSharedData();

    static void destroySharedData();

    const char *uri() const { return m_uri.c_str(); }

    const char *name() const { return m_name.c_str(); }

    const Revision &revision() const { return m_revision; }

    const GError *ioError() const { return m_ioError; }

    /**
     * @return True iff the document is being closed.
     */
    bool closing() const { return m_closing; }

    /**
     * @return True iff the document is being loaded.
     */
    bool loading() const { return m_loading; }

    /**
     * @return True iff the document is being saved.
     */
    bool saving() const { return m_saving; }

    bool changed() const { return m_editCount; }

    /**
     * Request to load the document from the external file.  The document cannot
     * be loaded if it is being closed, loaded or saved, or is frozen.  When
     * loading, the document is frozen.  The caller can get notified of the
     * completion of the loading by adding a callback.
     * @return True iff the loading is started.
     */
    bool load();

    /**
     * Request to save the document into the external file.  The document cannot
     * be saved if it is being closed, loaded or saved, or is frozen.  When
     * saving, the document is frozen.  The caller can get notified of the
     * completion of the saving by adding a callback.
     * @return True iff the saving is started.
     */
    bool save();

    /**
     * @return The whole text contents, in a memory chunk allocated by GTK+.
     */
    char *getText() const;

    /**
     * @param beginLine The line number of the first character to be returned,
     * starting from 0.
     * @param beginColumn The column number of the first character to be
     * returned, the character index, starting from 0.
     * @param endLine The line number of the exclusive last character to be
     * returned, starting from 0.
     * @param endColumn The column number of the exclusive last character to be
     * returned, the character index, starting from 0.
     * @return The text contents, in a memory chunk allocated by GTK+.
     */
    char *getText(int beginLine, int beginColumn,
                  int endLine, int endColumn) const;

    /**
     * @return True iff the document can be edited.
     */
    bool editable() const
    { return !m_closing && !frozen(); }

    /**
     * @param line The line number of the insertion position, starting from 0.
     * @param column The column number of the insertion position, the
     * character index, starting from 0.
     * @param text The text to be inserted.
     * @param length The number of the bytes to be inserted, or -1 if inserting
     * the text until '\0'.
     * @return True iff successful, i.e., the document can be edited.
     */
    bool insert(int line, int column, const char *text, int length);

    /**
     * @param beginLine The line number of the first character to be removed,
     * starting from 0.
     * @param beginColumn The column number of the first character to be
     * removed, the character index, starting from 0.
     * @param endLine The line number of the exclusive last character to be
     * removed, starting from 0.
     * @param endColumn The column number of the exclusive last character to be
     * removed, the character index, starting from 0.
     * @return True iff successful, i.e., the document can be edited.
     */
    bool remove(int beginLine, int beginColumn, int endLine, int endColumn);

    /**
     * @return True iff successful, i.e., the document can be edited.
     */
    bool edit(const Edit &edit)
    { return edit.execute(*this); }

    /**
     * @return True iff successful, i.e., the document can be edited.
     */
    bool beginUserAction();

    /**
     * @return True iff successful, i.e., the document can be edited.
     */
    bool endUserAction();

    bool undoable() const
    { return !m_undoHistory.empty() && !m_superUndo; }

    bool redoable() const
    { return !m_redoHistory.empty() && !m_superUndo; }

    /**
     * @return True iff successful, i.e., the document can be edited and undone.
     */
    bool undo();

    /**
     * @return True iff successful, i.e., the document can be edited and redone.
     */
    bool redo();

    bool frozen() const { return m_freezeCount; }

    /**
     * Disallow the user to edit the document.
     */
    void freeze();

    /**
     * Allow the user to edit the document.
     */
    void unfreeze();

    boost::signals2::connection
    addClosedCallback(const Closed::slot_type &callback)
    { return m_closed.connect(callback); }

    boost::signals2::connection
    addLoadedCallback(const Loaded::slot_type &callback)
    { return m_loaded.connect(callback); }

    boost::signals2::connection
    addSavedCallback(const Saved::slot_type &callback)
    { return m_saved.connect(callback); }

    boost::signals2::connection
    addTextInsertedCallback(const TextInserted::slot_type &callback)
    { return m_textInserted.connect(callback); }

    boost::signals2::connection
    addTextRemovedCallback(const TextRemoved::slot_type &callback)
    { return m_textRemoved.connect(callback); }

private:
    /**
     * A stack of text edits.
     */
    class EditStack: public Edit
    {
    public:
        virtual ~EditStack() { clear(); }

        virtual bool execute(Document &document) const;

        template<class EditT> bool push(EditT *edit, bool mergeable)
        {
            if (m_edits.empty() || !mergeable)
            {
                m_edits.push(edit);
                return false;
            }
            if (m_edits.top()->merge(edit))
            {
                delete edit;
                return true;
            }
            m_edits.push(edit);
            return false;
        }

        Edit *top() const { return m_edits.top(); }

        void pop()
        {
            delete m_edits.top();
            m_edits.pop();
        }

        bool empty() const { return m_edits.empty(); }

        void clear();

    private:
        std::stack<Edit *> m_edits;
    };

    // Functions called by the document manager.
    Document(const char *uri);

    ~Document();

    /**
     * Request to close the document.  Closing the document is cooperatively
     * done by the document manager and the document itself.  This function gets
     * the document ready for closing, and then the document manager destroys
     * the editors and the document.  If the document is being loaded, cancel
     * the loading and discard the user changes, if any.  If it is being saved,
     * wait for the completion of the saving.  If it was changed by the user,
     * ask the user whether we need to save or discard the changes, or cancel
     * closing it.  The document manager will get notified when the document is
     * ready for closing.
     * @return True iff the closing is started.
     */
    bool close();

    void addEditor(Editor &editor);

    void removeEditor(Editor &editor);

    static gboolean onLoadedInMainThread(gpointer param);

    static gboolean onSavedInMainThread(gpointer param);

    void onLoaded(Worker &worker);

    void onSaved(Worker &worker);

    static void onTextInserted(GtkTextBuffer *buffer,
                               GtkTextIter *position,
                               gchar *text,
                               gint length,
                               gpointer document);

    static void onTextRemoved(GtkTextBuffer *buffer,
                              GtkTextIter *begin,
                              GtkTextIter *end,
                              gpointer document);

    static void onUserActionBegun(GtkTextBuffer *buffer,
                                  gpointer document);

    static void onUserActionEnded(GtkTextBuffer *buffer,
                                  gpointer document);

    template<class EditT> void saveUndo(EditT *undo)
    {
        if (m_undoing)
            m_redoHistory.push(undo, false);
        else if (m_redoing)
            m_undoHistory.push(undo, false);
        else
        {
            // If the edit count is zero, do not merge undoes.
            m_undoHistory.push(undo, !m_editCount);
        }
    }

    /**
     * The tag table shared by all the GTK+ text buffers.
     */
    static GtkTextTagTable *s_sharedTagTable;

    std::string m_uri;

    std::string m_name;

    bool m_closing;

    bool m_loading;

    bool m_saving;

    bool m_undoing;

    bool m_redoing;

    /**
     * The GTK+ text buffer.
     */
    GtkTextBuffer *m_buffer;

    Revision m_revision;

    GError *m_ioError;

    EditStack m_undoHistory;

    EditStack m_redoHistory;

    EditStack *m_superUndo;

    /**
     * The number of text edits performed since the latest document loading or
     * saving.  It may become negative if the user undoes edits.
     */
    int m_editCount;

    /**
     * The editors that are editing this document.
     */
    Editor *m_editors;

    int m_freezeCount;

    /**
     * The worker for reading the text file.  We memorize it so that we can
     * cancel it later.
     */
    TextFileReadWorker *m_fileReader;

    ReferencePointer<FileSource> m_source;
    ReferencePointer<FileAst> m_ast;

    Closed m_closed;
    Loaded m_loaded;
    Saved m_saved;
    TextInserted m_textInserted;
    TextRemoved m_textRemoved;

    friend class DocumentManager;
};

}

#endif
