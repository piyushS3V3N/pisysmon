#include "ui.h"
#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

// Global layout manager instance
UILayout g_layout = {0};

// Signal handler for window resize
void handle_resize(int sig) {
    (void)sig; // Suppress unused parameter warning
    g_layout.layout_dirty = 1;
}

int ui_init(void) {
    // Initialize ncurses
    if (initscr() == NULL) {
        return -1;
    }

    // Configure ncurses
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);

    // Initialize colors if supported

    if (has_colors()) {
        start_color();
        init_pair(COLOR_CPU, COLOR_RED, COLOR_BLACK);
        init_pair(COLOR_MEMORY, COLOR_GREEN, COLOR_BLACK);
        init_pair(COLOR_DISK, COLOR_BLUE, COLOR_BLACK);
        init_pair(COLOR_NETWORK, COLOR_YELLOW, COLOR_BLACK);
        init_pair(COLOR_HEADER, COLOR_WHITE, COLOR_BLACK);
    }

    // Set up signal handler for window resize
    signal(SIGWINCH, handle_resize);

    // Initialize layout
    g_layout.num_components = 0;
    g_layout.layout_dirty = 1;

    // Get initial terminal dimensions
    getmaxyx(stdscr, g_layout.terminal_height, g_layout.terminal_width);

    return 0;
}

void ui_cleanup(void) {
    // Clean up all component windows
    for (int i = 0; i < g_layout.num_components; i++) {
        if (g_layout.components[i].window) {
            delwin(g_layout.components[i].window);
            g_layout.components[i].window = NULL;
        }
    }

    // End ncurses
    endwin();
}

void ui_calculate_layout(void) {
    // Get current terminal dimensions
    getmaxyx(stdscr, g_layout.terminal_height, g_layout.terminal_width);

    // Minimum dimensions check
    if (g_layout.terminal_width < 80 || g_layout.terminal_height < 24) {
        // Terminal too small, use minimum layout
        g_layout.terminal_width = 80;
        g_layout.terminal_height = 24;
    }

    // Calculate component dimensions based on terminal size
    int margin = 2;
    int component_spacing = 1;
    int available_width = g_layout.terminal_width - (2 * margin);
    int available_height = g_layout.terminal_height - (2 * margin);

    // For 4 components, arrange in 2x2 grid
    int component_width = (available_width - component_spacing) / 2;
    int component_height = (available_height - component_spacing) / 2;

    // Ensure minimum component size
    if (component_width < 35) component_width = 35;
    if (component_height < 8) component_height = 8;

    // CPU component (top-left)
    if (g_layout.num_components > COMPONENT_CPU) {
        g_layout.components[COMPONENT_CPU].x = margin;
        g_layout.components[COMPONENT_CPU].y = margin;
        g_layout.components[COMPONENT_CPU].width = component_width;
        g_layout.components[COMPONENT_CPU].height = component_height;
    }

    // Memory component (top-right)
    if (g_layout.num_components > COMPONENT_MEMORY) {
        g_layout.components[COMPONENT_MEMORY].x = margin + component_width + component_spacing;
        g_layout.components[COMPONENT_MEMORY].y = margin;
        g_layout.components[COMPONENT_MEMORY].width = component_width;
        g_layout.components[COMPONENT_MEMORY].height = component_height;
    }

    // Disk component (bottom-left)
    if (g_layout.num_components > COMPONENT_DISK) {
        g_layout.components[COMPONENT_DISK].x = margin;
        g_layout.components[COMPONENT_DISK].y = margin + component_height + component_spacing;
        g_layout.components[COMPONENT_DISK].width = component_width;
        g_layout.components[COMPONENT_DISK].height = component_height;
    }

    // Network component (bottom-right)
    if (g_layout.num_components > COMPONENT_NETWORK) {
        g_layout.components[COMPONENT_NETWORK].x = margin + component_width + component_spacing;
        g_layout.components[COMPONENT_NETWORK].y = margin + component_height + component_spacing;
        g_layout.components[COMPONENT_NETWORK].width = component_width;
        g_layout.components[COMPONENT_NETWORK].height = component_height;
    }

    // FIXED: Create windows after calculating layout
    for (int i = 0; i < g_layout.num_components; i++) {
        UIComponent *comp = &g_layout.components[i];

        // Delete existing window if it exists
        if (comp->window) {
            delwin(comp->window);
            comp->window = NULL;
        }

        // Create new window with calculated dimensions
        comp->window = newwin(comp->height, comp->width, comp->y, comp->x);
        if (comp->window == NULL) {
            fprintf(stderr, "Warning: Failed to create window for component %d\n", i);
            continue;
        }
    }

    g_layout.layout_dirty = 0;
}

void ui_handle_resize(void) {
    if (g_layout.layout_dirty) {
        // Delete existing windows
        for (int i = 0; i < g_layout.num_components; i++) {
            if (g_layout.components[i].window) {
                delwin(g_layout.components[i].window);
                g_layout.components[i].window = NULL;
            }
        }

        // Recalculate layout (this will create new windows)
        ui_calculate_layout();

        // Clear and refresh
        clear();
        refresh();
    }
}

void ui_refresh_all(void) {
    // Handle any pending resize
    ui_handle_resize();

    // Refresh all component windows
    for (int i = 0; i < g_layout.num_components; i++) {
        if (g_layout.components[i].window) {
            wrefresh(g_layout.components[i].window);
        }
    }

    // Refresh main screen
    refresh();
}

