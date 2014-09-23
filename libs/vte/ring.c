/*
 * Copyright (C) 2002,2009,2010 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Red Hat Author(s): Nalin Dahyabhai, Behdad Esfahbod
 */

#include <config.h>

#include "debug.h"
#include "ring.h"

#include <string.h>

/*
 * VteRing: A buffer ring
 */

#ifdef VTE_DEBUG
static void
_vte_ring_validate (VteRing * ring)
{
	g_assert(ring != NULL);
	_vte_debug_print(VTE_DEBUG_RING,
			" Delta = %lu, Length = %lu, Next = %lu, Max = %lu, Writable = %lu.\n",
			ring->start, ring->end - ring->start, ring->end,
			ring->max, ring->end - ring->writable);

	g_assert (ring->start <= ring->writable);
	g_assert (ring->writable <= ring->end);

	g_assert (ring->end - ring->start <= ring->max);
	g_assert (ring->end - ring->writable <= ring->mask);
}
#else
#define _vte_ring_validate(ring) G_STMT_START {} G_STMT_END
#endif


void
_vte_ring_init (VteRing *ring, gulong max_rows)
{
	_vte_debug_print(VTE_DEBUG_RING, "New ring %p.\n", ring);

	memset (ring, 0, sizeof (*ring));

	ring->max = MAX (max_rows, 3);

	ring->mask = 31;
	ring->array = g_malloc0 (sizeof (ring->array[0]) * (ring->mask + 1));

	ring->attr_stream = _vte_file_stream_new ();
	ring->text_stream = _vte_file_stream_new ();
	ring->row_stream = _vte_file_stream_new ();

	ring->last_attr_text_start_offset = 0;
	ring->last_attr.i = basic_cell.i.attr;
	ring->utf8_buffer = g_string_sized_new (128);

	_vte_row_data_init (&ring->cached_row);
	ring->cached_row_num = (gulong) -1;

	ring->visible_rows_hint = 0;

	_vte_ring_validate(ring);
}

void
_vte_ring_fini (VteRing *ring)
{
	gulong i;

	for (i = 0; i <= ring->mask; i++)
		_vte_row_data_fini (&ring->array[i]);

	g_free (ring->array);

	g_object_unref (ring->attr_stream);
	g_object_unref (ring->text_stream);
	g_object_unref (ring->row_stream);

	g_string_free (ring->utf8_buffer, TRUE);

	_vte_row_data_fini (&ring->cached_row);
}

typedef struct _VteRowRecord {
	gsize text_start_offset;  /* offset where text of this row begins */
	gsize attr_start_offset;  /* offset of the first character's attributes */
	int soft_wrapped: 1;      /* end of line is not '\n' */
	int is_ascii: 1;          /* for rewrapping speedup: guarantees that line contains 32..126 bytes only. Can be 0 even when ascii only. */
} VteRowRecord;

/* Represents a cell position, see ../doc/rewrap.txt */
typedef struct _VteCellTextOffset {
	gsize text_offset;    /* byte offset in text_stream (or perhaps beyond) */
	gint fragment_cells;  /* extra number of cells to walk within a multicell character */
	gint eol_cells;       /* -1 if over a character, >=0 if at EOL or beyond */
} VteCellTextOffset;

static gboolean
_vte_ring_read_row_record (VteRing *ring, VteRowRecord *record, gulong position)
{
	return _vte_stream_read (ring->row_stream, position * sizeof (*record), (char *) record, sizeof (*record));
}

static void
_vte_ring_append_row_record (VteRing *ring, const VteRowRecord *record, gulong position)
{
	_vte_stream_append (ring->row_stream, (const char *) record, sizeof (*record));
}

