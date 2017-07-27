
#include "cterm.h"

static char* config_file_opt = NULL;
static char** extra_opts = NULL;

static GOptionEntry options[] = {
    {"config-file", 'c', 0, G_OPTION_ARG_STRING, &config_file_opt, "Specifies config file to use", "<file>"},
    {"option", 'o', 0, G_OPTION_ARG_STRING_ARRAY, &extra_opts, "Specifies a single config option in the form of \"option=value\". Any options available in the config file can be overwritten by this. Note: Reloading the configuration will overwrite any options given here.", "\"<option> = <value>\""},
    { NULL }
};

int main(int argc, char** argv) {
    GError *gerror = NULL;
    struct sigaction ignore_children;
    char* env_cterm_rc;
    struct passwd* user;
    char* initial_directory = NULL;
    int n;

    // Avoid zombies when executing external programs by explicitly setting the
    // handler to SIG_IGN
    ignore_children.sa_handler = SIG_IGN;
    ignore_children.sa_flags = 0;
    sigemptyset(&ignore_children.sa_mask);
    sigaction(SIGCHLD, &ignore_children, NULL);

    // Option Parsing
    GOptionContext* context = g_option_context_new("- A very simple libvte based terminal");
    g_option_context_add_main_entries(context, options, NULL);
    g_option_context_add_group(context, gtk_get_option_group(true));
    g_option_context_parse(context, &argc, &argv, &gerror);
    if(gerror != NULL) {
        fprintf(stderr, "Error parsing arguments: %s\n", gerror->message);
        exit(1);
    }
    if(argc == 2) {
        initial_directory = argv[1];
    } else if(argc != 1) {
        fprintf(stderr, "Error: Too many positional arguments\n");
        exit(1);
    }
    g_option_context_free(context);

    // Initialize GTK
    gtk_init(&argc, &argv);

    // Get config file
    char* config_file;
    if(config_file_opt != NULL) {
        // From -c option
        config_file = config_file_opt;
    } else if((env_cterm_rc = getenv("CTERM_RC")) != NULL) {
        // From CTERM_RC environment variable
        config_file = env_cterm_rc;
    } else {
        // Default to ~/.ctermrc
        user = getpwuid(geteuid());
        n = strlen(user->pw_dir) + strlen(CONFIG_FILE) + 2;
        config_file = malloc(sizeof(char) * n);
        snprintf(config_file, n, "%s/%s", user->pw_dir, CONFIG_FILE);
    }

    // Initialize CTerm data structure
    CTerm* term = cterm_term_new(config_file, extra_opts, initial_directory);

    // Free config strings just used
    free(config_file);
    if(extra_opts != NULL) {
        g_strfreev(extra_opts);
    }

    // Show window and enter main event loop
    gtk_widget_show_all(GTK_WIDGET(term->window));
    gtk_main();

    cterm_term_free(term);

    return 0;
}
