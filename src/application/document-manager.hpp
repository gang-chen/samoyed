// Document manager.
// Copyright (C) 2011 Gang Chen.

#ifndef SMYD_DOCUMENT_MANAGER_HPP
#define SMYD_DOCUMENT_MANAGER_HPP

#include "utilities/pointer-comparator.hpp"
#include <utility>
#include <string>
#include <boost/signals2/signal.hpp>
#include <map>

namespace Samoyed
{

class Document;
class Editor;
class EditorGroup;

class DocumentManager
{
private:
    typedef boost::signals2::signal<void (const Document &doc)> DocumentOpened;
    typedef boost::signals2::signal<void (const Document &doc)> DocumentClosed;
    typedef boost::signals2::signal<void (const Document &doc)> DocumentLoaded;
    typedef boost::signals2::signal<void (const Document &doc)> DocumentSaved;
    typedef boost::signals2::signal<void (const Document &doc,
                                          int line,
                                          int column,
                                          const char *text,
                                          int length)> DocumentTextInserted;
    typedef boost::signals2::signal<void (const Document &doc,
                                          int beginLine,
                                          int beginColumn,
                                          int endLine,
                                          int endColumn)> DocumentTextRemoved;

public:
    /**
     * Retrieve the opened document that is mapped to a file.
     */
    Document *get(const char *uri);

    /**
     * Load a file into a document and open an editor to edit it if the file is
     * not opened.  Otherwise, retrieve the existing document and select an
     * editor for it or open a new editor if requested.
     * @param line The line number of the initial cursor, starting from 0.
     * @param column The column number of the initial cursor, the character
     * index, starting from 0.
     * @param newEditor True if request to always open a new editor.
     * @param editorContainer The editor group to contain the new editor, or 0
     * if put the new editor into the current editor group.
     * @param editorIndex The index of the new editor in the containing editor
     * group, or -1 if put the new editor after the current editor.
     */
    std::pair<Document *, Editor *> open(const char *uri,
                                         int line,
                                         int column,
                                         bool newEditor,
                                         EditorGroup *editorContainer,
                                         int editorIndex);

    /**
     * Close an editor.  If this editor is the sole editor of the edited
     * document and the document was changed, ask the user to save or discard
     * the changes, or cancel closing the editor.
     * @return True iff the editor is closed.
     */
    bool closeEditor(Editor *editor);

private:
    typedef std::map<std::string *, Document *, PointerComparator<std::string> >
	DocumentStore;

    DocumentStore m_documents;

    DocumentOpened m_docOpened;
    DocumentClosed m_docClosed;
    DocumentLoaded m_docLoaded;
    DocumentSaved m_docSaved;
    DocumentTextInserted m_docTextInserted;
    DocumentTextRemoved m_docTextRemoved;
};

}

#endif
