// Opened file.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_FILE_HPP
#define SMYD_FILE_HPP

#include "utilities/miscellaneous.hpp"
#include "utilities/revision.hpp"
#include "utilities/worker.hpp"
#include <utility>
#include <list>
#include <string>
#include <map>
#include <boost/utility.hpp>
#include <boost/function.hpp>
#include <boost/signals2/signal.hpp>
#include <boost/any.hpp>
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
 *
 * To edit a file, the derived class performs the edit, notifies the base class
 * of the change and requests the base class to save the reverse edit.  To undo
 * or redo an edit, the base class executes the saved edit, which performs the
 * real edit and notifies the base class of the change.
 */
class File: public boost::noncopyable
{
public:
    typedef boost::signals2::signal<void (File &file)> Opened;
    typedef boost::signals2::signal<void (File &file)> Close;
    typedef boost::signals2::signal<void (File &file)> Loaded;
    typedef boost::signals2::signal<void (File &file)> Saved;

    typedef
    boost::function<File *(const char *uri, Project *project,
                           const std::map<std::string, boost::any> &options)>
    	Factory;

    /**
     * A change.  Derived classes should define their concrete changes.
     */
    class Change
    {
    public:
        enum Type
        {
            TYPE_INIT
        };
        int m_type;
        Change(int type): m_type(type) {}
    };

    /**
     * @param change The change that was made.
     * @param loading True iff the change was due to file loading.
     */
    typedef boost::signals2::signal<void (const File &file,
                                          const Change &change,
                                          bool loading)> Changed;

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
     * An edit primitive.  Derived classes should define their concrete edit
     * primitives.
     */
    class EditPrimitive: public Edit {};

    /**
     * A stack of edits.
     */
    class EditStack: public Edit
    {
    public:
        virtual ~EditStack() { clear(); }

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

        void clear();

    private:
        std::list<Edit *> m_edits;
    };

    class OptionSetters
    {
    public:
        virtual ~OptionSetters() {}
        virtual GtkWidget *takeGtkWidget() = 0;
        virtual void setOptions(std::map<std::string, boost::any> &options) = 0;
    };

    typedef boost::function<OptionSetters *()> OptionSettersFactory;

    static void registerType(const char *type,
                             const Factory &factory,
                             const OptionSettersFactory &optSettersFactory);

    static void installHistories();

    /**
     * Open a file in an editor.
     * @param uri The URI of the new file.
     * @param project The project context, or NULL if none.
     * @param options Additional file-type-specific options specifying the
     * behavior.
     * @param newEditor True to create a new editor even if the file is already
     * opened.
     */
    static std::pair<File *, Editor *>
    open(const char *uri, Project *project,
         const std::map<std::string, boost::any> &options,
         bool newEditor);

    /**
     * Open a dialog to let the user choose the file to open and set additional
     * optins, and open the chosen file if any.
     * @param project The project context, or NULL if none.
     */
    static void openByDialog(Project *project,
                             std::list<std::pair<File *, Editor *> > &opened);

    /**
     * Close a file by closing all editors.
     * @return False iff the user cancels closing the file.
     */
    bool close();

    /**
     * This function can be called by the application instance only.
     */
    virtual ~File();

    /**
     * Create an editor.
     */
    Editor *createEditor(Project *project);

    /**
     * Close an editor.
     * @return False iff the user cancels closing the editor.
     */
    bool closeEditor(Editor &editor);

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

    void addEditor(Editor &editor);
    void removeEditor(Editor &editor);

    Editor *editors() { return m_firstEditor; }
    const Editor *editors() const { return m_firstEditor; }

    static boost::signals2::connection
    addOpenedCallback(const Opened::slot_type &callback)
    { return s_opened.connect(callback); }

    boost::signals2::connection
    addCloseCallback(const Close::slot_type &callback)
    { return m_close.connect(callback); }

    boost::signals2::connection
    addLoadedCallback(const Loaded::slot_type &callback)
    { return m_loaded.connect(callback); }

    boost::signals2::connection
    addSavedCallback(const Saved::slot_type &callback)
    { return m_saved.connect(callback); }

    boost::signals2::connection
    addChangedCallback(const Changed::slot_type &callback)
    { return m_changed.connect(callback); }

protected:
    struct TypeRecord
    {
        std::string m_type;
        Factory m_factory;
        OptionSettersFactory m_optSettersFactory;
        TypeRecord(const char *type,
                   const Factory &factory,
                   const OptionSettersFactory &optSettersFactory):
            m_type(type),
            m_factory(factory),
            m_optSettersFactory(optSettersFactory)
        {}
    };

    File(const char *uri);

    /**
     * This function is called by a derived class to notify all editors and
     * observers after changed.
     */
    void onChanged(const Change &change, bool loading);

    /**
     * This function is called by a derived class to save the reverse edit of a
     * newly performed edit primitive.
     */
    void saveUndo(EditPrimitive *undo);

    virtual Editor *createEditorInternally(Project *project) = 0;

    virtual FileLoader *createLoader(unsigned int priority,
                                     const Worker::Callback &callback) = 0;

    virtual FileSaver *createSaver(unsigned int priority,
                                   const Worker::Callback &callback) = 0;

    virtual void onLoaded(FileLoader &loader) = 0;

    virtual void onSaved(FileSaver &saver) {}

private:
    static const Factory *findFactory(const char *type);

    void continueClosing();

    void freezeInternally();

    void unfreezeInternally();

    void resetEditCount();

    void increaseEditCount();

    void decreaseEditCount();

    static gboolean onLoadedInMainThread(gpointer param);

    static gboolean onSavedInMainThread(gpointer param);

    void onLoadedWrapper(Worker &worker);

    void onSavedWrapper(Worker &worker);

    static std::list<TypeRecord> s_typeRegistry;

    const std::string m_uri;

    const std::string m_name;

    bool m_closing;

    bool m_reopening;

    bool m_loading;

    bool m_saving;

    Revision m_revision;

    GError *m_ioError;

    EditStack m_undoHistory;

    EditStack m_redoHistory;

    EditStack *m_superUndo;

    /**
     * The number of edits performed since the last loading or saving.
     */
    int m_editCount;

    /**
     * The editors that are editing this file.
     */
    Editor *m_firstEditor;
    Editor *m_lastEditor;

    int m_freezeCount;
    int m_internalFreezeCount;

    // We memorize the file loader so that we can cancel it later.
    FileLoader *m_loader;

    static Opened s_opened;
    Close m_close;
    Loaded m_loaded;
    Saved m_saved;
    Changed m_changed;

    SAMOYED_DEFINE_DOUBLY_LINKED(File)
};

}

#endif
