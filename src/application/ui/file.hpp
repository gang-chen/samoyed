// Open file.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_FILE_HPP
#define SMYD_FILE_HPP

#include "utilities/miscellaneous.hpp"
#include "utilities/worker.hpp"
#include "utilities/property-tree.hpp"
#include <utility>
#include <list>
#include <set>
#include <string>
#include <boost/utility.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/signals2/signal.hpp>

namespace Samoyed
{

class Editor;
class Project;
class FileLoader;
class FileSaver;

/**
 * A file represents an open file.  It is the in-memory buffer for a file being
 * edited by the user.  Each open physical file has one and only one file
 * instance.  A file has one or more editors.  The file accepts each user edit
 * from an editor, and broadcasts the edit to the other editors.
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
    boost::function<File *(const char *uri,
                           const char *mimeType,
                           const PropertyTree &options)>
    	Factory;

    /**
     * A change.  Derived classes should define their concrete changes.
     */
    struct Change
    {
        enum Type
        {
            TYPE_INIT
        };
        int type;
        Change(int type): type(type) {}
        virtual ~Change() {}
    };

    /**
     * @param change The change that was made.
     * @param loading True iff the change was due to file loading.
     */
    typedef boost::signals2::signal<void (File &file,
                                          const Change &change,
                                          bool loading)> Changed;

    static void installHistories();

    /**
     * Get the default options for a specific type of files.  These default
     * options can be used as the definitions of the options for this type of
     * files.
     * @return The default options, or NULL if the type is unsupported.
     */
    static const PropertyTree *defaultOptionsForType(const char *mimeType);

    /**
     * Get the default options for general files.  These default options are a
     * part of the default options for derived types of files.
     */
    static const PropertyTree &defaultOptions() { return s_defaultOptions; }

    /**
     * Open a file in an editor.
     * @param uri The URI of the file.
     * @param project The project context, or NULL if none.
     * @param mimeType The type of the file, or NULL if unknown.
     * @param options Additional file-type-specific options specifying how to
     * open the file, or NULL if none.
     * @param newEditor True to create a new editor even if the file is already
     * open.
     */
    static std::pair<File *, Editor *>
    open(const char *uri, Project *project,
         const char *mimeType,
         const PropertyTree *options,
         bool newEditor);

    /**
     * Open a dialog to let the user choose the file to open and set additional
     * options, and open the chosen file if any.
     * @param project The project context, or NULL if none.
     */
    static void openByDialog(Project *project,
                             std::list<std::pair<File *, Editor *> > &opened);

    /**
     * @return True iff the file is being closed.
     */
    bool closing() const { return m_closing; }

    /**
     * Close a file by closing all editors.
     * @return False iff the user cancels closing the file.
     */
    bool close();

    void cancelClosing() { m_closing = false; }

    /**
     * This function can be called by the application instance only.
     */
    virtual ~File();

    /**
     * Create an editor.
     */
    Editor *createEditor(Project *project);

    /**
     * Close an editor.  This function is called by the editor when it is
     * requested to be closed.
     * @return False iff the user cancels closing the editor.
     */
    bool closeEditor(Editor &editor);

    const char *uri() const { return m_uri.c_str(); }

    int type() const { return m_type; }

    const char *mimeType() const { return m_mimeType.c_str(); }

    const Time &modifiedTime() const { return m_modifiedTime; }

    /**
     * @return The options, which must be deleted by the caller.
     */
    virtual PropertyTree *options() const
    { return new PropertyTree(defaultOptions()); }

    virtual bool structured() const { return false; }

    /**
     * @return True iff the file is being loaded.
     */
    bool loading() const { return m_loader; }

    /**
     * @return True iff the file is being saved.
     */
    bool saving() const { return m_saver; }

    /**
     * @return True iff the file was edited.
     */
    bool edited() const
    {
        return m_editCount || (m_superUndo && !m_superUndo->empty());
    }

    /**
     * Request to load the file.  The file cannot be loaded if it is being
     * closed, loaded or saved, or is frozen.  When loading, the file is frozen.
     * The caller can get notified of the completion of the loading by adding a
     * callback.
     * @return True iff the load operation is started.
     */
    bool load(bool userRequest);

    /**
     * Request to save the file.  The file cannot be saved if it is being
     * closed, loaded or saved, or is frozen.  When saving, the file is frozen.
     * The caller can get notified of the completion of the saving by adding a
     * callback.
     * @return True iff the save operation is started.
     */
    bool save();

    bool closeable() const { return m_pinned.empty(); }

    /**
     * Disallow the user to close the file.
     */
    void pin(const char *reason) { m_pinned.insert(reason); }

    /**
     * Allow the user to close the file.
     */
    void unpin(const char *reason) { m_pinned.erase(reason); }

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

    virtual void updateStructure() {}

protected:
    class EditPrimitive;

    class Edit
    {
    public:
        virtual ~Edit() {}

        /**
         * @param file The file to be edited.
         * @param changes The changes resulting from the execution of the edit.
         * @return The reverse edit.
         */
        virtual Edit *execute(File &file,
                              std::list<Change *> &changes) const = 0;

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

