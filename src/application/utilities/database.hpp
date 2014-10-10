// Database.
// Copyright (C) 2014 Gang Chen.

#ifndef SMYD_DATABASE_HPP
#define SMYD_DATABASE_HPP

#include <string>
#include <boost/utility>

namespace Samoyed
{

class Database: public boost::noncopyable
{
public:
    struct Configuration
    {
        int blockSize;
        char pathDelimiter;
    };

    struct Record
    {
        const char *data;
        int length;
    };

    class Table
    {
    public:
        class Iterator
        {
        public:
            void get(Record &record);
            void set(const Record &record);
            void append(const Record &record);
            void delete();
        };

        /**
         * @param record The holder of the retrieved data, or NULL if the data is not needed.
         * @return True iff the record exists.
         */
        bool get(const char *path, Record *record);

        enum ActionIfExisting
        {
            ABORT_IF_EXISTING,
            APPEND_IF_EXISTING,
            REPLACE_IF_EXISTING
        };

        /**
         * @return False iff aborted due to the existence of the record.
         */
        bool set(const char *path, const Record &record, ActionIfExisting action);

        /**
         * @return False iff aborted due to the nonexistence of the record.
         */
        bool delete(const char *path);

    private:
        Database &m_database;
    };

    static Database *create(const char *fileName, const Configuration &config);

    static Database *open(const char *fileName);

    void close();

    const Configuration &configuration() const;

    Table *root();

private:
    struct HeaderBlock
    {
        Configuration config;
        int rootBlock;
        int firstFreeBlock;
    };

    struct IndexBlock
    {
        int parent;
        int next;
        int next;
    };

    struct DataBlock
    {
    };

    struct FreeBlock
    {
        int next;
    };

    Database(const char *fileName):
        m_fileName(fileName),
        m_fd(-1),
        m_headerBlock(NULL)
    {
    }

    std::string m_fileName;

    int m_fd;

    HeaderBlock *m_headerBlock;
};

#endif