static void
_vte_ring_freeze_row (VteRing *ring, gulong position, const VteRowData *row)
{
	VteRowRecord record;
	VteCell *cell;
	GString *buffer = ring->utf8_buffer;
	int i;

	_vte_debug_print (VTE_DEBUG_RING, "Freezing row %lu.\n", position);

	memset(&record, 0, sizeof (record));
	record.text_start_offset = _vte_stream_head (ring->text_stream);
	record.attr_start_offset = _vte_stream_head (ring->attr_stream);
	record.is_ascii = 1;

	g_string_set_size (buffer, 0);
	for (i = 0, cell = row->cells; i < row->len; i++, cell++) {
		VteIntCellAttr attr;
		int num_chars;

		/* Attr storage:
		 *
		 * 1. We don't store attrs for fragments.  They can be
		 * reconstructed using the columns of their start cell.
		 *
		 * 2. We store one attr per vteunistr character starting
		 * from the second character, with columns=0.
		 *
		 * That's enough to reconstruct the attrs, and to store
		 * the text in real UTF-8.
		 */
		attr.s = cell->attr;
		if (G_LIKELY (!attr.s.fragment)) {
			VteCellAttrChange attr_change;

			if (ring->last_attr.i != attr.i) {
				ring->last_attr_text_start_offset = record.text_start_offset + buffer->len;
				memset(&attr_change, 0, sizeof (attr_change));
				attr_change.text_end_offset = ring->last_attr_text_start_offset;
				attr_change.attr = ring->last_attr;
				_vte_stream_append (ring->attr_stream, (const char *) &attr_change, sizeof (attr_change));
				if (!buffer->len)
					/* This row doesn't use last_attr, adjust */
					record.attr_start_offset += sizeof (attr_change);
				ring->last_attr = attr;
			}

			num_chars = _vte_unistr_strlen (cell->c);
			if (num_chars > 1) {
				attr.s.columns = 0;
				ring->last_attr_text_start_offset = record.text_start_offset + buffer->len
								  + g_unichar_to_utf8 (_vte_unistr_get_base (cell->c), NULL);
				memset(&attr_change, 0, sizeof (attr_change));
				attr_change.text_end_offset = ring->last_attr_text_start_offset;
				attr_change.attr = ring->last_attr;
				_vte_stream_append (ring->attr_stream, (const char *) &attr_change, sizeof (attr_change));
				ring->last_attr = attr;
			}

			if (cell->c < 32 || cell->c > 126) record.is_ascii = 0;
			_vte_unistr_append_to_string (cell->c, buffer);
		}
	}
	if (!row->attr.soft_wrapped)
		g_string_append_c (buffer, '\n');
	record.soft_wrapped = row->attr.soft_wrapped;

	_vte_stream_append (ring->text_stream, buffer->str, buffer->len);
	_vte_ring_append_row_record (ring, &record, position);
}

static void
_vte_ring_thaw_row (VteRing *ring, gulong position, VteRowData *row, gboolean do_truncate)
{
	VteRowRecord records[2], record;
	VteIntCellAttr attr;
	VteCellAttrChange attr_change;
	VteCell cell;
	const char *p, *q, *end;
	GString *buffer = ring->utf8_buffer;

	_vte_debug_print (VTE_DEBUG_RING, "Thawing row %lu.\n", position);

	_vte_row_data_clear (row);

	attr_change.text_end_offset = 0;

	if (!_vte_ring_read_row_record (ring, &records[0], position))
		return;
	if ((position + 1) * sizeof (records[0]) < _vte_stream_head (ring->row_stream)) {
		if (!_vte_ring_read_row_record (ring, &records[1], position + 1))
			return;
	} else
		records[1].text_start_offset = _vte_stream_head (ring->text_stream);

	g_string_set_size (buffer, records[1].text_start_offset - records[0].text_start_offset);
	if (!_vte_stream_read (ring->text_stream, records[0].text_start_offset, buffer->str, buffer->len))
		return;

	record = records[0];

	if (G_LIKELY (buffer->len && buffer->str[buffer->len - 1] == '\n'))
		buffer->len--;
	else
		row->attr.soft_wrapped = TRUE;

	p = buffer->str;
	end = p + buffer->len;
	while (p < end) {

		if (record.text_start_offset >= ring->last_attr_text_start_offset) {
			attr = ring->last_attr;
		} else {
			if (record.text_start_offset >= attr_change.text_end_offset) {
				if (!_vte_stream_read (ring->attr_stream, record.attr_start_offset, (char *) &attr_change, sizeof (attr_change)))
					return;
				record.attr_start_offset += sizeof (attr_change);
			}
			attr = attr_change.attr;
		}

		cell.attr = attr.s;
		cell.c = g_utf8_get_char (p);

		q = g_utf8_next_char (p);
		record.text_start_offset += q - p;
		p = q;

		if (G_UNLIKELY (cell.attr.columns == 0)) {
			if (G_LIKELY (row->len)) {
				/* Combine it */
				row->cells[row->len - 1].c = _vte_unistr_append_unichar (row->cells[row->len - 1].c, cell.c);
			} else {
				cell.attr.columns = 1;
				_vte_row_data_append (row, &cell);
			}
		} else {
			_vte_row_data_append (row, &cell);
			if (cell.attr.columns > 1) {
				/* Add the fragments */
				int i, columns = cell.attr.columns;
				cell.attr.fragment = 1;
				cell.attr.columns = 1;
				for (i = 1; i < columns; i++)
					_vte_row_data_append (row, &cell);
			}
		}
	}

	if (do_truncate) {
		gsize attr_stream_truncate_at = records[0].attr_start_offset;
		_vte_debug_print (VTE_DEBUG_RING, "Truncating\n");
		if (records[0].text_start_offset <= ring->last_attr_text_start_offset) {
			/* Check the previous attr record. If its text ends where truncating, this attr record also needs to be removed. */
			if (_vte_stream_read (ring->attr_stream, attr_stream_truncate_at - sizeof (attr_change), (char *) &attr_change, sizeof (attr_change))) {
				if (records[0].text_start_offset == attr_change.text_end_offset) {
					_vte_debug_print (VTE_DEBUG_RING, "... at attribute change\n");
					attr_stream_truncate_at -= sizeof (attr_change);
				}
			}
			/* Reconstruct last_attr from the first record of attr_stream that we cut off,
			   last_attr_text_start_offset from the last record that we keep. */
			if (_vte_stream_read (ring->attr_stream, attr_stream_truncate_at, (char *) &attr_change, sizeof (attr_change))) {
				ring->last_attr = attr_change.attr;
				if (_vte_stream_read (ring->attr_stream, attr_stream_truncate_at - sizeof (attr_change), (char *) &attr_change, sizeof (attr_change))) {
					ring->last_attr_text_start_offset = attr_change.text_end_offset;
				} else {
					ring->last_attr_text_start_offset = 0;
				}
			} else {
				ring->last_attr_text_start_offset = 0;
				ring->last_attr.i = basic_cell.i.attr;
			}
		}
		_vte_stream_truncate (ring->row_stream, position * sizeof (record));
		_vte_stream_truncate (ring->attr_stream, attr_stream_truncate_at);
		_vte_stream_truncate (ring->text_stream, records[0].text_start_offset);
	}
}