        virtual Edit *execute(File &file, std::list<Change *> &changes) const;

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

    class OptionsSetter
    {
    public:
        virtual ~OptionsSetter() {}
        virtual GtkWidget *gtkWidget() { return NULL; }
        /**
         * @return The options, which must be deleted by the caller.
         */
        virtual PropertyTree *options() const;
    };

    typedef boost::function<OptionsSetter *()> OptionsSetterFactory;
    typedef boost::function<const PropertyTree &()> OptionsGetter;
    typedef boost::function<bool (const PropertyTree &, const PropertyTree &)>
        OptionsEqual;
    typedef boost::function<void (const PropertyTree &, std::string &)>
        OptionsDescriber;

    static File::OptionsSetter *createOptionsSetter();

    static bool optionsEqual(const PropertyTree &options1,
                             const PropertyTree &options2)
    { return true; }

    /**
     * Describe the options in text.
     * @param options The options.
     * @param desc The text description of the options.
     */
    static void describeOptions(const PropertyTree &options,
                                std::string &desc)
    {}

    static void registerType(const char *mimeType,
                             const Factory &factory,
                             const OptionsSetterFactory &optSetterFactory,
                             const OptionsGetter &defOptGetter,
                             const OptionsEqual &optEqual,
                             const OptionsDescriber &optDescriber,
                             const char *description);

    static void registerTypeSet(const char *mimeTypeSet,
                                const char *masterMimeType,
                                const char *description);

    File(const char *uri,
         int type,
         const char *mimeType,
         const PropertyTree &options);

    /**
     * This function is called by a derived class to notify all editors and
     * observers after changed.
     */
    virtual void onChanged(const Change &change, bool loading);

    /**
     * This function is called by a derived class to save the reverse edit of a
     * newly performed edit primitive.
     */
    void saveUndo(EditPrimitive *undo);

    virtual Editor *createEditorInternally(Project *project) = 0;

    virtual FileLoader *createLoader(unsigned int priority) = 0;

    virtual FileSaver *createSaver(unsigned int priority) = 0;

    virtual void onLoaded(const boost::shared_ptr<FileLoader> &loader) = 0;

    virtual void onSaved(const boost::shared_ptr<FileSaver> &saver) {}

private:
    struct TypeRecord
    {
        std::string type;
        Factory factory;
        OptionsSetterFactory optSetterFactory;
        OptionsGetter defOptGetter;
        OptionsEqual optEqual;
        OptionsDescriber optDescriber;
        std::string description;
        TypeRecord(const char *type,
                   const Factory &factory,
                   const OptionsSetterFactory &optSetterFactory,
                   const OptionsGetter &defOptGetter,
                   const OptionsEqual &optEqual,
                   const OptionsDescriber &optDescriber,
                   const char *description):
            type(type),
            factory(factory),
            optSetterFactory(optSetterFactory),
            defOptGetter(defOptGetter),
            optEqual(optEqual),
            optDescriber(optDescriber),
            description(description)
        {}
    };

    struct TypeSet
    {
        std::list<std::string> mimeTypes;
        const TypeRecord *masterType;
        std::string description;
    };

    struct FilterChangedParam
    {
        std::map<GtkFileFilter *, const OptionsSetterFactory *> filter2Factory;
        OptionsSetter *optSetter;
        FilterChangedParam(): optSetter(NULL) {}
    };

    static const TypeRecord *findTypeRecord(const char *type);

    void finishClosing();

    void freezeInternally();

    void unfreezeInternally();

    void resetEditCount();

    void increaseEditCount();

    void decreaseEditCount();

    void onLoaderFinished(const boost::shared_ptr<Worker> &worker);
    void onLoaderCanceled(const boost::shared_ptr<Worker> &worker);

    void onSaverFinished(const boost::shared_ptr<Worker> &worker);
    void onSaverCanceled(const boost::shared_ptr<Worker> &worker);

    static void onFileChooserFilterChanged(GtkFileChooser *dialog,
                                           GParamSpec *spec,
                                           FilterChangedParam *param);

    static std::list<TypeRecord> s_typeRegistry;

    static std::list<TypeSet> s_typeSetRegistry;

    static PropertyTree s_defaultOptions;

    const std::string m_uri;

    int m_type;

    const std::string m_mimeType;

    bool m_closing;

    Time m_modifiedTime;

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

    std::set<std::string> m_pinned;

    int m_freezeCount;
    int m_internalFreezeCount;

    boost::shared_ptr<FileLoader> m_loader;
    boost::signals2::connection m_loaderFinishedConn;
    boost::signals2::connection m_loaderCanceledConn;

    boost::shared_ptr<FileSaver> m_saver;
    boost::signals2::connection m_saverFinishedConn;
    boost::signals2::connection m_saverCanceledConn;

    static Opened s_opened;
    Close m_close;
    Loaded m_loaded;
    Saved m_saved;
    Changed m_changed;

    SAMOYED_DEFINE_DOUBLY_LINKED(File)
};

}

#endif
