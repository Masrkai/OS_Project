#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <glib.h>

// Assuming you run the GUI from the 'UI' folder and binaries are in '../build/release/'
// Adjust this path if your directory structure differs.
#define EXECUTABLE_PATH "../build/release/"


typedef struct {
    GtkWindow *window;
    GtkSpinButton *process_count_spin;
    GtkButton *generate_button;
    GtkComboBoxText *algorithm_combo;
    GtkButton *run_button;
    GtkTextView *output_textview;
    GtkLabel *status_label;
    gboolean processes_generated;
} AppWidgets;

// Callback data structure for widget sensitivity
typedef struct {
    GtkWidget *widget;
    gboolean sensitive;
} WidgetSensitivityData;








static void append_to_output(GtkTextView *textview, const char *text) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(textview);
    GtkTextIter iter;

    gtk_text_buffer_get_end_iter(buffer, &iter);
    gtk_text_buffer_insert(buffer, &iter, text, -1);

    // Scroll to bottom
    GtkTextMark *mark = gtk_text_buffer_get_insert(buffer);
    gtk_text_view_scroll_to_mark(textview, mark, 0.0, TRUE, 0.0, 1.0);
}


static gboolean read_child_output(GIOChannel *channel, GIOCondition condition, gpointer data) {
    GtkTextView *textview = GTK_TEXT_VIEW(data);
    GError *error = NULL;
    gchar buf[1024];
    gsize bytes_read;

    if (condition & G_IO_IN) {
        // Read raw characters instead of lines
        GIOStatus status = g_io_channel_read_chars(channel, buf, sizeof(buf) - 1, &bytes_read, &error);

        if (status == G_IO_STATUS_NORMAL) {
            buf[bytes_read] = '\0'; // Null terminate the buffer
            append_to_output(textview, buf);
            return TRUE;
        } else if (status == G_IO_STATUS_EOF) {
            return FALSE; // Stop the watch
        } else if (status == G_IO_STATUS_AGAIN) {
            return TRUE; // Resource temporarily unavailable, keep waiting
        }
    }

    if (error) {
        g_warning("Error reading from child: %s", error->message);
        g_error_free(error);
    }

    return FALSE;
}


static gboolean set_widget_sensitive_cb(gpointer data) {
    WidgetSensitivityData *sensitivity_data = (WidgetSensitivityData *)data;
    gtk_widget_set_sensitive(sensitivity_data->widget, sensitivity_data->sensitive);
    g_free(sensitivity_data);
    return FALSE; // Remove the timeout source
}

static void run_command_with_input(AppWidgets *app, const char *command, const char *input) {
    int stdin_pipe[2];
    int stdout_pipe[2];
    pid_t pid;

    if (pipe(stdin_pipe) == -1 || pipe(stdout_pipe) == -1) {
        perror("pipe");
        return;
    }

    pid = fork();

    if (pid == -1) {
        perror("fork");
        return;
    }

    if (pid == 0) {
        // Child
        close(stdin_pipe[1]);
        close(stdout_pipe[0]);

        dup2(stdin_pipe[0], STDIN_FILENO);
        dup2(stdout_pipe[1], STDOUT_FILENO);
        dup2(stdout_pipe[1], STDERR_FILENO);

        // Force line buffering for the child's stdout so it flushes to pipe immediately
        setvbuf(stdout, NULL, _IOLBF, 0);

        close(stdin_pipe[0]);
        close(stdout_pipe[1]);

        execl(command, command, NULL);
        perror("execl");
        exit(EXIT_FAILURE);
    } else {
        // Parent
        close(stdin_pipe[0]);
        close(stdout_pipe[1]);

        if (input) {
            if (write(stdin_pipe[1], input, strlen(input)) == -1) {
                perror("write input");
            }
        }
        close(stdin_pipe[1]);

        // Set non-blocking
        int flags = fcntl(stdout_pipe[0], F_GETFL, 0);
        fcntl(stdout_pipe[0], F_SETFL, flags | O_NONBLOCK);

        GIOChannel *channel = g_io_channel_unix_new(stdout_pipe[0]);
        if (channel) {
            // BINARY encoding avoids "invalid byte sequence" errors if the child outputs weird data
            // but we assume the child outputs valid ASCII/UTF-8 for the TextView.
            g_io_channel_set_encoding(channel, NULL, NULL);
            g_io_channel_set_buffered(channel, FALSE); // Unbuffered for real-time feel

            g_io_add_watch(channel, G_IO_IN | G_IO_HUP | G_IO_ERR,
                          (GIOFunc)read_child_output, app->output_textview);
            g_io_channel_unref(channel);
        }

        GThread *thread = g_thread_new("waitpid_thread", (GThreadFunc)waitpid, (gpointer)(intptr_t)pid);
        g_thread_unref(thread);
    }
}

