/*
 * ash — a terminal that stays out of your way
 *
 * features:
 *   - proper terminal identification (TERM_PROGRAM, COLORTERM, VTE_VERSION)
 *   - tabs (Ctrl+Shift+T, Ctrl+Shift+W, Ctrl+PgUp/PgDn, Alt+1-9)
 *   - transparency (set BG_ALPHA in config.h, needs a compositor)
 *   - URL detection — hover highlights, click opens with xdg-open
 *   - right-click context menu (copy / paste / open URL)
 *   - inherit working directory from current tab on new tab
 *   - -e / --command flag to run a program directly
 *   - --hold flag to keep the window open after command exits
 *   - --title flag to set a fixed window title
 *   - bold, dim, italic, strikethrough text rendering
 *   - 24-bit true color (COLORTERM=truecolor)
 *   - word-char set for smarter double-click selection
 *   - per-tab close button
 *
 * build:
 *   gcc $(pkg-config --cflags --libs gtk+-3.0 vte-2.91) -O2 -o ash ash.c
 */

#include <gtk/gtk.h>
#include <vte/vte.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include "config.h"

/* ── globals ─────────────────────────────────────────────────────────── */

static GtkWidget *win;
static GtkWidget *notebook;

/* CLI options */
static char    **opt_exec  = NULL;
static gboolean  opt_hold  = FALSE;
static char     *opt_title = NULL;
static char     *opt_dir   = NULL;

#define URL_REGEX \
    "(?:[a-zA-Z][a-zA-Z0-9+\\-.]*:/{0,3}[^\\s/$.?#][^\\s]*)"\
    "|(?:www\\.)[^\\s/$.?#][^\\s]*"


/* ── per-tab state ───────────────────────────────────────────────────── */

typedef struct {
    GtkWidget *vte;
    GtkWidget *label;
    GtkWidget *close_btn;
    char      *last_url;
} Tab;

static Tab *
current_tab(void)
{
    int n = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
    if (n < 0) return NULL;
    GtkWidget *page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), n);
    return g_object_get_data(G_OBJECT(page), "tab");
}

static Tab *
tab_at(int n)
{
    GtkWidget *page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), n);
    return page ? g_object_get_data(G_OBJECT(page), "tab") : NULL;
}


/* ── appearance ──────────────────────────────────────────────────────── */

static void
apply_colors_to(GtkWidget *vte_widget)
{
    GdkRGBA fg, bg, palette[16];
    gdk_rgba_parse(&fg, COLOR_FG);
    gdk_rgba_parse(&bg, COLOR_BG);
    bg.alpha = BG_ALPHA;

    const char *cols[16] = {
        COLOR_BLACK,   COLOR_RED,     COLOR_GREEN,   COLOR_YELLOW,
        COLOR_BLUE,    COLOR_MAGENTA, COLOR_CYAN,    COLOR_WHITE,
        COLOR_BBLACK,  COLOR_BRED,    COLOR_BGREEN,  COLOR_BYELLOW,
        COLOR_BBLUE,   COLOR_BMAGENTA,COLOR_BCYAN,   COLOR_BWHITE,
    };
    for (int i = 0; i < 16; i++)
        gdk_rgba_parse(&palette[i], cols[i]);

    vte_terminal_set_colors(VTE_TERMINAL(vte_widget), &fg, &bg, palette, 16);
}

static void
apply_font_to(GtkWidget *vte_widget)
{
    PangoFontDescription *fd = pango_font_description_from_string(FONT);
    vte_terminal_set_font(VTE_TERMINAL(vte_widget), fd);
    pango_font_description_free(fd);
    vte_terminal_set_font_scale(VTE_TERMINAL(vte_widget), FONT_SCALE);
}

static void
apply_padding_to(GtkWidget *vte_widget)
{
    GtkCssProvider *css = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css,
        "vte-terminal { padding: " PADDING "; }", -1, NULL);
    gtk_style_context_add_provider(
        gtk_widget_get_style_context(vte_widget),
        GTK_STYLE_PROVIDER(css),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css);
}

