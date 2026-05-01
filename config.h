/*
 * config.h — everything you might want to change
 * edit this file, run `make`, done.
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ── identity ────────────────────────────────────────────────────────── */
#define APP_NAME            "ash"

/* ── window ──────────────────────────────────────────────────────────── */
#define DEFAULT_WIDTH       800
#define DEFAULT_HEIGHT      480
#define HIDE_DECORATIONS    0       /* 1 = no title bar */

/* ── font ────────────────────────────────────────────────────────────── */
#define FONT                "JetBrains Mono 11"
#define FONT_SCALE          1.0

/* ── padding ─────────────────────────────────────────────────────────── */
#define PADDING             "6px 8px"

/* ── cursor ──────────────────────────────────────────────────────────── */
#define CURSOR_SHAPE        VTE_CURSOR_SHAPE_BLOCK
#define CURSOR_BLINK        VTE_CURSOR_BLINK_OFF

/* ── text rendering ──────────────────────────────────────────────────── */
/* BOLD_IS_BRIGHT: 1 = bold text uses the bright color palette entry     */
/*                 0 = bold is rendered with the same hue, just bolder   */
#define BOLD_IS_BRIGHT      0

/* Characters treated as part of a "word" for double-click selection.    */
/* Add any chars you want selected as a unit (e.g. paths, URLs).         */
#define WORD_CHAR_EXCEPTIONS "-,./?%&#:_=+@~"

/* ── scrollback ──────────────────────────────────────────────────────── */
#define SCROLLBACK_LINES    10000

/* ── shell ───────────────────────────────────────────────────────────── */
#define FALLBACK_SHELL      "/bin/sh"

/* ── transparency ────────────────────────────────────────────────────── */
/* 1.0 = fully opaque, 0.0 = fully transparent                           */
/* requires a compositor (e.g. picom, kwin, mutter)                      */
#define BG_ALPHA            1.0

/* ── tabs ────────────────────────────────────────────────────────────── */
/* 0 = hide tab bar when only one tab is open (recommended)              */
/* 1 = always show the tab bar                                           */
#define ALWAYS_SHOW_TABS    0

/* ── tab bar colors ──────────────────────────────────────────────────── */
#define TAB_BAR_BG          "#16172a"
#define TAB_BG_ACTIVE       "#1a1b26"
#define TAB_BG_HOVER        "#20213a"
#define TAB_FG_ACTIVE       "#c0caf5"
#define TAB_FG_INACTIVE     "#565f89"

/* ── terminal colors: tokyo night dark ───────────────────────────────── */
#define COLOR_BG            "#1a1b26"
#define COLOR_FG            "#c0caf5"

#define COLOR_BLACK         "#15161e"
#define COLOR_RED           "#f7768e"
#define COLOR_GREEN         "#9ece6a"
#define COLOR_YELLOW        "#e0af68"
#define COLOR_BLUE          "#7aa2f7"
#define COLOR_MAGENTA       "#bb9af7"
#define COLOR_CYAN          "#7dcfff"
#define COLOR_WHITE         "#a9b1d6"

#define COLOR_BBLACK        "#414868"
#define COLOR_BRED          "#f7768e"
#define COLOR_BGREEN        "#9ece6a"
#define COLOR_BYELLOW       "#e0af68"
#define COLOR_BBLUE         "#7aa2f7"
#define COLOR_BMAGENTA      "#bb9af7"
#define COLOR_BCYAN         "#7dcfff"
#define COLOR_BWHITE        "#c0caf5"

/* ── alternate themes ────────────────────────────────────────────────── */

/*
 * gruvbox dark
 *
#define COLOR_BG            "#282828"
#define COLOR_FG            "#ebdbb2"
#define COLOR_BLACK         "#282828"
#define COLOR_RED           "#cc241d"
#define COLOR_GREEN         "#98971a"
#define COLOR_YELLOW        "#d79921"
#define COLOR_BLUE          "#458588"
#define COLOR_MAGENTA       "#b16286"
#define COLOR_CYAN          "#689d6a"
#define COLOR_WHITE         "#a89984"
#define COLOR_BBLACK        "#928374"
#define COLOR_BRED          "#fb4934"
#define COLOR_BGREEN        "#b8bb26"
#define COLOR_BYELLOW       "#fabd2f"
#define COLOR_BBLUE         "#83a598"
#define COLOR_BMAGENTA      "#d3869b"
#define COLOR_BCYAN         "#8ec07c"
#define COLOR_BWHITE        "#ebdbb2"
#define TAB_BAR_BG          "#1d2021"
#define TAB_BG_ACTIVE       "#282828"
#define TAB_BG_HOVER        "#32302f"
#define TAB_FG_ACTIVE       "#ebdbb2"
#define TAB_FG_INACTIVE     "#7c6f64"
*/

/*
 * dracula
 *
#define COLOR_BG            "#282a36"
#define COLOR_FG            "#f8f8f2"
#define COLOR_BLACK         "#21222c"
#define COLOR_RED           "#ff5555"
#define COLOR_GREEN         "#50fa7b"
#define COLOR_YELLOW        "#f1fa8c"
#define COLOR_BLUE          "#bd93f9"
#define COLOR_MAGENTA       "#ff79c6"
#define COLOR_CYAN          "#8be9fd"
#define COLOR_WHITE         "#f8f8f2"
#define COLOR_BBLACK        "#6272a4"
#define COLOR_BRED          "#ff6e6e"
#define COLOR_BGREEN        "#69ff94"
#define COLOR_BYELLOW       "#ffffa5"
#define COLOR_BBLUE         "#d6acff"
#define COLOR_BMAGENTA      "#ff92df"
#define COLOR_BCYAN         "#a4ffff"
#define COLOR_BWHITE        "#ffffff"
#define TAB_BAR_BG          "#1e1f29"
#define TAB_BG_ACTIVE       "#282a36"
#define TAB_BG_HOVER        "#343746"
#define TAB_FG_ACTIVE       "#f8f8f2"
#define TAB_FG_INACTIVE     "#6272a4"
*/

/*
 * solarized light
 *
#define COLOR_BG            "#fdf6e3"
#define COLOR_FG            "#657b83"
#define COLOR_BLACK         "#073642"
#define COLOR_RED           "#dc322f"
#define COLOR_GREEN         "#859900"
#define COLOR_YELLOW        "#b58900"
#define COLOR_BLUE          "#268bd2"
#define COLOR_MAGENTA       "#d33682"
#define COLOR_CYAN          "#2aa198"
#define COLOR_WHITE         "#eee8d5"
#define COLOR_BBLACK        "#002b36"
#define COLOR_BRED          "#cb4b16"
#define COLOR_BGREEN        "#586e75"
#define COLOR_BYELLOW       "#657b83"
#define COLOR_BBLUE         "#839496"
#define COLOR_BMAGENTA      "#6c71c4"
#define COLOR_BCYAN         "#93a1a1"
#define COLOR_BWHITE        "#fdf6e3"
#define TAB_BAR_BG          "#eee8d5"
#define TAB_BG_ACTIVE       "#fdf6e3"
#define TAB_BG_HOVER        "#e8e2cf"
#define TAB_FG_ACTIVE       "#073642"
#define TAB_FG_INACTIVE     "#93a1a1"
*/

#endif /* CONFIG_H */
