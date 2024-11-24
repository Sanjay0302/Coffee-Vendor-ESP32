/*
 * Download GTK 3 library : sudo apt-get install libgtk-3-dev
 *
 * gcc coffee_server.c -o coffee_server `pkg-config --cflags --libs gtk+-3.0`
 * else use Makefile
 * 1. `make` : to compile
 * 2. `make clean` : execute this before executing make
 */

#include <gtk/gtk.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define PORT 8080
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 400
#define HANDSHAKE_REQUEST "COFFEE_HANDSHAKE"
#define HANDSHAKE_ACK "COFFEE_ACK"
#define HANDSHAKE_TIMEOUT_MS 3000
#define RECEIVE_BUFFER_SIZE 255

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
    guint handshake_timeout_id;
    guint receive_check_id;
    char receive_buffer[RECEIVE_BUFFER_SIZE];
    gboolean handshake_in_progress;
} AppWidgets;

// Button access rights until connection complete
void set_controls_sensitivity(AppWidgets *app, gboolean sensitive)
{
    // Set sensitivity for all beverage buttons
    for (int i = 0; i < 5; i++)
    {
        gtk_widget_set_sensitive(app->beverage_buttons[i], sensitive);
    }
    // Set sensitivity for quantity combo box
    gtk_widget_set_sensitive(app->quantity_combo, sensitive);
}

void set_socket_nonblocking(int sock)
{
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
}

// Send control signal to ESP32
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

gboolean check_udp_messages(gpointer data)
{
    AppWidgets *app = (AppWidgets *)data;
    struct sockaddr_in sender_addr;
    socklen_t sender_addr_len = sizeof(sender_addr);

    int received = recvfrom(app->sock, app->receive_buffer, RECEIVE_BUFFER_SIZE - 1, 0,
                            (struct sockaddr *)&sender_addr, &sender_addr_len);

    if (received > 0)
    {
        app->receive_buffer[received] = '\0';

        if (app->handshake_in_progress && strcmp(app->receive_buffer, HANDSHAKE_ACK) == 0)
        {
            // Handshake completed successfully
            app->handshake_in_progress = FALSE;
            if (app->handshake_timeout_id)
            {
                g_source_remove(app->handshake_timeout_id);
                app->handshake_timeout_id = 0;
            }

            gtk_button_set_label(GTK_BUTTON(app->connect_button), "Disconnect");
            gtk_label_set_text(GTK_LABEL(app->connection_status_label), "Connected");
            app->is_connected = TRUE;
            set_controls_sensitivity(app, TRUE);
            gtk_text_buffer_insert_at_cursor(app->log_buffer, "Connection established!\n", -1);
        }
    }

    return G_SOURCE_CONTINUE;
}

gboolean handshake_timeout(gpointer data)
{
    AppWidgets *app = (AppWidgets *)data;

    if (app->handshake_in_progress)
    {
        app->handshake_in_progress = FALSE;
        app->is_connected = FALSE;
        close(app->sock);

        gtk_button_set_label(GTK_BUTTON(app->connect_button), "Connect");
        gtk_label_set_text(GTK_LABEL(app->connection_status_label), "Disconnected");
        gtk_text_buffer_insert_at_cursor(app->log_buffer, "Handshake timeout - no response from client\n", -1);
        set_controls_sensitivity(app, FALSE);
    }

    app->handshake_timeout_id = 0;
    return G_SOURCE_REMOVE;
}

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

void on_connect_button_clicked(GtkWidget *widget, gpointer data)
{
    AppWidgets *app = (AppWidgets *)data;
    const char *button_label = gtk_button_get_label(GTK_BUTTON(widget));

    if (strcmp(button_label, "Connect") == 0)
    {
        set_controls_sensitivity(app, FALSE);

        // Create UDP socket
        app->sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (app->sock < 0)
        {
            gtk_text_buffer_insert_at_cursor(app->log_buffer, "Socket creation error\n", -1);
            return;
        }

        // Make socket non-blocking
        set_socket_nonblocking(app->sock);

        // Set up ESP32 address
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

        // Start handshake process
        app->handshake_in_progress = TRUE;
        gtk_label_set_text(GTK_LABEL(app->connection_status_label), "Connecting...");

        // Send handshake request
        if (sendto(app->sock, HANDSHAKE_REQUEST, strlen(HANDSHAKE_REQUEST), 0,
                   (struct sockaddr *)&app->esp32_addr, sizeof(app->esp32_addr)) < 0)
        {
            gtk_text_buffer_insert_at_cursor(app->log_buffer, "Failed to send handshake request\n", -1);
            close(app->sock);
            return;
        }

        // Start timeout timer
        app->handshake_timeout_id = g_timeout_add(HANDSHAKE_TIMEOUT_MS, handshake_timeout, app);

        // Start checking for responses
        app->receive_check_id = g_timeout_add(100, check_udp_messages, app);
    }
    else
    {
        // Disconnect
        if (app->receive_check_id)
        {
            g_source_remove(app->receive_check_id);
            app->receive_check_id = 0;
        }
        if (app->handshake_timeout_id)
        {
            g_source_remove(app->handshake_timeout_id);
            app->handshake_timeout_id = 0;
        }

        close(app->sock);
        gtk_button_set_label(GTK_BUTTON(widget), "Connect");
        gtk_label_set_text(GTK_LABEL(app->connection_status_label), "Disconnected");
        app->is_connected = FALSE;
        app->handshake_in_progress = FALSE;

        set_controls_sensitivity(app, FALSE);
    }
}