static void
style_notebook(void)
{
    GtkCssProvider *css = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css,
        "notebook header {"
        "  background: " TAB_BAR_BG ";"
        "  border: none;"
        "  padding: 2px 4px 0 4px; }"
        "notebook tab {"
        "  background: transparent;"
        "  color: " TAB_FG_INACTIVE ";"
        "  padding: 3px 8px 3px 10px;"
        "  border: none;"
        "  border-radius: 4px 4px 0 0; }"
        "notebook tab:checked {"
        "  background: " TAB_BG_ACTIVE ";"
        "  color: " TAB_FG_ACTIVE "; }"
        "notebook > header > tabs > tab:hover {"
        "  background: " TAB_BG_HOVER "; }"
        "button.tab-close {"
        "  padding: 0; min-width: 14px; min-height: 14px;"
        "  border: none; background: transparent;"
        "  color: inherit; border-radius: 3px; }"
        "button.tab-close:hover { background: rgba(255,255,255,0.15); }",
        -1, NULL);

    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(css),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css);
}

static void
setup_visual(void)
{
    GdkScreen *screen = gtk_widget_get_screen(win);
    GdkVisual *visual = gdk_screen_get_rgba_visual(screen);
    if (visual) {
        gtk_widget_set_visual(win, visual);
        gtk_widget_set_app_paintable(win, TRUE);
    }
}


/* ── URL handling ────────────────────────────────────────────────────── */

static void
setup_url_match(Tab *tab)
{
    GError   *err = NULL;
    VteRegex *re  = vte_regex_new_for_match(
        URL_REGEX, -1, 0x00000008u /* PCRE2_CASELESS */, &err);

    if (err) {
        g_warning("ash: URL regex: %s", err->message);
        g_error_free(err);
        return;
    }

    int tag = vte_terminal_match_add_regex(VTE_TERMINAL(tab->vte), re, 0);
    vte_terminal_match_set_cursor_name(VTE_TERMINAL(tab->vte), tag, "pointer");
    vte_regex_unref(re);
}

static void
open_url(const char *url)
{
    char *argv[] = { "xdg-open", (char *)url, NULL };
    g_spawn_async(NULL, argv, NULL,
        G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
        NULL, NULL, NULL, NULL);
}


/* ── context menu ────────────────────────────────────────────────────── */

static void
menu_copy(GtkMenuItem *item, gpointer data)
{
    (void)item;
    vte_terminal_copy_clipboard_format(VTE_TERMINAL(data), VTE_FORMAT_TEXT);
}

static void
menu_paste(GtkMenuItem *item, gpointer data)
{
    (void)item;
    vte_terminal_paste_clipboard(VTE_TERMINAL(data));
}

static void
menu_open_url(GtkMenuItem *item, gpointer data)
{
    (void)item;
    open_url((const char *)data);
}

static gboolean
on_button_press(GtkWidget *widget, GdkEventButton *ev, gpointer data)
{
    Tab *tab = (Tab *)data;

    if (ev->button == 1 && ev->type == GDK_BUTTON_PRESS) {
        char *url = vte_terminal_match_check_event(
            VTE_TERMINAL(widget), (GdkEvent *)ev, NULL);
        if (url) { open_url(url); g_free(url); return TRUE; }
    }

    if (ev->button == 3) {
        g_free(tab->last_url);
        tab->last_url = vte_terminal_match_check_event(
            VTE_TERMINAL(widget), (GdkEvent *)ev, NULL);

        GtkWidget *menu = gtk_menu_new();

        GtkWidget *copy  = gtk_menu_item_new_with_label("Copy");
        GtkWidget *paste = gtk_menu_item_new_with_label("Paste");
        g_signal_connect(copy,  "activate", G_CALLBACK(menu_copy),  widget);
        g_signal_connect(paste, "activate", G_CALLBACK(menu_paste), widget);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), copy);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), paste);

        if (tab->last_url) {
            gtk_menu_shell_append(GTK_MENU_SHELL(menu),
                gtk_separator_menu_item_new());
            char lbl[256];
            snprintf(lbl, sizeof(lbl), "Open: %.60s…", tab->last_url);
            GtkWidget *urlitem = gtk_menu_item_new_with_label(lbl);
            g_signal_connect(urlitem, "activate",
                G_CALLBACK(menu_open_url), tab->last_url);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), urlitem);
        }

        gtk_widget_show_all(menu);
        gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent *)ev);
        return TRUE;
    }

    return FALSE;
}


