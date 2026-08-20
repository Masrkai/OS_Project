#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

/* Global widgets */
static GtkWidget *window;
static GtkWidget *algorithm_combo;
static GtkWidget *quantum_entry;
static GtkWidget *quantum_label;
static GtkWidget *process_count_entry;
static GtkWidget *text_view;
static GtkWidget *start_button;
static GtkWidget *stop_button;
static GtkWidget *generate_button;
static GtkTextBuffer *text_buffer;
static GtkWidget *progress_bar;
static GtkWidget *cpu_util_label;
static GtkWidget *avg_wta_label;
static GtkWidget *avg_wait_label;
static GtkWidget *std_wta_label;

/* Process tracking */
static pid_t generator_pid = -1;
static gboolean simulation_running = FALSE;
static guint update_timer = 0;

/* Function declarations */
void append_log(const char *text);
void update_performance_metrics(void);
gboolean monitor_log_file(gpointer data);
void on_generate_clicked(GtkButton *button, gpointer data);
void on_start_clicked(GtkButton *button, gpointer data);
void on_stop_clicked(GtkButton *button, gpointer data);
void on_algorithm_changed(GtkComboBox *combo, gpointer data);
void build_ui(void);

/* Append text to the log view */
void append_log(const char *text) {
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(text_buffer, &end);
    gtk_text_buffer_insert(text_buffer, &end, text, -1);
    gtk_text_buffer_insert(text_buffer, &end, "\n", -1);
    
    /* Auto-scroll to bottom */
    GtkTextMark *mark = gtk_text_buffer_get_insert(text_buffer);
    gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(text_view), mark);
}

/* Read and display performance metrics */
void update_performance_metrics(void) {
    FILE *perf = fopen("scheduler.perf", "r");
    if (!perf) return;
    
    char line[256];
    double cpu_util = 0, avg_wta = 0, avg_wait = 0, std_wta = 0;
    
    while (fgets(line, sizeof(line), perf)) {
        if (sscanf(line, "CPU utilization = %lf%%", &cpu_util) == 1) {
            char buf[64];
            snprintf(buf, sizeof(buf), "%.2f%%", cpu_util);
            gtk_label_set_text(GTK_LABEL(cpu_util_label), buf);
        } else if (sscanf(line, "Avg WTA = %lf", &avg_wta) == 1) {
            char buf[64];
            snprintf(buf, sizeof(buf), "%.2f", avg_wta);
            gtk_label_set_text(GTK_LABEL(avg_wta_label), buf);
        } else if (sscanf(line, "Avg Waiting = %lf", &avg_wait) == 1) {
            char buf[64];
            snprintf(buf, sizeof(buf), "%.2f", avg_wait);
            gtk_label_set_text(GTK_LABEL(avg_wait_label), buf);
        } else if (sscanf(line, "Std WTA = %lf", &std_wta) == 1) {
            char buf[64];
            snprintf(buf, sizeof(buf), "%.2f", std_wta);
            gtk_label_set_text(GTK_LABEL(std_wta_label), buf);
        }
    }
    
    fclose(perf);
}

/* Monitor log file and update display */
gboolean monitor_log_file(gpointer data) {
    static long last_pos = 0;
    FILE *log = fopen("scheduler.log", "r");
    
    (void)data; /* Suppress unused parameter warning */
    
    if (log) {
        fseek(log, last_pos, SEEK_SET);
        char line[512];
        
        while (fgets(line, sizeof(line), log)) {
            /* Remove newline */
            line[strcspn(line, "\n")] = 0;
            if (strlen(line) > 0) {
                append_log(line);
            }
        }
        
        last_pos = ftell(log);
        fclose(log);
    }
    
    /* Update performance metrics */
    update_performance_metrics();
    
    /* Check if simulation is still running */
    if (generator_pid > 0) {
        int status;
        pid_t result = waitpid(generator_pid, &status, WNOHANG);
        if (result != 0) {
            /* Process finished */
            append_log("\n=== Simulation Complete ===");
            simulation_running = FALSE;
            generator_pid = -1;
            
            gtk_widget_set_sensitive(start_button, TRUE);
            gtk_widget_set_sensitive(stop_button, FALSE);
            gtk_widget_set_sensitive(generate_button, TRUE);
            gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), 1.0);
            gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress_bar), "Complete");
            
            update_performance_metrics();
            return G_SOURCE_REMOVE;
        } else {
            /* Still running - pulse progress bar */
            gtk_progress_bar_pulse(GTK_PROGRESS_BAR(progress_bar));
        }
    }
    
    return G_SOURCE_CONTINUE;
}

