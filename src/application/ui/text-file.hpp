// Open text file.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_TEXT_FILE_HPP
#define SMYD_TEXT_FILE_HPP

#include "file.hpp"
#include "utilities/property-tree.hpp"
#include <boost/shared_ptr.hpp>
#include <gtk/gtk.h>

namespace Samoyed
{

/**
 * A text file represents an open text file.
 *
 * A text file does not contain any data memorizing its contents.  The
 * associated text editors contain data memorizing the file contents.  Accesses
 * to the file contents are redirected to the text editors.
 */
class TextFile: public File
{
public:
    struct Change: public File::Change
    {
        enum Type
        {
            TYPE_INSERTION = TYPE_INIT + 1,
            TYPE_REMOVAL
        };
        union Value
        {
            struct Insertion
            {
                int line;
                int column;
                const char *text;
                int length;
                int newLine;
                int newColumn;
            } insertion;
            struct Removal
            {
                int beginLine;
                int beginColumn;
                int endLine;
                int endColumn;
            } removal;
        } value;
        Change(int line, int column,
               const char *text, int length,
               int newLine, int newColumn):
            File::Change(TYPE_INSERTION)
        {
            value.insertion.line = line;
            value.insertion.column = column;
            value.insertion.text = text;
            value.insertion.length = length;
            value.insertion.newLine = newLine;
            value.insertion.newColumn = newColumn;
        }
        Change(int beginLine, int beginColumn, int endLine, int endColumn):
            File::Change(TYPE_REMOVAL)
        {
            value.removal.beginLine = beginLine;
            value.removal.beginColumn = beginColumn;
            value.removal.endLine = endLine;
            value.removal.endColumn = endColumn;
        }
    };

    static void installHistories();

    static bool isSupportedType(const char *mimeType);

    static void registerType();

    static const PropertyTree &defaultOptions();

    static const int TYPE = 1;

    virtual PropertyTree *options() const;

    const char *encoding() const { return m_encoding.c_str(); }

    int characterCount() const;

    int lineCount() const;

    int maxColumnInLine(int line) const;

    /**
     * @param beginLine The line number of the first character to be returned,
     * starting from 0.
     * @param beginColumn The column number of the first character to be
     * returned, the character index, starting from 0.
     * @param endLine The line number of the exclusive last character to be
     * retrieved, starting from 0; or -1 to retrieve the text until the last
     * line.
     * @param endColumn The column number of the exclusive last character to be
     * retrieved, the character index, starting from 0; or -1 to retrieve the
     * text until the last column.
     */
    boost::shared_ptr<char> text(int beginLine, int beginColumn,
                                 int endLine, int endColumn) const;

    /**
     * @param line The line number of the insertion position, starting from 0;
     * or -1 to append.
     * @param column The column number of the insertion position, the
     * character index, starting from 0; or -1 to append.
     * @param text The text to be inserted.
     * @param length The number of the bytes to be inserted, or -1 to insert the
     * text until '\0'.
     * @param newLine The holder of the line number of the position after the
     * insertion, or NULL if not needed.
     * @param newColumn The holder of the column number of the position after
     * the insertion, or NULL if not needed.
     * @return True iff inserted successfully.
     */
    bool insert(int line, int column, const char *text, int length,
                int *newLine, int *newColumn);

    /**
     * @param beginLine The line number of the first character to be removed,
     * starting from 0.
     * @param beginColumn The column number of the first character to be
     * removed, the character index, starting from 0.
     * @param endLine The line number of the exclusive last character to be
     * removed, starting from 0; or -1 to remove the text until the last line.
     * @param endColumn The column number of the exclusive last character to be
     * removed, the character index, starting from 0; or -1 to remove the text
     * until the last column.
     * @return True iff removed successfully.
     */
    bool remove(int beginLine, int beginColumn, int endLine, int endColumn);

    virtual bool supportSyntaxHighlight() const { return false; }

    virtual void highlightSyntax() {}

    virtual void unhighlightSyntax() {}

protected:
    class EditPrimitive: public File::EditPrimitive
    {
    public:
        enum Type
        {
            TYPE_INSERTION,
            TYPE_REMOVAL
        };

        EditPrimitive(Type type): m_type(type) {}

        Type type() const { return m_type; }

    private:
        Type m_type;
    };

    class Insertion: public EditPrimitive
    {
    public:
        /**
         * @param line The line number of the insertion position, starting from
         * 0.
         * @param column The column number of the insertion position, the
         * character index, starting from 0.
         * @param text The text to be inserted.
         * @param length The number of the bytes to be inserted, or -1 to insert
         * the text until '\0'.
         */
        Insertion(int line, int column, const char *text, int length):
            EditPrimitive(TYPE_INSERTION),
            m_line(line),
            m_column(column)
        {
            if (length == -1)
                m_text = text;
            else
                m_text.append(text, length);
        }

        virtual Edit *execute(File &file,
                              std::list<File::Change *> &changes) const;

        virtual bool merge(const File::EditPrimitive *edit);

    private:
        int m_line;
        int m_column;
        std::string m_text;
    };

    class Removal: public EditPrimitive
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
            EditPrimitive(TYPE_REMOVAL),
            m_beginLine(beginLine),
            m_beginColumn(beginColumn),
            m_endLine(endLine),
            m_endColumn(endColumn)
        {}

        virtual Edit *execute(File &file,
                              std::list<File::Change *> &changes) const;

        virtual bool merge(const File::EditPrimitive *edit);

    private:
        int m_beginLine;
        int m_beginColumn;
        int m_endLine;
        int m_endColumn;
    };

    class OptionsSetter: public File::OptionsSetter
    {
    public:
        OptionsSetter();
        virtual GtkWidget *gtkWidget() { return m_gtkWidget; }
        virtual PropertyTree *options() const;
    private:
        GtkWidget *m_gtkWidget;
    };

    static File::OptionsSetter *createOptionsSetter();

    static bool optionsEqual(const PropertyTree &options1,
                             const PropertyTree &options2);

    static void describeOptions(const PropertyTree &options,
                                std::string &desc);

    TextFile(const char *uri,
             int type,
             const char *mimeType,
             const PropertyTree &options);

    virtual Editor *createEditorInternally(Project *project);

    virtual FileLoader *createLoader(unsigned int priority);

    virtual FileSaver *createSaver(unsigned int priority);

    virtual void onLoaded(const boost::shared_ptr<FileLoader> &loader);

private:
    static File *create(const char *uri,
                        const char *mimeType,
                        const PropertyTree &options);

    Removal *insertOnly(int line, int column,
                        const char *text, int length,
                        Change *&change);

    Insertion *removeOnly(int beginLine, int beginColumn,
                          int endLine, int endColumn,
                          Change *&change);

    static PropertyTree s_defaultOptions;

    std::string m_encoding;
};

}

#endif
