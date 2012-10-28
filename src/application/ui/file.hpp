// Opened file.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_FILE_HPP
#define SMYD_FILE_HPP

#include "../utilities/revision.hpp"
#include "../utilities/worker.hpp"
#include <utility>
#include <string>
#include <vector>
#include <stack>
#include <boost/signals2/signal.hpp>

namespace Samoyed
{

class Editor;
class Project;

/**
 * A file represents an opened file.  It is the in-memory buffer for a file
 * being edited by the user.  Each opened physical file has one and only one
 * file instance.  A file has one or more editors.  The file accepts each user
 * edit from an editor, and broadcasts the edit to the other editors.
 *
 * Files are user interface objects that can be accessed in the main thread
 * only.
 *
 * A file saves the editing history, supporting undo and redo.
 */
class File
{
public:
    class EditPrimitive;

    class Edit
    {
    public:
        virtual ~Edit() {}

        /**
         * @return True iff successful.
         */
        virtual bool execute(File &file) const = 0;

        /**
         * Merge this edit with the edit that will be executed immediately
         * before this edit.
         */
        virtual bool merge(const EditPrimitive &edit) { return false; }
    };

    /**
     * An edit primitive.
     */
    class EditPrimitive: public Edit
    {
    };

    /**
     * A group of edits.  It is used to group a sequence of edits that will be
     * executed together as one edit from the user's view.
     */
    class EditGroup: public Edit
    {
    public:
        virtual ~EditGroup();

        virtual bool execute(File &file) const;

        void add(Edit *edit) { m_edits.push_back(edit); }

        bool empty() const { return m_edits.empty(); }

    private:
        std::vector<Edit *> m_edits;
    };

    typedef boost::signals2::signal<void (const File &file)> Close;
    typedef boost::signals2::signal<void (const File &file)> Loaded;
    typedef boost::signals2::signal<void (const File &file)> Saved;
    typedef boost::signals2::signal<void (const File &file, const Edit &)>
    	Edited;

    static std::pair<File *, Editor *> create(const char *uri,
                                              Project &project);

    Editor *createEditor(Project &project);

    bool destroyEditor(Editor &editor);

    const char *uri() const { return m_uri.c_str(); }

    const char *name() const { return m_name.c_str(); }

    const Revision &revision() const { return m_revision; }

    const GError *ioError() const { return m_ioError; }

    /**
     * @return True iff the file is being closed.
     */
    bool closing() const { return m_closing; }

    /**
     * @return True iff the file is being loaded.
     */
    bool loading() const { return m_loading; }

    /**
     * @return True iff the file is being saved.
     */
    bool saving() const { return m_saving; }

    bool edited() const { return m_editCount; }

    /**
     * Request to load the file.  The file cannot be loaded if it is being
     * closed, loaded or saved, or is frozen.  When loading, the file is frozen.
     * The caller can get notified of the completion of the loading by adding a
     * callback.
     * @return True iff the loading is started.
     */
    bool load();

    /**
     * Request to save the file.  The file cannot be saved if it is being
     * closed, loaded or saved, or is frozen.  When saving, the file is frozen.
     * The caller can get notified of the completion of the saving by adding a
     * callback.
     * @return True iff the saving is started.
     */
    bool save();

    /**
     * @return True iff the file can be edited.
     */
    bool editable() const
    { return !m_closing && !frozen(); }

    /**
     * @return True iff successful, i.e., the file can be edited.
     */
    bool edit(const Edit &edit)
    { return edit.execute(*this); }

    /**
     * @return True iff successful, i.e., the file can be edited.
     */
    bool beginEditGroup();

    /**
     * @return True iff successful, i.e., the file can be edited.
     */
    bool endEditGroup();

    bool undoable() const
    { return !m_undoHistory.empty() && !m_superUndo; }

    bool redoable() const
    { return !m_redoHistory.empty() && !m_superUndo; }

    /**
     * @return True iff successful, i.e., the file can be edited and undone.
     */
    bool undo();

    /**
     * @return True iff successful, i.e., the file can be edited and redone.
     */
    bool redo();

    bool frozen() const { return m_freezeCount; }

    /**
     * Disallow the user to edit the file.
     */
    void freeze();

    /**
     * Allow the user to edit the file.
     */
    void unfreeze();

    boost::signals2::connection
    addCloseCallback(const Closed::slot_type &callback)
    { return m_close.connect(callback); }

    boost::signals2::connection
    addLoadedCallback(const Loaded::slot_type &callback)
    { return m_loaded.connect(callback); }

    boost::signals2::connection
    addSavedCallback(const Saved::slot_type &callback)
    { return m_saved.connect(callback); }

    boost::signals2::connection
    addEditedCallback(const Edited::slot_type &callback)
    { return m_edited.connect(callback); }

protected:
    class Loader: public Worker
    {
    };

    class Saver: public Worker
    {
    };

    File(const char *uri);

    ~File();

    virtual Editor *newEditor(Project &project) = 0;

    virtual Loader *createLoader(unsigned int priority,
                                 const Worker::Callback &callback) = 0;

    virtual Saver *createSaver(unsigned int priority,
                               const Worker::Callback &callback) = 0;

    virtual void onLoaded(Loader &loader) = 0;

    virtual void onSaved(Saver &saver) = 0;

private:
    /**
     * A stack of edits.
     */
    class EditStack: public Edit
    {
    public:
        virtual ~EditStack() { clear(); }

        virtual bool execute(File &file) const;

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

    void continueClosing();

    static gboolean onLoadedInMainThread(gpointer param);

    static gboolean onSavedInMainThread(gpointer param);

    void onLoadedBase(Worker &worker);

    void onSavedBase(Worker &worker);

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

    const std::string m_uri;

    const std::string m_name;

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

    Close m_close;
    Loaded m_loaded;
    Saved m_saved;
    Edited m_edited;
};

}

#endif