static void
_vte_ring_reset_streams (VteRing *ring, gulong position)
{
	_vte_debug_print (VTE_DEBUG_RING, "Reseting streams to %lu.\n", position);

	_vte_stream_reset (ring->row_stream, position * sizeof (VteRowRecord));
	_vte_stream_reset (ring->text_stream, 0);
	_vte_stream_reset (ring->attr_stream, 0);

	ring->last_attr_text_start_offset = 0;
	ring->last_attr.i = basic_cell.i.attr;
}

static inline VteRowData *
_vte_ring_writable_index (VteRing *ring, gulong position)
{
	return &ring->array[position & ring->mask];
}

const VteRowData *
_vte_ring_index (VteRing *ring, gulong position)
{
	if (G_LIKELY (position >= ring->writable))
		return _vte_ring_writable_index (ring, position);

	if (ring->cached_row_num != position) {
		_vte_debug_print(VTE_DEBUG_RING, "Caching row %lu.\n", position);
		_vte_ring_thaw_row (ring, position, &ring->cached_row, FALSE);
		ring->cached_row_num = position;
	}

	return &ring->cached_row;
}

static void _vte_ring_ensure_writable (VteRing *ring, gulong position);
static void _vte_ring_ensure_writable_room (VteRing *ring);

VteRowData *
_vte_ring_index_writable (VteRing *ring, gulong position)
{
	_vte_ring_ensure_writable (ring, position);
	return _vte_ring_writable_index (ring, position);
}

static void
_vte_ring_freeze_one_row (VteRing *ring)
{
	VteRowData *row;

	if (G_UNLIKELY (ring->writable == ring->start))
		_vte_ring_reset_streams (ring, ring->writable);

	row = _vte_ring_writable_index (ring, ring->writable);
	_vte_ring_freeze_row (ring, ring->writable, row);

	ring->writable++;
}

static void
_vte_ring_thaw_one_row (VteRing *ring)
{
	VteRowData *row;

	g_assert (ring->start < ring->writable);

	_vte_ring_ensure_writable_room (ring);

	ring->writable--;

	if (ring->writable == ring->cached_row_num)
		ring->cached_row_num = (gulong) -1; /* Invalidate cached row */

	row = _vte_ring_writable_index (ring, ring->writable);

	_vte_ring_thaw_row (ring, ring->writable, row, TRUE);
}

static void
_vte_ring_discard_one_row (VteRing *ring)
{
	ring->start++;
	if (G_UNLIKELY (ring->start == ring->writable)) {
		_vte_ring_reset_streams (ring, ring->writable);
	} else {
		VteRowRecord record;
		_vte_stream_advance_tail (ring->row_stream, ring->start * sizeof (record));
		if (G_LIKELY (_vte_ring_read_row_record (ring, &record, ring->start))) {
			_vte_stream_advance_tail (ring->text_stream, record.text_start_offset);
			_vte_stream_advance_tail (ring->attr_stream, record.attr_start_offset);
		}
	}
	if (ring->start > ring->writable)
		ring->writable = ring->start;
}

static void
_vte_ring_maybe_freeze_one_row (VteRing *ring)
{
	if (G_LIKELY (ring->mask >= ring->visible_rows_hint && ring->writable + ring->mask + 1 == ring->end))
		_vte_ring_freeze_one_row (ring);
	else
		_vte_ring_ensure_writable_room (ring);
}

static void
_vte_ring_maybe_discard_one_row (VteRing *ring)
{
	if ((gulong) _vte_ring_length (ring) == ring->max)
		_vte_ring_discard_one_row (ring);
}

static void
_vte_ring_ensure_writable_room (VteRing *ring)
{
	gulong new_mask, old_mask, i, end;
	VteRowData *old_array, *new_array;;

	if (G_LIKELY (ring->mask >= ring->visible_rows_hint && ring->writable + ring->mask + 1 > ring->end))
		return;

	old_mask = ring->mask;
	old_array = ring->array;

	do {
		ring->mask = (ring->mask << 1) + 1;
	} while (ring->mask < ring->visible_rows_hint || ring->writable + ring->mask + 1 <= ring->end);

	_vte_debug_print(VTE_DEBUG_RING, "Enlarging writable array from %lu to %lu\n", old_mask, ring->mask);

	ring->array = g_malloc0 (sizeof (ring->array[0]) * (ring->mask + 1));

	new_mask = ring->mask;
	new_array = ring->array;

	end = ring->writable + old_mask + 1;
	for (i = ring->writable; i < end; i++)
		new_array[i & new_mask] = old_array[i & old_mask];

	g_free (old_array);
}

