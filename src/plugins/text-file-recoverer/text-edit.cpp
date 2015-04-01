// Various text edits.
// Copyright (C) 2014 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "text-edit.hpp"
#include "application.hpp"
#include "utilities/utf8.hpp"
#include "ui/text-file.hpp"
#include "ui/window.hpp"
#include <stddef.h>
#include <stdio.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

namespace
{

enum Operator
{
    OP_INIT,
    OP_INSERTION,
    OP_REMOVAL
};

const int TEXT_INSERTION_MERGE_LENGTH_THRESHOLD = 100;

bool writeInteger(int i, FILE *file)
{
    gint32 i32 = i;
    i32 = GINT32_TO_LE(i32);
    return fwrite(&i32, sizeof(gint32), 1, file) == 1;
}

bool readInteger(int &i, const char *&byteCode, int &length)
{
    gint32 i32;
    if (static_cast<size_t>(length) < sizeof(gint32))
        return false;
    memcpy(&i32, byteCode, sizeof(gint32));
    byteCode += sizeof(gint32);
    length -= sizeof(gint32);
    i = GINT32_FROM_LE(i32);
    return true;
}

}

namespace Samoyed
{

namespace TextFileRecoverer
{

bool TextInsertion::merge(const TextInsertion *ins)
{
    if (m_line == ins->m_line && m_column == ins->m_column)
    {
        m_text.insert(0, ins->m_text);
        return true;
    }
    if (m_text.length() >
        static_cast<const std::string::size_type>(
            TEXT_INSERTION_MERGE_LENGTH_THRESHOLD))
        return false;
    const char *cp = m_text.c_str();
    int line = m_line;
    int column = m_column;
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
    if (line == ins->m_line && column == ins->m_column)
    {
        m_text.append(ins->m_text);
        return true;
    }
    return false;
}

bool TextRemoval::merge(const TextRemoval *rem)
{
    if (m_beginLine == rem->m_beginLine && m_beginColumn == rem->m_beginColumn)
    {
        if (rem->m_beginLine == rem->m_endLine)
            m_endColumn += rem->m_endColumn - rem->m_beginColumn;
        else
        {
            m_endLine += rem->m_endLine - rem->m_beginLine;
            m_endColumn = rem->m_endColumn;
        }
        return true;
    }
    if (m_beginLine == rem->m_endLine && m_beginColumn == rem->m_endColumn)
    {
        m_beginLine = rem->m_beginLine;
        m_beginColumn = rem->m_beginColumn;
        return true;
    }
    return false;
}

bool TextInit::write(FILE *file) const
{
    if (fputc(OP_INIT, file) == EOF)
        return false;
    if (!writeInteger(m_length, file))
        return false;
    return fwrite(m_text, sizeof(char), m_length, file) ==
        sizeof(char) * m_length;
}

bool TextInsertion::write(FILE *file) const
{
    if (fputc(OP_INSERTION, file) == EOF)
        return false;
    if (!writeInteger(m_line, file))
        return false;
    if (!writeInteger(m_column, file))
        return false;
    if (!writeInteger(m_text.length(), file))
        return false;
    return fwrite(m_text.c_str(), sizeof(char), m_text.length(), file) ==
        sizeof(char) * m_text.length();
}

bool TextRemoval::write(FILE *file) const
{
    if (fputc(OP_REMOVAL, file) == EOF)
        return false;
    if (!writeInteger(m_beginLine, file))
        return false;
    if (!writeInteger(m_beginColumn, file))
        return false;
    if (!writeInteger(m_endLine, file))
        return false;
    if (!writeInteger(m_endColumn, file))
        return false;
    return true;
}

bool TextEdit::replay(TextFile &file, const char *&byteCode, int &length)
{
    bool successful = true;
    while (length > 0 && successful)
    {
        if (*byteCode == OP_INIT)
            successful = TextInit::replay(file, byteCode, length);
        else if (*byteCode == OP_INSERTION)
            successful = TextInsertion::replay(file, byteCode, length);
        else if (*byteCode == OP_REMOVAL)
            successful = TextRemoval::replay(file, byteCode, length);
    }
    return successful;
}

bool TextInit::replay(TextFile &file, const char *&byteCode, int &length)
{
    int len;
    byteCode++;
    length--;
    if (!readInteger(len, byteCode, length))
        return false;
    if (length < len)
        return false;
    char *text = file.text(0, 0, -1, -1);
    if (*(text + len) != '\0' || memcmp(text, byteCode, len) != 0)
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            GTK_WINDOW(Application::instance().currentWindow().gtkWidget()),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_QUESTION,
            GTK_BUTTONS_YES_NO,
            _("File \"%s\" have been changed since Samoyed crashed. The "
              "changes will be discarded if you recover the file. Continue "
              "recovering it?"),
            file.uri());
        gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_NO);
        int response = gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);

        if (response == GTK_RESPONSE_YES)
        {
            file.remove(0, 0, -1, -1);
            file.insert(0, 0, byteCode, len, NULL, NULL);
        }
        else
        {
            g_free(text);
            byteCode += len;
            length -= len;
            return false;
        }
    }
    g_free(text);
    byteCode += len;
    length -= len;
    return true;
}

bool TextInsertion::replay(TextFile &file, const char *&byteCode, int &length)
{
    int line, column, len;
    byteCode++;
    length--;
    if (!readInteger(line, byteCode, length))
        return false;
    if (!readInteger(column, byteCode, length))
        return false;
    if (!readInteger(len, byteCode, length))
        return false;
    if (length < len)
        return false;
    if (!file.insert(line, column, byteCode, len, NULL, NULL))
        return false;
    byteCode += len;
    length -= len;
    return true;
}

bool TextRemoval::replay(TextFile &file, const char *&byteCode, int &length)
{
    int beginLine, beginColumn, endLine, endColumn;
    byteCode++;
    length--;
    if (!readInteger(beginLine, byteCode, length))
        return false;
    if (!readInteger(beginColumn, byteCode, length))
        return false;
    if (!readInteger(endLine, byteCode, length))
        return false;
    if (!readInteger(endColumn, byteCode, length))
        return false;
    if (!file.remove(beginLine, beginColumn, endLine, endColumn))
        return false;
    return true;
}

}

}
