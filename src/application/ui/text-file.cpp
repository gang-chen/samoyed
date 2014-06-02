// Opened text file.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "text-file.hpp"
#include "text-editor.hpp"
#include "project.hpp"
#include "utilities/utf8.hpp"
#include "utilities/text-buffer.hpp"
#include "utilities/text-file-loader.hpp"
#include "utilities/text-file-saver.hpp"
#include "utilities/property-tree.hpp"
#include "application.hpp"
#include <string.h>
#include <algorithm>
#include <list>
#include <string>
#include <glib/gi18n.h>
#include <gio/gio.h>

#define FILE_OPTIONS "file-options"
#define TEXT_FILE_OPTIONS "text-file-options"
#define ENCODING "encoding"
#define FILE_OPEN "file-open"
#define TEXT_ENCODING "text-encoding"
#define DEFAULT_ENCODING "UTF-8"

namespace
{

const int TEXT_FILE_INSERTION_MERGE_LENGTH_THRESHOLD = 100;

}

namespace Samoyed
{

File::Edit *TextFile::Insertion::execute(File &file) const
{
    return static_cast<TextFile &>(file).
        insertOnly(m_line, m_column, m_text.c_str(), m_text.length());
}

bool TextFile::Insertion::merge(const File::EditPrimitive *edit)
{
    if (static_cast<const TextFile::EditPrimitive *>(edit)->type() !=
        TYPE_INSERTION)
        return false;
    const Insertion *ins = static_cast<const Insertion *>(edit);
    if (m_line == ins->m_line && m_column == ins->m_column)
    {
        m_text.append(ins->m_text);
        return true;
    }
    if (ins->m_text.length() >
        static_cast<const std::string::size_type>(
            TEXT_FILE_INSERTION_MERGE_LENGTH_THRESHOLD))
        return false;
    const char *cp = ins->m_text.c_str();
    int line = ins->m_line;
    int column = ins->m_column;
    while (*cp)
    {
        if (*cp == '\n')
        {
            ++cp;
            ++line;
            column = 0;
        }
        else if (*cp == '\r')
        {
            ++cp;
            if (*cp == '\n')
                ++cp;
            ++line;
            column = 0;
        }
        else
        {
            cp += Utf8::length(cp);
            ++column;
        }
    }
    if (m_line == line && m_column == column)
    {
        m_line = ins->m_line;
        m_column = ins->m_column;
        m_text.insert(0, ins->m_text);
        return true;
    }
    return false;
}

File::Edit *TextFile::Removal::execute(File &file) const
{
    return static_cast<TextFile &>(file).
        removeOnly(m_beginLine, m_beginColumn, m_endLine, m_endColumn);
}

bool TextFile::Removal::merge(const File::EditPrimitive *edit)
{
    if (static_cast<const TextFile::EditPrimitive *>(edit)->type() !=
        TYPE_REMOVAL)
        return false;
    const Removal *rem = static_cast<const Removal *>(edit);
    if (m_beginLine == rem->m_beginLine)
    {
        if (m_beginColumn == rem->m_beginColumn &&
            rem->m_beginLine == rem->m_endLine)
        {
            if (m_beginLine == m_endLine)
                m_endColumn += rem->m_endColumn - rem->m_beginColumn;
            return true;
        }
        if (m_beginLine == m_endLine && m_endColumn == rem->m_beginColumn)
        {
            m_endLine = rem->m_endLine;
            m_endColumn = rem->m_endColumn;
            return true;
        }
    }
    return false;
}

TextFile::OptionsSetter::OptionsSetter()
{
    m_gtkWidget = gtk_combo_box_text_new();
    const char *lastEncoding = Application::instance().histories().
        get<std::string>(FILE_OPEN "/" TEXT_ENCODING).c_str();
    int lastEncIndex = 0, i = 0;
    for (const char **encoding = getTextEncodings(); *encoding; ++encoding, ++i)
    {
        gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(m_gtkWidget),
                                  NULL,
                                  *encoding);
        if (strcmp(*encoding, lastEncoding) == 0)
            lastEncIndex = i;
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(m_gtkWidget), lastEncIndex);
}

