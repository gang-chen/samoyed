/* ANSI-C code produced by gperf version 3.0.4 */
/* Command-line: gperf -m 100 vteseq-n.gperf  */
/* Computed positions: -k'1,8,12,18' */

#if !((' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) \
      && ('%' == 37) && ('&' == 38) && ('\'' == 39) && ('(' == 40) \
      && (')' == 41) && ('*' == 42) && ('+' == 43) && (',' == 44) \
      && ('-' == 45) && ('.' == 46) && ('/' == 47) && ('0' == 48) \
      && ('1' == 49) && ('2' == 50) && ('3' == 51) && ('4' == 52) \
      && ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) \
      && ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) \
      && ('=' == 61) && ('>' == 62) && ('?' == 63) && ('A' == 65) \
      && ('B' == 66) && ('C' == 67) && ('D' == 68) && ('E' == 69) \
      && ('F' == 70) && ('G' == 71) && ('H' == 72) && ('I' == 73) \
      && ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) \
      && ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) \
      && ('R' == 82) && ('S' == 83) && ('T' == 84) && ('U' == 85) \
      && ('V' == 86) && ('W' == 87) && ('X' == 88) && ('Y' == 89) \
      && ('Z' == 90) && ('[' == 91) && ('\\' == 92) && (']' == 93) \
      && ('^' == 94) && ('_' == 95) && ('a' == 97) && ('b' == 98) \
      && ('c' == 99) && ('d' == 100) && ('e' == 101) && ('f' == 102) \
      && ('g' == 103) && ('h' == 104) && ('i' == 105) && ('j' == 106) \
      && ('k' == 107) && ('l' == 108) && ('m' == 109) && ('n' == 110) \
      && ('o' == 111) && ('p' == 112) && ('q' == 113) && ('r' == 114) \
      && ('s' == 115) && ('t' == 116) && ('u' == 117) && ('v' == 118) \
      && ('w' == 119) && ('x' == 120) && ('y' == 121) && ('z' == 122) \
      && ('{' == 123) && ('|' == 124) && ('}' == 125) && ('~' == 126))
/* The character set is not based on ISO-646.  */
#error "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gnu-gperf@gnu.org>."
#endif

#line 16 "vteseq-n.gperf"
struct vteseq_n_struct {
	int seq;
	VteTerminalSequenceHandler handler;
};
#include <string.h>
/* maximum key range = 120, duplicates = 0 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
vteseq_n_hash (register const char *str, register unsigned int len)
{
  static const unsigned char asso_values[] =
    {
      128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
      128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
      128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
      128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
      128, 128, 128, 128, 128,  20, 128, 128, 128, 128,
      128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
      128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
      128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
      128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
      128, 128, 128, 128, 128, 128, 128,  37,  42,   3,
       14,  16,   8,  20,   3,   4, 128,  15,  42,  10,
       17,   3,  14, 128,   4,   6,   5,  57,   5,  13,
      128,  15, 128, 128, 128, 128, 128, 128, 128, 128,
      128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
      128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
      128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
      128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
      128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
      128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
      128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
      128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
      128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
      128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
      128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
      128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
      128, 128, 128, 128, 128, 128
    };
  register int hval = len;

  switch (hval)
    {
      default:
        hval += asso_values[(unsigned char)str[17]];
      /*FALLTHROUGH*/
      case 17:
      case 16:
      case 15:
      case 14:
      case 13:
      case 12:
        hval += asso_values[(unsigned char)str[11]];
      /*FALLTHROUGH*/
      case 11:
      case 10:
      case 9:
      case 8:
        hval += asso_values[(unsigned char)str[7]];
      /*FALLTHROUGH*/
      case 7:
      case 6:
      case 5:
      case 4:
      case 3:
      case 2:
      case 1:
        hval += asso_values[(unsigned char)str[0]];
        break;
    }
  return hval;
}

