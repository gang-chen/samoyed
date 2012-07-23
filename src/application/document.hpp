// Document.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_DOCUMENT_HPP
#define SMYD_DOCUMENT_HPP

#include "document-loader.hpp"
#include "document-saver.hpp"
#include "utilities/manager.hpp"
#include <string>
#include <vector>
#include <stack>
#include <boost/thread/mutex.hpp>
#include <gtk/gtk.h>

namespace Samoyed
{

class Editor;
class SourceImage;
class CompiledImage;

/**
 * A document is the model of an editor or, actually, the model of the text view
 * in an editor.  A document is implemented by a GTK+ text buffer.
 *
 * A document saves the editing history, supporting undo and redo.
 *
 * If a document is a source file, it notifies the source image of text edits so
 * that the source image can update itself and notify the compiled image.  The
 * document also collects information on the abstract syntax tree and
 * diagnostics from the compiled image to perform syntax highlighting, code
 * folding, diagnostics highlighting, etc.
 */
class Document
{
public:
    class Insertion;
    class Removal;

    class Edit
    {
    public:
        virtual ~Edit() {}

        virtual void execute(Document &document) const = 0;

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
         * @param numChars The number of the characters to be inserted, or -1 if
         * the caller doesn't know it.
         */
        Insertion(int line, int column, const char *text, int length, int numChars):
            m_line(line),
            m_column(column),
            m_text(text),
            m_numChars(numChars)
        {}

        virtual void execute(Document &document) const
        { document.insert(m_line, m_column, m_text.c_str(), -1); }

        virtual bool merge(const Insertion &ins);

    private:
        int m_line;
        int m_column;
        std::string m_text;
        int m_numChars;
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

        virtual void execute(Document &document) const
        {
            document.remove(m_beginLine, m_beginColumn, m_endLine, m_endColumn);
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

        virtual void execute(Document &document) const;

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

    const char *fileName() const { return m_fileName.c_str(); }

    /**
     * @return The whole text contents, in a memory chunk allocated by GTK+.
     */
    char *getText() const
    {
        GtkTextIter begin, end;
        gtk_text_buffer_get_start_iter(m_gtkBuffer, &begin);
        gtk_text_buffer_get_end_iter(m_gtkBuffer, &end);
        return gtk_text_buffer_get_text(m_gtkBuffer, &begin, &end, TRUE);
    }

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
    char *getText(int beginLine,
                  int beginColumn,
                  int endLine,
                  int endColumn) const
    {
        GtkTextIter begin, end;
        gtk_text_buffer_get_iter_at_line_offset(m_gtkBuffer,
                                                &begin,
                                                beginLine,
                                                beginColumn);
        gtk_text_buffer_get_iter_at_line_offset(m_gtkBuffer,
                                                &end,
                                                endLine,
                                                endColumn);
        return gtk_text_buffer_get_text(m_gtkBuffer, &begin, &end, TRUE);
    }

    const Revision &revision() const { return m_revision; }

    int editCount() const { return m_editCount; }

    /**
     * Load the specified file into the document.
     */
    void load(const char *fileName, bool convertEncoding);

    /**
     * Save the document into the specified file.
     */
    void save(const char *fileName, bool convertEncoding);

    bool frozen() const { return m_frozen; }

    /**
     * Disallow the user to edit this document through the text views.
     */
    void freeze();

    /**
     * Allow the user to edit this document through the text views.
     */
    void unfreeze();

    /**
     * @param line The line number of the insertion position, starting from 0.
     * @param column The column number of the insertion position, the
     * character index, starting from 0.
     * @param text The text to be inserted.
     * @param length The number of the bytes to be inserted, or -1 if inserting
     * the text until '\0'.
     */
    void insert(int line, int column, const char *text, int length);

    /**
     * @param beginLine The line number of the first character to be removed,
     * starting from 0.
     * @param beginColumn The column number of the first character to be
     * removed, the character index, starting from 0.
     * @param endLine The line number of the exclusive last character to be
     * removed, starting from 0.
     * @param endColumn The column number of the exclusive last character to be
     * removed, the character index, starting from 0.
     */
    void remove(int beginLine, int beginColumn, int endLine, int endColumn);

    void edit(const Edit &edit)
    { edit.execute(*this); }

    void beginUserAction()
    { gtk_buffer_begin_user_action(m_gtkBuffer); }

    void endUserAction()
    { gtk_buffer_end_user_action(m_gtkBuffer); }

    bool canUndo() const
    { return !m_undoHistory.empty() && !m_superUndo; }

    bool canRedo() const
    { return !m_redoHistory.empty() && !m_superUndo; }

    void undo();

    void redo();

    void addEditor(Editor &editor);

    void removeEditor(Editor &editor);

private:
    /**
     * A stack of text edits.
     */
    class EditStack: public Edit
    {
    public:
        virtual ~EditStack() { clear(); }

        virtual void execute(Document &document) const;

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

    Document(const char *fileName);

    ~Document();

    void onLoaded(Worker &loader);

    void onSaved(Worker &saver);

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

    template<class Edit> void saveUndo(Edit *undo)
    {
        if (m_undoing)
        {
            // If the edit count is zero, do not merge undoes.
        }
    }

    /**
     * The tag table shared by all the GTK+ text buffers.
     */
    static GtkTextTagTable *s_sharedTagTable;

    std::string m_fileName;

    std::string m_shortName;

    bool m_initialized;

    bool m_frozen;

    bool m_loading;

    bool m_saving;

    bool m_undoing;

    bool m_redoing;

    /**
     * The GTK+ text buffer.
     */
    GtkTextBuffer *m_gtkBuffer;

    /**
     * The editors that are editing this document.
     */
    std::vector<Editor *> m_editors;

    EditStack m_undoHistory;

    EditStack m_redoHistory;

    EditStack *m_superUndo;

    int m_editCount;

    ReferencePointer<SourceImage> m_source;
    ReferencePointer<CompiledImage> m_compiled;

    friend class DocumentManager;
    friend class DocumentLoader;
};

}

#endif