void ui_clear_all(void) {
    // Clear main screen
    clear();

    // Clear all component windows
    for (int i = 0; i < g_layout.num_components; i++) {
        if (g_layout.components[i].window) {
            wclear(g_layout.components[i].window);
        }
    }
}

int ui_create_component(const char* title, int color_pair) {
    if (g_layout.num_components >= MAX_COMPONENTS) {
        return -1; // Too many components
    }

    int component_id = g_layout.num_components;
    UIComponent *comp = &g_layout.components[component_id];

    // Set component properties
    strncpy(comp->title, title, sizeof(comp->title) - 1);
    comp->title[sizeof(comp->title) - 1] = '\0';
    comp->color_pair = color_pair;
    comp->window = NULL; // Will be created during layout calculation

    g_layout.num_components++;
    g_layout.layout_dirty = 1;

    return component_id;
}

void ui_update_component(int component_id, const char* content) {
    if (component_id < 0 || component_id >= g_layout.num_components) {
        return;
    }

    UIComponent *comp = &g_layout.components[component_id];
    if (!comp->window) {
        fprintf(stderr, "Warning: Component %d window is NULL\n", component_id);
        return;
    }

    // Clear only the content area, not the entire window
    for (int y = 2; y < comp->height - 1; y++) {
        for (int x = 1; x < comp->width - 1; x++) {
            mvwaddch(comp->window, y, x, ' ');
        }
    }

    // Draw border
    ui_draw_component_border(component_id);

    // Draw title
    ui_draw_component_title(component_id);

    // Draw content
    if (content && strlen(content) > 0) {
        ui_wrap_text(comp->window, 2, 1, content, ui_get_max_content_width(component_id));
    }
}

void ui_draw_component_border(int component_id) {
    if (component_id < 0 || component_id >= g_layout.num_components) {
        return;
    }

    UIComponent *comp = &g_layout.components[component_id];
    if (!comp->window) {
        return;
    }

    // Enable color for this component
    if (has_colors()) {
        wattron(comp->window, COLOR_PAIR(comp->color_pair));
    }

    // Draw border
    box(comp->window, 0, 0);

    // Disable color
    if (has_colors()) {
        wattroff(comp->window, COLOR_PAIR(comp->color_pair));
    }
}

void ui_draw_component_title(int component_id) {
    if (component_id < 0 || component_id >= g_layout.num_components) {
        return;
    }

    UIComponent *comp = &g_layout.components[component_id];
    if (!comp->window) {
        return;
    }

    // Enable color and bold for title
    if (has_colors()) {
        wattron(comp->window, COLOR_PAIR(comp->color_pair) | A_BOLD);
    }

    // Draw title centered on top border
    ui_center_text(comp->window, 0, comp->title, comp->width - 2);

    // Disable attributes
    if (has_colors()) {
        wattroff(comp->window, COLOR_PAIR(comp->color_pair) | A_BOLD);
    }
}

void ui_center_text(WINDOW* win, int y, const char* text, int width) {
    if (!win || !text) return;

    int text_len = strlen(text);
    int start_x = (width - text_len) / 2;
    if (start_x < 0) start_x = 1;
    else start_x += 1; // Account for border

    mvwprintw(win, y, start_x, "%.*s", width - 2, text);
}

void ui_wrap_text(WINDOW* win, int start_y, int start_x, const char* text, int max_width) {
    if (!win || !text || max_width <= 0) return;

    int current_y = start_y;
    int current_x = start_x;
    int text_len = strlen(text);
    int pos = 0;

    // Find the component that owns this window for height checking
    UIComponent *comp = NULL;
    for (int i = 0; i < g_layout.num_components; i++) {
        if (g_layout.components[i].window == win) {
            comp = &g_layout.components[i];
            break;
        }
    }

    if (!comp) return;

    while (pos < text_len && current_y < comp->height - 1) {
        // Handle newlines in the text
        char *newline = strchr(&text[pos], '\n');
        int line_end = pos + max_width;

        if (newline && (newline - text) < line_end) {
            line_end = newline - text;
        }

        // If this would be the last part of the text
        if (line_end >= text_len) {
            mvwprintw(win, current_y, current_x, "%s", &text[pos]);
            break;
        }

        // Find the last space within the allowed width
        int space_pos = line_end;
        while (space_pos > pos && text[space_pos] != ' ' && text[space_pos] != '\n') {
            space_pos--;
        }

        // If no space found, break at max width
        if (space_pos <= pos) {
            space_pos = line_end;
        }

        // Print the line
        int line_length = space_pos - pos;
        mvwprintw(win, current_y, current_x, "%.*s", line_length, &text[pos]);

        // Move to next line
        current_y++;
        pos = space_pos;

        // Skip spaces and newlines at the beginning of the next line
        while (pos < text_len && (text[pos] == ' ' || text[pos] == '\n')) {
            pos++;
        }
    }
}

int ui_get_max_content_width(int component_id) {
    if (component_id < 0 || component_id >= g_layout.num_components) {
        return 0;
    }

    UIComponent *comp = &g_layout.components[component_id];
    return comp->width - 2; // Account for borders
}

int ui_get_max_content_height(int component_id) {
    if (component_id < 0 || component_id >= g_layout.num_components) {
        return 0;
    }

    UIComponent *comp = &g_layout.components[component_id];
    return comp->height - 3; // Account for borders and title
}
