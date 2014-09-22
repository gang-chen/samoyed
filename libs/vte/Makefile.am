noinst_LTLIBRARIES = libvte.la

libvte_la_SOURCES = \
    caps.c \
    debug.c \
    iso2022.c \
    keymap.c \
    matcher.c \
    table.c \
    vte.c \
    vteconv.c \
    vteseq.c \
    vtetree.c \
    buffer.h \
    caps.h \
    debug.h \
    iso2022.h \
    keymap.h \
    matcher.h \
    table.h \
    vte.h \
    vteconv.h \
    vtetree.h

EXTRA_libvte_la_SOURCES = \
    mkunitables.sh \
    unitable.CNS11643 \
    unitable.CP437 \
    unitable.GB12345 \
    unitable.GB2312 \
    unitable.JIS0201 \
    unitable.JIS0208 \
    unitable.JIS0212 \
    unitable.KSX1001 \
    vteseq-n.gperf \
    vteseq-n.c

BUILT_SOURCES = vteseq-list.h vteseq-n.c

MAINTAINERCLEANFILES = vteseq-list.h vteseq-n.c

libvte_la_CPPFLAGS = $(GTK_CFLAGS)

vteseq-n.c: vteseq-n.gperf
	$(AM_V_GEN) $(top_srcdir)/missing --run gperf -m 100 $< > $@.tmp && \
	mv -f $@.tmp $@

vteseq-list.h: vteseq-n.gperf
	$(AM_V_GEN) echo '/* Generated file.  Do not edit */' > $@.tmp && \
	cat $^ | grep -v '^#' | grep '\<VTE_SEQUENCE_HANDLER\>'| sed 's/.*, //' | LANG=C sort -u >> $@.tmp && \
	mv -f $@.tmp $@
