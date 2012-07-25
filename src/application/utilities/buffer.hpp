// Buffer.
// Copyright (C) 2010 Gang Chen.

#ifndef SMYD_BUFFER_HPP
#define SMYD_BUFFER_HPP

#include "int-or-nil.hpp"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <boost/utility.hpp>

namespace Samoyed
{

/**
 * A buffer stores a stream of atoms in memory.  An atom is an indivisible
 * stream of units.  For each atom, atom traits define how many units compose
 * it, called the length of the atom.  Unit offsets of atoms in a buffer are
 * used as the primary indexes to locate atoms.  Atom indexes in a buffer are
 * used as the secondary indexes optionally.  Buffer reads and writes are on the
 * basis of atoms.
 *
 * A buffer prefers sequential and localized reads and writes.  It provides
 * iterators that can be set randomly and moved sequentially.  The stored atoms
 * that iterators point to can be read out.  A buffer owns a special iterator,
 * called the cursor.  All buffer writes are performed at the cursor.  The
 * other iterators are read-only iterators and they become invalid whenever the
 * buffer is changed.
 *
 * An optional tertiary integer indexing can be defined by atom traits.  For
 * each atom, the increment from the starting index to the ending index, called
 * the length of the atom in the tertiary index, and the increment from the
 * ending index of the previous atom, or zero if none, to the starting index of
 * this atom, called the skip, are defined.  This implies that the tertiary
 * indexes monotonously increase starting from zero.
 *
 * Internally, a buffer consists of a series of separate equal-sized memory
 * blocks.  The blocks are in a doubly-linked list.  If not full, each block
 * has a gap, where the future manipulations to the content are expected to
 * occur.  This implies the size of any atom is limited by the block size.
 *
 * @param Unit The unit type.
 * @param AtomTraits The atom traits that define atom lengths and skips.  It is
 * a struct that has four member functions.
 *  - length(const Unit *atom)
 *    Returns the length of an atom in the primary index.
 *  - previousAtomLength(const Unit *atom)
 *    Returns the length of an atom in the primary index before a given
 *    position.
 *  - length3(const Unit *atom)
 *    Returns the length of an atom in the tertiary index.
 *  - skip3(const Unit *atom)
 *    Returns the increment from the ending index of the previous atom, or zero
 *    if none, to the starting index of this atom, in the tertiary index.
 * @param BLOCK_SIZE The number of units stored in each memory block.  The hard
 * lower limit of the block size is the maximum allowed length of the stored
 * atoms.  Actually the block size should be selected smartly to avoid
 * performance or memory utilization issues.  If the block size is too small,
 * then too many blocks may be allocated and the block utilization may be too
 * low.  If the block size is too large, then too many units may be moved when
 * the cursor is changed, and too many units may be scanned when indexed.
 * @param UNIFORM_ATOM_LENGTH The uniform atom length if all the atoms have the
 * same length, or -1 otherwise.
 * @param SECONDARY_INDEXING True iff the secondary indexing is required.
 * @param TERTIARY_INDEXING True iff the tertiary indexing is required.
 */
template<class Unit, class AtomTraits, int BLOCK_SIZE, int UNIFORM_ATOM_LENGTH,
         bool SECONDARY_INDEXING, bool TERTIARY_INDEXING>
class Buffer: public boost::noncopyable
{
private:
    /**
     * A fixed-sized memory block.
     */
    class Block: public boost::noncopyable
    {
    public:
        Block():
            m_gapBegin(m_content),
            m_gapEnd(m_gapBegin + BLOCK_SIZE),
            m_length(0),
            m_length2(0),
            m_length3(0),
            m_halfLength2(0),
            m_halfLength3(0),
            m_next(NULL),
            m_previous(NULL)
        {}

        /**
         * Clear the content stored in this block.
         */
        void clear()
        {
            m_gapBegin = m_content;
            m_gapEnd = m_gapBegin + BLOCK_SIZE;
            m_length = 0;
            m_length2 = 0;
            m_length3 = 0;
            m_halfLength2 = 0;
            m_halfLength3 = 0;
        }

        Unit *begin() { return m_content; }
        const Unit *begin() const { return m_content; }
        Unit *end() { return m_content + BLOCK_SIZE; }
        const Unit *end() const { return m_content + BLOCK_SIZE; }
        Unit *gapBegin() { return m_gapBegin; }
        const Unit *gapBegin() const { return m_gapBegin; }
        Unit *gapEnd() { return m_gapEnd; }
        const Unit *gapEnd() const { return m_gapEnd; }
        int length() const { return m_length; }
        int length2() const { return m_length2; }
        int length3() const { return m_length3; }
        int halfLength() const { return m_gapBegin - m_content; }
        int halfLength2() const { return m_halfLength2; }
        int halfLength3() const { return m_halfLength3; }
        Block *next() { return m_next; }
        const Block *next() const { return m_next; }
        Block *previous() { return m_previous; }
        const Block *previous() const { return m_previous; }
        int gapSize() { return m_gapEnd - m_gapBegin; }

    private:
        /**
         * The beginning of the gap.
         */
        Unit *m_gapBegin;

        /**
         * The end of the gap.
         */
        Unit *m_gapEnd;

        /**
         * The total length of the stored atoms in the primary index, i.e., the
         * unit count.
         */
        int m_length;

        /**
         * The total length of the stored atoms in the secondary index, i.e.,
         * the atom count.
         */
        IntOrNil<SECONDARY_INDEXING> m_length2;

        /**
         * The total length of the stored atoms in the tertiary index.
         */
        IntOrNil<TERTIARY_INDEXING> m_length3;

        /**
         * The total length of the stored atoms that precede the gap in the
         * secondary index, i.e., the number of the atoms preceding the gap.
         */
        IntOrNil<SECONDARY_INDEXING> m_halfLength2;

        /**
         * The total length of the stored atoms that precede the gap in the
         * tertiary index.
         */
        IntOrNil<TERTIARY_INDEXING> m_halfLength3;

        /**
         * The next and previous blocks in the block list of the owning buffer.
         */
        Block *m_next;
        Block *m_previous;

        /**
         * The stored units.
         */
        Unit m_content[BLOCK_SIZE]
            __attribute__ ((aligned (__BIGGEST_ALIGNMENT__)));

        friend class Buffer;
    };

    /**
     * The iterator template used by the iterator and constant iterator
     * classes.
     */
    template<class BufferT, class BlockT, class Pointer>
    class IteratorTemplate
    {
    public:
        /**
         * Construct an iterator at the specified position in a buffer.
         * @param buffer The associated buffer.
         * @param index The primary index, or -1 if not specified.
         * @param index2 The secondary index, or -1 if not specified.
         * @param index3 The tertiary index, or -1 if not specified.  It is
         * required that one and only one of the primary index, the secondary
         * index, and the tertiary index be specified.
         */
        IteratorTemplate(BufferT &buffer, int index, int index2, int index3):
            m_buffer(buffer),
            m_block(buffer.m_first),
            m_pointer(m_block->begin() == m_block->gapBegin() ?
                      m_block->gapEnd() : m_block->begin()),
            m_index(0),
            m_index2(0),
            m_index3(0),
            m_indexInBlock(0),
            m_index2InBlock(0),
            m_index3InBlock(0)
        {
            if (TERTIARY_INDEXING)
                addSkip3();
            if (index >= 0)
            {
                assert(index2 == -1);
                assert(index3 == -1);
                set(index);
            }
            else if (index2 >= 0)
            {
                assert(index == -1);
                assert(index3 == -1);
                set2(index2);
            }
            else
            {
                assert(index == -1);
                assert(index2 == -1);
                set3(index3);
            }
        }

        const IteratorTemplate &operator=(const IteratorTemplate &rhs)
        {
            if (this != &rhs)
            {
                assert(&m_buffer == &rhs.m_buffer);
                m_block = rhs.m_block;
                m_pointer = rhs.m_pointer;
                m_index = rhs.m_index;
                m_index2 = rhs.m_index2;
                m_index3 = rhs.m_index3;
                m_indexInBlock = rhs.m_indexInBlock;
                m_index2InBlock = rhs.m_index2InBlock;
                m_index3InBlock = rhs.m_index3InBlock;
            }
            return *this;
        }

        /**
         * Set this iterator at the specified primary index.
         * @param index The primary index.
         */
        void set(int index);

        /**
         * Set this iterator at the specified secondary index.
         * @param index2 The secondary index.
         */
        void set2(int index2);

        /**
         * Set this iterator at the specified tertiaary index.  Set to the
         * beginning of the atom such that the ending index of the previous
         * atom, or zero if none, is not less than the specified index.  If
         * there are a range of atoms satisfying this condition, set to the
         * beginning of the first atom.
         * @param index3 The tertiary index.
         */
        void set3(int index3);

        /**
         * Move this iterator by the specified primary index delta.
         * @param delta The primary index delta.
         */
        void move(int delta)
        {
            set(m_index + delta);
        }

        /**
         * Move this iterator by the specified secondary index delta.
         * @param delta2 The secondary index delta.
         */
        void move2(int delta2)
        {
            set2(m_index2 + delta2);
        }

        /**
         * Move this iterator by the specified tertiary index delta.
         * @param delta3 The tertiary index delta.
         */
        void move3(int delta3)
        {
            set3(m_index3 + delta3);
        }

        /**
         * Advance this iterator by one atom.  This function is quicker than
         * 'move()'.
         */
        IteratorTemplate &operator++();

        /**
         * Move back this iterator by one atom.  This function is quicker than
         * 'move()'.
         */
        IteratorTemplate &operator--();

        /**
         * Check to see whether this iterator is at the end of the buffer.  If
         * so, this iterator points to nothing.
         */
        bool isEnd() const { return m_index == m_buffer.m_length; }

        int index() const { return m_index; }

        int index2() const { return m_index2; }

        int index3() const { return m_index3; }

        Pointer atom() const { return m_pointer; }
    
        // The following four functions are used for sequential bulk reads.
        /**
         * Move this iterator to the beginning of the next collection of
         * contiguous atoms, if existing.
         * @return True iff moved.
         */
        bool goToNextBulk();

        /**
         * Move this iterator to the beginning of the previous collection of
         * contiguous atoms, if existing.
         * @return True iff moved.
         */
        bool goToPreviousBulk();

        /**
         * Get the contiguous atoms surrounding this iterator.
         * @param begin The beginning of the found atoms.
         * @param end The end of the found atoms.
         * @return True iff found.
         */
        bool getAtomsBulk(Pointer &begin, Pointer &end) const;

        /**
         * Get the contiguous atoms surrounding this iterator.
         * @param atoms The beginning of the found atoms.
         * @param index The starting primary index of the found atoms.
         * @param index2 The starting secondary index of the found atoms.
         * @param index3 The starting tertiary index of the found atoms.
         * @param length The length of the found atoms in the primary index.
         * @param length2 The length of the found atoms in the secondary index.
         * @param length3 The length of the found atoms in the tertiary index.
         * @return True iff found.
         */
        bool getAtomsBulk(Pointer &atoms,
                          int &index,
                          int &index2,
                          int &index3,
                          int &length,
                          int &length2,
                          int &length3) const;

        bool operator==(const IteratorTemplate &rhs) const
        {
            assert(&m_buffer == &rhs.m_buffer);
            return m_index == rhs.m_index;
        }

        bool operator!=(const IteratorTemplate &rhs) const
        {
            return !(*this == rhs);
        }

    protected:
        /**
         * This constructor allows the derived class to initialize our data
         * members directly.
         */
        IteratorTemplate(BufferT &buffer,
                         BlockT *block,
                         Pointer pointer,
                         int index,
                         IntOrNil<SECONDARY_INDEXING> index2,
                         IntOrNil<TERTIARY_INDEXING> index3,
                         int indexInBlock,
                         IntOrNil<SECONDARY_INDEXING> index2InBlock,
                         IntOrNil<TERTIARY_INDEXING> index3InBlock):
            m_buffer(buffer),
            m_block(block),
            m_pointer(pointer),
            m_index(index),
            m_index2(index2),
            m_index3(index3),
            m_indexInBlock(indexInBlock),
            m_index2InBlock(index2InBlock),
            m_index3InBlock(index3InBlock)
        {}