PropertyTree *TextFile::OptionsSetter::options() const
{
    PropertyTree *options = new PropertyTree(TEXT_FILE_OPTIONS);
    options->addChild(*File::OptionsSetter::options());
    options->addChild(ENCODING, std::string(DEFAULT_ENCODING));
    char *encoding =
        gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(m_gtkWidget));
    char *cp1, *cp2;
    cp1 = strchr(encoding, '(');
    if (cp1)
    {
        cp1++;
        cp2 = strchr(cp1, ')');
        if (cp2)
        {
            *cp2 = '\0';
            std::list<std::string> errors;
            options->set(ENCODING, std::string(cp1), false, errors);
            Application::instance().histories().
                set(FILE_OPEN "/" TEXT_ENCODING,
                    std::string(encoding),
                    false,
                    errors);
        }
    }
    g_free(encoding);
    return options;
}

File::OptionsSetter *TextFile::createOptionsSetter()
{
    return new TextFile::OptionsSetter;
}

PropertyTree TextFile::s_defaultOptions(TEXT_FILE_OPTIONS);

const PropertyTree &TextFile::defaultOptions()
{
    if (s_defaultOptions.empty())
    {
        s_defaultOptions.addChild(*(new PropertyTree(File::defaultOptions())));
        s_defaultOptions.addChild(ENCODING, std::string(DEFAULT_ENCODING));
    }
    return s_defaultOptions;
}

bool TextFile::optionsEqual(const PropertyTree &options1,
                            const PropertyTree &options2)
{
    return (options1.get<std::string>(ENCODING) ==
            options2.get<std::string>(ENCODING) &&
            File::optionsEqual(options1.child(FILE_OPTIONS),
                               options2.child(FILE_OPTIONS)));
}

void TextFile::describeOptions(const PropertyTree &options,
                               std::string &desc)
{
    std::string encoding = options.get<std::string>(ENCODING);
    desc += "in encoding \"";
    desc += encoding;
    desc += "\"";

    std::string fileDesc;
    File::describeOptions(options.child(FILE_OPTIONS), fileDesc);
    if (!fileDesc.empty())
    {
        desc += ", ";
        desc += fileDesc;
    }
}

void TextFile::installHistories()
{
    Application::instance().histories().child(FILE_OPEN).
        addChild(TEXT_ENCODING, std::string(DEFAULT_ENCODING));
}

TextFile::TextFile(const char *uri, const PropertyTree &options):
    File(uri,
         strcmp(options.name(), TEXT_FILE_OPTIONS) == 0 ?
         options.child(FILE_OPTIONS) : options)
{
    if (strcmp(options.name(), TEXT_FILE_OPTIONS) == 0)
        m_encoding = options.get<std::string>(ENCODING);
    else
        m_encoding = DEFAULT_ENCODING;
}

File *TextFile::create(const char *uri, const PropertyTree &options)
{
    return new TextFile(uri, options);
}

bool TextFile::isSupportedType(const char *type)
{
    char *textType = g_content_type_from_mime_type("text/plain");
    bool supported = g_content_type_is_a(type, textType);
    g_free(textType);
    return supported;
}

void TextFile::registerType()
{
    char *type = g_content_type_from_mime_type("text/plain");
    File::registerType(type,
                       create,
                       createOptionsSetter,
                       defaultOptions,
                       optionsEqual,
                       describeOptions);
    g_free(type);
}

PropertyTree *TextFile::options() const
{
    PropertyTree *options = new PropertyTree(TEXT_FILE_OPTIONS);
    options->addChild(*File::options());
    options->addChild(ENCODING, std::string(encoding()));
    return options;
}

int TextFile::characterCount() const
{
    return static_cast<const TextEditor *>(editors())->characterCount();
}

int TextFile::lineCount() const
{
    return static_cast<const TextEditor *>(editors())->lineCount();
}

int TextFile::maxColumnInLine(int line) const
{
    return static_cast<const TextEditor *>(editors())->maxColumnInLine(line);
}

char *TextFile::text(int beginLine, int beginColumn,
                     int endLine, int endColumn) const
{
    return static_cast<const TextEditor *>(editors())->
        text(beginLine, beginColumn, endLine, endColumn);
}