void activate(GtkApplication *app, gpointer user_data)
{
    AppWidgets *widgets = (AppWidgets *)user_data;

    widgets->window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(widgets->window), "Coffee Vending Server");
    gtk_window_set_default_size(GTK_WINDOW(widgets->window), WINDOW_WIDTH, WINDOW_HEIGHT);
    gtk_window_set_resizable(GTK_WINDOW(widgets->window), FALSE);

    widgets->grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(widgets->grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(widgets->grid), 10);
    gtk_container_add(GTK_CONTAINER(widgets->window), widgets->grid);

    // Beverage buttons
    const char *coffee_types[] = {
        "Espresso",
        "Cappuccino",
        "Latte",
        "Americano",
        "Hot Water"};

    for (int i = 0; i < G_N_ELEMENTS(coffee_types); i++)
    {
        widgets->beverage_buttons[i] = gtk_button_new_with_label(coffee_types[i]);
        g_signal_connect(widgets->beverage_buttons[i], "clicked", G_CALLBACK(on_coffee_button_clicked), widgets);
        gtk_grid_attach(GTK_GRID(widgets->grid), widgets->beverage_buttons[i],
                        i % 1, i / 1, 1, 1);
    }

    widgets->quantity_combo = gtk_combo_box_text_new();
    for (int i = 1; i <= 10; i++)
    {
        char quantity[10];
        sprintf(quantity, "%d", i);
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widgets->quantity_combo), quantity);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(widgets->quantity_combo), 0);
    gtk_grid_attach(GTK_GRID(widgets->grid), widgets->quantity_combo, 0, 5, 1, 1);

    widgets->client_ip_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(widgets->client_ip_entry), "Enter Client IP");
    gtk_grid_attach(GTK_GRID(widgets->grid), widgets->client_ip_entry, 0, 6, 1, 1);

    widgets->connect_button = gtk_button_new_with_label("Connect");
    g_signal_connect(widgets->connect_button, "clicked", G_CALLBACK(on_connect_button_clicked), widgets);
    gtk_grid_attach(GTK_GRID(widgets->grid), widgets->connect_button, 0, 7, 1, 1);

    widgets->connection_status_label = gtk_label_new("Disconnected");
    gtk_grid_attach(GTK_GRID(widgets->grid), widgets->connection_status_label, 0, 8, 1, 1);

    widgets->log_buffer = gtk_text_buffer_new(NULL);
    widgets->log_view = gtk_text_view_new_with_buffer(widgets->log_buffer);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(widgets->log_view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(widgets->log_view), FALSE);

    GtkWidget *log_scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(log_scrolled_window, 300, 400);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(log_scrolled_window),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);

    gtk_container_add(GTK_CONTAINER(log_scrolled_window), widgets->log_view);
    gtk_widget_set_hexpand(log_scrolled_window, TRUE);
    gtk_widget_set_vexpand(log_scrolled_window, TRUE);
    gtk_grid_attach(GTK_GRID(widgets->grid), log_scrolled_window, 2, 0, 1, 9);
    set_controls_sensitivity(widgets, FALSE);
    gtk_widget_show_all(widgets->window);
}

int main(int argc, char **argv)
{
    AppWidgets app_widgets = {0};
    app_widgets.is_connected = FALSE;
    app_widgets.handshake_in_progress = FALSE;
    app_widgets.handshake_timeout_id = 0;
    app_widgets.receive_check_id = 0;

    GtkApplication *app = gtk_application_new("org.example.coffee_server",
                                              G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), &app_widgets);

    int status = g_application_run(G_APPLICATION(app), argc, argv);

    g_object_unref(app);

    return status;
}