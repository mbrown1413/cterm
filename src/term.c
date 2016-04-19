
#include "cterm.h"

#define FREE_IF_NOT_NULL(x) if(x != NULL) { free(x); }

CTerm* cterm_term_new(const char* config_file, char** extra_opts) {
    CTerm* term = calloc(sizeof(CTerm), 1);

    // Initialize CTerm data structure
    term->terminal_procs = g_hash_table_new(NULL, g_int_equal);
    term->window = (GtkWindow*) gtk_window_new(GTK_WINDOW_TOPLEVEL);
    term->notebook = (GtkNotebook*) gtk_notebook_new();
    term->count = 0;
    term->config.file_name = strdup(config_file);

    cterm_init_config_defaults(term);
    cterm_reread_config(term, extra_opts);

    // Set title
    if(term->config.initial_title) {
        gtk_window_set_title(term->window, term->config.initial_title);
    } else {
        gtk_window_set_title(term->window, "cterm");
    }

    // Optionally hide window from taskbar
    // TODO: Seems like this should be an option, not an env val.
    if(getenv("CTERM_HIDE") != NULL) {
        gtk_window_set_skip_taskbar_hint(term->window, true);
        gtk_window_set_skip_pager_hint(term->window, true);
    }

    gtk_notebook_set_scrollable(term->notebook, FALSE);
    gtk_notebook_set_show_tabs(term->notebook, FALSE);
    gtk_notebook_set_show_border(term->notebook, FALSE);

    g_object_set(G_OBJECT(term->notebook), "show-border", FALSE, NULL);
    g_object_set(G_OBJECT(term->notebook), "homogeneous", TRUE, NULL);

    // Disable all borders on notebook
    GtkRcStyle* style = gtk_rc_style_new();
    style->xthickness = 0;
    style->ythickness = 0;
    gtk_widget_modify_style(GTK_WIDGET(term->notebook), style);
    g_object_unref(style);

    // Connect signals
    g_signal_connect(term->notebook, "switch-page", G_CALLBACK(cterm_ontabchange), term);

    // Build main window
    gtk_container_add(GTK_CONTAINER(term->window), GTK_WIDGET(term->notebook));

    // Confirm exit on window close.
    // Event propagates to gtk_main_quit if cterm_onwindowclose returns FALSE.
    g_signal_connect(term->window, "delete-event", G_CALLBACK(cterm_onwindowclose), term);
    g_signal_connect(term->window, "delete-event", gtk_main_quit, NULL);

    // Open initial tab
    cterm_open_tab(term);

    // Resize Window
    cterm_set_term_size(term,
                        term->config.initial_width, term->config.initial_height,
                        term->config.width_unit, term->config.height_unit);

    return term;
}

void cterm_term_free(CTerm* term) {
    FREE_IF_NOT_NULL(term->config.initial_title);
    FREE_IF_NOT_NULL(term->config.font);
    FREE_IF_NOT_NULL(term->config.word_chars);
    FREE_IF_NOT_NULL(term->config.external_program);
    FREE_IF_NOT_NULL(term->config.url_program);
    free(term);
}
