#include <gtk/gtk.h>

static void activate(GtkApplication *app, gpointer user_data) {
    GtkBuilder *builder = gtk_builder_new_from_file("window.ui");
    GObject *window_obj = gtk_builder_get_object(builder, "main_window");
    GtkWidget *window = GTK_WIDGET(window_obj);
    
    // Essential: Link window to application
    gtk_window_set_application(GTK_WINDOW(window), app);

    GtkCssProvider *css_provider = gtk_css_provider_new();
    GFile *css_file = g_file_new_for_path("styles.css");
    gtk_css_provider_load_from_file(css_provider, css_file);
    g_object_unref(css_file);


    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(css_provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
    
    gtk_window_present(GTK_WINDOW(window));
    g_object_unref(builder);
}


int main(int argc, char **argv) {
    GtkApplication *app;
    int status;

    g_setenv("GSK_RENDERER", "opengl", TRUE);

    app = gtk_application_new("com.scheduler.viewer", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}