/* ── environment for spawned shell ───────────────────────────────────── */

static char **
build_child_env(void)
{
    extern char **environ;

    int n = 0;
    while (environ[n]) n++;

    const char *force_keys[] = {
        "TERM", "TERM_PROGRAM", "COLORTERM", "VTE_VERSION", NULL
    };

    char **env = g_new0(char *, n + 5 + 1);
    int out = 0;

    for (int i = 0; i < n; i++) {
        gboolean skip = FALSE;
        for (int j = 0; force_keys[j]; j++) {
            size_t klen = strlen(force_keys[j]);
            if (strncmp(environ[i], force_keys[j], klen) == 0 &&
                environ[i][klen] == '=') {
                skip = TRUE; break;
            }
        }
        if (!skip) env[out++] = g_strdup(environ[i]);
    }

    env[out++] = g_strdup("TERM=xterm-256color");
    env[out++] = g_strdup("TERM_PROGRAM=ash");
    env[out++] = g_strdup("COLORTERM=truecolor");

    /* VTE_VERSION: e.g. VTE 0.74.0 → "7400" */
    env[out++] = g_strdup_printf("VTE_VERSION=%u",
        vte_get_major_version() * 10000 +
        vte_get_minor_version() * 100  +
        vte_get_micro_version());

    env[out] = NULL;
    return env;
}


/* ── working directory detection ─────────────────────────────────────── */

static char *
get_cwd_of(GtkWidget *vte_widget)
{
    VtePty *pty = vte_terminal_get_pty(VTE_TERMINAL(vte_widget));
    if (!pty) return NULL;
    int fd = vte_pty_get_fd(pty);
    if (fd < 0) return NULL;

    pid_t fg = tcgetpgrp(fd);
    if (fg <= 0) return NULL;

    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/cwd", (int)fg);
    return g_file_read_link(path, NULL);
}


/* ── spawn ───────────────────────────────────────────────────────────── */

typedef struct { char **env; } SpawnCB;

static void
on_spawn_done(VteTerminal *t, GPid pid, GError *err, gpointer data)
{
    SpawnCB *cb = data;
    g_strfreev(cb->env);
    g_free(cb);
    if (err) g_warning("ash: spawn: %s", err->message);
    (void)t; (void)pid;
}

static void
spawn_in(GtkWidget *vte_widget, const char *cwd, char **exec_argv, gboolean hold)
{
    char **env  = build_child_env();
    char  *shell = (char *)g_getenv("SHELL");
    if (!shell) shell = FALLBACK_SHELL;

    char *shell_argv[] = { shell, NULL };
    char **argv = (exec_argv && exec_argv[0]) ? exec_argv : shell_argv;

    char *hold_cmd = NULL;
    char *hold_argv[4];

    if (hold && exec_argv && exec_argv[0]) {
        GString *cmd = g_string_new(NULL);
        for (int i = 0; exec_argv[i]; i++) {
            if (i) g_string_append_c(cmd, ' ');
            char *q = g_shell_quote(exec_argv[i]);
            g_string_append(cmd, q);
            g_free(q);
        }
        g_string_append_printf(cmd, "; exec %s", shell);
        hold_cmd = g_string_free(cmd, FALSE);
        hold_argv[0] = shell;
        hold_argv[1] = (char *)"-c";
        hold_argv[2] = hold_cmd;
        hold_argv[3] = NULL;
        argv = hold_argv;
    }

    SpawnCB *cb = g_new0(SpawnCB, 1);
    cb->env = env;

    vte_terminal_spawn_async(
        VTE_TERMINAL(vte_widget),
        VTE_PTY_DEFAULT,
        cwd, argv, env,
        G_SPAWN_SEARCH_PATH,
        NULL, NULL, NULL,
        -1, NULL,
        on_spawn_done, cb);

    g_free(hold_cmd);
}