Editor *TextFile::createEditorInternally(Project *project)
{
    return TextEditor::create(*this, project);
}

FileLoader *TextFile::createLoader(unsigned int priority,
                                   const Worker::Callback &callback)
{
    return new TextFileLoader(Application::instance().scheduler(),
                              priority,
                              callback,
                              uri(),
                              encoding());
}

FileSaver *TextFile::createSaver(unsigned int priority,
                                 const Worker::Callback &callback)
{
    return new TextFileSaver(Application::instance().scheduler(),
                             priority,
                             callback,
                             uri(),
                             text(0, 0, -1, -1),
                             -1,
                             encoding());
}

void TextFile::onLoaded(FileLoader &loader)
{
    TextFileLoader &ld = static_cast<TextFileLoader &>(loader);

    // Copy the contents in the loader's buffer to the editors.
    const TextBuffer *buffer = ld.buffer();
    TextBuffer::ConstIterator it(*buffer, 0, -1, -1);
    onChanged(Change(0, 0, -1, -1), true);
    do
    {
        const char *begin, *end;
        if (it.getAtomsBulk(begin, end))
            onChanged(Change(-1, -1, begin, end - begin, -1, -1), true);
    }
    while (it.goToNextBulk());

    // Notify editors.
    for (TextEditor *editor = static_cast<TextEditor *>(editors());
         editor;
         editor = static_cast<TextEditor *>(editor->nextInFile()))
        editor->onFileLoaded();
}

bool TextFile::insert(int line, int column, const char *text, int length)
{
    if (!static_cast<TextEditor *>(editors())->isValidCursor(line, column))
        return false;
    Removal *undo = insertOnly(line, column, text, length);
    saveUndo(undo);
    return true;
}

bool TextFile::remove(int beginLine, int beginColumn,
                      int endLine, int endColumn)
{
    if (!static_cast<TextEditor *>(editors())->isValidCursor(beginLine,
                                                             beginColumn) ||
        !static_cast<TextEditor *>(editors())->isValidCursor(endLine,
                                                             endColumn))
        return false;

    // Order the two positions.
    if (beginLine > endLine)
    {
        std::swap(beginLine, endLine);
        std::swap(beginColumn, endColumn);
    }
    else if (beginLine == endLine && beginColumn > endColumn)
        std::swap(beginColumn, endColumn);

    Insertion *undo = removeOnly(beginLine, beginColumn,
                                 endLine, endColumn);
    saveUndo(undo);
    return true;
}

TextFile::Removal *
TextFile::insertOnly(int line, int column, const char *text, int length)
{
    // Compute the new position after insertion.
    int nLines = 0, nColumns = 0;
    if (length == -1)
        length = strlen(text);
    const char *cp = text + length - 1;
    while (cp >= text)
    {
        if (*cp == '\n')
        {
            --cp;
            if (cp >= text && *cp == '\r')
                --cp;
            nLines++;
            break;
        }
        if (*cp == '\r')
        {
            --cp;
            nLines++;
            break;
        }
        cp = Utf8::begin(cp - 1);
        ++nColumns;
    }
    while (cp >= text)
    {
        if (*cp == '\n')
        {
            --cp;
            if (cp >= text && *cp == '\r')
                --cp;
            nLines++;
        }
        else if (*cp == '\r')
        {
            --cp;
            nLines++;
        }
        else
            cp = Utf8::begin(cp - 1);
    }
    int newLine, newColumn;
    newLine = line + nLines;
    if (nLines)
        newColumn = nColumns;
    else
        newColumn = column + nColumns;

    onChanged(Change(line, column, text, length, newLine, newColumn), false);
    Removal *undo = new Removal(line, column, newLine, newColumn);
    return undo;
}

TextFile::Insertion *
TextFile::removeOnly(int beginLine, int beginColumn,
                     int endLine, int endColumn)
{
    char *removed = text(beginLine, beginColumn, endLine, endColumn);
    Insertion *undo = new Insertion(beginLine, beginColumn, removed, -1);
    g_free(removed);
    onChanged(Change(beginLine, beginColumn, endLine, endColumn), false);
    return undo;
}

}
