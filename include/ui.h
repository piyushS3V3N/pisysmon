#ifndef UI_H
#define UI_H

#include <ncurses.h>
#include <stdbool.h>

// Component IDs

#define COMPONENT_CPU     0
#define COMPONENT_MEMORY  1
#define COMPONENT_DISK    2
#define COMPONENT_NETWORK 3

// Color pairs
#define COLOR_CPU     1
#define COLOR_MEMORY  2
#define COLOR_DISK    3
#define COLOR_NETWORK 4
#define COLOR_HEADER  5

// Maximum number of UI components
#define MAX_COMPONENTS 10

// UI Component structure
typedef struct {
    char title[64];
    int x, y;
    int width, height;
    int color_pair;
    WINDOW *window;
} UIComponent;

// Layout manager structure
typedef struct {
    UIComponent components[MAX_COMPONENTS];
    int num_components;
    int terminal_width, terminal_height;
    int layout_dirty;
} UILayout;

// Global layout manager instance
extern UILayout g_layout;

// UI initialization and cleanup
int ui_init(void);
void ui_cleanup(void);

// Layout management
void ui_calculate_layout(void);
void ui_handle_resize(void);

// Component management
int ui_create_component(const char* title, int color_pair);
void ui_update_component(int component_id, const char* content);

// Drawing functions
void ui_refresh_all(void);
void ui_clear_all(void);
void ui_draw_component_border(int component_id);
void ui_draw_component_title(int component_id);

// Text utilities
void ui_center_text(WINDOW* win, int y, const char* text, int width);
void ui_wrap_text(WINDOW* win, int start_y, int start_x, const char* text, int max_width);
int ui_get_max_content_width(int component_id);
int ui_get_max_content_height(int component_id);

#endif // UI_H