static void
_vte_ring_ensure_writable (VteRing *ring, gulong position)
{
	if (G_LIKELY (position >= ring->writable))
		return;

	_vte_debug_print(VTE_DEBUG_RING, "Ensure writable %lu.\n", position);

	while (position < ring->writable)
		_vte_ring_thaw_one_row (ring);
}

/**
 * _vte_ring_resize:
 * @ring: a #VteRing
 * @max_rows: new maximum numbers of rows in the ring
 *
 * Changes the number of lines the ring can contain.
 */
void
_vte_ring_resize (VteRing *ring, gulong max_rows)
{
	_vte_debug_print(VTE_DEBUG_RING, "Resizing to %lu.\n", max_rows);
	_vte_ring_validate(ring);

	/* Adjust the start of tail chunk now */
	if ((gulong) _vte_ring_length (ring) > max_rows) {
		ring->start = ring->end - max_rows;
		if (ring->start >= ring->writable) {
			_vte_ring_reset_streams (ring, ring->writable);
			ring->writable = ring->start;
		}
	}

	ring->max = max_rows;
}

void
_vte_ring_shrink (VteRing *ring, gulong max_len)
{
	if ((gulong) _vte_ring_length (ring) <= max_len)
		return;

	_vte_debug_print(VTE_DEBUG_RING, "Shrinking to %lu.\n", max_len);
	_vte_ring_validate(ring);

	if (ring->writable - ring->start <= max_len)
		ring->end = ring->start + max_len;
	else {
		while (ring->writable - ring->start > max_len) {
			_vte_ring_ensure_writable (ring, ring->writable - 1);
			ring->end = ring->writable;
		}
	}

	/* TODO May want to shrink down ring->array */

	_vte_ring_validate(ring);
}

/**
 * _vte_ring_insert_internal:
 * @ring: a #VteRing
 * @position: an index
 *
 * Inserts a new, empty, row into @ring at the @position'th offset.
 * The item at that position and any items after that are shifted down.
 *
 * Return: the newly added row.
 */
VteRowData *
_vte_ring_insert (VteRing *ring, gulong position)
{
	gulong i;
	VteRowData *row, tmp;

	_vte_debug_print(VTE_DEBUG_RING, "Inserting at position %lu.\n", position);
	_vte_ring_validate(ring);

	_vte_ring_maybe_discard_one_row (ring);

	_vte_ring_ensure_writable (ring, position);
	_vte_ring_ensure_writable_room (ring);

	g_assert (position >= ring->writable && position <= ring->end);

	tmp = *_vte_ring_writable_index (ring, ring->end);
	for (i = ring->end; i > position; i--)
		*_vte_ring_writable_index (ring, i) = *_vte_ring_writable_index (ring, i - 1);
	*_vte_ring_writable_index (ring, position) = tmp;

	row = _vte_ring_writable_index (ring, position);
	_vte_row_data_clear (row);
	ring->end++;

	_vte_ring_maybe_freeze_one_row (ring);

	_vte_ring_validate(ring);
	return row;
}

/**
 * _vte_ring_remove:
 * @ring: a #VteRing
 * @position: an index
 *
 * Removes the @position'th item from @ring.
 */
void
_vte_ring_remove (VteRing * ring, gulong position)
{
	gulong i;
	VteRowData tmp;

	_vte_debug_print(VTE_DEBUG_RING, "Removing item at position %lu.\n", position);
	_vte_ring_validate(ring);

	if (G_UNLIKELY (!_vte_ring_contains (ring, position)))
		return;

	_vte_ring_ensure_writable (ring, position);

	tmp = *_vte_ring_writable_index (ring, position);
	for (i = position; i < ring->end - 1; i++)
		*_vte_ring_writable_index (ring, i) = *_vte_ring_writable_index (ring, i + 1);
	*_vte_ring_writable_index (ring, ring->end - 1) = tmp;

	if (ring->end > ring->writable)
		ring->end--;

	_vte_ring_validate(ring);
}


/**
 * _vte_ring_append:
 * @ring: a #VteRing
 * @data: the new item
 *
 * Appends a new item to the ring.
 *
 * Return: the newly added row.
 */
VteRowData *
_vte_ring_append (VteRing * ring)
{
	return _vte_ring_insert (ring, _vte_ring_next (ring));
}


/**
 * _vte_ring_set_visible_rows_hint:
 * @ring: a #VteRing
 *
 * Set the number of visible rows, a hint only for better performance.
 */
void
_vte_ring_set_visible_rows_hint (VteRing *ring, gulong rows)
{
	ring->visible_rows_hint = rows;
}