/* ── tab title ───────────────────────────────────────────────────────── */

static void
on_title_changed(VteTerminal *t, gpointer data)
{
    Tab *tab = data;

    if (opt_title) {
        gtk_label_set_text(GTK_LABEL(tab->label), opt_title);
        gtk_window_set_title(GTK_WINDOW(win), opt_title);
        return;
    }

    const char *title = vte_terminal_get_window_title(t);
    if (!title || !*title) return;

    char short_title[25];
    strncpy(short_title, title, 24);
    short_title[24] = '\0';
    gtk_label_set_text(GTK_LABEL(tab->label), short_title);

    Tab *cur = current_tab();
    if (cur == tab)
        gtk_window_set_title(GTK_WINDOW(win), title);
}

static void
on_tab_switch(GtkNotebook *nb, GtkWidget *page, guint n, gpointer data)
{
    (void)nb; (void)page; (void)data;
    Tab *tab = tab_at(n);
    if (!tab) return;

    if (opt_title) {
        gtk_window_set_title(GTK_WINDOW(win), opt_title);
    } else {
        const char *title = vte_terminal_get_window_title(
            VTE_TERMINAL(tab->vte));
        gtk_window_set_title(GTK_WINDOW(win), title ? title : APP_NAME);
    }
    gtk_widget_grab_focus(tab->vte);
}


/* ── tab lifecycle ───────────────────────────────────────────────────── */

static void
maybe_update_tabbar(void)
{
    int n = gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook));
#if ALWAYS_SHOW_TABS
    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), TRUE);
#else
    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), n > 1);
#endif
}

static void close_tab(int n);

static void
on_close_btn_clicked(GtkButton *btn, gpointer data)
{
    (void)btn;
    GtkWidget *vte = GTK_WIDGET(data);
    int total = gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook));
    for (int i = 0; i < total; i++) {
        Tab *tab = tab_at(i);
        if (tab && tab->vte == vte) { close_tab(i); return; }
    }
}

static void
close_tab(int n)
{
    if (gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)) == 1) {
        gtk_main_quit();
        return;
    }
    gtk_notebook_remove_page(GTK_NOTEBOOK(notebook), n);
    maybe_update_tabbar();
}

static void
on_child_exit(VteTerminal *t, int status, gpointer data)
{
    (void)status; (void)data;
    if (opt_hold) return;

    int total = gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook));
    for (int i = 0; i < total; i++) {
        Tab *tab = tab_at(i);
        if (tab && tab->vte == GTK_WIDGET(t)) { close_tab(i); return; }
    }
}

static void new_tab(void);