/* Generate processes file */
void on_generate_clicked(GtkButton *button, gpointer data) {
    const char *count_text;
    int count;
    char cwd[1024];
    
    (void)button; /* Suppress unused parameter warning */
    (void)data;
    
    count_text = gtk_entry_get_text(GTK_ENTRY(process_count_entry));
    count = atoi(count_text);
    
    if (count <= 0 || count > 100) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window),
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_OK,
            "Please enter a valid process count (1-100)");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    
    /* Get current working directory */
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd");
        return;
    }
    
    /* Check if test_generator.out exists */
    char test_gen_path[1024];
    snprintf(test_gen_path, sizeof(test_gen_path), "%s/build/release/test_generator.out", cwd);
    
    if (access(test_gen_path, X_OK) != 0) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window),
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_OK,
            "test_generator.out not found!\n\nPath checked: %s\n\nPlease run 'make release' first.",
            test_gen_path);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    
    /* Create input file for test generator */
    FILE *input = fopen(".gen_input", "w");
    if (input) {
        fprintf(input, "%d\n", count);
        fclose(input);
    }
    
    /* Run test generator with absolute path */
    char cmd[2048];
    snprintf(cmd, sizeof(cmd), "\"%s\" < .gen_input", test_gen_path);
    int result = system(cmd);
    
    if (result == 0) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Generated %d processes in processes.txt", count);
        append_log(msg);
        
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window),
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_INFO,
            GTK_BUTTONS_OK,
            "Successfully generated %d processes!", count);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    } else {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window),
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_OK,
            "Failed to generate processes.\nExit code: %d", result);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }
    
    unlink(".gen_input");
}

/* Start simulation */
void on_start_clicked(GtkButton *button, gpointer data) {
    int algorithm;
    int quantum = 0;
    const char *quantum_text;
    char cwd[1024];
    char process_gen_path[1024];
    
    (void)button; /* Suppress unused parameter warning */
    (void)data;
    
    /* Get current working directory */
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd");
        return;
    }
    
    /* Build path to process_generator.out */
    snprintf(process_gen_path, sizeof(process_gen_path), 
             "%s/build/release/process_generator.out", cwd);
    
    /* Get algorithm selection */
    algorithm = gtk_combo_box_get_active(GTK_COMBO_BOX(algorithm_combo)) + 1;
    
    /* Get quantum if needed */
    if (algorithm == 3 || algorithm == 4) {
        quantum_text = gtk_entry_get_text(GTK_ENTRY(quantum_entry));
        quantum = atoi(quantum_text);
        
        if (algorithm == 3 && quantum <= 0) {
            GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window),
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_OK,
                "Please enter a valid quantum value for Round Robin");
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            return;
        }
    }
    
    /* Check if process_generator.out exists */
    if (access(process_gen_path, X_OK) != 0) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window),
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_OK,
            "process_generator.out not found!\n\nPath: %s\n\nPlease run 'make release' first.",
            process_gen_path);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    
    /* Check if processes.txt exists */
    if (access("processes.txt", F_OK) != 0) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window),
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_OK,
            "processes.txt not found! Please generate processes first.");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    
    /* Clear log */
    gtk_text_buffer_set_text(text_buffer, "", -1);
    gtk_label_set_text(GTK_LABEL(cpu_util_label), "0.00%");
    gtk_label_set_text(GTK_LABEL(avg_wta_label), "0.00");
    gtk_label_set_text(GTK_LABEL(avg_wait_label), "0.00");
    gtk_label_set_text(GTK_LABEL(std_wta_label), "0.00");
    
    /* Remove old log files */
    unlink("scheduler.log");
    unlink("scheduler.perf");
    
    {
        const char *algo_names[] = {"", "HPF", "SJN", "Round Robin", "MLFQ"};
        char start_msg[256];
        if (algorithm == 3) {
            snprintf(start_msg, sizeof(start_msg), 
                    "Starting simulation with %s (Quantum=%d)...", 
                    algo_names[algorithm], quantum);
        } else if (algorithm == 4) {
            snprintf(start_msg, sizeof(start_msg), 
                    "Starting simulation with %s (Adaptive Quantums)...", 
                    algo_names[algorithm]);
        } else {
            snprintf(start_msg, sizeof(start_msg), 
                    "Starting simulation with %s...", 
                    algo_names[algorithm]);
        }
        append_log(start_msg);
    }
    
    /* Create input file for process generator */
    {
        FILE *input = fopen(".pg_input", "w");
        if (input) {
            fprintf(input, "%d\n", algorithm);
            if (algorithm == 3) {
                fprintf(input, "%d\n", quantum);
            }
            fclose(input);
        }
    }
    
    /* Fork and run process generator */
    generator_pid = fork();
    if (generator_pid == 0) {
        /* Child process */
        int fd = open(".pg_input", O_RDONLY);
        dup2(fd, STDIN_FILENO);
        close(fd);
        
        /* Execute with absolute path */
        execl(process_gen_path, "process_generator.out", NULL);
        perror("execl failed");
        exit(1);
    } else if (generator_pid > 0) {
        /* Parent process */
        simulation_running = TRUE;
        
        gtk_widget_set_sensitive(start_button, FALSE);
        gtk_widget_set_sensitive(stop_button, TRUE);
        gtk_widget_set_sensitive(generate_button, FALSE);
        
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), 0.0);
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress_bar), "Running...");
        
        /* Start monitoring */
        update_timer = g_timeout_add(500, monitor_log_file, NULL);
    }
}