/* Convert a (row,col) into a VteCellTextOffset.
 * Requires the row to be frozen, or be outsize the range covered by the ring.
 */
static gboolean
_vte_frozen_row_column_to_text_offset (VteRing *ring,
				       gulong position,
				       gulong column,
				       VteCellTextOffset *offset)
{
	VteRowRecord records[2];
	VteCell *cell;
	GString *buffer = ring->utf8_buffer;
	const VteRowData *row;
	unsigned int i, num_chars, off;

	if (position >= ring->end) {
		offset->text_offset = _vte_stream_head (ring->text_stream) + position - ring->end;
		offset->fragment_cells = 0;
		offset->eol_cells = column;
		return TRUE;
	}

	if (G_UNLIKELY (position < ring->start)) {
		/* This happens when the marker (saved cursor position) is
		   scrolled off at the top of the scrollback buffer. */
		position = ring->start;
		column = 0;
		/* go on */
	}

	g_assert(position < ring->writable);
	if (!_vte_ring_read_row_record (ring, &records[0], position))
		return FALSE;
	if ((position + 1) * sizeof (records[0]) < _vte_stream_head (ring->row_stream)) {
		if (!_vte_ring_read_row_record (ring, &records[1], position + 1))
			return FALSE;
	} else
		records[1].text_start_offset = _vte_stream_head (ring->text_stream);

	g_string_set_size (buffer, records[1].text_start_offset - records[0].text_start_offset);
	if (!_vte_stream_read (ring->text_stream, records[0].text_start_offset, buffer->str, buffer->len))
		return FALSE;

	if (G_LIKELY (buffer->len && buffer->str[buffer->len - 1] == '\n'))
		buffer->len--;

	row = _vte_ring_index(ring, position);

	/* row and buffer now contain the same text, in different representation */

	/* count the number of characters up to the given column */
	offset->fragment_cells = 0;
	offset->eol_cells = -1;
	num_chars = 0;
	for (i = 0, cell = row->cells; i < row->len && i < column; i++, cell++) {
		if (G_LIKELY (!cell->attr.fragment)) {
			if (G_UNLIKELY (i + cell->attr.columns > column)) {
				offset->fragment_cells = column - i;
				break;
			}
			num_chars += _vte_unistr_strlen(cell->c);
		}
	}
	if (i >= row->len) {
		offset->eol_cells = column - i;
	}

	/* count the number of UTF-8 bytes for the given number of characters */
	off = 0;
	while (num_chars > 0 && off < buffer->len) {
		off++;
		if ((buffer->str[off] & 0xC0) != 0x80) num_chars--;
	}
	offset->text_offset = records[0].text_start_offset + off;
	return TRUE;
}


/* Given a row number and a VteCellTextOffset, compute the column within that row.
   It's the caller's responsibility to ensure that VteCellTextOffset really falls into that row.
   Requires the row to be frozen, or be outsize the range covered by the ring.
 */
static gboolean
_vte_frozen_row_text_offset_to_column (VteRing *ring,
				       gulong position,
				       const VteCellTextOffset *offset,
				       long *column)
{
	VteRowRecord records[2];
	VteCell *cell;
	GString *buffer = ring->utf8_buffer;
	const VteRowData *row;
	unsigned int i, off, num_chars, nc;

	if (position >= ring->end) {
		*column = offset->eol_cells;
		return TRUE;
	}

	if (G_UNLIKELY (position < ring->start)) {
		/* This happens when the marker (saved cursor position) is
		   scrolled off at the top of the scrollback buffer. */
		*column = 0;
		return TRUE;
	}

	g_assert(position < ring->writable);
	if (!_vte_ring_read_row_record (ring, &records[0], position))
		return FALSE;
	if ((position + 1) * sizeof (records[0]) < _vte_stream_head (ring->row_stream)) {
		if (!_vte_ring_read_row_record (ring, &records[1], position + 1))
			return FALSE;
	} else
		records[1].text_start_offset = _vte_stream_head (ring->text_stream);

	g_assert(offset->text_offset >= records[0].text_start_offset && offset->text_offset < records[1].text_start_offset);

	g_string_set_size (buffer, records[1].text_start_offset - records[0].text_start_offset);
	if (!_vte_stream_read (ring->text_stream, records[0].text_start_offset, buffer->str, buffer->len))
		return FALSE;

	if (G_LIKELY (buffer->len && buffer->str[buffer->len - 1] == '\n'))
		buffer->len--;

	row = _vte_ring_index(ring, position);

	/* row and buffer now contain the same text, in different representation */

	/* count the number of characters for the given UTF-8 text offset */
	off = offset->text_offset - records[0].text_start_offset;
	num_chars = 0;
	for (i = 0; i < off && i < buffer->len; i++) {
		if ((buffer->str[i] & 0xC0) != 0x80) num_chars++;
	}

	/* count the number of columns for the given number of characters */
	for (i = 0, cell = row->cells; i < row->len; i++, cell++) {
		if (G_LIKELY (!cell->attr.fragment)) {
			if (num_chars == 0) break;
			nc = _vte_unistr_strlen(cell->c);
			if (nc > num_chars) break;
			num_chars -= nc;
		}
	}

	/* always add fragment_cells, but add eol_cells only if we're at eol */
	i += offset->fragment_cells;
	if (G_UNLIKELY (offset->eol_cells >= 0 && i == row->len))
		i += offset->eol_cells;
	*column = i;
	return TRUE;
}