        /**
         * Add back the skip in the tertiaary index.
         */
        void addSkip3();

        /**
         * Cancel the skip in the tertiary index.
         */
        void cancelSkip3();

        /**
         * The associated buffer.
         */
        BufferT &m_buffer;

        /**
         * The memory block in which we are.
         */
        BlockT *m_block;

        /**
         * The pointer to the content.
         */
        Pointer m_pointer;

        /**
         * The primary index.
         */
        int m_index;

        /**
         * The secondary index.
         */
        IntOrNil<SECONDARY_INDEXING> m_index2;

        /**
         * The tertiary index.
         */
        IntOrNil<TERTIARY_INDEXING> m_index3;

        /**
         * The primary index from the beginning of the containing memory block.
         */
        int m_indexInBlock;

        /**
         * The secondary index from the beginning of the containing memory
         * block.
         */
        IntOrNil<SECONDARY_INDEXING> m_index2InBlock;

        /**
         * The tertiary index from the beginning of the containing memory block.
         */
        IntOrNil<TERTIARY_INDEXING> m_index3InBlock;

        friend class Buffer;
    };

    typedef IteratorTemplate<Buffer, Block, Unit *> IteratorBase;
    typedef IteratorTemplate<const Buffer, const Block, const Unit *>
        ConstIteratorBase;

public:
    class ConstIterator;

private:
    /**
     * A read-write iterator only used by the buffer class.
     */
    class Iterator: public IteratorBase
    {
    public:
        Iterator(Buffer &buffer, int index, int index2, int index3):
            IteratorBase(buffer, index, index2, index3)
        {}

    private:
        friend class ConstIterator;
    };

public:
    /**
     * A read-only iterator.
     */
    class ConstIterator: public ConstIteratorBase
    {
    public:
        ConstIterator(const Buffer &buffer, int index, int index2, int index3):
            ConstIteratorBase(buffer, index, index2, index3)
        {}

        ConstIterator(const Iterator &iter):
            ConstIteratorBase(iter.m_buffer,
                              iter.m_block,
                              iter.m_pointer,
                              iter.m_index,
                              iter.m_index2,
                              iter.m_index3,
                              iter.m_indexInBlock,
                              iter.m_index2InBlock,
                              iter.m_index3InBlock)
        {}

        const ConstIterator &operator=(const Iterator &rhs)
        {
            assert(&this->m_buffer == &rhs.m_buffer);
            this->m_block = rhs.m_block;
            this->m_pointer = rhs.m_pointer;
            this->m_index = rhs.m_index;
            this->m_index2 = rhs.m_index2;
            this->m_index3 = rhs.m_index3;
            this->m_indexInBlock = rhs.m_indexInBlock;
            this->m_index2InBlock = rhs.m_index2InBlock;
            this->m_index3InBlock = rhs.m_index3InBlock;
            return *this;
        }
    };

    Buffer():
        m_first(new Block),
        m_last(m_first),
        m_length(0),
        m_length2(0),
        m_length3(0),
        m_cursor(*this, 0, -1, -1)
    {}

    virtual ~Buffer();

    int length() const { return m_length; }

    int length2() const { return m_length2; }

    int length3() const { return m_length3; }

    ConstIterator iteratorAtCursor() const { return m_cursor; }

    int cursor() const { return m_cursor.m_index; }

    int cursor2() const { return m_cursor.m_index2; }

    int cursor3() const { return m_cursor.m_index3; }

    void setCursor(int index) { m_cursor.set(index); }

    void setCursor2(int index2) { m_cursor.set2(index2); }

    void setCursor3(int index3) { m_cursor.set3(index3); }

    void moveCursor(int delta) { m_cursor.move(delta); }

    void moveCursor2(int delta2) { m_cursor.move2(delta2); }

    void moveCursor3(int delta3) { m_cursor.move3(delta3); }

    /**
     * Insert a stream of atoms at the cursor and move the cursor to the end of
     * the inserted stream.
     * @param atoms The atoms to be inserted.
     * @param length The total length of the atoms in the primary index.
     * @param length2 The total length of the atoms in the secondary index, or
     * -1 if the caller doesn't know it.
     * @param length3 The total length of the atoms in the tertiary index, or -1
     * if the caller doesn't know it.
     */
    void insert(const Unit *atoms, int length, int length2, int length3);

    /**
     * Remove a stream of atoms starting from the cursor.
     * @param length The total length of the atoms to be removed in the primary
     * index.
     * @param length2 The total length of the atoms in the secondary index, or
     * -1 if the caller doesn't know it.
     * @param length3 The total length of the atoms in the tertiary index, or -1
     * if the caller doesn't know it.
     */
    void remove(int length, int length2, int length3);

    /**
     * Replace a stream of atoms starting from the cursor with new atoms.  It is
     * required that the two streams have the same number of atoms, and the
     * corresponding atoms have the same lengths and the same tertiary indexes.
     * @param atoms The new atoms.
     * @param length The total length of the new atoms in the primary index.
     */
    void replace(const Unit *atoms, int length);

    /**
     * Make a writable gap at the cursor.
     * @param length The requested length of the gap in the primary index when
     * input, and the length of the actually made gap in the primary index,
     * which may be greater or less than the requested length, when output.
     * @return The beginning of the made writable gap.
     */
    Unit *makeGap(int &length);

    /**
     * Insert a stream of atoms that already resides in the gap.
     * @param atoms The atoms to be inserted, which should be stored in the made
     * gap, right from the beginning of the gap.
     * @param length The total length of the atoms in the primary index.
     * @param length2 The total length of the atoms in the secondary index, or
     * -1 if the caller doesn't know it.
     * @param length3 The total length of the atoms in the tertiary index, or -1
     * if the caller doesn't know it.
     */
    void insertInGap(const Unit *atoms, int length, int length2, int length3);

    /**
     * Read a stream of atoms starting from a position.
     * @param index The starting primary index.
     * @param length The total length of the atoms to be read in the primary
     * index.
     * @param atoms The read atoms.  The memory should be large enough.
     */
    void getAtoms(int index, int length, Unit *atoms) const;

    void removeAll()
    {
        setCursor(0);
        remove(m_length, m_length2, m_length3);
    }

    void transformIndex(int *index, int *index2, int *index3) const
    {
        int i = index ? *index : -1;
        int i2 = index2 ? *index2 : -1;
        int i3 = index3 ? *index3 : -1;
        ConstIterator iter(i, i2, i3);
        if (index)
            *index = iter.index();
        if (index2)
            *index2 = iter.index2();
        if (index3)
            *index3 = iter.index3();
    }

private:
    void countLength(const Unit *atoms, int length,
                     IntOrNil<SECONDARY_INDEXING> &length2,
                     IntOrNil<TERTIARY_INDEXING> &length3) const;

    /**
     * Append as many atoms at the end of a block as possible.  It is required
     * that the gap of the block be at the end.  Note that only the block will
     * be changed while the buffer itself and the cursor won't be affected.
     * @param block The target block.
     * @param atoms The atoms to be appended.
     * @param length The length of the atoms in the primary index.
     * @param appendedLength The length of the actually appended atoms in the
     * primary index.
     * @param appendedLength2 The length of the actually appended atoms in the
     * secondary index.
     * @param appendedLength3 The length of the actually appended atoms in the
     * tertiary index.
     */
    void appendInBlock(Block *block, const Unit *atoms, int length,
                       int &appendedLength,
                       IntOrNil<SECONDARY_INDEXING> &appendedLength2,
                       IntOrNil<TERTIARY_INDEXING> &appendedLength3);

    /**
     * The first and last block in this buffer.
     */
    Block *m_first;
    Block *m_last;

    /**
     * The total length of the stored atoms in the primary index, i.e., the unit
     * count.
     */
    int m_length;

    /**
     * The total length of the stored atoms in the secondary index, i.e., the
     * atom count.
     */
    IntOrNil<SECONDARY_INDEXING> m_length2;

    /**
     * The total length of the stored atoms in the tertiary index.
     */
    IntOrNil<TERTIARY_INDEXING> m_length3;

    /**
     * The cursor.
     */
    Iterator m_cursor;

