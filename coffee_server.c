/*
 * gcc coffee_server.c -o coffee_server `pkg-config --cflags --libs gtk+-3.0`
 */

#include <gtk/gtk.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

#define PORT 8080
#define ESP32_IP "192.168.226.52" // Replace with your ESP32's IP address

typedef struct
{
    GtkWidget *window;
    GtkWidget *grid;
    int sock;
    struct sockaddr_in esp32_addr;
} AppWidgets;

// Function to send control signal to ESP32
void send_control_signal(AppWidgets *app, const char *signal)
{
    if (sendto(app->sock, signal, strlen(signal), 0, (struct sockaddr *)&app->esp32_addr, sizeof(app->esp32_addr)) < 0)
    {
        g_print("Failed to send signal\n");
    }
    else
    {
        g_print("Sent signal: %s\n", signal);
    }
}

// Callback for Coffee buttons
void on_coffee_button_clicked(GtkWidget *widget, gpointer data)
{
    AppWidgets *app = (AppWidgets *)data;
    const char *button_name = gtk_button_get_label(GTK_BUTTON(widget));

    send_control_signal(app, button_name);
}

// Initialize network socket
int init_socket(AppWidgets *app)
{
    // Create UDP socket
    app->sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (app->sock < 0)
    {
        g_print("Socket creation error\n");
        return -1;
    }

    // Configure ESP32 address
    memset(&app->esp32_addr, 0, sizeof(app->esp32_addr));
    app->esp32_addr.sin_family = AF_INET;   // Set the IP address
    app->esp32_addr.sin_port = htons(PORT); // set listening port
    if (inet_pton(AF_INET, ESP32_IP, &app->esp32_addr.sin_addr) <= 0)
    {
        g_print("Invalid address\n");
        return -1;
    }

    return 0;
}

// Create main application window
void activate(GtkApplication *app, gpointer user_data)
{
    AppWidgets *widgets = (AppWidgets *)user_data;

    // Create main window
    widgets->window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(widgets->window), "Coffee Vending Server");
    gtk_window_set_default_size(GTK_WINDOW(widgets->window), 400, 300);

    // Create grid for buttons
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
        GtkWidget *button = gtk_button_new_with_label(coffee_types[i]);
        g_signal_connect(button, "clicked", G_CALLBACK(on_coffee_button_clicked), widgets);

        // Place buttons in a grid
        gtk_grid_attach(GTK_GRID(widgets->grid), button,
                        i % 3, // column
                        i / 3, // row
                        1, 1); // width, height
    }

    // Show all widgets
    gtk_widget_show_all(widgets->window);
}

int main(int argc, char **argv)
{
    AppWidgets app_widgets;

    // Initialize socket
    if (init_socket(&app_widgets) < 0)
    {
        return 1;
    }

    // Create GTK application
    GtkApplication *app = gtk_application_new("org.example.coffee_server",
                                              G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), &app_widgets);

    // Run the application
    int status = g_application_run(G_APPLICATION(app), argc, argv);

    // Close socket
    close(app_widgets.sock);

    // Destroy application
    g_object_unref(app);

    return status;
}