// Opened file.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_FILE_HPP
#define SMYD_FILE_HPP

#include "../utilities/revision.hpp"
#include <utility>
#include <vector>
#include <string>
#include <boost/utility.hpp>
#include <boost/signals2/signal.hpp>
#include <glib.h>

namespace Samoyed
{

class Editor;
class Project;
class FileLoader;
class FileSaver;

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
class File: public boost::noncopyable
{
public:
    class EditPrimitive;

    class Edit
    {
    public:
        virtual ~Edit() {}

        /**
         * @return The reverse edit.
         */
        virtual Edit *execute(File &file) const = 0;

        /**
         * Merge this edit with the edit that will be executed immediately
         * before this edit.
         * @return True iff the given edit is merged.
         */
        virtual bool merge(const EditPrimitive *edit) { return false; }
    };

    /**
     * An edit primitive.
     */
    class EditPrimitive: public Edit
    {
    };

    /**
     * A stack of edits.
     */
    class EditStack: public Edit
    {
    public:
        virtual ~EditStack();

        virtual Edit *execute(File &file) const;

        bool empty() const { return m_edits.empty(); }

        Edit *top() const { return m_edits.back(); }

        /**
         * @param edit The edit to be pushed.  It will be owned by the stack
         * after pushed.
         */
        void push(Edit *edit) { m_edits.push_back(edit); }

        /**
         * @param edit The edit to be merged or pushed.  It will be owned by the
         * stack after merged or pushed.
         * @return True iff the given edit is merged.
         */
        bool mergePush(EditPrimitive *edit);

        void pop() { m_edits.pop_back(); }

    private:
        std::vector<Edit *> m_edits;
    };

    typedef boost::signals2::signal<void (const File &file)> Close;
    typedef boost::signals2::signal<void (const File &file)> Loaded;
    typedef boost::signals2::signal<void (const File &file)> Saved;
    typedef boost::signals2::signal<void (const File &file,
                                          const EditPrimitive &edit)> Edited;

    static std::pair<File *, Editor *> create(const char *uri,
                                              Project &project);

    Editor *createEditor(Project &project);

    /**
     * @return False iff the user cancels destroying the editor.
     */
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

    /**
     * @return True iff the file was edited.
     */
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
     */
    void save();

    /**
     * @return True iff the file can be edited.
     */
    bool editable() const
    { return !frozen(); }

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

    Editor *editors() const { return m_editors; }

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

    virtual ~File();

    // Functions implemented by subclasses and called by the base class.
    virtual Editor *newEditor(Project &project) = 0;

    virtual FileLoader *createLoader(unsigned int priority,
                                     const Worker::Callback &callback) = 0;

    virtual FileSaver *createSaver(unsigned int priority,
                                   const Worker::Callback &callback) = 0;

    virtual void onLoaded(FileLoader &loader) = 0;

    virtual void onSaved(FileSaver &saver) = 0;

    /**
     * This function is called by a subclass to notify all editors except the
     * committer and observers after edited.
     */
    void onEdited(const EditPrimitive &edit, const Editor *committer);

    /**
     * This function is called by a subclass to save the reverse edit of a newly
     * committed edit primitive.
     */
    void saveUndo(EditPrimitive *undo);

private:
    void continueClosing();

    void freezeInternally();

    void unfreezeInternally();

    static gboolean onLoadedInMainThread(gpointer param);

    static gboolean onSavedInMainThread(gpointer param);

    void onLoadedWrapper(Worker &worker);

    void onSavedWrapper(Worker &worker);

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
    FileLoader *m_loader;

    Close m_close;
    Loaded m_loaded;
    Saved m_saved;
    Edited m_edited;
};

}

#endif
