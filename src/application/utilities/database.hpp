// Database.
// Copyright (C) 2014 Gang Chen.

#ifndef SMYD_DATABASE_HPP
#define SMYD_DATABASE_HPP

#include <string>
#include <utility>
#include <vector>
#include <boost/utility.hpp>

namespace Samoyed
{

class Database: public boost::noncopyable
{
public:
    typedef int BlockNo;
    typedef int EntryNo;
    typedef int Offset;

    struct Key
    {
        const char *data;
        int length;
    };

    struct Value
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
            /**
             * @return True if this is a null iterator.
             */
            bool null();

            /**
             * Move this iterator forward.
             * @return False if the iterator cannot be moved because it is
             * already the last one.
             */
            bool next();

            /**
             * Move this iterator back.
             * @return False if the iterator cannot be moved because it is
             * already the first one.
             */
            bool previous();

            /**
             * Retrieve the stored value.
             * @return False if there are no stored value.
             */
            bool get(Value &value);

            /**
             * Store the value.
             */
            void set(const Value &value);

            /**
             * Remove the index, the stored value, and the associated child
             * table, if any.
             */
            void remove();

            /**
             * Remove the stored value.
             * @return False if no value is stored.
             */
            bool removeValue();

            /**
             * Remove the index.
             */
            void removeIndex();

            bool ();
            /**
             * Retrive the child table associated with the index.
             */
            Table *getChildTable();

            /**
             * Create a child table and associate it with the index.
             * @return The new or existing child table, and a flag that is false
             * if a child table is already associated.
             */
            std::pair<Table *, bool> newChildTable();

            /**
             * Delete the associated child table.
             * @return False if no child table is associated.
             */
            bool deleteChildTable();

        private:
            Iterator(Table &table,
                     BlockIndex block,
                     EntryIndex entry):
                m_table(table),
                m_block(block),
                m_entry(entry)
            {
            }

            Table &m_table;
            BlockIndex m_block;
            EntryIndex m_entry;
        };

        Iterator first();
        Iterator last();
        Iterator find(const Key &key);
        std::pair<Iterator, bool> insert(const Key &key);

    private:
        Table(Database &database,
              BlockIndex block):
            m_database(database),
            m_block(block)
        {
        }

        Database &m_database;
        BlockIndex m_block;
    };

    static Database *create(const char *fileName);

    static Database *open(const char *fileName);

    void close();

    Table *root(int indexing);

private:
    struct HeaderBlock
    {
        Offset blockSize;
        int indexingCount;
        BlockIndex freeBlockCount;
        BlockIndex firstFreeBlock;
        BlockIndex roots[1];
    };

    struct IndexBlock
    {
        BlockIndex parent;
        BlockIndex next;
        int childCount;
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
        m_fd(-1)
    {
    }

    void *getBlock(int index);
    int allocateBlock();
    void freeBlock(int index);

    std::string m_fileName;

    int m_fd;

    std::vector<void *> m_blocks;
};

}

#endif