struct vteseq_n_pool_t
  {
    char vteseq_n_pool_str8[sizeof("tab")];
    char vteseq_n_pool_str9[sizeof("index")];
    char vteseq_n_pool_str12[sizeof("tab-set")];
    char vteseq_n_pool_str17[sizeof("reset-mode")];
    char vteseq_n_pool_str18[sizeof("reset-color")];
    char vteseq_n_pool_str20[sizeof("decset")];
    char vteseq_n_pool_str21[sizeof("save-cursor")];
    char vteseq_n_pool_str22[sizeof("soft-reset")];
    char vteseq_n_pool_str24[sizeof("full-reset")];
    char vteseq_n_pool_str25[sizeof("change-color-st")];
    char vteseq_n_pool_str26[sizeof("change-color-bel")];
    char vteseq_n_pool_str27[sizeof("decreset")];
    char vteseq_n_pool_str28[sizeof("cursor-down")];
    char vteseq_n_pool_str29[sizeof("save-mode")];
    char vteseq_n_pool_str30[sizeof("set-mode")];
    char vteseq_n_pool_str31[sizeof("scroll-down")];
    char vteseq_n_pool_str33[sizeof("form-feed")];
    char vteseq_n_pool_str34[sizeof("change-cursor-color-st")];
    char vteseq_n_pool_str35[sizeof("change-cursor-color-bel")];
    char vteseq_n_pool_str36[sizeof("reset-foreground-color")];
    char vteseq_n_pool_str37[sizeof("cursor-position")];
    char vteseq_n_pool_str38[sizeof("erase-characters")];
    char vteseq_n_pool_str39[sizeof("carriage-return")];
    char vteseq_n_pool_str40[sizeof("return-terminal-status")];
    char vteseq_n_pool_str41[sizeof("set-window-title")];
    char vteseq_n_pool_str42[sizeof("set-icon-title")];
    char vteseq_n_pool_str43[sizeof("next-line")];
    char vteseq_n_pool_str44[sizeof("restore-cursor")];
    char vteseq_n_pool_str45[sizeof("return-terminal-id")];
    char vteseq_n_pool_str46[sizeof("bell")];
    char vteseq_n_pool_str47[sizeof("reset-highlight-foreground-color")];
    char vteseq_n_pool_str48[sizeof("cursor-position-top-row")];
    char vteseq_n_pool_str49[sizeof("character-attributes")];
    char vteseq_n_pool_str50[sizeof("set-scrolling-region")];
    char vteseq_n_pool_str51[sizeof("tab-clear")];
    char vteseq_n_pool_str52[sizeof("restore-mode")];
    char vteseq_n_pool_str53[sizeof("reverse-index")];
    char vteseq_n_pool_str54[sizeof("backspace")];
    char vteseq_n_pool_str55[sizeof("erase-in-display")];
    char vteseq_n_pool_str56[sizeof("cursor-next-line")];
    char vteseq_n_pool_str57[sizeof("set-scrolling-region-to-end")];
    char vteseq_n_pool_str58[sizeof("send-primary-device-attributes")];
    char vteseq_n_pool_str59[sizeof("window-manipulation")];
    char vteseq_n_pool_str60[sizeof("set-current-directory-uri")];
    char vteseq_n_pool_str61[sizeof("set-scrolling-region-from-start")];
    char vteseq_n_pool_str62[sizeof("cursor-forward")];
    char vteseq_n_pool_str63[sizeof("erase-in-line")];
    char vteseq_n_pool_str64[sizeof("insert-lines")];
    char vteseq_n_pool_str65[sizeof("set-icon-and-window-title")];
    char vteseq_n_pool_str66[sizeof("character-position-absolute")];
    char vteseq_n_pool_str67[sizeof("line-feed")];
    char vteseq_n_pool_str68[sizeof("dec-device-status-report")];
    char vteseq_n_pool_str69[sizeof("cursor-up")];
    char vteseq_n_pool_str70[sizeof("reset-background-color")];
    char vteseq_n_pool_str71[sizeof("delete-characters")];
    char vteseq_n_pool_str72[sizeof("scroll-up")];
    char vteseq_n_pool_str73[sizeof("cursor-backward")];
    char vteseq_n_pool_str74[sizeof("delete-lines")];
    char vteseq_n_pool_str75[sizeof("request-terminal-parameters")];
    char vteseq_n_pool_str76[sizeof("line-position-absolute")];
    char vteseq_n_pool_str77[sizeof("change-foreground-color-st")];
    char vteseq_n_pool_str78[sizeof("change-foreground-color-bel")];
    char vteseq_n_pool_str79[sizeof("default-character-set")];
    char vteseq_n_pool_str80[sizeof("cursor-back-tab")];
    char vteseq_n_pool_str81[sizeof("reset-highlight-background-color")];
    char vteseq_n_pool_str82[sizeof("normal-keypad")];
    char vteseq_n_pool_str83[sizeof("send-secondary-device-attributes")];
    char vteseq_n_pool_str86[sizeof("screen-alignment-test")];
    char vteseq_n_pool_str87[sizeof("reset-cursor-color")];
    char vteseq_n_pool_str88[sizeof("alternate-character-set-end")];
    char vteseq_n_pool_str90[sizeof("alternate-character-set-start")];
    char vteseq_n_pool_str92[sizeof("change-highlight-foreground-color-st")];
    char vteseq_n_pool_str93[sizeof("change-highlight-foreground-color-bel")];
    char vteseq_n_pool_str94[sizeof("application-keypad")];
    char vteseq_n_pool_str96[sizeof("cursor-preceding-line")];
    char vteseq_n_pool_str98[sizeof("utf-8-character-set")];
    char vteseq_n_pool_str100[sizeof("device-status-report")];
    char vteseq_n_pool_str101[sizeof("vertical-tab")];
    char vteseq_n_pool_str105[sizeof("cursor-character-absolute")];
    char vteseq_n_pool_str107[sizeof("set-current-file-uri")];
    char vteseq_n_pool_str111[sizeof("change-background-color-st")];
    char vteseq_n_pool_str112[sizeof("change-background-color-bel")];
    char vteseq_n_pool_str115[sizeof("cursor-forward-tabulation")];
    char vteseq_n_pool_str121[sizeof("insert-blank-characters")];
    char vteseq_n_pool_str124[sizeof("linux-console-cursor-attributes")];
    char vteseq_n_pool_str126[sizeof("change-highlight-background-color-st")];
    char vteseq_n_pool_str127[sizeof("change-highlight-background-color-bel")];
  };
