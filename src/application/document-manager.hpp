// Document manager.
// Copyright (C) 2011 Gang Chen.

#ifndef SHELL_DOCUMENT_MANAGER_HPP_INCLUDED
#define SHELL_DOCUMENT_MANAGER_HPP_INCLUDED

#include "../utilities/pointer-comparer.hpp"
#include <utility>
#include <string>
#include <map>

namespace Shell
{

class Document;
class Editor;
class EditorGroup;

class DocumentManager
{
public:
	/**
	 * Retrieve the opened document that is mapped to a file.
	 */
	Document *get(const char *fileName);

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
	std::pair<Document *, Editor *> open(const char *fileName,
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
	typedef std::map<std::string *, Document *,
					 Utilities::PointerComparer<std::string> > DocumentStore;

	DocumentStore m_documents;
};

}

#endif