static void on_generate_clicked(GtkButton *button, gpointer user_data) {
    AppWidgets *app = (AppWidgets *)user_data;

    // Get process count
    gint process_count = gtk_spin_button_get_value_as_int(app->process_count_spin);
    char input[16];
    snprintf(input, sizeof(input), "%d\n", process_count);

    // Update status
    gtk_label_set_label(app->status_label, "Generating processes...");
    gtk_widget_set_sensitive(GTK_WIDGET(app->generate_button), FALSE);

    // Run the test_generator
    char command[512];
    snprintf(command, sizeof(command), "%s%s", EXECUTABLE_PATH, "test_generator.out");

    run_command_with_input(app, command, input);

    // Enable run button after a short delay (simulation of process generation time)
    WidgetSensitivityData *sensitivity_data = g_new(WidgetSensitivityData, 1);
    sensitivity_data->widget = GTK_WIDGET(app->run_button);
    sensitivity_data->sensitive = TRUE;
    g_timeout_add(1000, set_widget_sensitive_cb, sensitivity_data);

    // Re-enable generate button
    WidgetSensitivityData *gen_data = g_new(WidgetSensitivityData, 1);
    gen_data->widget = GTK_WIDGET(app->generate_button);
    gen_data->sensitive = TRUE;
    g_timeout_add(1000, set_widget_sensitive_cb, gen_data);

    app->processes_generated = TRUE;
}

static void on_run_clicked(GtkButton *button, gpointer user_data) {
    AppWidgets *app = (AppWidgets *)user_data;

    if (!app->processes_generated) {
        gtk_label_set_label(app->status_label, "Please generate processes first!");
        return;
    }

    // Get selected algorithm ID.
    // Note: Cast to GtkComboBox is required as _get_active_id is a base class method
    const gchar *algorithm_id = gtk_combo_box_get_active_id(GTK_COMBO_BOX(app->algorithm_combo));

    char input[16];
    // Default to "1" if nothing selected, though UI defaults to 1
    snprintf(input, sizeof(input), "%s\n", algorithm_id ? algorithm_id : "1");

    // Update status
    gtk_label_set_label(app->status_label, "Running scheduler...");
    gtk_widget_set_sensitive(GTK_WIDGET(app->run_button), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(app->generate_button), FALSE);

    // Run the process_generator
    char command[512];
    snprintf(command, sizeof(command), "%s%s", EXECUTABLE_PATH, "process_generator.out");

    // Append a clear line to output
    append_to_output(app->output_textview, "\n--- Starting Simulation ---\n");

    run_command_with_input(app, command, input);

    // Re-enable buttons after a delay.
    // In a production app, use g_child_watch_add to detect exactly when it finishes.
    WidgetSensitivityData *gen_data = g_new(WidgetSensitivityData, 1);
    gen_data->widget = GTK_WIDGET(app->generate_button);
    gen_data->sensitive = TRUE;
    g_timeout_add(5000, set_widget_sensitive_cb, gen_data);

    WidgetSensitivityData *run_data = g_new(WidgetSensitivityData, 1);
    run_data->widget = GTK_WIDGET(app->run_button);
    run_data->sensitive = TRUE;
    g_timeout_add(5000, set_widget_sensitive_cb, run_data);
}