/* Stop simulation */
void on_stop_clicked(GtkButton *button, gpointer data) {
    (void)button; /* Suppress unused parameter warning */
    (void)data;
    
    if (generator_pid > 0) {
        append_log("\nStopping simulation...");
        kill(generator_pid, SIGINT);
        
        /* Wait for cleanup */
        sleep(1);
        
        /* Force kill if still running */
        kill(generator_pid, SIGKILL);
        waitpid(generator_pid, NULL, 0);
        
        generator_pid = -1;
        simulation_running = FALSE;
        
        gtk_widget_set_sensitive(start_button, TRUE);
        gtk_widget_set_sensitive(stop_button, FALSE);
        gtk_widget_set_sensitive(generate_button, TRUE);
        
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), 0.0);
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress_bar), "Stopped");
        
        if (update_timer) {
            g_source_remove(update_timer);
            update_timer = 0;
        }
    }
}

/* Algorithm selection changed */
void on_algorithm_changed(GtkComboBox *combo, gpointer data) {
    int algorithm;
    
    (void)data; /* Suppress unused parameter warning */
    
    algorithm = gtk_combo_box_get_active(combo) + 1;
    
    /* Show/hide quantum input based on algorithm */
    if (algorithm == 3) { /* Round Robin */
        gtk_widget_set_visible(quantum_label, TRUE);
        gtk_widget_set_visible(quantum_entry, TRUE);
        gtk_entry_set_text(GTK_ENTRY(quantum_entry), "2");
    } else if (algorithm == 4) { /* MLFQ */
        gtk_widget_set_visible(quantum_label, TRUE);
        gtk_widget_set_visible(quantum_entry, FALSE);
        gtk_label_set_text(GTK_LABEL(quantum_label), "Quantum: Adaptive (Q0=2, Q1=4, Q2=8)");
    } else {
        gtk_widget_set_visible(quantum_label, FALSE);
        gtk_widget_set_visible(quantum_entry, FALSE);
    }
}