/**
 * _vte_ring_rewrap:
 * @ring: a #VteRing
 * @columns: new number of columns
 * @markers: NULL-terminated array of #VteVisualPosition
 *
 * Reflow the @ring to match the new number of @columns.
 * For all @markers, find the cell at that position and update them to
 * reflect the cell's new position.
 */
/* See ../doc/rewrap.txt for design and implementation details. */
void
_vte_ring_rewrap (VteRing *ring,
		  glong columns,
		  VteVisualPosition **markers)
{
	gulong old_row_index, new_row_index;
	int i;
	int num_markers = 0;
	VteCellTextOffset *marker_text_offsets;
	VteVisualPosition *new_markers;
	VteRowRecord old_record;
	VteCellAttrChange attr_change;
	VteStream *new_row_stream;
	gsize paragraph_start_text_offset;
	gsize paragraph_end_text_offset;
	gsize paragraph_len;  /* excluding trailing '\n' */
	gsize attr_offset;
	gsize old_ring_end;

	if (_vte_ring_length(ring) == 0)
		return;
	_vte_debug_print(VTE_DEBUG_RING, "Ring before rewrapping:\n");
	_vte_ring_validate(ring);
	new_row_stream = _vte_file_stream_new ();

	/* Freeze everything, because rewrapping is really complicated and we don't want to
	   duplicate the code for frozen and thawed rows. */
	while (ring->writable < ring->end)
		_vte_ring_freeze_one_row(ring);

	/* For markers given as (row,col) pairs find their offsets in the text stream.
	   This code requires that the rows are already frozen. */
	while (markers[num_markers] != NULL)
		num_markers++;
	marker_text_offsets = g_malloc(num_markers * sizeof (marker_text_offsets[0]));
	new_markers = g_malloc(num_markers * sizeof (new_markers[0]));
	for (i = 0; i < num_markers; i++) {
		/* Convert visual column into byte offset */
		if (!_vte_frozen_row_column_to_text_offset(ring, markers[i]->row, markers[i]->col, &marker_text_offsets[i]))
			goto err;
		new_markers[i].row = new_markers[i].col = -1;
		_vte_debug_print(VTE_DEBUG_RING,
				"Marker #%d old coords:  row %ld  col %ld  ->  text_offset %" G_GSIZE_FORMAT " fragment_cells %d  eol_cells %d\n",
				i, markers[i]->row, markers[i]->col, marker_text_offsets[i].text_offset,
				marker_text_offsets[i].fragment_cells, marker_text_offsets[i].eol_cells);
	}

	/* Prepare for rewrapping */
	if (!_vte_ring_read_row_record(ring, &old_record, ring->start))
		goto err;
	paragraph_start_text_offset = old_record.text_start_offset;
	paragraph_end_text_offset = _vte_stream_head (ring->text_stream);  /* initialized to silence gcc */
	new_row_index = 0;

	attr_offset = old_record.attr_start_offset;
	if (!_vte_stream_read(ring->attr_stream, attr_offset, (char *) &attr_change, sizeof (attr_change))) {
		attr_change.attr = ring->last_attr;
		attr_change.text_end_offset = _vte_stream_head (ring->text_stream);
	}

	old_row_index = ring->start + 1;
	while (paragraph_start_text_offset < _vte_stream_head (ring->text_stream)) {
		/* Find the boundaries of the next paragraph */
		gboolean prev_record_was_soft_wrapped = FALSE;
		gboolean paragraph_is_ascii = TRUE;
		gsize paragraph_start_row = old_row_index - 1;
		gsize paragraph_end_row;  /* points to beyond the end */
		gsize text_offset = paragraph_start_text_offset;
		VteRowRecord new_record;
		glong col = 0;
		_vte_debug_print(VTE_DEBUG_RING,
				"  Old paragraph:  row %" G_GSIZE_FORMAT "  (text_offset %" G_GSIZE_FORMAT ")  up to (exclusive)  ",  /* no '\n' */
				paragraph_start_row, paragraph_start_text_offset);
		while (old_row_index <= ring->end) {
			prev_record_was_soft_wrapped = old_record.soft_wrapped;
			paragraph_is_ascii = paragraph_is_ascii && old_record.is_ascii;
			if (G_LIKELY (old_row_index < ring->end)) {
				if (!_vte_ring_read_row_record(ring, &old_record, old_row_index))
					goto err;
				paragraph_end_text_offset = old_record.text_start_offset;
			} else {
				paragraph_end_text_offset = _vte_stream_head (ring->text_stream);
			}
			old_row_index++;
			if (!prev_record_was_soft_wrapped)
				break;
		}
		paragraph_end_row = old_row_index - 1;
		paragraph_len = paragraph_end_text_offset - paragraph_start_text_offset;
		if (!prev_record_was_soft_wrapped)  /* The last paragraph can be soft wrapped! */
			paragraph_len--;  /* Strip trailing '\n' */
		_vte_debug_print(VTE_DEBUG_RING,
				"row %" G_GSIZE_FORMAT "  (text_offset %" G_GSIZE_FORMAT ")%s  len %" G_GSIZE_FORMAT "  is_ascii %d\n",
				paragraph_end_row, paragraph_end_text_offset,
				prev_record_was_soft_wrapped ? "  soft_wrapped" : "",
				paragraph_len, paragraph_is_ascii);

		/* Wrap the paragraph */
		if (attr_change.text_end_offset <= text_offset) {
			/* Attr change at paragraph boundary, advance to next attr. */
			attr_offset += sizeof (attr_change);
			if (!_vte_stream_read(ring->attr_stream, attr_offset, (char *) &attr_change, sizeof (attr_change))) {
				attr_change.attr = ring->last_attr;
				attr_change.text_end_offset = _vte_stream_head (ring->text_stream);
			}
		}
		memset(&new_record, 0, sizeof (new_record));
		new_record.text_start_offset = text_offset;
		new_record.attr_start_offset = attr_offset;
		new_record.is_ascii = paragraph_is_ascii;

		while (paragraph_len > 0) {
			/* Wrap one continuous run of identical attributes within the paragraph. */
			gsize runlength;  /* number of bytes we process in one run: identical attributes, within paragraph */
			if (attr_change.text_end_offset <= text_offset) {
				/* Attr change at line boundary, advance to next attr. */
				attr_offset += sizeof (attr_change);
				if (!_vte_stream_read(ring->attr_stream, attr_offset, (char *) &attr_change, sizeof (attr_change))) {
					attr_change.attr = ring->last_attr;
					attr_change.text_end_offset = _vte_stream_head (ring->text_stream);
				}
			}
			runlength = MIN(paragraph_len, attr_change.text_end_offset - text_offset);

			if (G_UNLIKELY (attr_change.attr.s.columns == 0)) {
				/* Combining characters all fit in the current row */
				text_offset += runlength;
				paragraph_len -= runlength;
			} else {
				while (runlength) {
					if (col >= columns - attr_change.attr.s.columns + 1) {
						/* Wrap now, write the soft wrapped row's record */
						new_record.soft_wrapped = 1;
						_vte_stream_append(new_row_stream, (const char *) &new_record, sizeof (new_record));
						_vte_debug_print(VTE_DEBUG_RING,
								"    New row %ld  text_offset %" G_GSIZE_FORMAT "  attr_offset %" G_GSIZE_FORMAT "  soft_wrapped\n",
								new_row_index,
								new_record.text_start_offset, new_record.attr_start_offset);
						for (i = 0; i < num_markers; i++) {
							if (G_UNLIKELY (marker_text_offsets[i].text_offset >= new_record.text_start_offset &&
									marker_text_offsets[i].text_offset < text_offset)) {
								new_markers[i].row = new_row_index;
								_vte_debug_print(VTE_DEBUG_RING,
										"      Marker #%d will be here in row %lu\n", i, new_row_index);
							}
						}
						new_row_index++;
						new_record.text_start_offset = text_offset;
						new_record.attr_start_offset = attr_offset;
						col = 0;
					}
					if (paragraph_is_ascii) {
						/* Shortcut for quickly wrapping ASCII (excluding TAB) text.
						   Don't read text_stream, and advance by a whole row of characters. */
						int len = MIN(runlength, (gsize) (columns - col));
						col += len;
						text_offset += len;
						paragraph_len -= len;
						runlength -= len;
					} else {
						/* Process one character only. */
						char textbuf[6];  /* fits at least one UTF-8 character */
						int textbuf_len;
						col += attr_change.attr.s.columns;
						/* Find beginning of next UTF-8 character */
						text_offset++; paragraph_len--; runlength--;
						textbuf_len = MIN(runlength, sizeof (textbuf));
						if (!_vte_stream_read(ring->text_stream, text_offset, textbuf, textbuf_len))
							goto err;
						for (i = 0; i < textbuf_len && (textbuf[i] & 0xC0) == 0x80; i++) {
							text_offset++; paragraph_len--; runlength--;
						}
					}
				}
			}
		}

		/* Write the record of the paragraph's last row. */
		/* Hard wrapped, except maybe at the end of the very last paragraph */
		new_record.soft_wrapped = prev_record_was_soft_wrapped;
		_vte_stream_append(new_row_stream, (const char *) &new_record, sizeof (new_record));
		_vte_debug_print(VTE_DEBUG_RING,
				"    New row %ld  text_offset %" G_GSIZE_FORMAT "  attr_offset %" G_GSIZE_FORMAT "\n",
				new_row_index,
				new_record.text_start_offset, new_record.attr_start_offset);
		for (i = 0; i < num_markers; i++) {
			if (G_UNLIKELY (marker_text_offsets[i].text_offset >= new_record.text_start_offset &&
					marker_text_offsets[i].text_offset < paragraph_end_text_offset)) {
				new_markers[i].row = new_row_index;
				_vte_debug_print(VTE_DEBUG_RING,
						"      Marker #%d will be here in row %lu\n", i, new_row_index);
			}
		}
		new_row_index++;
		paragraph_start_text_offset = paragraph_end_text_offset;
	}

	/* Update the ring. */
	old_ring_end = ring->end;
	g_object_unref(ring->row_stream);
	ring->row_stream = new_row_stream;
	ring->writable = ring->end = new_row_index;
	ring->start = 0;
	if (ring->end > ring->max)
		ring->start = ring->end - ring->max;
	ring->cached_row_num = (gulong) -1;

	/* Find the markers. This requires that the ring is already updated. */
	for (i = 0; i < num_markers; i++) {
		/* Compute the row for markers beyond the ring */
		if (new_markers[i].row == -1)
			new_markers[i].row = markers[i]->row - old_ring_end + ring->end;
		/* Convert byte offset into visual column */
		if (!_vte_frozen_row_text_offset_to_column(ring, new_markers[i].row, &marker_text_offsets[i], &new_markers[i].col))
			goto err;
		_vte_debug_print(VTE_DEBUG_RING,
				"Marker #%d new coords:  text_offset %" G_GSIZE_FORMAT "  fragment_cells %d  eol_cells %d  ->  row %ld  col %ld\n",
				i, marker_text_offsets[i].text_offset, marker_text_offsets[i].fragment_cells,
				marker_text_offsets[i].eol_cells, new_markers[i].row, new_markers[i].col);
		markers[i]->row = new_markers[i].row;
		markers[i]->col = new_markers[i].col;
	}
	g_free(marker_text_offsets);
	g_free(new_markers);

	_vte_debug_print(VTE_DEBUG_RING, "Ring after rewrapping:\n");
	_vte_ring_validate(ring);
	return;

err:
#ifdef VTE_DEBUG
	_vte_debug_print(VTE_DEBUG_RING,
			"Error while rewrapping\n");
	g_assert_not_reached();
#endif
	g_object_unref(new_row_stream);
	g_free(marker_text_offsets);
	g_free(new_markers);
}


