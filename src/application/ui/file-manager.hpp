// Manager of opened files.
// Copyright (C) 2011 Gang Chen.

#ifndef SMYD_FILE_MANAGER_HPP
#define SMYD_FILE_MANAGER_HPP

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

    std::pair<Document *, Editor *> open(const char *uri,
                                         int line,
                                         int column,
                                         bool newEditor,
                                         EditorGroup *editorContainer,
                                         int editorIndex);

    /**
     * Close an editor.  If this editor is the sole editor of the edited
     * document, close the document, which may be canceled by the user, and the
     * editor.
     * @return True iff the closing is started.
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
