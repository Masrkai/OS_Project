#include <gtk/gtk.h>

static void activate(GtkApplication *app, gpointer user_data) {
    GtkBuilder *builder;
    GObject *window;
    GtkCssProvider *css_provider;
    GtkStyleContext *context;

    // Create GtkBuilder instance
    builder = gtk_builder_new();

    // Load UI file
    gtk_builder_add_from_file(builder, "window.ui", NULL);

    // Get the window object from UI file
    window = gtk_builder_get_object(builder, "main_window");
    gtk_window_set_application(GTK_WINDOW(window), app);

    // Create CSS provider
    css_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_file(css_provider,
                                   g_file_new_for_path("style2.css"));

    // Apply CSS to the window
    context = gtk_widget_get_style_context(GTK_WIDGET(window));
    gtk_style_context_add_provider(context,
                                  GTK_STYLE_PROVIDER(css_provider),
                                  GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    // Apply CSS to button
    GtkWidget *button = GTK_WIDGET(gtk_builder_get_object(builder, "my_button"));
    context = gtk_widget_get_style_context(button);
    gtk_style_context_add_provider(context,
                                  GTK_STYLE_PROVIDER(css_provider),
                                  GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    // Show the window
    gtk_widget_show(GTK_WIDGET(window));

    // Clean up
    g_object_unref(builder);
    g_object_unref(css_provider);
}

int main(int argc, char **argv) {
    GtkApplication *app;
    int status;

    app = gtk_application_new("com.example.myapp", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}