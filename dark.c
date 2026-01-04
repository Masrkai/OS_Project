#include <gtk/gtk.h>
#include "resources/style_embedded.h"

static void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window;
    GtkWidget *header_bar;
    GtkCssProvider *css_provider;

    // Create window
    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Elegant Scheduler");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 800);

    // Create header bar
    header_bar = gtk_header_bar_new();
    gtk_header_bar_set_show_title_buttons(GTK_HEADER_BAR(header_bar), TRUE);
    gtk_window_set_titlebar(GTK_WINDOW(window), header_bar);

    // Apply dark theme CSS from embedded data
    css_provider = gtk_css_provider_new();
    gchar *css_string = g_strndup((const gchar *)style_css, style_css_len);
    gtk_css_provider_load_from_string(css_provider, css_string);
    g_free(css_string); // cleanup

    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(css_provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );

    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv) {
    GtkApplication *app;
    int status;

    g_setenv("GSK_RENDERER", "opengl", TRUE);

    app = gtk_application_new("com.example.elegantdark", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}