static void
new_tab(void)
{
    Tab *tab   = g_new0(Tab, 1);
    tab->vte   = vte_terminal_new();
    tab->label = gtk_label_new(opt_title ? opt_title : APP_NAME);

    vte_terminal_set_cursor_shape(VTE_TERMINAL(tab->vte), CURSOR_SHAPE);
    vte_terminal_set_cursor_blink_mode(VTE_TERMINAL(tab->vte), CURSOR_BLINK);
    vte_terminal_set_scrollback_lines(VTE_TERMINAL(tab->vte), SCROLLBACK_LINES);
    vte_terminal_set_audible_bell(VTE_TERMINAL(tab->vte), FALSE);
    vte_terminal_set_mouse_autohide(VTE_TERMINAL(tab->vte), TRUE);
    vte_terminal_set_allow_hyperlink(VTE_TERMINAL(tab->vte), TRUE);
    vte_terminal_set_bold_is_bright(VTE_TERMINAL(tab->vte), BOLD_IS_BRIGHT);
    vte_terminal_set_word_char_exceptions(VTE_TERMINAL(tab->vte),
        WORD_CHAR_EXCEPTIONS);

    apply_font_to(tab->vte);
    apply_padding_to(tab->vte);
    apply_colors_to(tab->vte);
    setup_url_match(tab);

    char *cwd = NULL;
    Tab  *cur = current_tab();
    if (cur) cwd = get_cwd_of(cur->vte);
    if (!cwd && opt_dir) cwd = g_strdup(opt_dir);

    gboolean is_first = (gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)) == 0);
    spawn_in(tab->vte, cwd,
             (is_first && opt_exec) ? opt_exec : NULL,
             is_first ? opt_hold : FALSE);
    g_free(cwd);

    /* tab label: text + close button */
    GtkWidget *tab_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_box_pack_start(GTK_BOX(tab_box), tab->label, TRUE, TRUE, 0);

    tab->close_btn = gtk_button_new();
    gtk_button_set_relief(GTK_BUTTON(tab->close_btn), GTK_RELIEF_NONE);
    gtk_style_context_add_class(
        gtk_widget_get_style_context(tab->close_btn), "tab-close");
    gtk_container_add(GTK_CONTAINER(tab->close_btn),
        gtk_image_new_from_icon_name("window-close-symbolic",
                                     GTK_ICON_SIZE_MENU));
    gtk_box_pack_end(GTK_BOX(tab_box), tab->close_btn, FALSE, FALSE, 0);
    gtk_widget_show_all(tab_box);

    g_object_set_data_full(G_OBJECT(tab->vte), "tab", tab, g_free);

    g_signal_connect(tab->vte, "child-exited",
                     G_CALLBACK(on_child_exit),    NULL);
    g_signal_connect(tab->vte, "window-title-changed",
                     G_CALLBACK(on_title_changed), tab);
    g_signal_connect(tab->vte, "button-press-event",
                     G_CALLBACK(on_button_press),  tab);
    g_signal_connect(tab->close_btn, "clicked",
                     G_CALLBACK(on_close_btn_clicked), tab->vte);

    int page = gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                                        tab->vte, tab_box);
    gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(notebook), tab->vte, TRUE);
    gtk_widget_show_all(notebook);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), page);
    gtk_widget_grab_focus(tab->vte);
    maybe_update_tabbar();
}


/* ── keyboard shortcuts ──────────────────────────────────────────────── */

static gboolean
on_key_press(GtkWidget *widget, GdkEventKey *ev, gpointer data)
{
    (void)widget; (void)data;

    Tab     *tab   = current_tab();
    gboolean ctrl  = ev->state & GDK_CONTROL_MASK;
    gboolean shift = ev->state & GDK_SHIFT_MASK;
    gboolean alt   = ev->state & GDK_MOD1_MASK;
    guint    key   = ev->keyval;

    if (ctrl && shift && key == GDK_KEY_c) {
        if (tab) vte_terminal_copy_clipboard_format(
            VTE_TERMINAL(tab->vte), VTE_FORMAT_TEXT);
        return TRUE;
    }
    if (ctrl && shift && key == GDK_KEY_v) {
        if (tab) vte_terminal_paste_clipboard(VTE_TERMINAL(tab->vte));
        return TRUE;
    }

    if (ctrl && shift && key == GDK_KEY_t) { new_tab(); return TRUE; }
    if (ctrl && shift && key == GDK_KEY_w) {
        close_tab(gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)));
        return TRUE;
    }
    if (ctrl && key == GDK_KEY_Page_Up)   {
        gtk_notebook_prev_page(GTK_NOTEBOOK(notebook)); return TRUE;
    }
    if (ctrl && key == GDK_KEY_Page_Down) {
        gtk_notebook_next_page(GTK_NOTEBOOK(notebook)); return TRUE;
    }

    if (alt && key >= GDK_KEY_1 && key <= GDK_KEY_9) {
        int target = key - GDK_KEY_1;
        if (target < gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)))
            gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), target);
        return TRUE;
    }

    if (tab) {
        VteTerminal *t = VTE_TERMINAL(tab->vte);

        if (ctrl && (key == GDK_KEY_equal || key == GDK_KEY_plus)) {
            vte_terminal_set_font_scale(t, vte_terminal_get_font_scale(t) + 0.1);
            return TRUE;
        }
        if (ctrl && key == GDK_KEY_minus) {
            double s = vte_terminal_get_font_scale(t) - 0.1;
            if (s > 0.2) vte_terminal_set_font_scale(t, s);
            return TRUE;
        }
        if (ctrl && key == GDK_KEY_0) {
            vte_terminal_set_font_scale(t, FONT_SCALE);
            return TRUE;
        }

        /* half-page scroll */
        if (shift && key == GDK_KEY_Page_Up) {
            GtkAdjustment *a = gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(t));
            gtk_adjustment_set_value(a,
                gtk_adjustment_get_value(a) -
                gtk_adjustment_get_page_size(a) * 0.5);
            return TRUE;
        }
        if (shift && key == GDK_KEY_Page_Down) {
            GtkAdjustment *a = gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(t));
            gtk_adjustment_set_value(a,
                gtk_adjustment_get_value(a) +
                gtk_adjustment_get_page_size(a) * 0.5);
            return TRUE;
        }
        if (ctrl && shift && key == GDK_KEY_Up) {
            GtkAdjustment *a = gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(t));
            gtk_adjustment_set_value(a, gtk_adjustment_get_value(a) - 3);
            return TRUE;
        }
        if (ctrl && shift && key == GDK_KEY_Down) {
            GtkAdjustment *a = gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(t));
            gtk_adjustment_set_value(a, gtk_adjustment_get_value(a) + 3);
            return TRUE;
        }
    }

    return FALSE;
}


