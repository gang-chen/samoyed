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
#include <glib.h>

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
        virtual void execute(File &file, Editor *editor) const = 0;

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
     * A group of edit primitives.  It is used to group a sequence of edit
     * primitives that will be executed together as one edit from the user's
     * view.
     */
    class EditGroup: public Edit
    {
    public:
        virtual ~EditGroup();

        virtual void execute(File &file, Editor *editor) const;

        void add(EditPrimitive *edit) { m_edits.push_back(edit); }

        bool empty() const { return m_edits.empty(); }

    private:
        std::vector<EditPrimitive *> m_edits;
    };

    class Loader: public Worker
    {
    };

    class Saver: public Worker
    {
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
    bool load(bool userRequest);

    /**
     * Request to save the file.  The file cannot be saved if it is being
     * closed, loaded or saved, or is frozen.  When saving, the file is frozen.
     * The caller can get notified of the completion of the saving by adding a
     * callback.
     * @return True iff the saving is started.
     */
    bool save(bool userRequest);

    /**
     * @return True iff the file can be edited.
     */
    bool editable() const
    { return !frozen(); }

    void edit(const Edit &edit, Editor *editor)
    { edit.execute(*this, editor); }

    bool inEditGroup() const
    { return m_superUndo; }

    void beginEditGroup();

    void endEditGroup();

    bool undoable() const
    { return editable() && !m_undoHistory.empty() && !m_superUndo; }

    bool redoable() const
    { return editable() && !m_redoHistory.empty() && !m_superUndo; }

    void undo();

    void redo();

    bool frozen() const
    { return m_freezeCount || m_internalFreezeCount; }

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
    File(const char *uri);

    ~File();

    virtual Editor *newEditor(Project &project) = 0;

    virtual Loader *createLoader(unsigned int priority,
                                 const Worker::Callback &callback) = 0;

    virtual Saver *createSaver(unsigned int priority,
                               const Worker::Callback &callback) = 0;

    virtual void onLoaded(Loader &loader) = 0;

    virtual void onSaved(Saver &saver) = 0;

    virtual PrimitiveEdit *edit(PrimitiveEdit *edit) = 0;

private:
    /**
     * A stack of edits.
     */
    class EditStack
    {
    public:
        ~EditStack();

        void push(Edit *edit) { m_edits.push(edit); }

        bool mergePush(EditPrimitive *edit);

        Edit *top() const { return m_edits.top(); }

        Edit *pop()
        {
            Edit *edit = m_edits.top();
            m_edits.pop();
            return edit;
        }

        bool empty() const { return m_edits.empty(); }

    private:
        std::stack<Edit *> m_edits;
    };

    void continueClosing();

    void freezeInternally();

    void unfreezeInternally();

    static gboolean onLoadedInMainThread(gpointer param);

    static gboolean onSavedInMainThread(gpointer param);

    void onLoadedBase(Worker &worker);

    void onSavedBase(Worker &worker);

    const std::string m_uri;

    const std::string m_name;

    bool m_closing;

    bool m_loading;

    bool m_saving;

    Revision m_revision;

    GError *m_ioError;

    EditStack m_undoHistory;

    EditStack m_redoHistory;

    EditStack *m_superUndo;

    /**
     * The number of edits performed since the latest loading or saving.
     */
    int m_editCount;

    /**
     * The editors that are editing this file.
     */
    Editor *m_editors;

    int m_freezeCount;
    int m_internalFreezeCount;

    // We memorize the file loader so that we can cancel it later.
    Loader *m_loader;

    Close m_close;
    Loaded m_loaded;
    Saved m_saved;
    Edited m_edited;
};

}

#endif