/* Main window */
void build_ui(void) {
    GtkWidget *main_box;
    GtkWidget *title;
    GtkWidget *config_frame, *config_box;
    GtkWidget *gen_box, *count_label;
    GtkWidget *algo_box, *algo_label;
    GtkWidget *quantum_box;
    GtkWidget *button_box;
    GtkWidget *perf_frame, *perf_grid;
    GtkWidget *log_frame, *scrolled;
    PangoFontDescription *font;
    
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "PrismScheduler - OS Process Scheduler Simulator");
    gtk_window_set_default_size(GTK_WINDOW(window), 900, 700);
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    
    main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(window), main_box);
    
    /* Title */
    title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), 
        "<span size='x-large' weight='bold'>PrismScheduler - OS Process Scheduler Simulator</span>");
    gtk_box_pack_start(GTK_BOX(main_box), title, FALSE, FALSE, 5);
    
    /* Configuration frame */
    config_frame = gtk_frame_new("Configuration");
    gtk_box_pack_start(GTK_BOX(main_box), config_frame, FALSE, FALSE, 0);
    
    config_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(config_box), 10);
    gtk_container_add(GTK_CONTAINER(config_frame), config_box);
    
    /* Process generation */
    gen_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(config_box), gen_box, FALSE, FALSE, 0);
    
    count_label = gtk_label_new("Number of Processes:");
    gtk_box_pack_start(GTK_BOX(gen_box), count_label, FALSE, FALSE, 0);
    
    process_count_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(process_count_entry), "10");
    gtk_entry_set_max_length(GTK_ENTRY(process_count_entry), 3);
    gtk_entry_set_width_chars(GTK_ENTRY(process_count_entry), 5);
    gtk_box_pack_start(GTK_BOX(gen_box), process_count_entry, FALSE, FALSE, 0);
    
    generate_button = gtk_button_new_with_label("Generate Processes");
    g_signal_connect(generate_button, "clicked", G_CALLBACK(on_generate_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(gen_box), generate_button, FALSE, FALSE, 0);
    
    /* Algorithm selection */
    algo_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(config_box), algo_box, FALSE, FALSE, 0);
    
    algo_label = gtk_label_new("Scheduling Algorithm:");
    gtk_box_pack_start(GTK_BOX(algo_box), algo_label, FALSE, FALSE, 0);
    
    algorithm_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(algorithm_combo), 
        "Preemptive Highest Priority First (HPF)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(algorithm_combo), 
        "Shortest Job Next (SJN)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(algorithm_combo), 
        "Round Robin (RR)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(algorithm_combo), 
        "Multi-Level Feedback Queue (MLFQ)");
    gtk_combo_box_set_active(GTK_COMBO_BOX(algorithm_combo), 0);
    g_signal_connect(algorithm_combo, "changed", G_CALLBACK(on_algorithm_changed), NULL);
    gtk_box_pack_start(GTK_BOX(algo_box), algorithm_combo, TRUE, TRUE, 0);
    
    /* Quantum input */
    quantum_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(config_box), quantum_box, FALSE, FALSE, 0);
    
    quantum_label = gtk_label_new("Time Quantum:");
    gtk_box_pack_start(GTK_BOX(quantum_box), quantum_label, FALSE, FALSE, 0);
    
    quantum_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(quantum_entry), "2");
    gtk_entry_set_max_length(GTK_ENTRY(quantum_entry), 3);
    gtk_entry_set_width_chars(GTK_ENTRY(quantum_entry), 5);
    gtk_box_pack_start(GTK_BOX(quantum_box), quantum_entry, FALSE, FALSE, 0);
    
    gtk_widget_set_visible(quantum_label, FALSE);
    gtk_widget_set_visible(quantum_entry, FALSE);
    
    /* Control buttons */
    button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(config_box), button_box, FALSE, FALSE, 5);
    
    start_button = gtk_button_new_with_label("▶ Start Simulation");
    gtk_widget_set_size_request(start_button, 150, 40);
    g_signal_connect(start_button, "clicked", G_CALLBACK(on_start_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(button_box), start_button, TRUE, TRUE, 0);
    
    stop_button = gtk_button_new_with_label("⏹ Stop Simulation");
    gtk_widget_set_size_request(stop_button, 150, 40);
    gtk_widget_set_sensitive(stop_button, FALSE);
    g_signal_connect(stop_button, "clicked", G_CALLBACK(on_stop_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(button_box), stop_button, TRUE, TRUE, 0);
    
    /* Progress bar */
    progress_bar = gtk_progress_bar_new();
    gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(progress_bar), TRUE);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress_bar), "Ready");
    gtk_box_pack_start(GTK_BOX(main_box), progress_bar, FALSE, FALSE, 0);
    
    /* Performance metrics frame */
    perf_frame = gtk_frame_new("Performance Metrics");
    gtk_box_pack_start(GTK_BOX(main_box), perf_frame, FALSE, FALSE, 0);
    
    perf_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(perf_grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(perf_grid), 15);
    gtk_container_set_border_width(GTK_CONTAINER(perf_grid), 10);
    gtk_container_add(GTK_CONTAINER(perf_frame), perf_grid);
    
    gtk_grid_attach(GTK_GRID(perf_grid), gtk_label_new("CPU Utilization:"), 0, 0, 1, 1);
    cpu_util_label = gtk_label_new("0.00%");
    gtk_widget_set_halign(cpu_util_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(perf_grid), cpu_util_label, 1, 0, 1, 1);
    
    gtk_grid_attach(GTK_GRID(perf_grid), gtk_label_new("Avg WTA:"), 2, 0, 1, 1);
    avg_wta_label = gtk_label_new("0.00");
    gtk_widget_set_halign(avg_wta_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(perf_grid), avg_wta_label, 3, 0, 1, 1);
    
    gtk_grid_attach(GTK_GRID(perf_grid), gtk_label_new("Avg Waiting:"), 0, 1, 1, 1);
    avg_wait_label = gtk_label_new("0.00");
    gtk_widget_set_halign(avg_wait_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(perf_grid), avg_wait_label, 1, 1, 1, 1);
    
    gtk_grid_attach(GTK_GRID(perf_grid), gtk_label_new("Std WTA:"), 2, 1, 1, 1);
    std_wta_label = gtk_label_new("0.00");
    gtk_widget_set_halign(std_wta_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(perf_grid), std_wta_label, 3, 1, 1, 1);
    
    /* Log viewer frame */
    log_frame = gtk_frame_new("Scheduler Log");
    gtk_box_pack_start(GTK_BOX(main_box), log_frame, TRUE, TRUE, 0);
    
    scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(log_frame), scrolled);
    
    text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD);
    font = pango_font_description_from_string("Monospace 9");
    gtk_widget_override_font(text_view, font);
    pango_font_description_free(font);
    gtk_container_add(GTK_CONTAINER(scrolled), text_view);
    
    text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    
    gtk_widget_show_all(window);
    
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    build_ui();
    gtk_main();
    
    /* Cleanup on exit */
    if (generator_pid > 0) {
        kill(generator_pid, SIGKILL);
    }
    
    return 0;
}