static gboolean
_vte_ring_write_row (VteRing *ring,
		     GOutputStream *stream,
		     VteRowData *row,
		     VteWriteFlags flags,
		     GCancellable *cancellable,
		     GError **error)
{
	VteCell *cell;
	GString *buffer = ring->utf8_buffer;
	int i;
	gsize bytes_written;

	/* Simple version of the loop in _vte_ring_freeze_row().
	 * TODO Should unify one day */
	g_string_set_size (buffer, 0);
	for (i = 0, cell = row->cells; i < row->len; i++, cell++) {
		if (G_LIKELY (!cell->attr.fragment))
			_vte_unistr_append_to_string (cell->c, buffer);
	}
	if (!row->attr.soft_wrapped)
		g_string_append_c (buffer, '\n');

	return g_output_stream_write_all (stream, buffer->str, buffer->len, &bytes_written, cancellable, error);
}

/**
 * _vte_ring_write_contents:
 * @ring: a #VteRing
 * @stream: a #GOutputStream to write to
 * @flags: a set of #VteWriteFlags
 * @cancellable: optional #GCancellable object, %NULL to ignore
 * @error: a #GError location to store the error occuring, or %NULL to ignore
 *
 * Write entire ring contents to @stream according to @flags.
 *
 * Return: %TRUE on success, %FALSE if there was an error
 */
