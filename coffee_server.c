#include <gtk/gtk.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

#define PORT 8080
#define ESP32_IP "192.168.1.100" // Replace with your ESP32's IP address
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

typedef struct
{
    GtkWidget *window;
    GtkWidget *grid;
    GtkWidget *connect_button;
    GtkWidget *client_ip_entry;
    GtkWidget *quantity_combo;
    GtkWidget *connection_status_label;
    GtkWidget *log_view;
    GtkTextBuffer *log_buffer;
    GtkWidget *beverage_buttons[5];
    int sock;
    struct sockaddr_in esp32_addr;
    gboolean is_connected;
} AppWidgets;

// Function to send control signal to ESP32
void send_control_signal(AppWidgets *app, const char *signal, int quantity)
{
    char message[100];
    snprintf(message, sizeof(message), "%s,%d", signal, quantity);

    if (sendto(app->sock, message, strlen(message), 0,
               (struct sockaddr *)&app->esp32_addr, sizeof(app->esp32_addr)) < 0)
    {
        gtk_text_buffer_insert_at_cursor(app->log_buffer, "Failed to send signal\n", -1);
    }
    else
    {
        gtk_text_buffer_insert_at_cursor(app->log_buffer, "Sent signal: ", -1);
        gtk_text_buffer_insert_at_cursor(app->log_buffer, message, -1);
        gtk_text_buffer_insert_at_cursor(app->log_buffer, "\n", -1);
    }
}

// Callback for Coffee buttons
void on_coffee_button_clicked(GtkWidget *widget, gpointer data)
{
    AppWidgets *app = (AppWidgets *)data;
    const char *button_name = gtk_button_get_label(GTK_BUTTON(widget));
    int quantity = gtk_combo_box_get_active(GTK_COMBO_BOX(app->quantity_combo)) + 1;

    if (app->is_connected)
    {
        send_control_signal(app, button_name, quantity);
    }
    else
    {
        gtk_text_buffer_insert_at_cursor(app->log_buffer, "Not connected to client. Unable to send signal.\n", -1);
    }
}

// Callback for Connect/Disconnect button
void on_connect_button_clicked(GtkWidget *widget, gpointer data)
{
    AppWidgets *app = (AppWidgets *)data;
    const char *button_label = gtk_button_get_label(GTK_BUTTON(widget));

    if (strcmp(button_label, "Connect") == 0)
    {
        // Create UDP socket
        app->sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (app->sock < 0)
        {
            gtk_text_buffer_insert_at_cursor(app->log_buffer, "Socket creation error\n", -1);
            return;
        }

        // Configure ESP32 address
        memset(&app->esp32_addr, 0, sizeof(app->esp32_addr));
        app->esp32_addr.sin_family = AF_INET;
        app->esp32_addr.sin_port = htons(PORT);
        const char *client_ip = gtk_entry_get_text(GTK_ENTRY(app->client_ip_entry));
        if (inet_pton(AF_INET, client_ip, &app->esp32_addr.sin_addr) <= 0)
        {
            gtk_text_buffer_insert_at_cursor(app->log_buffer, "Invalid IP address\n", -1);
            close(app->sock);
            return;
        }

        gtk_button_set_label(GTK_BUTTON(widget), "Disconnect");
        gtk_label_set_text(GTK_LABEL(app->connection_status_label), "Connected");
        app->is_connected = TRUE;
    }
    else
    {
        close(app->sock);
        gtk_button_set_label(GTK_BUTTON(widget), "Connect");
        gtk_label_set_text(GTK_LABEL(app->connection_status_label), "Disconnected");
        app->is_connected = FALSE;
    }
}

// Initialize application
void activate(GtkApplication *app, gpointer user_data)
{
    AppWidgets *widgets = (AppWidgets *)user_data;

    // Create main window
    widgets->window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(widgets->window), "Coffee Vending Server");
    gtk_window_set_default_size(GTK_WINDOW(widgets->window), WINDOW_WIDTH, WINDOW_HEIGHT);
    gtk_window_set_resizable(GTK_WINDOW(widgets->window), FALSE);

    // Create grid for UI elements
    widgets->grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(widgets->window), widgets->grid);

    // Coffee type buttons
    const char *coffee_types[] = {
        "Espresso",
        "Cappuccino",
        "Latte",
        "Americano",
        "Hot Water"};

    // Create and place buttons
    for (int i = 0; i < G_N_ELEMENTS(coffee_types); i++)
    {
        widgets->beverage_buttons[i] = gtk_button_new_with_label(coffee_types[i]);
        g_signal_connect(widgets->beverage_buttons[i], "clicked", G_CALLBACK(on_coffee_button_clicked), widgets);

        // Place buttons in a grid
        gtk_grid_attach(GTK_GRID(widgets->grid), widgets->beverage_buttons[i],
                        i % 3, // column
                        i / 3, // row
                        1, 1); // width, height
    }

    // Create quantity combo box
    widgets->quantity_combo = gtk_combo_box_text_new();
    for (int i = 1; i <= 10; i++)
    {
        char quantity[10];
        sprintf(quantity, "%d", i);
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widgets->quantity_combo), quantity);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(widgets->quantity_combo), 0);
    gtk_grid_attach(GTK_GRID(widgets->grid), widgets->quantity_combo, 0, 5, 3, 1);

    // Create client IP entry
    widgets->client_ip_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(widgets->client_ip_entry), "Enter Client IP");
    gtk_grid_attach(GTK_GRID(widgets->grid), widgets->client_ip_entry, 0, 6, 3, 1);

    // Create connect/disconnect button
    widgets->connect_button = gtk_button_new_with_label("Connect");
    g_signal_connect(widgets->connect_button, "clicked", G_CALLBACK(on_connect_button_clicked), widgets);
    gtk_grid_attach(GTK_GRID(widgets->grid), widgets->connect_button, 0, 7, 3, 1);

    // Create connection status label
    widgets->connection_status_label = gtk_label_new("Disconnected");
    gtk_grid_attach(GTK_GRID(widgets->grid), widgets->connection_status_label, 0, 8, 3, 1);

    // Create log view
    widgets->log_buffer = gtk_text_buffer_new(NULL);
    widgets->log_view = gtk_text_view_new_with_buffer(widgets->log_buffer);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(widgets->log_view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(widgets->log_view), FALSE);
    GtkWidget *log_scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(log_scrolled_window), widgets->log_view);
    gtk_grid_attach(GTK_GRID(widgets->grid), log_scrolled_window, 3, 0, 5, 9);

    // Show all widgets
    gtk_widget_show_all(widgets->window);
}

int main(int argc, char **argv)
{
    AppWidgets app_widgets = {0};
    app_widgets.is_connected = FALSE;

    // Create GTK application
    GtkApplication *app = gtk_application_new("org.example.coffee_server",
                                              G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), &app_widgets);

    // Run the application
    int status = g_application_run(G_APPLICATION(app), argc, argv);

    // Destroy application
    g_object_unref(app);

    return status;
}