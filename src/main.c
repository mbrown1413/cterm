
#include "cterm.h"

static char* config_file = NULL;
static char** extra_opts = NULL;

static GOptionEntry options[] = {
    {"config-file", 'c', 0, G_OPTION_ARG_STRING, &config_file, "Specifies config file to use", "<file>"},
    {"option", 'o', 0, G_OPTION_ARG_STRING_ARRAY, &extra_opts, "Specifies a single config option in the form of \"option=value\". Any options available in the config file can be overwritten by this. Note: Reloading the configuration will overwrite any options given here.", "\"<option> = <value>\""},
    { NULL }
};

int main(int argc, char** argv) {
    CTerm term;
    GError *gerror = NULL;
    GtkRcStyle* style;
    GOptionContext* context;
    struct sigaction ignore_children;
    char* env_cterm_rc;
    struct passwd* user;
    int n;

    /* Avoid zombies when executing external programs by explicitly setting the
       handler to SIG_IGN */
    ignore_children.sa_handler = SIG_IGN;
    ignore_children.sa_flags = 0;
    sigemptyset(&ignore_children.sa_mask);
    sigaction(SIGCHLD, &ignore_children, NULL);

    /* Option Parsing */
    context = g_option_context_new("- A very simple libvte based terminal");
    g_option_context_add_main_entries(context, options, NULL);
    g_option_context_add_group(context, gtk_get_option_group(true));
    g_option_context_parse(context, &argc, &argv, &gerror);
    if(gerror != NULL) {
        fprintf(stderr, "Error parsing arguments: %s\n", gerror->message);
        exit(1);
    }

    /* Initialize GTK */
    gtk_init(&argc, &argv);

    /* Initialize CTerm data structure */
    term.terminal_procs = g_hash_table_new(NULL, g_int_equal);
    term.window = (GtkWindow*) gtk_window_new(GTK_WINDOW_TOPLEVEL);
    term.notebook = (GtkNotebook*) gtk_notebook_new();
    term.count = 0;

    /* Set config file */
    if(config_file != NULL) {
        /* From -c option */
        term.config.file_name = config_file;
    } else if((env_cterm_rc = getenv("CTERM_RC")) != NULL) {
        /* From CTERM_RC environment variable */
        term.config.file_name = env_cterm_rc;
    } else {
        /* Default to ~/.ctermrc */
        user = getpwuid(geteuid());
        n = strlen(user->pw_dir) + strlen(CONFIG_FILE) + 2;
        term.config.file_name = malloc(sizeof(char) * n);
        snprintf(term.config.file_name, n, "%s/%s", user->pw_dir, CONFIG_FILE);
    }

    /* Load configuration options */
    cterm_init_config_defaults(&term);
    cterm_reread_config(&term, extra_opts);
    if(extra_opts != NULL) {
        g_strfreev(extra_opts);
    }

    /* Set title */
    gtk_window_set_title(term.window, "cterm");

    /* Optionally hide window from taskbar */
    if(getenv("CTERM_HIDE") != NULL) {
        gtk_window_set_skip_taskbar_hint(term.window, true);
        gtk_window_set_skip_pager_hint(term.window, true);
    }

    gtk_notebook_set_scrollable(term.notebook, FALSE);
    gtk_notebook_set_show_tabs(term.notebook, FALSE);
    gtk_notebook_set_show_border(term.notebook, FALSE);

    g_object_set(G_OBJECT(term.notebook), "show-border", FALSE, NULL);
    g_object_set(G_OBJECT(term.notebook), "homogeneous", TRUE, NULL);

    /* Disable all borders on notebook */
    style = gtk_rc_style_new();
    style->xthickness = 0;
    style->ythickness = 0;
    gtk_widget_modify_style(GTK_WIDGET(term.notebook), style);

    /* Connect signals */
    g_signal_connect(term.notebook, "switch-page", G_CALLBACK(cterm_ontabchange), &term);

    /* Build main window */
    gtk_container_add(GTK_CONTAINER(term.window), GTK_WIDGET(term.notebook));

    /* Confirm exit on window close.
       Event propagates to gtk_main_quit if cterm_onwindowclose returns FALSE. */
    g_signal_connect(term.window, "delete-event", G_CALLBACK(cterm_onwindowclose), &term);
    g_signal_connect(term.window, "delete-event", gtk_main_quit, NULL);

    /* Open initial tab */
    cterm_open_tab(&term);

    /* Resize Window */
    cterm_set_term_size(&term,
                        term.config.initial_width, term.config.initial_height,
                        term.config.width_unit, term.config.height_unit);

    /* Show window and enter main event loop */
    gtk_widget_show_all(GTK_WIDGET(term.window));
    gtk_main();

    return 0;
}