gboolean
_vte_ring_write_contents (VteRing *ring,
			  GOutputStream *stream,
			  VteWriteFlags flags,
			  GCancellable *cancellable,
			  GError **error)
{
	gulong i;

	_vte_debug_print(VTE_DEBUG_RING, "Writing contents to GOutputStream.\n");

	if (ring->start < ring->writable)
	{
		VteRowRecord record;

		if (_vte_ring_read_row_record (ring, &record, ring->start))
		{
			gsize start_offset = record.text_start_offset;
			gsize end_offset = _vte_stream_head (ring->text_stream);
			char buf[4096];
			while (start_offset < end_offset)
			{
				gsize bytes_written, len;

				len = MIN (G_N_ELEMENTS (buf), end_offset - start_offset);

				if (!_vte_stream_read (ring->text_stream, start_offset,
						       buf, len))
					return FALSE;

				if (!g_output_stream_write_all (stream, buf, len,
								&bytes_written, cancellable,
								error))
					return FALSE;

				start_offset += len;
			}
		}
		else
			return FALSE;
	}

	for (i = ring->writable; i < ring->end; i++) {
		if (!_vte_ring_write_row (ring, stream,
					  _vte_ring_writable_index (ring, i),
					  flags, cancellable, error))
			return FALSE;
	}

	return TRUE;
}