static const struct vteseq_n_pool_t vteseq_n_pool_contents =
  {
    "tab",
    "index",
    "tab-set",
    "reset-mode",
    "reset-color",
    "decset",
    "save-cursor",
    "soft-reset",
    "full-reset",
    "change-color-st",
    "change-color-bel",
    "decreset",
    "cursor-down",
    "save-mode",
    "set-mode",
    "scroll-down",
    "form-feed",
    "change-cursor-color-st",
    "change-cursor-color-bel",
    "reset-foreground-color",
    "cursor-position",
    "erase-characters",
    "carriage-return",
    "return-terminal-status",
    "set-window-title",
    "set-icon-title",
    "next-line",
    "restore-cursor",
    "return-terminal-id",
    "bell",
    "reset-highlight-foreground-color",
    "cursor-position-top-row",
    "character-attributes",
    "set-scrolling-region",
    "tab-clear",
    "restore-mode",
    "reverse-index",
    "backspace",
    "erase-in-display",
    "cursor-next-line",
    "set-scrolling-region-to-end",
    "send-primary-device-attributes",
    "window-manipulation",
    "set-current-directory-uri",
    "set-scrolling-region-from-start",
    "cursor-forward",
    "erase-in-line",
    "insert-lines",
    "set-icon-and-window-title",
    "character-position-absolute",
    "line-feed",
    "dec-device-status-report",
    "cursor-up",
    "reset-background-color",
    "delete-characters",
    "scroll-up",
    "cursor-backward",
    "delete-lines",
    "request-terminal-parameters",
    "line-position-absolute",
    "change-foreground-color-st",
    "change-foreground-color-bel",
    "default-character-set",
    "cursor-back-tab",
    "reset-highlight-background-color",
    "normal-keypad",
    "send-secondary-device-attributes",
    "screen-alignment-test",
    "reset-cursor-color",
    "alternate-character-set-end",
    "alternate-character-set-start",
    "change-highlight-foreground-color-st",
    "change-highlight-foreground-color-bel",
    "application-keypad",
    "cursor-preceding-line",
    "utf-8-character-set",
    "device-status-report",
    "vertical-tab",
    "cursor-character-absolute",
    "set-current-file-uri",
    "change-background-color-st",
    "change-background-color-bel",
    "cursor-forward-tabulation",
    "insert-blank-characters",
    "linux-console-cursor-attributes",
    "change-highlight-background-color-st",
    "change-highlight-background-color-bel"
  };