    template<class BufferT, class BlockT, class Pointer>
        friend class IteratorTemplate;
};

template<class Unit, class AtomTraits, int BLOCK_SIZE, int UNIFORM_ATOM_LENGTH,
         bool SECONDARY_INDEXING, bool TERTIARY_INDEXING>
template<class BufferT, class BlockT, class Pointer>
void Buffer<Unit, AtomTraits, BLOCK_SIZE, UNIFORM_ATOM_LENGTH,
            SECONDARY_INDEXING, TERTIARY_INDEXING>::
    IteratorTemplate<BufferT, BlockT, Pointer>::set(int index)
{
    assert(index >= 0 && index <= m_buffer.m_length);

    // If the cursor is closer to the target, go to the cursor.
    if (abs(index - m_buffer.m_cursor.m_index) < abs(index - m_index))
    {
        m_block = m_buffer.m_cursor.m_block;
        m_pointer = m_buffer.m_cursor.m_pointer;
        m_index = m_buffer.m_cursor.m_index;
        m_index2 = m_buffer.m_cursor.m_index2;
        m_index3 = m_buffer.m_cursor.m_index3;
        m_indexInBlock = m_buffer.m_cursor.m_indexInBlock;
        m_index2InBlock = m_buffer.m_cursor.m_index2InBlock;
        m_index3InBlock = m_buffer.m_cursor.m_index3InBlock;
    }

    if (TERTIARY_INDEXING)
        cancelSkip3();

    // Iterate to the target block from the closest position.
    if (index < m_index / 2)
    {
        // Iterate forward from the beginning.
        m_block = m_buffer.m_first;
        m_index = 0;
        m_index2 = 0;
        m_index3 = 0;
        while (m_index + m_block->length() < index)
        {
            m_index += m_block->length();
            m_index2 += m_block->length2();
            m_index3 += m_block->length3();
            m_block = m_block->next();
        }
        m_pointer = m_block->begin();
        m_indexInBlock = 0;
        m_index2InBlock = 0;
        m_index3InBlock = 0;
    }
    else if (index < (m_index + m_buffer.m_length) / 2)
    {
        // Iterate from here.
        if (index < m_index - m_indexInBlock)
        {
            // Iterate backward.
            m_index -= m_indexInBlock;
            m_index2 -= m_index2InBlock;
            m_index3 -= m_index3InBlock;
            m_block = m_block->previous();
            while (m_index - m_block->length() > index)
            {
                m_index -= m_block->length();
                m_index2 -= m_block->length2();
                m_index3 -= m_block->length3();
                m_block = m_block->previous();
            }
            m_pointer = m_block->end();
            m_indexInBlock = m_block->length();
            m_index2InBlock = m_block->length2();
            m_index3InBlock = m_block->length3();
        }
        else if (index >= m_index - m_indexInBlock + m_block->length())
        {
            // Iterate forward.
            m_index -= m_indexInBlock;
            m_index2 -= m_index2InBlock;
            m_index3 -= m_index3InBlock;
            while (m_index + m_block->length() < index)
            {
                m_index += m_block->length();
                m_index2 += m_block->length2();
                m_index3 += m_block->length3();
                m_block = m_block->next();
            }
            m_pointer = m_block->begin();
            m_indexInBlock = 0;
            m_index2InBlock = 0;
            m_index3InBlock = 0;
        }
    }
    else
    {
        // Iterate backward from the end.
        m_block = m_buffer.m_last;
        m_index = m_buffer.m_length;
        m_index2 = m_buffer.m_length2;
        m_index3 = m_buffer.m_length3;
        while (m_index - m_block->length() > index)
        {
            m_index -= m_block->length();
            m_index2 -= m_block->length2();
            m_index3 -= m_block->length3();
            m_block = m_block->previous();
        }
        m_pointer = m_block->end();
        m_indexInBlock = m_block->length();
        m_index2InBlock = m_block->length2();
        m_index3InBlock = m_block->length3();
    }

    int baseIndex = m_index - m_indexInBlock;
    IntOrNil<SECONDARY_INDEXING> baseIndex2(m_index2 - m_index2InBlock);
    IntOrNil<TERTIARY_INDEXING> baseIndex3(m_index3 - m_index3InBlock);
    int indexInBlock = index - baseIndex;
    int halfLength = m_block->halfLength();
    IntOrNil<SECONDARY_INDEXING> halfLength2(m_block->halfLength2());
    IntOrNil<TERTIARY_INDEXING> halfLength3(m_block->halfLength3());

    if ((SECONDARY_INDEXING && UNIFORM_ATOM_LENGTH == -1) ||
        TERTIARY_INDEXING)
    {
        // If the gap is closer to the target, go to the gap.  If we are at the
        // gap, adjust the pointer to be in the half surrounding the target.
        if (abs(indexInBlock - halfLength) <=
            abs(indexInBlock - m_indexInBlock))
        {
            if (indexInBlock < halfLength)
                m_pointer = m_block->gapBegin();
            else
                m_pointer = m_block->gapEnd();
            m_indexInBlock = halfLength;
            m_index2InBlock = halfLength2;
            m_index3InBlock = halfLength3;
        }

        // Find the half surrounding the target.
        Pointer begin;
        int beginIndex;
        IntOrNil<SECONDARY_INDEXING> beginIndex2;
        IntOrNil<TERTIARY_INDEXING> beginIndex3;
        Pointer end;
        int endIndex;
        IntOrNil<SECONDARY_INDEXING> endIndex2;
        IntOrNil<TERTIARY_INDEXING> endIndex3;
        if (indexInBlock < halfLength)
        {
            begin = m_block->begin();
            beginIndex = 0;
            beginIndex2 = 0;
            beginIndex3 = 0;
            end = m_block->gapBegin();
            endIndex = halfLength;
            endIndex2 = halfLength2;
            endIndex3 = halfLength3;
        }
        else
        {
            begin = m_block->gapEnd();
            beginIndex = halfLength;
            beginIndex2 = halfLength2;
            beginIndex3 = halfLength3;
            end = m_block->end();
            endIndex = m_block->length();
            endIndex2 = m_block->length2();
            endIndex3 = m_block->length3();
        }

        // Assert that the pointer is in the half surrounding the target.
        assert(begin <= m_pointer);
        assert(m_pointer <= end);

        // Iterate to the target in this block from the closest position.
        if (indexInBlock < (beginIndex + m_indexInBlock) / 2)
        {
            // Iterate forward from the beginning.
            m_pointer = begin;
            m_index2InBlock = beginIndex2;
            m_index3InBlock = beginIndex3;
            Pointer stop = m_pointer + indexInBlock - beginIndex;
            assert(stop <= end);
            while (m_pointer < stop)
            {
                ++m_index2InBlock;
                m_index3InBlock += AtomTraits::skip3(m_pointer) +
                    AtomTraits::length3(m_pointer);
                m_pointer += AtomTraits::length(m_pointer);
            }
        }
        else if (indexInBlock < m_indexInBlock)
        {
            // Iterate backward from here.
            Pointer stop = m_pointer + indexInBlock - m_indexInBlock;
            assert(stop >= begin);
            while (m_pointer > stop)
            {
                m_pointer -= AtomTraits::previousAtomLength(m_pointer);
                --m_index2InBlock;
                m_index3InBlock -= AtomTraits::skip3(m_pointer) +
                    AtomTraits::length3(m_pointer);
            }
        }
        else if (indexInBlock < (m_indexInBlock + endIndex) / 2)
        {
            // Iterate forward from here.
            Pointer stop = m_pointer + indexInBlock - m_indexInBlock;
            assert(stop <= end);
            while (m_pointer < stop)
            {
                ++m_index2InBlock;
                m_index3InBlock += AtomTraits::skip3(m_pointer) +
                    AtomTraits::length3(m_pointer);
                m_pointer += AtomTraits::length(m_pointer);
            }
        }
        else
        {
            // Iterate backward from the end.
            m_pointer = end;
            m_index2InBlock = endIndex2;
            m_index3InBlock = endIndex3;
            Pointer stop = m_pointer + indexInBlock - endIndex;
            assert(stop >= begin);
            while (m_pointer > stop)
            {
                m_pointer -= AtomTraits::previousAtomLength(m_pointer);
                --m_index2InBlock;
                m_index3InBlock -= AtomTraits::skip3(m_pointer) +
                    AtomTraits::length3(m_pointer);
            }
        }
    }
    else
    {
        // Directly go to the target since the unit offset is known.
        if (indexInBlock < halfLength)
            m_pointer = m_block->begin() + indexInBlock;
        else
            m_pointer = m_block->gapEnd() + indexInBlock - halfLength;
        m_index2InBlock = indexInBlock / UNIFORM_ATOM_LENGTH;
    }

    // Update the other data members.
    m_index = index;
    m_indexInBlock = indexInBlock;
    m_index2 = baseIndex2 + m_index2InBlock;
    m_index3 = baseIndex3 + m_index3InBlock;

    // If we are at the end of a block and the block is not the last one, go to
    // the next block.
    if (m_indexInBlock == m_block->length() && m_block->next())
    {
        m_block = m_block->next();
        m_pointer = m_block->begin() == m_block->gapBegin() ?
            m_block->gapEnd() : m_block->begin();
        m_indexInBlock = 0;
        m_index2InBlock = 0;
        m_index3InBlock = 0;
    }

    if (TERTIARY_INDEXING)
        addSkip3();
}

template<class Unit, class AtomTraits, int BLOCK_SIZE, int UNIFORM_ATOM_LENGTH,
         bool SECONDARY_INDEXING, bool TERTIARY_INDEXING>
template<class BufferT, class BlockT, class Pointer>
void Buffer<Unit, AtomTraits, BLOCK_SIZE, UNIFORM_ATOM_LENGTH,
             SECONDARY_INDEXING, TERTIARY_INDEXING>::
    IteratorTemplate<BufferT, BlockT, Pointer>::set2(int index2)
{
    assert(index2 >= 0 && index2 <= m_buffer.m_length2);

    if (UNIFORM_ATOM_LENGTH != -1)
    {
        set(UNIFORM_ATOM_LENGTH * index2);
        return;
    }

    // If the cursor is closer to the target, go to the cursor.
    if (abs(index2 - m_buffer.m_cursor.m_index2) < abs(index2 - m_index2))
    {
        m_block = m_buffer.m_cursor.m_block;
        m_pointer = m_buffer.m_cursor.m_pointer;
        m_index = m_buffer.m_cursor.m_index;
        m_index2 = m_buffer.m_cursor.m_index2;
        m_index3 = m_buffer.m_cursor.m_index3;
        m_indexInBlock = m_buffer.m_cursor.m_indexInBlock;
        m_index2InBlock = m_buffer.m_cursor.m_index2InBlock;
        m_index3InBlock = m_buffer.m_cursor.m_index3InBlock;
    }

    if (TERTIARY_INDEXING)
        cancelSkip3();

    // Iterate to the target block from the closest position.
    if (index2 < m_index2 / 2)
    {
        // Iterate forward from the beginning.
        m_block = m_buffer.m_first;
        m_index = 0;
        m_index2 = 0;
        m_index3 = 0;
        while (m_index2 + m_block->length2() < index2)
        {
            m_index += m_block->length();
            m_index2 += m_block->length2();
            m_index3 += m_block->length3();
            m_block = m_block->next();
        }
        m_pointer = m_block->begin();
        m_indexInBlock = 0;
        m_index2InBlock = 0;
        m_index3InBlock = 0;
    }
    else if (index2 < (m_index2 + m_buffer.m_length2) / 2)
    {
        // Iterate from here.
        if (index2 < m_index2 - m_index2InBlock)
        {
            // Iterate backward.
            m_index -= m_indexInBlock;
            m_index2 -= m_index2InBlock;
            m_index3 -= m_index3InBlock;
            m_block = m_block->previous();
            while (m_index2 - m_block->length2() > index2)
            {
                m_index -= m_block->length();
                m_index2 -= m_block->length2();
                m_index3 -= m_block->length3();
                m_block = m_block->previous();
            }
            m_pointer = m_block->end();
            m_indexInBlock = m_block->length();
            m_index2InBlock = m_block->length2();
            m_index3InBlock = m_block->length3();
        }
        else if (index2 >= m_index2 - m_index2InBlock + m_block->length2())
        {
            // Iterate forward.
            m_index -= m_indexInBlock;
            m_index2 -= m_index2InBlock;
            m_index3 -= m_index3InBlock;
            while (m_index2 + m_block->length2() < index2)
            {
                m_index += m_block->length();
                m_index2 += m_block->length2();
                m_index3 += m_block->length3();
                m_block = m_block->next();
            }
            m_pointer = m_block->begin();
            m_indexInBlock = 0;
            m_index2InBlock = 0;
            m_index3InBlock = 0;
        }
    }
    else
    {
        // Iterate backward from the end.
        m_block = m_buffer.m_last;
        m_index = m_buffer.m_length;
        m_index2 = m_buffer.m_length2;
        m_index3 = m_buffer.m_length3;
        while (m_index2 - m_block->length2() > index2)
        {
            m_index -= m_block->length();
            m_index2 -= m_block->length2();
            m_index3 -= m_block->length3();
            m_block = m_block->previous();
        }
        m_pointer = m_block->end();
        m_indexInBlock = m_block->length();
        m_index2InBlock = m_block->length2();
        m_index3InBlock = m_block->length3();
    }

    int baseIndex = m_index - m_indexInBlock;
    int baseIndex2 = m_index2 - m_index2InBlock;
    IntOrNil<TERTIARY_INDEXING> baseIndex3(m_index3 - m_index3InBlock);
    int index2InBlock = index2 - baseIndex2;
    int halfLength = m_block->halfLength();
    int halfLength2 = m_block->halfLength2();
    IntOrNil<TERTIARY_INDEXING> halfLength3(m_block->halfLength3());

    // If the gap is closer to the target, go to the gap.  If we are at the gap,
    // adjust the pointer to be in the half surrounding the target.
    if (abs(index2InBlock - halfLength2) <=
        abs(index2InBlock - m_index2InBlock))
    {
        if (index2InBlock < halfLength2)
            m_pointer = m_block->gapBegin();
        else
            m_pointer = m_block->gapEnd();
        m_indexInBlock = halfLength;
        m_index2InBlock = halfLength2;
        m_index3InBlock = halfLength3;
    }

    // Find the half surrounding the target.
    Pointer begin;
    int beginIndex;
    int beginIndex2;
    IntOrNil<TERTIARY_INDEXING> beginIndex3;
    Pointer end;
    int endIndex;
    int endIndex2;
    IntOrNil<TERTIARY_INDEXING> endIndex3;
    if (index2InBlock < halfLength2)
    {
        begin = m_block->begin();
        beginIndex = 0;
        beginIndex2 = 0;
        beginIndex3 = 0;
        end = m_block->gapBegin();
        endIndex = halfLength;
        endIndex2 = halfLength2;
        endIndex3 = halfLength3;
    }
    else
    {
        begin = m_block->gapEnd();
        beginIndex = halfLength;
        beginIndex2 = halfLength2;
        beginIndex3 = halfLength3;
        end = m_block->end();
        endIndex = m_block->length();
        endIndex2 = m_block->length2();
        endIndex3 = m_block->length3();
    }

    // Assert that the pointer is in the half surrounding the target.
    assert(begin <= m_pointer);
    assert(m_pointer <= end);

    // Iterate to the target in this block from the closest position.
    if (index2InBlock < (beginIndex2 + m_index2InBlock) / 2)
    {
        // Iterate forward from the beginning.
        m_pointer = begin;
        m_index2InBlock = beginIndex2;
        m_index3InBlock = beginIndex3;
        while (m_index2InBlock < index2InBlock)
        {
            assert(m_pointer < end);
            ++m_index2InBlock;
            m_index3InBlock += AtomTraits::skip3(m_pointer) +
                AtomTraits::length3(m_pointer);
            m_pointer += AtomTraits::length(m_pointer);
        }
        m_indexInBlock = beginIndex + m_pointer - begin;
    }
    else if (index2InBlock < m_index2InBlock)
    {
        // Iterate backward from here.
        Pointer save = m_pointer;
        while (m_index2InBlock > index2InBlock)
        {
            assert(m_pointer > begin);
            m_pointer -= AtomTraits::previousAtomLength(m_pointer);
            --m_index2InBlock;
            m_index3InBlock -= AtomTraits::skip3(m_pointer) +
                AtomTraits::length3(m_pointer);
        }
        m_indexInBlock += m_pointer - save;
    }
    else if (index2InBlock < (m_index2InBlock + endIndex2) / 2)
    {
        // Iterate forward from here.
        Pointer save = m_pointer;
        while (m_index2InBlock < index2InBlock)
        {
            assert(m_pointer < end);
            ++m_index2InBlock;
            m_index3InBlock += AtomTraits::skip3(m_pointer) +
                AtomTraits::length3(m_pointer);
            m_pointer += AtomTraits::length(m_pointer);
        }
        m_indexInBlock += m_pointer - save;
    }
    else
    {
        // Iterate backward from the end.
        m_pointer = end;
        m_index2InBlock = endIndex2;
        m_index3InBlock = endIndex3;
        while (m_index2InBlock > index2InBlock)
        {
            assert(m_pointer > begin);
            m_pointer -= AtomTraits::previousAtomLength(m_pointer);
            --m_index2InBlock;
            m_index3InBlock -= AtomTraits::skip3(m_pointer) +
                AtomTraits::length3(m_pointer);
        }
        m_indexInBlock = endIndex + m_pointer - end;
    }

    // Update the other data members.
    m_index = baseIndex + m_indexInBlock;
    m_index2 = baseIndex2 + m_index2InBlock;
    m_index3 = baseIndex3 + m_index3InBlock;

    // If we are at the end of a block and the block is not the last one, go to
    // the next block.
    if (m_indexInBlock == m_block->length() && m_block->next())
    {
        m_block = m_block->next();
        m_pointer = m_block->begin() == m_block->gapBegin() ?
            m_block->gapEnd() : m_block->begin();
        m_indexInBlock = 0;
        m_index2InBlock = 0;
        m_index3InBlock = 0;
    }

    if (TERTIARY_INDEXING)
        addSkip3();
}

template<class Unit, class AtomTraits, int BLOCK_SIZE, int UNIFORM_ATOM_LENGTH,
         bool SECONDARY_INDEXING, bool TERTIARY_INDEXING>
template<class BufferT, class BlockT, class Pointer>
void Buffer<Unit, AtomTraits, BLOCK_SIZE, UNIFORM_ATOM_LENGTH,
            SECONDARY_INDEXING, TERTIARY_INDEXING>::
    IteratorTemplate<BufferT, BlockT, Pointer>::set3(int index3)
{
    assert(index3 >= 0 && index3 <= m_buffer.m_length3);

    // If the cursor is closer to the target, go to the cursor.
    if (abs(index3 - m_buffer.m_cursor.m_index3) < abs(index3 - m_index3))
    {
        m_block = m_buffer.m_cursor.m_block;
        m_pointer = m_buffer.m_cursor.m_pointer;
        m_index = m_buffer.m_cursor.m_index;
        m_index2 = m_buffer.m_cursor.m_index2;
        m_index3 = m_buffer.m_cursor.m_index3;
        m_indexInBlock = m_buffer.m_cursor.m_indexInBlock;
        m_index2InBlock = m_buffer.m_cursor.m_index2InBlock;
        m_index3InBlock = m_buffer.m_cursor.m_index3InBlock;
    }

    cancelSkip3();

    // Iterate to the target block from the closest position.
    if (index3 < m_index3 / 2)
    {
        // Iterate forward from the beginning.
        m_block = m_buffer.m_first;
        m_index = 0;
        m_index2 = 0;
        m_index3 = 0;
        while (index3 > m_index3 + m_block->length3())
        {
            m_index += m_block->length();
            m_index2 += m_block->length2();
            m_index3 += m_block->length3();
            m_block = m_block->next();
        }
        m_pointer = m_block->begin();
        m_indexInBlock = 0;
        m_index2InBlock = 0;
        m_index3InBlock = 0;
    }
    else if (index3 < (m_index3 + m_buffer.m_length3) / 2)
    {
        // Iterate from here.
        if (index3 < m_index3 - m_index3InBlock)
        {
            // Iterate backward.
            m_index -= m_indexInBlock;
            m_index2 -= m_index2InBlock;
            m_index3 -= m_index3InBlock;
            m_block = m_block->previous();
            while (index3 <= m_index3 - m_block->length3() &&
                   m_block->previous())
            {
                m_index -= m_block->length();
                m_index2 -= m_block->length2();
                m_index3 -= m_block->length3();
                m_block = m_block->previous();
            }
            m_pointer = m_block->end();
            m_indexInBlock = m_block->length();
            m_index2InBlock = m_block->length2();
            m_index3InBlock = m_block->length3();
        }
        else if (index3 >= m_index3 - m_index3InBlock + m_block->length3())
        {
            // Iterate forward.
            m_index -= m_indexInBlock;
            m_index2 -= m_index2InBlock;
            m_index3 -= m_index3InBlock;
            while (index3 > m_index3 + m_block->length3())
            {
                m_index += m_block->length();
                m_index2 += m_block->length2();
                m_index3 += m_block->length3();
                m_block = m_block->next();
            }
            m_pointer = m_block->begin();
            m_indexInBlock = 0;
            m_index2InBlock = 0;
            m_index3InBlock = 0;
        }
    }
    else
    {
        // Iterate backward from the end.
        m_block = m_buffer.m_last;
        m_index = m_buffer.m_length;
        m_index2 = m_buffer.m_length2;
        m_index3 = m_buffer.m_length3;
        while (index3 <= m_index3 - m_block->length3() &&
               m_block->previous())
        {
            m_index -= m_block->length();
            m_index2 -= m_block->length2();
            m_index3 -= m_block->length3();
            m_block = m_block->previous();
        }
        m_pointer = m_block->end();
        m_indexInBlock = m_block->length();
        m_index2InBlock = m_block->length2();
        m_index3InBlock = m_block->length3();
    }

    int baseIndex = m_index - m_indexInBlock;
    IntOrNil<SECONDARY_INDEXING> baseIndex2(m_index2 - m_index2InBlock);
    int baseIndex3 = m_index3 - m_index3InBlock;
    int index3InBlock = index3 - baseIndex3;
    int halfLength = m_block->halfLength();
    IntOrNil<SECONDARY_INDEXING> halfLength2(m_block->halfLength2());
    int halfLength3 = m_block->halfLength3();

    // If the gap is closer to the target, go to the gap.  If we are at the gap,
    // adjust the pointer to be in the half surrounding the target.
    if (abs(index3InBlock - halfLength3) <=
        abs(index3InBlock - m_index3InBlock))
    {
        if (index3InBlock <= halfLength3)
            m_pointer = m_block->gapBegin();
        else
            m_pointer = m_block->gapEnd();
        m_indexInBlock = halfLength;
        m_index2InBlock = halfLength2;
        m_index3InBlock = halfLength3;
    }

    // Find the half surrounding the target.
    Pointer begin;
    int beginIndex;
    IntOrNil<SECONDARY_INDEXING> beginIndex2;
    int beginIndex3;
    Pointer end;
    int endIndex;
    IntOrNil<SECONDARY_INDEXING> endIndex2;
    int endIndex3;
    if (index3InBlock <= halfLength3)
    {
        begin = m_block->begin();
        beginIndex = 0;
        beginIndex2 = 0;
        beginIndex3 = 0;
        end = m_block->gapBegin();
        endIndex = halfLength;
        endIndex2 = halfLength2;
        endIndex3 = halfLength3;
    }
    else
    {
        begin = m_block->gapEnd();
        beginIndex = halfLength;
        beginIndex2 = halfLength2;
        beginIndex3 = halfLength3;
        end = m_block->end();
        endIndex = m_block->length();
        endIndex2 = m_block->length2();
        endIndex3 = m_block->length3();
    }

    // Assert that the pointer is in the half surrounding the target.
    assert(begin <= m_pointer);
    assert(m_pointer <= end);

    // Iterate to the target in this block from the closest position.
    if (index3InBlock < (beginIndex3 + m_index3InBlock) / 2)
    {
        // Iterate forward from the beginning.
        m_pointer = begin;
        m_index2InBlock = beginIndex2;
        m_index3InBlock = beginIndex3;
        while (m_index3InBlock + AtomTraits::skip3(m_pointer) < index3InBlock)
        {
            assert(m_pointer < end);
            ++m_index2InBlock;
            m_index3InBlock += AtomTraits::skip3(m_pointer) +
                AtomTraits::length3(m_pointer);
            m_pointer += AtomTraits::length(m_pointer);
        }
        m_indexInBlock = beginIndex + m_pointer - begin;
    }
    else if (index3InBlock < m_index3InBlock)
    {
        // Iterate backward from here.
        Pointer save = m_pointer;
        while (m_pointer > begin && m_index3InBlock >= index3InBlock)
        {
            Pointer tp = m_pointer -
                AtomTraits::previousAtomLength(m_pointer);
            int ti = m_index3InBlock - AtomTraits::length3(tp);
            if (ti < index3InBlock)
                break;
            m_pointer = tp;
            m_index3InBlock = ti - AtomTraits::skip3(tp);
            --m_index2InBlock;
        }
        m_indexInBlock += m_pointer - save;
    }
    else if (index3InBlock < (m_index3InBlock + endIndex3) / 2)
    {
        // Iterate forward from here.
        Pointer save = m_pointer;
        while (m_index3InBlock + AtomTraits::skip3(m_pointer) < index3InBlock)
        {
            assert(m_pointer < end);
            ++m_index2InBlock;
            m_index3InBlock += AtomTraits::skip3(m_pointer) +
                AtomTraits::length3(m_pointer);
            m_pointer += AtomTraits::length(m_pointer);
        }
        m_indexInBlock += m_pointer - save;
    }
    else
    {
        // Iterate backward from the end.
        m_pointer = end;
        m_index2InBlock = endIndex2;
        m_index3InBlock = endIndex3;
        while (m_pointer > begin && m_index3InBlock >= index3InBlock)
        {
            Pointer tp = m_pointer -
                AtomTraits::previousAtomLength(m_pointer);
            int ti = m_index3InBlock - AtomTraits::length3(tp);
            if (ti < index3InBlock)
                break;
            m_pointer = tp;
            m_index3InBlock = ti - AtomTraits::skip3(tp);
            --m_index2InBlock;
        }
        m_indexInBlock = endIndex + m_pointer - end;
    }

    // Update the other data members.
    m_index = baseIndex + m_indexInBlock;
    m_index2 = baseIndex2 + m_index2InBlock;
    m_index3 = baseIndex3 + m_index3InBlock;

    // If we are at the beginning of the gap, go to the end of the gap.
    if (m_pointer == m_block->gapBegin())
        m_pointer = m_block->gapEnd();

    // If we are at the end of a block and the block is not the last one, go to
    // the next block.
    if (m_indexInBlock == m_block->length() && m_block->next())
    {
        m_block = m_block->next();
        m_pointer = m_block->begin() == m_block->gapBegin() ?
            m_block->gapEnd() : m_block->begin();
        m_indexInBlock = 0;
        m_index2InBlock = 0;
        m_index3InBlock = 0;
    }

    addSkip3();
}

template<class Unit, class AtomTraits, int BLOCK_SIZE, int UNIFORM_ATOM_LENGTH,
         bool SECONDARY_INDEXING, bool TERTIARY_INDEXING>
template<class BufferT, class BlockT, class Pointer>
Buffer<Unit, AtomTraits, BLOCK_SIZE, UNIFORM_ATOM_LENGTH,
       SECONDARY_INDEXING, TERTIARY_INDEXING>::
    IteratorTemplate<BufferT, BlockT, Pointer> &
Buffer<Unit, AtomTraits, BLOCK_SIZE, UNIFORM_ATOM_LENGTH,
       SECONDARY_INDEXING, TERTIARY_INDEXING>::
    IteratorTemplate<BufferT, BlockT, Pointer>::operator++()
{
    assert(m_index < m_buffer.m_length);
    assert(m_indexInBlock < m_block->length());

    int delta = AtomTraits::length(m_pointer);
    IntOrNil<TERTIARY_INDEXING> delta3(AtomTraits::length3(m_pointer));
    m_pointer += delta;
    m_index += delta;
    ++m_index2;
    m_index3 += delta3;
    m_indexInBlock += delta;
    ++m_index2InBlock;
    m_index3InBlock += delta3;

    if (m_pointer == m_block->gapBegin())
        m_pointer = m_block->gapEnd();
    if (m_pointer == m_block->end() && m_block != m_buffer.m_last)
    {
        m_block = m_block->next();
        if (m_block->begin() == m_block->gapBegin())
            m_pointer = m_block->gapEnd();
        else
            m_pointer = m_block->begin();
        m_indexInBlock = 0;
        m_index2InBlock = 0;
        m_index3InBlock = 0;
    }

    if (TERTIARY_INDEXING)
        addSkip3();

    return *this;
}

template<class Unit, class AtomTraits, int BLOCK_SIZE, int UNIFORM_ATOM_LENGTH,
         bool SECONDARY_INDEXING, bool TERTIARY_INDEXING>
template<class BufferT, class BlockT, class Pointer>
Buffer<Unit, AtomTraits, BLOCK_SIZE, UNIFORM_ATOM_LENGTH,
       SECONDARY_INDEXING, TERTIARY_INDEXING>::
    IteratorTemplate<BufferT, BlockT, Pointer> &
Buffer<Unit, AtomTraits, BLOCK_SIZE, UNIFORM_ATOM_LENGTH,
       SECONDARY_INDEXING, TERTIARY_INDEXING>::
    IteratorTemplate<BufferT, BlockT, Pointer>::operator--()
{
    assert(m_index > 0);

    if (TERTIARY_INDEXING)
        cancelSkip3();

    if (m_pointer == m_block->gapEnd())
        m_pointer = m_block->gapBegin();
    if (m_pointer == m_block->begin())
    {
        m_block = m_block->previous();
        if (m_block->end() == m_block->gapEnd())
            m_pointer = m_block->gapBegin();
        else
            m_pointer = m_block->end();
        m_indexInBlock = m_block->length();
        m_index2InBlock = m_block->length2();
        m_index3InBlock = m_block->length3();
    }

    int delta = AtomTraits::previousAtomLength(m_pointer);
    m_pointer -= delta;
    IntOrNil<TERTIARY_INDEXING> delta3(AtomTraits::length3(m_pointer));
    m_index -= delta;
    --m_index2;
    m_index3 -= delta3;
    m_indexInBlock -= delta;
    --m_index2InBlock;
    m_index3InBlock -= delta3;

    return *this;
}

template<class Unit, class AtomTraits, int BLOCK_SIZE, int UNIFORM_ATOM_LENGTH,
         bool SECONDARY_INDEXING, bool TERTIARY_INDEXING>
template<class BufferT, class BlockT, class Pointer>
bool Buffer<Unit, AtomTraits, BLOCK_SIZE, UNIFORM_ATOM_LENGTH,
            SECONDARY_INDEXING, TERTIARY_INDEXING>::
    IteratorTemplate<BufferT, BlockT, Pointer>::goToNextBulk()
{
    if (m_block->gapBegin() == m_block->gapEnd() ||
        m_pointer >= m_block->gapEnd() ||
        m_block->gapEnd() == m_block->end())
    {
        if (!m_block->next())
            return false;
        if (TERTIARY_INDEXING)
            cancelSkip3();
        m_block = m_block->next();
        if (m_block->begin() == m_block->gapBegin())
            m_pointer = m_block->gapEnd();
        else
            m_pointer = m_block->begin();
        m_index -= m_indexInBlock;
        m_index += m_block->previous()->length();
        m_indexInBlock = 0;
        m_index2 -= m_index2InBlock;
        m_index2 += m_block->previous()->length2();
        m_index2InBlock = 0;
        m_index3 -= m_index3InBlock;
        m_index3 += m_block->previous()->length3();
        m_index3InBlock = 0;
        if (TERTIARY_INDEXING)
            addSkip3();
        return true;
    }
    if (TERTIARY_INDEXING)
        cancelSkip3();
    m_pointer = m_block->gapEnd();
    int delta = m_block->halfLength() - m_indexInBlock;
    m_index += delta;
    m_indexInBlock += delta;
    IntOrNil<SECONDARY_INDEXING> delta2(m_block->halfLength2() - m_index2InBlock);
    m_index2 += delta2;
    m_index2InBlock += delta2;
    IntOrNil<TERTIARY_INDEXING> delta3(m_block->halfLength3() - m_index3InBlock);
    m_index3 += delta3;
    m_index3InBlock += delta3;
    if (TERTIARY_INDEXING)
        addSkip3();
    return true;
}

template<class Unit, class AtomTraits, int BLOCK_SIZE, int UNIFORM_ATOM_LENGTH,
         bool SECONDARY_INDEXING, bool TERTIARY_INDEXING>
template<class BufferT, class BlockT, class Pointer>
bool Buffer<Unit, AtomTraits, BLOCK_SIZE, UNIFORM_ATOM_LENGTH,
            SECONDARY_INDEXING, TERTIARY_INDEXING>::
    IteratorTemplate<BufferT, BlockT, Pointer>::goToPreviousBulk()
{
    if (m_block->gapBegin() == m_block->gapEnd() ||
        m_pointer < m_block->gapBegin() ||
        m_block->gapBegin() == m_block->begin())
    {
        if (!m_block->previous())
            return false;
        if (TERTIARY_INDEXING)
            cancelSkip3();
        m_block = m_block->previous();
        m_index -= m_indexInBlock + m_block->length();
        m_index2 -= m_index2InBlock + m_block->length2();
        m_index3 -= m_index3InBlock + m_block->length3();
        if (m_block->end() == m_block->gapEnd())
        {
            m_pointer = m_block->begin();
            m_indexInBlock = 0;
            m_index2InBlock = 0;
            m_index3InBlock = 0;
        }
        else
        {
            m_pointer = m_block->gapEnd();
            m_indexInBlock = m_block->halfLength();
            m_index += m_indexInBlock;
            m_index2InBlock = m_block->halfLength2();
            m_index2 += m_index2InBlock;
            m_index3InBlock = m_block->halfLength3();
            m_index3 += m_index3InBlock;
        }
        if (TERTIARY_INDEXING)
            addSkip3();
        return true;
    }
    if (TERTIARY_INDEXING)
        cancelSkip3();
    m_pointer = m_block->begin();
    m_index -= m_indexInBlock;
    m_indexInBlock = 0;
    m_index2 -= m_index2InBlock;
    m_index2InBlock = 0;
    m_index3 -= m_index3InBlock;
    m_index3InBlock = 0;
    if (TERTIARY_INDEXING)
        addSkip3();
    return true;
}

template<class Unit, class AtomTraits, int BLOCK_SIZE, int UNIFORM_ATOM_LENGTH,
         bool SECONDARY_INDEXING, bool TERTIARY_INDEXING>
template<class BufferT, class BlockT, class Pointer>
bool Buffer<Unit, AtomTraits, BLOCK_SIZE, UNIFORM_ATOM_LENGTH,
            SECONDARY_INDEXING, TERTIARY_INDEXING>::
    IteratorTemplate<BufferT, BlockT, Pointer>::getAtomsBulk(
        Pointer &begin, Pointer &end) const
{
    if (m_block->gapBegin() == m_block->gapEnd())
    {
        begin = m_block->begin();
        end = m_block->end();
    }
    else if (m_pointer < m_block->gapBegin())
    {
        begin = m_block->begin();
        end = m_block->gapBegin();
    }
    else
    {
        begin = m_block->gapEnd();
        end = m_block->end();
    }
    return begin < end;
}

template<class Unit, class AtomTraits, int BLOCK_SIZE, int UNIFORM_ATOM_LENGTH,
         bool SECONDARY_INDEXING, bool TERTIARY_INDEXING>
template<class BufferT, class BlockT, class Pointer>
bool Buffer<Unit, AtomTraits, BLOCK_SIZE, UNIFORM_ATOM_LENGTH,
            SECONDARY_INDEXING, TERTIARY_INDEXING>::
    IteratorTemplate<BufferT, BlockT, Pointer>::getAtomsBulk(
        Pointer &atoms,
        int &index, int &index2, int &index3,
        int &length, int &length2, int &length3)
    const
{
    if (m_block->gapBegin() == m_block->gapEnd())
    {
        atoms = m_block->begin();
        index = m_index - m_indexInBlock;
        index2 = m_index2 - m_index2InBlock;
        index3 = m_index3 - m_index3InBlock;
        length = m_block->length();
        length2 = m_block->length2();
        length3 = m_block->length3();
    }
    else if (m_pointer < m_block->gapBegin())
    {
        atoms = m_block->begin();
        index = m_index - m_indexInBlock;
        index2 = m_index2 - m_index2InBlock;
        index3 = m_index3 - m_index3InBlock;
        length = m_block->halfLength();
        length2 = m_block->halfLength2();
        length3 = m_block->halfLength3();
    }
    else
    {
        atoms = m_block->gapEnd();
        index = m_index - m_indexInBlock + m_block->halfLength();
        index2 = m_index2 - m_index2InBlock + m_block->halfLength2();
        index3 = m_index3 - m_index3InBlock + m_block->halfLength3();
        length = m_block->length() - m_block->halfLength();
        length2 = m_block->length2() - m_block->halfLength2();
        length3 = m_block->length3() - m_block->halfLength3();
    }
    return length;
}

template<class Unit, class AtomTraits, int BLOCK_SIZE, int UNIFORM_ATOM_LENGTH,
         bool SECONDARY_INDEXING, bool TERTIARY_INDEXING>
template<class BufferT, class BlockT, class Pointer>
void Buffer<Unit, AtomTraits, BLOCK_SIZE, UNIFORM_ATOM_LENGTH,
            SECONDARY_INDEXING, TERTIARY_INDEXING>::
    IteratorTemplate<BufferT, BlockT, Pointer>::addSkip3()
{
    if (m_index < m_buffer.m_length)
    {
        int skip = AtomTraits::skip3(m_pointer);
        m_index3 += skip;
        m_index3InBlock += skip;
    }
}

template<class Unit, class AtomTraits, int BLOCK_SIZE, int UNIFORM_ATOM_LENGTH,
         bool SECONDARY_INDEXING, bool TERTIARY_INDEXING>
template<class BufferT, class BlockT, class Pointer>
void Buffer<Unit, AtomTraits, BLOCK_SIZE, UNIFORM_ATOM_LENGTH,
            SECONDARY_INDEXING, TERTIARY_INDEXING>::
    IteratorTemplate<BufferT, BlockT, Pointer>::cancelSkip3()
{
    if (m_index < m_buffer.m_length)
    {
        int skip = AtomTraits::skip3(m_pointer);
        m_index3 -= skip;
        m_index3InBlock -= skip;
    }
}

template<class Unit, class AtomTraits, int BLOCK_SIZE, int UNIFORM_ATOM_LENGTH,
         bool SECONDARY_INDEXING, bool TERTIARY_INDEXING>
Buffer<Unit, AtomTraits, BLOCK_SIZE, UNIFORM_ATOM_LENGTH, SECONDARY_INDEXING,
       TERTIARY_INDEXING>::
    ~Buffer()
{
    // Destroy all the memory blocks.
    while (m_first)
    {
        Block *next = m_first->next();
        delete m_first;
        m_first = next;
    }
}

template<class Unit, class AtomTraits, int BLOCK_SIZE, int UNIFORM_ATOM_LENGTH,
         bool SECONDARY_INDEXING, bool TERTIARY_INDEXING>
void Buffer<Unit, AtomTraits, BLOCK_SIZE, UNIFORM_ATOM_LENGTH,
            SECONDARY_INDEXING, TERTIARY_INDEXING>::
    insert(const Unit *atoms, int length, int length2, int length3)
{
    assert(length >= 0);
    assert(length2 == -1 || length2 >= 0);
    assert(length3 == -1 || length3 >= 0);

    if (length == 0)
        return;

    if (TERTIARY_INDEXING)
        m_cursor.cancelSkip3();

    Block *block = m_cursor.m_block;

    // A special case.  If the cursor is at the beginning of this block and the
    // gap in the previous block is at the end, insert the atoms at the end of
    // the previous block.
    if (m_cursor.atom() == (block->begin() == block->gapBegin() ?
                            block->gapEnd() : block->begin()) &&
        block->previous() &&
        block->previous()->end() == block->previous()->gapEnd())
        block = block->previous();

    Block *next = block->next();

    // If this block can hold the atoms, do it.
    if (block->gapSize() >= length)
    {
        if (m_cursor.m_block == block)
        {
            // Move the gap to the cursor.
            if (m_cursor.atom() < block->gapBegin())
            {
                int len = block->gapBegin() - m_cursor.atom();
                memmove(m_cursor.atom() + block->gapSize(),
                        m_cursor.atom(),
                        sizeof(Unit) * len);
                block->m_gapBegin -= len;
                block->m_gapEnd -= len;
            }
            else if (m_cursor.atom() > block->gapEnd())
            {
                int len = m_cursor.atom() - block->gapEnd();
                memmove(block->gapBegin(),
                        block->gapEnd(),
                        sizeof(Unit) * len);
                block->m_gapBegin += len;
                block->m_gapEnd += len;
            }
            block->m_halfLength2 = m_cursor.m_index2InBlock;
            block->m_halfLength3 = m_cursor.m_index3InBlock;
        }

        // Insert the atoms.
        memcpy(block->gapBegin(), atoms, sizeof(Unit) * length);

        // Count the length in the secondary index or the tertiary index if not
        // given.
        IntOrNil<SECONDARY_INDEXING> len2;
        IntOrNil<TERTIARY_INDEXING> len3;
        bool needCount = false;
        if (SECONDARY_INDEXING)
        {
            if (length2 == -1)
            {
                if (UNIFORM_ATOM_LENGTH == -1)
                    needCount = true;
                else
                    len2 = length / UNIFORM_ATOM_LENGTH;
            }
            else
                len2 = length2;
        }
        if (TERTIARY_INDEXING)
        {
            if (length3 == -1)
                needCount = true;
            else
                len3 = length3;
        }
        if (needCount)
            countLength(atoms, length, len2, len3);

        // Update the data members.
        block->m_gapBegin += length;
        block->m_length += length;
        block->m_length2 += len2;
        block->m_length3 += len3;
        block->m_halfLength2 += len2;
        block->m_halfLength3 += len3;
        m_cursor.m_index += length;
        m_cursor.m_index2 += len2;
        m_cursor.m_index3 += len3;
        if (m_cursor.m_block == block)
        {
            m_cursor.m_pointer = block->gapEnd();
            m_cursor.m_indexInBlock = block->halfLength();
            m_cursor.m_index2InBlock = block->halfLength2();
            m_cursor.m_index3InBlock = block->halfLength3();
        }
        if (TERTIARY_INDEXING)
            m_cursor.addSkip3();
        m_length += length;
        m_length2 += len2;
        m_length3 += len3;
        return;
    }

    // Pop the content following the cursor out of this block.
    if (m_cursor.m_block == block &&
        m_cursor.m_indexInBlock < block->length())
    {
        // If the next block can hold the content, move them to the next block.
        // Otherwise, create a new block following this block to hold them.
        int len = block->length() - m_cursor.m_indexInBlock;
        IntOrNil<SECONDARY_INDEXING> len2 =
            block->length2() - m_cursor.m_index2InBlock;
        IntOrNil<TERTIARY_INDEXING> len3 =
            block->length3() - m_cursor.m_index3InBlock;
        if (next && next->gapSize() >= len)
        {
            if (next->begin() != next->gapBegin())
            {
                int l = next->gapBegin() - next->begin();
                memmove(next->gapEnd() - l, next->begin(), sizeof(Unit) * l);
                next->m_gapBegin -= l;
                next->m_gapEnd -= l;
                next->m_halfLength2 = 0;
                next->m_halfLength3 = 0;
            }
        }
        else
        {
            Block *newBlock = new Block;
            newBlock->m_previous = block;
            newBlock->m_next = next;
            block->m_next = newBlock;
            if (next)
                next->m_previous = newBlock;
            else
                m_last = newBlock;
            next = newBlock;
        }

        // Move the content following the cursor to the next block and move the
        // gap to the end of this block.
        if (m_cursor.atom() < block->gapBegin())
        {
            int sndHalfLen = block->end() - block->gapEnd();
            memcpy(next->gapEnd() - sndHalfLen,
                   block->gapEnd(),
                   sizeof(Unit) * sndHalfLen);
            next->m_gapEnd -= sndHalfLen;
            int cursorLen = block->gapBegin() - m_cursor.atom();
            memcpy(next->gapEnd() - cursorLen,
                   m_cursor.atom(),
                   sizeof(Unit) * cursorLen);
            next->m_gapEnd -= cursorLen;
            block->m_gapBegin -= cursorLen;
        }
        else
        {
            int cursorLen = block->end() - m_cursor.atom();
            memcpy(next->gapEnd() - cursorLen,
                   m_cursor.atom(),
                   sizeof(Unit) * cursorLen);
            next->m_gapEnd -= cursorLen;
            int sndHalfLen = m_cursor.atom() - block->gapEnd();
            memmove(block->gapBegin(),
                    block->gapEnd(),
                    sizeof(Unit) * sndHalfLen);
            block->m_gapBegin += sndHalfLen;
        }
        block->m_gapEnd = block->end();
        block->m_halfLength2 = m_cursor.m_index2InBlock;
        block->m_halfLength3 = m_cursor.m_index3InBlock;
        next->m_length += len;
        block->m_length -= len;
        next->m_length2 += len2;
        block->m_length2 -= len2;
        next->m_length3 += len3;
        block->m_length3 -= len3;
    }

    // First insert as many atoms to this block as possible.
    int leftLen = length;
    IntOrNil<SECONDARY_INDEXING> totalLen2;
    IntOrNil<TERTIARY_INDEXING> totalLen3;
    int appLen;
    IntOrNil<SECONDARY_INDEXING> appLen2;
    IntOrNil<TERTIARY_INDEXING> appLen3;
    appendInBlock(block, atoms, leftLen, appLen, appLen2, appLen3);
    atoms += appLen;
    leftLen -= appLen;
    totalLen2 += appLen2;
    totalLen3 += appLen3;

    // Split the atom stream and insert them into blocks.
    while (leftLen)
    {
        // Always try to insert the left atoms into the next block.
        if (next && next->gapSize() >= leftLen)
        {
            // If the gap in the next block is not at the beginning of the
            // block, move it.
            if (next->begin() != next->gapBegin())
            {
                int len = next->gapBegin() - next->begin();
                memmove(next->gapEnd() - len,
                        next->begin(),
                        sizeof(Unit) * len);
                next->m_gapBegin -= len;
                next->m_gapEnd -= len;
                next->m_halfLength2 = 0;
                next->m_halfLength3 = 0;
            }

            // Insert the atoms.
            memcpy(next->gapBegin(), atoms, sizeof(Unit) * leftLen);

            // Count the length in the secondary index or the tertiary index if
            // not given.
            IntOrNil<SECONDARY_INDEXING> len2;
            IntOrNil<TERTIARY_INDEXING> len3;
            bool needCount = false;
            if (SECONDARY_INDEXING)
            {
                if (length2 == -1)
                {
                    if (UNIFORM_ATOM_LENGTH == -1)
                        needCount = true;
                    else
                        len2 = leftLen / UNIFORM_ATOM_LENGTH;
                }
                else
                    len2 = length2 - totalLen2;
            }
            if (TERTIARY_INDEXING)
            {
                if (length3 == -1)
                    needCount = true;
                else
                    len3 = length3 - totalLen3;
            }
            if (needCount)
                countLength(atoms, leftLen, len2, len3);

            totalLen2 += len2;
            totalLen3 += len3;
            next->m_gapBegin += leftLen;
            next->m_length += leftLen;
            next->m_length2 += len2;
            next->m_halfLength2 += len2;
            next->m_length3 += len3;
            next->m_halfLength3 += len3;
            block = next;
            break;
        }

        // Create a new block to hold the left atoms.
        Block *newBlock = new Block;
        int appLen;
        IntOrNil<SECONDARY_INDEXING> appLen2;
        IntOrNil<TERTIARY_INDEXING> appLen3;
        appendInBlock(newBlock, atoms, leftLen, appLen, appLen2, appLen3);
        atoms += appLen;
        leftLen -= appLen;
        totalLen2 += appLen2;
        totalLen3 += appLen3;
        newBlock->m_previous = block;
        newBlock->m_next = next;
        block->m_next = newBlock;
        if (next)
            next->m_previous = newBlock;
        else
            m_last = newBlock;
        block = newBlock;
    }

    // Update the cursor.
    m_cursor.m_block = block;
    m_cursor.m_pointer = block->gapEnd();
    m_cursor.m_indexInBlock = block->halfLength();
    m_cursor.m_index2InBlock = block->halfLength2();
    m_cursor.m_index3InBlock = block->halfLength3();
    m_cursor.m_index += length;
    m_cursor.m_index2 += totalLen2;
    m_cursor.m_index3 += totalLen3;

    // If we are at the end of a block and the block is not the last one, go to
    // the next block.
    if (m_cursor.m_indexInBlock == m_cursor.m_block->length() &&
        m_cursor.m_block->next())
    {
        m_cursor.m_block = m_cursor.m_block->next();
        m_cursor.m_pointer =
            m_cursor.m_block->begin() == m_cursor.m_block->gapBegin() ?
            m_cursor.m_block->gapEnd() : m_cursor.m_block->begin();
        m_cursor.m_indexInBlock = 0;
        m_cursor.m_index2InBlock = 0;
        m_cursor.m_index3InBlock = 0;
    }

    if (TERTIARY_INDEXING)
        m_cursor.addSkip3();

    m_length += length;
    m_length2 += totalLen2;
    m_length3 += totalLen3;
}

template<class Unit, class AtomTraits, int BLOCK_SIZE, int UNIFORM_ATOM_LENGTH,
         bool SECONDARY_INDEXING, bool TERTIARY_INDEXING>
void Buffer<Unit, AtomTraits, BLOCK_SIZE, UNIFORM_ATOM_LENGTH,
            SECONDARY_INDEXING, TERTIARY_INDEXING>::
    remove(int length, int length2, int length3)
{
    assert(length >= 0);
    assert(m_cursor.index() + length <= m_length);
    assert(length2 == -1 || length2 >= 0);
    assert(length3 == -1 || length3 >= 0);

    if (length == 0)
        return;

    if (TERTIARY_INDEXING)
        m_cursor.cancelSkip3();

    // A special case.  If all the atoms are removed, destroy all the block
    // except the one that the cursor points to and clear the block.
    if (length == m_length)
    {
        assert(m_cursor.index() == 0);
        assert(m_cursor.m_block == m_first);
        while (m_last != m_first)
        {
            Block *prev = m_last->previous();
            delete m_last;
            m_last = prev;
        }
        m_first->clear();
        m_first->m_next = NULL;
        m_first->m_previous = NULL;
        m_length = 0;
        m_length2 = 0;
        m_length3 = 0;
        m_cursor.m_pointer = m_first->gapEnd();
        return;
    }

    // The block where the atoms are removed.  When done, this block is the last
    // block where the atoms were removed, if some atoms following the removed
    // atoms were left, or otherwise NULL.
    Block *block = m_cursor.m_block;
    // If the atoms are removed until the end of a block, we should set the
    // cursor to the next block, and this is the next block, which may be NULL
    // if the next block doesn't exist.  Otherwise, it is NULL.
    Block *next = NULL;
    int leftLen = length;
    IntOrNil<SECONDARY_INDEXING> totalLen2;
    IntOrNil<TERTIARY_INDEXING> totalLen3;
    int lenInBlock;
    IntOrNil<SECONDARY_INDEXING> len2InBlock;
    IntOrNil<TERTIARY_INDEXING> len3InBlock;

    // If the gap is not at the cursor, move it.  Also remove the atoms in this
    // block.
    if (m_cursor.atom() < block->gapBegin())
    {
        if (length <= block->gapBegin() - m_cursor.atom())
        {
            lenInBlock = length;
            int len = block->gapBegin() - m_cursor.atom() - length;
            bool needCount = false;
            if (SECONDARY_INDEXING)
            {
                if (length2 == -1)
                {
                    if (UNIFORM_ATOM_LENGTH == -1)
                        needCount = true;
                    else
                        len2InBlock = length / UNIFORM_ATOM_LENGTH;
                }
                else
                    len2InBlock = length2;
            }
            if (TERTIARY_INDEXING)
            {
                if (length3 == -1)
                    needCount = true;
                else
                    len3InBlock = length3;
            }
            if (needCount)
                countLength(m_cursor.atom(), length,
                            len2InBlock, len3InBlock);
            memmove(block->gapEnd() - len,
                    m_cursor.atom() + length,
                    sizeof(Unit) * len);
            block->m_gapEnd -= len;
        }
        else if (length <= block->length() - m_cursor.m_indexInBlock)
        {
            lenInBlock = length;
            int len = length - (block->gapBegin() - m_cursor.atom());
            bool needCount = false;
            if (SECONDARY_INDEXING)
            {
                if (length2 == -1)
                {
                    if (UNIFORM_ATOM_LENGTH == -1)
                        needCount = true;
                    else
                        len2InBlock = length / UNIFORM_ATOM_LENGTH;
                }
                else
                    len2InBlock = length2;
            }
            if (TERTIARY_INDEXING)
            {
                if (length3 == -1)
                    needCount = true;
                else
                    len3InBlock = length3;
            }
            if (needCount)
            {
                countLength(block->gapEnd(), len, len2InBlock, len3InBlock);
                len2InBlock += block->halfLength2() -
                    m_cursor.m_index2InBlock;
                len3InBlock += block->halfLength3() -
                    m_cursor.m_index3InBlock;
            }
            block->m_gapEnd += len;
        }
        else
        {
            lenInBlock = block->length() - m_cursor.m_indexInBlock;
            len2InBlock = block->length2() - m_cursor.m_index2InBlock;
            len3InBlock = block->length3() - m_cursor.m_index3InBlock;
            block->m_gapEnd = block->end();
        }
        block->m_gapBegin = m_cursor.atom();
    }
    else
    {
        int len = m_cursor.atom() - block->gapEnd();
        assert(len >= 0);
        memmove(block->gapBegin(), block->gapEnd(), sizeof(Unit) * len);
        block->m_gapBegin += len;
        if (length <= block->length() - m_cursor.m_indexInBlock)
        {
            lenInBlock = length;
            bool needCount = false;
            if (SECONDARY_INDEXING)
            {
                if (length2 == -1)
                {
                    if (UNIFORM_ATOM_LENGTH == -1)
                            needCount = true;
                    else
                        len2InBlock = length / UNIFORM_ATOM_LENGTH;
                }
                else
                    len2InBlock = length2;
            }
            if (TERTIARY_INDEXING)
            {
                if (length3 == -1)
                    needCount = true;
                else
                    len3InBlock = length3;
            }
            if (needCount)
                countLength(m_cursor.atom(), length,
                            len2InBlock, len3InBlock);
            block->m_gapEnd = m_cursor.atom() + length;
        }
        else
        {
            lenInBlock = block->length() - m_cursor.m_indexInBlock;
            len2InBlock = block->length2() - m_cursor.m_index2InBlock;
            len3InBlock = block->length3() - m_cursor.m_index3InBlock;
            block->m_gapEnd = block->end();
        }
    }
    block->m_halfLength2 = m_cursor.m_index2InBlock;
    block->m_halfLength3 = m_cursor.m_index3InBlock;
    block->m_length -= lenInBlock;
    block->m_length2 -= len2InBlock;
    block->m_length3 -= len3InBlock;
    leftLen -= lenInBlock;
    totalLen2 += len2InBlock;
    totalLen3 += len3InBlock;

    // If all the atoms in this block are removed, destroy this block.
    if (block->length() == 0)
    {
        assert(block->length2() == 0);
        Block *nb = block->next();
        Block *pb = block->previous();
        if (nb)
            nb->m_previous = pb;
        else
            m_last = pb;
        if (pb)
            pb->m_next = nb;
        else
            m_first = nb;
        delete block;

        // If remove until the end of this block, the cursor will point to the
        // next block if existing.
        if (leftLen == 0)
        {
            block = NULL;
            next = nb;
        }
        else
            block = nb;
    }
    else
    {
        if (leftLen)
        {
            block = block->next();
        }
    }

    // Remove the atoms in the following blocks.
    if (leftLen)
    {
        for(;;)
        {
            assert(block);
            if (leftLen < block->length())
            {
                // Remove some atoms in this block.
                IntOrNil<SECONDARY_INDEXING> len2;
                IntOrNil<TERTIARY_INDEXING> len3;
                if (leftLen < block->halfLength())
                {
                    bool needCount = false;
                    if (SECONDARY_INDEXING)
                    {
                        if (length2 == -1)
                        {
                            if (UNIFORM_ATOM_LENGTH == -1)
                                needCount = true;
                            else
                                len2 = leftLen / UNIFORM_ATOM_LENGTH;
                        }
                        else
                            len2 = length2 - totalLen2;
                    }
                    if (TERTIARY_INDEXING)
                    {
                        if (length3 == -1)
                            needCount = true;
                        else
                            len3 = length3 - totalLen3;
                    }
                    if (needCount)
                        countLength(block->begin(), leftLen, len2, len3);
                    int len = block->halfLength() - leftLen;
                    memmove(block->gapEnd() - len,
                            block->gapBegin() - len,
                            sizeof(Unit) * len);
                    block->m_gapEnd -= len;
                }
                else
                {
                    int len = leftLen - block->halfLength();
                    bool needCount = false;
                    if (SECONDARY_INDEXING)
                    {
                        if (length2 == -1)
                        {
                            if (UNIFORM_ATOM_LENGTH == -1)
                                needCount = true;
                            else
                                len2 = leftLen / UNIFORM_ATOM_LENGTH;
                        }
                        else
                            len2 = length2 - totalLen2;
                    }
                    if (TERTIARY_INDEXING)
                    {
                        if (length3 == -1)
                            needCount = true;
                        else
                            len3 = length3 - totalLen3;
                    }
                    if (needCount)
                    {
                        countLength(block->gapEnd(), len, len2, len3);
                        len2 += block->halfLength2();
                        len3 += block->halfLength3();
                    }
                    block->m_gapEnd += len;
                }
                block->m_gapBegin = block->begin();
                block->m_halfLength2 = 0;
                block->m_halfLength3 = 0;
                block->m_length -= leftLen;
                block->m_length2 -= len2;
                block->m_length3 -= len3;
                leftLen = 0;
                totalLen2 += len2;
                totalLen3 += len3;
                break;
            }

            // Then, remove all the atoms in this block.
            leftLen -= block->length();
            totalLen2 += block->length2();
            totalLen3 += block->length3();

            // Delete this block.
            Block *nb = block->next();
            Block *pb = block->previous();
            if (nb)
                nb->m_previous = pb;
            else
                m_last = pb;
            if (pb)
                pb->m_next = nb;
            else
                m_first = nb;
            delete block;

            // If remove until the end of this block, the cursor will point to
            // the next block if existing.
            if (leftLen == 0)
            {
                block = NULL;
                next = nb;
                break;
            }
            block = nb;
        }
    }

    // Update the cursor.
    if (next)
    {
        m_cursor.m_block = next;
        if (next->begin() == next->gapBegin())
            m_cursor.m_pointer = next->gapEnd();
        else
            m_cursor.m_pointer = next->begin();
        m_cursor.m_indexInBlock = 0;
        m_cursor.m_index2InBlock = 0;
        m_cursor.m_index3InBlock = 0;
    }
    else if (block)
    {
        m_cursor.m_block = block;
        m_cursor.m_pointer = block->gapEnd();
        m_cursor.m_indexInBlock = block->halfLength();
        m_cursor.m_index2InBlock = block->halfLength2();
        m_cursor.m_index3InBlock = block->halfLength3();
    }
    else
    {
        assert(m_last->end() == m_last->gapEnd());
        m_cursor.m_block = m_last;
        m_cursor.m_pointer = m_last->end();
        m_cursor.m_indexInBlock = m_last->length();
        m_cursor.m_index2InBlock = m_last->length2();
        m_cursor.m_index3InBlock = m_last->length3();
    }
    if (TERTIARY_INDEXING)
        m_cursor.addSkip3();

    m_length -= length;
    m_length2 -= totalLen2;
    m_length3 -= totalLen3;
}

template<class Unit, class AtomTraits, int BLOCK_SIZE, int UNIFORM_ATOM_LENGTH,
         bool SECONDARY_INDEXING, bool TERTIARY_INDEXING>
void Buffer<Unit, AtomTraits, BLOCK_SIZE, UNIFORM_ATOM_LENGTH,
            SECONDARY_INDEXING, TERTIARY_INDEXING>::
    replace(const Unit *atoms, int length)
{
    assert(m_cursor.index() + length <= m_length);
    Iterator iter(m_cursor);
    Unit *begin;
    Unit *end;
    int len;
    for (;;)
    {
        iter.getAtomsBulk(begin, end);
        len = end - iter.atom();
        if (length <= len)
        {
            memcpy(iter.atom(), atoms, sizeof(Unit) * length);
            break;
        }
        memcpy(iter.atom(), atoms, sizeof(Unit) * len);
        atoms += len;
        length -= len;
        bool moved = iter.goToNextBulk();
        assert(moved);
    }
}

template<class Unit, class AtomTraits, int BLOCK_SIZE, int UNIFORM_ATOM_LENGTH,
         bool SECONDARY_INDEXING, bool TERTIARY_INDEXING>
Unit *Buffer<Unit, AtomTraits, BLOCK_SIZE, UNIFORM_ATOM_LENGTH,
            SECONDARY_INDEXING, TERTIARY_INDEXING>::
    makeGap(int &length)
{
    assert(length >= 0);
    if (length == 0)
        return m_cursor.m_pointer;

    if (TERTIARY_INDEXING)
        m_cursor.cancelSkip3();

    Block *block = m_cursor.m_block;

    // If the cursor is at the beginning of this block and the gap in the
    // previous block is at the end, look at the gap in the previous block.
    if (m_cursor.atom() == (block->begin() == block->gapBegin() ?
                            block->gapEnd() : block->begin()) &&
        block->previous() &&
        block->previous()->end() == block->previous()->gapEnd())
        block = block->previous();

    Block *next = block->next();

    // If the gap in this block is big enough, use it.
    if (block->gapSize() >= length)
    {
        if (m_cursor.m_block == block)
        {
            // Move the gap to the cursor.
            if (m_cursor.atom() < block->gapBegin())
            {
                int len = block->gapBegin() - m_cursor.atom();
                memmove(m_cursor.atom() + block->gapSize(),
                        m_cursor.atom(),
                        sizeof(Unit) * len);
                block->m_gapBegin -= len;
                block->m_gapEnd -= len;
                m_cursor.m_pointer = block->gapEnd();
            }
            else if (m_cursor.atom() > block->gapEnd())
            {
                int len = m_cursor.atom() - block->gapEnd();
                memmove(block->gapBegin(),
                        block->gapEnd(),
                        sizeof(Unit) * len);
                block->m_gapBegin += len;
                block->m_gapEnd += len;
            }
            block->m_halfLength2 = m_cursor.m_index2InBlock;
            block->m_halfLength3 = m_cursor.m_index3InBlock;
        }

        if (TERTIARY_INDEXING)
            m_cursor.addSkip3();
        length = block->gapSize();
        return block->gapBegin();
    }

    // Pop the content following the cursor out of this block.
    if (m_cursor.m_block == block &&
        m_cursor.m_indexInBlock < block->length())
    {
        // If the next block can hold the content, move them to the next block.
        // Otherwise, create a new block following this block to hold them.
        int len = block->length() - m_cursor.m_indexInBlock;
        IntOrNil<SECONDARY_INDEXING> len2 =
            block->length2() - m_cursor.m_index2InBlock;
        IntOrNil<TERTIARY_INDEXING> len3 =
            block->length3() - m_cursor.m_index3InBlock;
        if (next && next->gapSize() >= len)
        {
            if (next->begin() != next->gapBegin())
            {
                int l = next->gapBegin() - next->begin();
                memmove(next->gapEnd() - l, next->begin(), sizeof(Unit) * l);
                next->m_gapBegin -= l;
                next->m_gapEnd -= l;
                next->m_halfLength2 = 0;
                next->m_halfLength3 = 0;
            }
        }
        else
        {
            Block *newBlock = new Block;
            newBlock->m_previous = block;
            newBlock->m_next = next;
            block->m_next = newBlock;
            if (next)
                next->m_previous = newBlock;
            else
                m_last = newBlock;
            next = newBlock;
        }

        // Move the content following the cursor to the next block and move the
        // gap to the end of this block.
        if (m_cursor.atom() < block->gapBegin())
        {
            int sndHalfLen = block->end() - block->gapEnd();
            memcpy(next->gapEnd() - sndHalfLen,
                   block->gapEnd(),
                   sizeof(Unit) * sndHalfLen);
            next->m_gapEnd -= sndHalfLen;
            int cursorLen = block->gapBegin() - m_cursor.atom();
            memcpy(next->gapEnd() - cursorLen,
                   m_cursor.atom(),
                   sizeof(Unit) * cursorLen);
            next->m_gapEnd -= cursorLen;
            block->m_gapBegin -= cursorLen;
        }
        else
        {
            int cursorLen = block->end() - m_cursor.atom();
            memcpy(next->gapEnd() - cursorLen,
                   m_cursor.atom(),
                   sizeof(Unit) * cursorLen);
            next->m_gapEnd -= cursorLen;
            int sndHalfLen = m_cursor.atom() - block->gapEnd();
            memmove(block->gapBegin(),
                    block->gapEnd(),
                    sizeof(Unit) * sndHalfLen);
            block->m_gapBegin += sndHalfLen;
        }
        block->m_gapEnd = block->end();
        block->m_halfLength2 = m_cursor.m_index2InBlock;
        block->m_halfLength3 = m_cursor.m_index3InBlock;
        next->m_length += len;
        block->m_length -= len;
        next->m_length2 += len2;
        block->m_length2 -= len2;
        next->m_length3 += len3;
        block->m_length3 -= len3;

        // If the gap in this block is big enough or this block is empty, use
        // it.
        if (block->gapSize() >= length || block->length() == 0)
        {
            m_cursor.m_block = next;
            m_cursor.m_pointer = next->gapEnd();
            m_cursor.m_indexInBlock = 0;
            m_cursor.m_index2InBlock = 0;
            m_cursor.m_index3InBlock = 0;
            if (TERTIARY_INDEXING)
                m_cursor.addSkip3();
            length = block->gapSize();
            return block->gapBegin();
        }
    }

    // If the gap in the next block is big enough, use it.
    if (next && next->gapSize() >= length)
    {
        // If the gap in the next block is not at the beginning of the block,
        // move it.
        if (next->begin() != next->gapBegin())
        {
            int len = next->gapBegin() - next->begin();
            memmove(next->gapEnd() - len, next->begin(), sizeof(Unit) * len);
            next->m_gapBegin -= len;
            next->m_gapEnd -= len;
            next->m_halfLength2 = 0;
            next->m_halfLength3 = 0;
        }
        m_cursor.m_block = next;
        m_cursor.m_pointer = next->gapEnd();
        m_cursor.m_indexInBlock = 0;
        m_cursor.m_index2InBlock = 0;
        m_cursor.m_index3InBlock = 0;
        if (TERTIARY_INDEXING)
            m_cursor.addSkip3();
        length = next->gapSize();
        return next->gapBegin();
    }

    // Create a new block and use it.
    Block *newBlock = new Block;
    newBlock->m_previous = block;
    newBlock->m_next = next;
    block->m_next = newBlock;
    if (next)
    {
        next->m_previous = newBlock;
        m_cursor.m_block = next;
        m_cursor.m_pointer = (next->begin() == next->gapBegin() ?
                              next->gapEnd() : next->begin());
    }
    else
    {
        m_last = newBlock;
        m_cursor.m_block = newBlock;
        m_cursor.m_pointer = newBlock->gapEnd();
    }
    m_cursor.m_indexInBlock = 0;
    m_cursor.m_index2InBlock = 0;
    m_cursor.m_index3InBlock = 0;
    if (TERTIARY_INDEXING)
        m_cursor.addSkip3();
    length = newBlock->gapSize();
    return newBlock->gapBegin();
}

template<class Unit, class AtomTraits, int BLOCK_SIZE, int UNIFORM_ATOM_LENGTH,
         bool SECONDARY_INDEXING, bool TERTIARY_INDEXING>
void Buffer<Unit, AtomTraits, BLOCK_SIZE, UNIFORM_ATOM_LENGTH,
            SECONDARY_INDEXING, TERTIARY_INDEXING>::
    insertInGap(const Unit *atoms, int length, int length2, int length3)
{
    assert(length >= 0);
    assert(length2 == -1 || length2 >= 0);
    assert(length3 == -1 || length3 >= 0);

    if (length == 0)
        return;

    if (TERTIARY_INDEXING)
        m_cursor.cancelSkip3();

    IntOrNil<SECONDARY_INDEXING> len2;
    IntOrNil<TERTIARY_INDEXING> len3;
    bool needCount = false;
    if (SECONDARY_INDEXING)
    {
        if (length2 == -1)
        {
            if (UNIFORM_ATOM_LENGTH == -1)
                needCount = true;
            else
                len2 = length / UNIFORM_ATOM_LENGTH;
        }
        else
            len2 = length2;
    }
    if (TERTIARY_INDEXING)
    {
        if (length3 == -1)
            needCount = true;
        else
            len3 = length3;
    }
    if (needCount)
        countLength(atoms, length, len2, len3);

    Block *block = m_cursor.m_block;

    // Check to see if the atoms are in the gap of this block or the previous
    // block.
    if (atoms == block->gapBegin())
    {
        m_cursor.m_indexInBlock += length;
        m_cursor.m_index2InBlock += len2;
        m_cursor.m_index3InBlock += len3;
    }
    else
    {
        block = block->previous();
        assert(block);
        assert(atoms == block->gapBegin());
    }

    assert(block->gapSize() >= length);
    block->m_gapBegin += length;
    block->m_length += length;
    block->m_length2 += len2;
    block->m_length3 += len3;
    block->m_halfLength2 += len2;
    block->m_halfLength3 += len3;
    m_cursor.m_index += length;
    m_cursor.m_index2 += len2;
    m_cursor.m_index3 += len3;

    if (TERTIARY_INDEXING)
        m_cursor.addSkip3();
    m_length += length;
    m_length2 += len2;
    m_length3 += len3;
}

template<class Unit, class AtomTraits, int BLOCK_SIZE, int UNIFORM_ATOM_LENGTH,
         bool SECONDARY_INDEXING, bool TERTIARY_INDEXING>
void Buffer<Unit, AtomTraits, BLOCK_SIZE, UNIFORM_ATOM_LENGTH,
            SECONDARY_INDEXING, TERTIARY_INDEXING>::
    getAtoms(int index, int length, Unit *atoms) const
{
    assert(index + length <= m_length);
    ConstIterator iter(*this, index, -1, -1);
    const Unit *begin;
    const Unit *end;
    int len;
    for (;;)
    {
        iter.getAtomsBulk(begin, end);
        len = end - iter.atom();
        if (length <= len)
        {
            memcpy(atoms, iter.atom(), sizeof(Unit) * length);
            break;
        }
        memcpy(atoms, iter.atom(), sizeof(Unit) * len);
        atoms += len;
        length -= len;
        bool moved = iter.goToNextBulk();
        assert(moved);
    }
}

template<class Unit, class AtomTraits, int BLOCK_SIZE, int UNIFORM_ATOM_LENGTH,
         bool SECONDARY_INDEXING, bool TERTIARY_INDEXING>
void Buffer<Unit, AtomTraits, BLOCK_SIZE, UNIFORM_ATOM_LENGTH,
            SECONDARY_INDEXING, TERTIARY_INDEXING>::
    countLength(const Unit *atoms, int length,
                IntOrNil<SECONDARY_INDEXING> &length2,
                IntOrNil<TERTIARY_INDEXING> &length3) const
{
    length2 = 0;
    length3 = 0;
    const Unit *end = atoms + length;
    for (; atoms < end; atoms += AtomTraits::length(atoms))
    {
        ++length2;
        length3 += AtomTraits::skip3(atoms) +
            AtomTraits::length3(atoms);
    }
}

template<class Unit, class AtomTraits, int BLOCK_SIZE, int UNIFORM_ATOM_LENGTH,
         bool SECONDARY_INDEXING, bool TERTIARY_INDEXING>
void Buffer<Unit, AtomTraits, BLOCK_SIZE, UNIFORM_ATOM_LENGTH,
             SECONDARY_INDEXING, TERTIARY_INDEXING>::
    appendInBlock(Block *block, const Unit *atoms, int length,
                  int &appendedLength,
                  IntOrNil<SECONDARY_INDEXING> &appendedLength2,
                  IntOrNil<TERTIARY_INDEXING> &appendedLength3)
{
    assert(block->end() == block->gapEnd());

    appendedLength = std::min(block->end() - block->gapBegin(), length);

    if (UNIFORM_ATOM_LENGTH != -1 && !TERTIARY_INDEXING)
    {
        appendedLength -= appendedLength % UNIFORM_ATOM_LENGTH;
        appendedLength2 = appendedLength / UNIFORM_ATOM_LENGTH;
    }
    else
    {
        const Unit *p = atoms;
        const Unit *end = atoms + appendedLength;
        appendedLength2 = 0;
        appendedLength3 = 0;
        for (; p < end; p += AtomTraits::length(p))
        {
            if (p + AtomTraits::length(p) > end)
                break;
            ++appendedLength2;
            appendedLength3 += AtomTraits::skip3(p) +
                AtomTraits::length3(p);
        }
        appendedLength = p - atoms;
    }

    memcpy(block->gapBegin(), atoms, sizeof(Unit) * appendedLength);
    block->m_gapBegin += appendedLength;
    block->m_length += appendedLength;
    block->m_halfLength2 += appendedLength2;
    block->m_length2 += appendedLength2;
    block->m_halfLength3 += appendedLength3;
    block->m_length3 += appendedLength3;
}

}

#endif