/* ── CLI options ─────────────────────────────────────────────────────── */

static GOptionEntry cli_entries[] = {
    { "hold", 0, 0, G_OPTION_ARG_NONE, &opt_hold,
      "Keep window open after command exits", NULL },
    { "title", 'T', 0, G_OPTION_ARG_STRING, &opt_title,
      "Set a fixed window title", "TITLE" },
    { "working-directory", 0, 0, G_OPTION_ARG_FILENAME, &opt_dir,
      "Set starting working directory", "DIR" },
    { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_STRING_ARRAY, &opt_exec,
      NULL, "COMMAND [ARGS…]" },
    { NULL }
};


/* ── main ────────────────────────────────────────────────────────────── */

int
main(int argc, char *argv[])
{
    /* Handle legacy -e flag before GOption sees argv */
    for (int i = 1; i < argc; i++) {
        if ((strcmp(argv[i], "-e") == 0) && i + 1 < argc) {
            opt_exec = &argv[i + 1];
            /* Remove -e from argv so GOption doesn't choke */
            for (int j = i; j < argc - 1; j++) argv[j] = argv[j+1];
            argv[--argc] = NULL;
            /* clear remaining so GOption's G_OPTION_REMAINING doesn't grab them */
            break;
        }
    }

    GError         *err = NULL;
    GOptionContext *ctx = g_option_context_new("[-- COMMAND [ARGS…]]");
    g_option_context_add_main_entries(ctx, cli_entries, NULL);
    g_option_context_add_group(ctx, gtk_get_option_group(TRUE));
    if (!g_option_context_parse(ctx, &argc, &argv, &err)) {
        fprintf(stderr, "ash: %s\n", err->message);
        g_error_free(err);
        return 1;
    }
    g_option_context_free(ctx);

    gtk_init(&argc, &argv);

    win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(win), opt_title ? opt_title : APP_NAME);
    gtk_window_set_default_size(GTK_WINDOW(win), DEFAULT_WIDTH, DEFAULT_HEIGHT);
    gtk_window_set_icon_name(GTK_WINDOW(win), APP_NAME);

    setup_visual();

#if HIDE_DECORATIONS
    gtk_window_set_decorated(GTK_WINDOW(win), FALSE);
#endif

    notebook = gtk_notebook_new();
    gtk_notebook_set_show_border(GTK_NOTEBOOK(notebook), FALSE);
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), TRUE);
    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), FALSE);

    style_notebook();

    g_signal_connect(win,      "destroy",         G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(win,      "key-press-event", G_CALLBACK(on_key_press),  NULL);
    g_signal_connect(notebook, "switch-page",     G_CALLBACK(on_tab_switch), NULL);

    gtk_container_add(GTK_CONTAINER(win), notebook);

    new_tab();

    gtk_widget_show_all(win);
    gtk_main();
    return 0;
}
