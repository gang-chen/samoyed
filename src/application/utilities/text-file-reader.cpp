// Text file reader.
// Copyright (C) 2012 Gang Chen.

/*
UNIT TEST BUILD
*/

#include "text-file-reader.hpp"
#include "../application.hpp"
#include "../resources/project-configuration-manager.hpp"
#include "../resources/project-configuration.hpp"

namespace Samoyed
{

TextFileReader::TextFileReader():
    m_file(NULL),
    m_fileStream(NULL),
    m_encodingConverter(NULL),
    m_converterStream(NULL),
    m_stream(NULL),
    m_buffer(NULL),
    m_error(NULL)
{
}

TextFileReader::~TextFileReader()
{
    close();
    if (m_buffer)
        delete m_buffer;
    if (m_error)
        g_error_free(m_error);
}

void TextFileReader::open(const char *uri)
{
    // Initialize the buffer.
    if (!m_buffer)
        m_buffer = new TextBuffer;
    else
        m_buffer->removeAll();

    // Clear the error code.
    if (m_error)
    {
        g_error_free(m_error);
        m_error = NULL;
    }

    // Get the external character encoding of the file.
    bool utf8 = true;
    std::string encoding;
    ReferencePointer<ProjectConfiguration> prjConfig =
        Application::instance()->projectConfigurationManager()->
        getProjectConfigurationForFile(uri);
    if (prjConfig)
    {
        prjConfig->;
    }

    // Open the file.
    m_file = g_file_new_for_uri(uri);
    m_fileStream = g_file_read(m_file, 0, &m_error);
    if (m_error)
        return;

    // Open the encoding converter and setup the input stream.
    if (utf8)
        m_stream = G_INPUT_STREAM(m_fileStream);
    else
    {
        m_encodingConverter =
            g_charset_converter_new("UTF-8", encoding.c_str(), &m_error);
        if (m_error)
            return;
        m_converterStream =
            g_converter_input_stream_new(G_INPUT_STREAM(m_fileStream),
                                         G_CONVERTER(m_encodingConverter));
        m_stream = m_converterStream;
    }

    // Get the revision.
    m_revision.synchronize(m_file, &m_error);
    if (m_error)
        return;

    // Read, convert and insert.

}

void TextFileReader::close()
{
    if (m_converterStream)
    {
        g_object_unref(m_converterStream);
        m_converterStream = NULL;
    }
    if (m_encodingConverter)
    {
        g_object_unref(m_encodingConverter);
        m_encodingConverter = NULL;
    }
    if (m_fileStream)
    {
        g_object_unref(m_fileStream);
        m_fileStream = NULL;
    }
    m_stream = NULL;
    if (m_file)
    {
        g_object_unref(file);
        m_file = NULL;
    }
}

}