#define vteseq_n_pool ((const char *) &vteseq_n_pool_contents)
#ifdef __GNUC__
__inline
#if defined __GNUC_STDC_INLINE__ || defined __GNUC_GNU_INLINE__
__attribute__ ((__gnu_inline__))
#endif
#endif
const struct vteseq_n_struct *
vteseq_n_lookup (register const char *str, register unsigned int len)
{
  enum
    {
      TOTAL_KEYWORDS = 87,
      MIN_WORD_LENGTH = 3,
      MAX_WORD_LENGTH = 37,
      MIN_HASH_VALUE = 8,
      MAX_HASH_VALUE = 127
    };

  static const unsigned char lengthtable[] =
    {
       0,  0,  0,  0,  0,  0,  0,  0,  3,  5,  0,  0,  7,  0,
       0,  0,  0, 10, 11,  0,  6, 11, 10,  0, 10, 15, 16,  8,
      11,  9,  8, 11,  0,  9, 22, 23, 22, 15, 16, 15, 22, 16,
      14,  9, 14, 18,  4, 32, 23, 20, 20,  9, 12, 13,  9, 16,
      16, 27, 30, 19, 25, 31, 14, 13, 12, 25, 27,  9, 24,  9,
      22, 17,  9, 15, 12, 27, 22, 26, 27, 21, 15, 32, 13, 32,
       0,  0, 21, 18, 27,  0, 29,  0, 36, 37, 18,  0, 21,  0,
      19,  0, 20, 12,  0,  0,  0, 25,  0, 20,  0,  0,  0, 26,
      27,  0,  0, 25,  0,  0,  0,  0,  0, 23,  0,  0, 31,  0,
      36, 37
    };
  static const struct vteseq_n_struct wordlist[] =
    {
      {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
#line 26 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str8, VTE_SEQUENCE_HANDLER(vte_sequence_handler_tab)},
#line 27 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str9, VTE_SEQUENCE_HANDLER(vte_sequence_handler_index)},
      {-1}, {-1},
#line 30 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str12, VTE_SEQUENCE_HANDLER(vte_sequence_handler_tab_set)},
      {-1}, {-1}, {-1}, {-1},
#line 41 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str17, VTE_SEQUENCE_HANDLER(vte_sequence_handler_reset_mode)},
#line 49 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str18, VTE_SEQUENCE_HANDLER(vte_sequence_handler_reset_color)},
      {-1},
#line 28 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str20, VTE_SEQUENCE_HANDLER(vte_sequence_handler_decset)},
#line 45 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str21, VTE_SEQUENCE_HANDLER(vte_sequence_handler_save_cursor)},
#line 42 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str22, VTE_SEQUENCE_HANDLER(vte_sequence_handler_soft_reset)},
      {-1},
#line 39 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str24, VTE_SEQUENCE_HANDLER(vte_sequence_handler_full_reset)},
#line 48 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str25, VTE_SEQUENCE_HANDLER(vte_sequence_handler_change_color_st)},
#line 47 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str26, VTE_SEQUENCE_HANDLER(vte_sequence_handler_change_color_bel)},
#line 31 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str27, VTE_SEQUENCE_HANDLER(vte_sequence_handler_decreset)},
#line 43 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str28, VTE_SEQUENCE_HANDLER(vte_sequence_handler_cursor_down)},
#line 36 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str29, VTE_SEQUENCE_HANDLER(vte_sequence_handler_save_mode)},
#line 32 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str30, VTE_SEQUENCE_HANDLER(vte_sequence_handler_set_mode)},
#line 46 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str31, VTE_SEQUENCE_HANDLER(vte_sequence_handler_scroll_down)},
      {-1},
#line 34 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str33, VTE_SEQUENCE_HANDLER(vte_sequence_handler_form_feed)},
#line 87 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str34, VTE_SEQUENCE_HANDLER(vte_sequence_handler_change_cursor_color_st)},
#line 86 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str35, VTE_SEQUENCE_HANDLER(vte_sequence_handler_change_cursor_color_bel)},
#line 127 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str36, VTE_SEQUENCE_HANDLER(vte_sequence_handler_reset_foreground_color)},
#line 69 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str37, VTE_SEQUENCE_HANDLER(vte_sequence_handler_cursor_position)},
#line 75 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str38, VTE_SEQUENCE_HANDLER(vte_sequence_handler_erase_characters)},
#line 23 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str39, VTE_SEQUENCE_HANDLER(vte_sequence_handler_carriage_return)},
#line 104 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str40, VTE_SEQUENCE_HANDLER(vte_sequence_handler_return_terminal_status)},
#line 77 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str41, VTE_SEQUENCE_HANDLER(vte_sequence_handler_set_window_title)},
#line 66 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str42, VTE_SEQUENCE_HANDLER(vte_sequence_handler_set_icon_title)},
#line 35 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str43, VTE_SEQUENCE_HANDLER(vte_sequence_handler_next_line)},
#line 65 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str44, VTE_SEQUENCE_HANDLER(vte_sequence_handler_restore_cursor)},
#line 82 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str45, VTE_SEQUENCE_HANDLER(vte_sequence_handler_return_terminal_id)},
#line 24 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str46, VTE_SEQUENCE_HANDLER(vte_sequence_handler_bell)},
#line 110 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str47, VTE_SEQUENCE_HANDLER(vte_sequence_handler_reset_highlight_foreground_color)},
#line 70 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str48, VTE_SEQUENCE_HANDLER(vte_sequence_handler_cursor_position_top_row)},
#line 89 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str49, VTE_SEQUENCE_HANDLER(vte_sequence_handler_character_attributes)},
#line 91 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str50, VTE_SEQUENCE_HANDLER(vte_sequence_handler_set_scrolling_region)},
#line 38 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str51, VTE_SEQUENCE_HANDLER(vte_sequence_handler_tab_clear)},
#line 53 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str52, VTE_SEQUENCE_HANDLER(vte_sequence_handler_restore_mode)},
#line 59 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str53, VTE_SEQUENCE_HANDLER(vte_sequence_handler_reverse_index)},
#line 25 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str54, VTE_SEQUENCE_HANDLER(vte_sequence_handler_backspace)},
#line 76 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str55, VTE_SEQUENCE_HANDLER(vte_sequence_handler_erase_in_display)},
#line 74 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str56, VTE_SEQUENCE_HANDLER(vte_sequence_handler_cursor_next_line)},
#line 93 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str57, VTE_SEQUENCE_HANDLER(vte_sequence_handler_set_scrolling_region_to_end)},
#line 152 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str58, VTE_SEQUENCE_HANDLER(vte_sequence_handler_send_primary_device_attributes)},
#line 85 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str59, VTE_SEQUENCE_HANDLER(vte_sequence_handler_window_manipulation)},
#line 161 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str60, VTE_SEQUENCE_HANDLER(vte_sequence_handler_set_current_directory_uri)},
#line 92 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str61, VTE_SEQUENCE_HANDLER(vte_sequence_handler_set_scrolling_region_from_start)},
#line 63 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str62, VTE_SEQUENCE_HANDLER(vte_sequence_handler_cursor_forward)},
#line 56 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str63, VTE_SEQUENCE_HANDLER(vte_sequence_handler_erase_in_line)},
#line 52 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str64, VTE_SEQUENCE_HANDLER(vte_sequence_handler_insert_lines)},
#line 134 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str65, VTE_SEQUENCE_HANDLER(vte_sequence_handler_set_icon_and_window_title)},
#line 136 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str66, VTE_SEQUENCE_HANDLER(vte_sequence_handler_character_position_absolute)},
#line 22 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str67, VTE_SEQUENCE_HANDLER(vte_sequence_handler_line_feed)},
#line 128 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str68, VTE_SEQUENCE_HANDLER(vte_sequence_handler_dec_device_status_report)},
#line 33 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str69, VTE_SEQUENCE_HANDLER(vte_sequence_handler_cursor_up)},
#line 124 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str70, VTE_SEQUENCE_HANDLER(vte_sequence_handler_reset_background_color)},
#line 79 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str71, VTE_SEQUENCE_HANDLER(vte_sequence_handler_delete_characters)},
#line 37 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str72, VTE_SEQUENCE_HANDLER(vte_sequence_handler_scroll_up)},
#line 68 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str73, VTE_SEQUENCE_HANDLER(vte_sequence_handler_cursor_backward)},
#line 50 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str74, VTE_SEQUENCE_HANDLER(vte_sequence_handler_delete_lines)},
#line 137 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str75, VTE_SEQUENCE_HANDLER(vte_sequence_handler_request_terminal_parameters)},
#line 103 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str76, VTE_SEQUENCE_HANDLER(vte_sequence_handler_line_position_absolute)},
#line 126 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str77, VTE_SEQUENCE_HANDLER(vte_sequence_handler_change_foreground_color_st)},
#line 125 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str78, VTE_SEQUENCE_HANDLER(vte_sequence_handler_change_foreground_color_bel)},
#line 115 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str79, VTE_SEQUENCE_HANDLER(vte_sequence_handler_default_character_set)},
#line 67 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str80, VTE_SEQUENCE_HANDLER(vte_sequence_handler_cursor_back_tab)},
#line 107 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str81, VTE_SEQUENCE_HANDLER(vte_sequence_handler_reset_highlight_background_color)},
#line 58 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str82, VTE_SEQUENCE_HANDLER(vte_sequence_handler_normal_keypad)},
#line 154 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str83, VTE_SEQUENCE_HANDLER(vte_sequence_handler_send_secondary_device_attributes)},
      {-1}, {-1},
#line 98 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str86, VTE_SEQUENCE_HANDLER(vte_sequence_handler_screen_alignment_test)},
#line 88 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str87, VTE_SEQUENCE_HANDLER(vte_sequence_handler_reset_cursor_color)},
#line 121 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str88, VTE_SEQUENCE_HANDLER(vte_sequence_handler_alternate_character_set_end)},
      {-1},
#line 120 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str90, VTE_SEQUENCE_HANDLER(vte_sequence_handler_alternate_character_set_start)},
      {-1},
#line 109 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str92, VTE_SEQUENCE_HANDLER(vte_sequence_handler_change_highlight_foreground_color_st)},
#line 108 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str93, VTE_SEQUENCE_HANDLER(vte_sequence_handler_change_highlight_foreground_color_bel)},
#line 80 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str94, VTE_SEQUENCE_HANDLER(vte_sequence_handler_application_keypad)},
      {-1},
#line 96 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str96, VTE_SEQUENCE_HANDLER(vte_sequence_handler_cursor_preceding_line)},
      {-1},
#line 84 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str98, VTE_SEQUENCE_HANDLER(vte_sequence_handler_utf_8_character_set)},
      {-1},
#line 90 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str100, VTE_SEQUENCE_HANDLER(vte_sequence_handler_device_status_report)},
#line 55 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str101, VTE_SEQUENCE_HANDLER(vte_sequence_handler_vertical_tab)},
      {-1}, {-1}, {-1},
#line 131 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str105, VTE_SEQUENCE_HANDLER(vte_sequence_handler_cursor_character_absolute)},
      {-1},
#line 162 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str107, VTE_SEQUENCE_HANDLER(vte_sequence_handler_set_current_file_uri)},
      {-1}, {-1}, {-1},
#line 123 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str111, VTE_SEQUENCE_HANDLER(vte_sequence_handler_change_background_color_st)},
#line 122 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str112, VTE_SEQUENCE_HANDLER(vte_sequence_handler_change_background_color_bel)},
      {-1}, {-1},
#line 132 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str115, VTE_SEQUENCE_HANDLER(vte_sequence_handler_cursor_forward_tabulation)},
      {-1}, {-1}, {-1}, {-1}, {-1},
#line 112 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str121, VTE_SEQUENCE_HANDLER(vte_sequence_handler_insert_blank_characters)},
      {-1}, {-1},
#line 153 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str124, VTE_SEQUENCE_HANDLER(vte_sequence_handler_linux_console_cursor_attributes)},
      {-1},
#line 106 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str126, VTE_SEQUENCE_HANDLER(vte_sequence_handler_change_highlight_background_color_st)},
#line 105 "vteseq-n.gperf"
      {(int)(long)&((struct vteseq_n_pool_t *)0)->vteseq_n_pool_str127, VTE_SEQUENCE_HANDLER(vte_sequence_handler_change_highlight_background_color_bel)}
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = vteseq_n_hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        if (len == lengthtable[key])
          {
            register const char *s = wordlist[key].seq + vteseq_n_pool;

            if (*str == *s && !memcmp (str + 1, s + 1, len - 1))
              return &wordlist[key];
          }
    }
  return 0;
}
