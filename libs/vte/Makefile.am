noinst_LTLIBRARIES = libvte.la

libvte_la_SOURCES = \
    caps.c \
    debug.c \
    iso2022.c \
    keymap.c \
    matcher.c \
    ring.c \
    table.c \
    vte.c \
    vteconv.c \
    vtedraw.c \
    vterowdata.c \
    vteseq.c \
    vtestream.c \
    vtetree.c \
    vteunistr.c \
    vteutils.c \
    buffer.h \
    caps.h \
    debug.h \
    iso2022.h \
    keymap.h \
    matcher.h \
    ring.h \
    table.h \
    vte.h \
    vte-private.h \
    vteconv.h \
    vtedraw.h \
    vteenums.h \
    vteglobals.h \
    vteint.h \
    vtemacros.h \
    vterowdata.h \
    vtestream.h \
    vtestream-base.h \
    vtestream-file.h \
    vteterminal.h \
    vtetree.h \
    vteunistr.h \
    vteutils.h

nodist_libvte_la_SOURCES = \
    box_drawing.h \
    vtetypebuiltins.c \
    vtetypebuiltins.h

BUILT_SOURCES = \
    box_drawing.h \
    vtetypebuiltins.c \
    vtetypebuiltins.h \
    vteseq-n.c \
    vteseq-list.h

EXTRA_DIST = \
    box_drawing_generate.sh \
    box_drawing.txt \
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
    vteseq-n.c \
    vteseq-list.h \
    vtetypebuiltins.c.template \
    vtetypebuiltins.h.template

MAINTAINERCLEANFILES = \
    vteseq-n.c \
    vteseq-list.h

CLEANFILES = \
    box_drawing.h \
    vtetypebuiltins.c \
    vtetypebuiltins.h \
    stamp-vtetypebuiltins.h

libvte_la_CPPFLAGS = \
    $(GTK_CFLAGS)
    -DGLIB_DISABLE_DEPRECATION_WARNINGS \
    -DVTE_COMPILATION

box_drawing.h: box_drawing.txt
	$(AM_V_GEN) $(srcdir)/box_drawing_generate.sh < $< > $@

vtetypebuiltins.h: stamp-vtetypebuiltins.h
	@true
stamp-vtetypebuiltins.h: vtetypebuiltins.h.template vteenums.h
	$(AM_V_GEN) $(GLIB_MKENUMS) --template $< $(filter %.h,$^) > xgen-vtbh \
	&& (cmp -s xgen-vtbh vtetypebuiltins.h || cp xgen-vtbh vtetypebuiltins.h ) \
	&& rm -f xgen-vtbh \
	&& echo timestamp > $(@F)

vtetypebuiltins.c: vtetypebuiltins.c.template vteenums.h
	$(AM_V_GEN) $(GLIB_MKENUMS) --template $< $(filter %.h,$^) > xgen-vtbc \
	&& (cmp -s xgen-vtbc vtetypebuiltins.c || cp xgen-vtbc vtetypebuiltins.c ) \
	&& rm -f xgen-vtbc

vteseq-n.c: vteseq-n.gperf
	$(AM_V_GEN) $(top_srcdir)/missing --run gperf -m 100 $< > $@.tmp && \
	mv -f $@.tmp $@

vteseq-list.h: vteseq-n.gperf
	$(AM_V_GEN) echo '/* Generated file.  Do not edit */' > $@.tmp && \
	cat $^ | grep -v '^#' | grep '\<VTE_SEQUENCE_HANDLER\>'| sed 's/.*, //' | LANG=C sort -u >> $@.tmp && \
	mv -f $@.tmp $@