static void on_clear_output_clicked(GtkButton *button, gpointer user_data) {
    AppWidgets *app = (AppWidgets *)user_data;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(app->output_textview);
    gtk_text_buffer_set_text(buffer, "", 0);
}

static void on_quit_clicked(GtkButton *button, gpointer user_data) {
    GtkApplication *app = GTK_APPLICATION(user_data);
    GtkWindow *window = GTK_WINDOW(gtk_application_get_active_window(app));
    if (window) {
        gtk_window_close(window);
    }
}

static void on_activate(GApplication *app, gpointer user_data) {
    GtkBuilder *builder;
    GError *error = NULL;
    AppWidgets *widgets = g_new0(AppWidgets, 1);

    builder = gtk_builder_new();

    // Load the UI file
    if (!gtk_builder_add_from_file(builder, "window2.ui", &error)) {
        g_warning("Couldn't load UI file: %s", error->message);
        g_error_free(error);
        g_free(widgets);
        return;
    }

    // Get widgets from the builder
    widgets->window = GTK_WINDOW(gtk_builder_get_object(builder, "main_window"));
    widgets->process_count_spin = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "process_count_spin"));
    widgets->generate_button = GTK_BUTTON(gtk_builder_get_object(builder, "generate_button"));
    widgets->algorithm_combo = GTK_COMBO_BOX_TEXT(gtk_builder_get_object(builder, "algorithm_combo"));
    widgets->run_button = GTK_BUTTON(gtk_builder_get_object(builder, "run_button"));
    widgets->output_textview = GTK_TEXT_VIEW(gtk_builder_get_object(builder, "output_textview"));
    widgets->status_label = GTK_LABEL(gtk_builder_get_object(builder, "status_label"));

    // Get additional buttons
    GtkButton *clear_button = GTK_BUTTON(gtk_builder_get_object(builder, "clear_button"));
    GtkButton *quit_button = GTK_BUTTON(gtk_builder_get_object(builder, "quit_button"));

    // Connect signals
    g_signal_connect(widgets->generate_button, "clicked",
                     G_CALLBACK(on_generate_clicked), widgets);

    g_signal_connect(widgets->run_button, "clicked",
                     G_CALLBACK(on_run_clicked), widgets);

    g_signal_connect(clear_button, "clicked",
                     G_CALLBACK(on_clear_output_clicked), widgets);

    g_signal_connect(quit_button, "clicked",
                     G_CALLBACK(on_quit_clicked), app);

    // Set initial state
    widgets->processes_generated = FALSE;
    // Keep run button insensitive until generated
    gtk_widget_set_sensitive(GTK_WIDGET(widgets->run_button), FALSE);

    // Set window application
    gtk_window_set_application(widgets->window, GTK_APPLICATION(app));

    // Show the window
    gtk_window_present(widgets->window);

    // Store widgets in application data
    g_object_set_data_full(G_OBJECT(app), "widgets", widgets, g_free);

    g_object_unref(builder);
}

static void on_startup(GApplication *app, gpointer user_data) {
    // Add CSS provider for styling
    GtkCssProvider *provider = gtk_css_provider_new();

    // Use load_from_string for newer GTK4 versions (deprecated load_from_data removed/flagged)
    gtk_css_provider_load_from_string(provider,
        ".dim-label { color: #666666; font-style: italic; }\n"
        "textview { font-family: monospace; font-size: 12px; }");

    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    g_object_unref(provider);
}

int main(int argc, char **argv) {
    GtkApplication *app;
    int status;

    // Use DEFAULT_FLAGS instead of FLAGS_NONE (deprecated)
    app = gtk_application_new("com.example.processscheduler", G_APPLICATION_DEFAULT_FLAGS);

    g_signal_connect(app, "startup", G_CALLBACK(on_startup), NULL);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);

    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}