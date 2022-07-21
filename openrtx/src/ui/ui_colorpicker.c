#include <ui_colorpicker.h>

color_t modified_color;
color_t * original;
current_column_t current_column;
bool active = false;

void color_picker_draw() {
    // Draw the ui based on the state of modified_color.a/r/g/b and current_column
    gfx_clearScreen();

    // Print "Background Color" on top bar
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, "Background Color");
    gfx_print(layout.bottom_pos, layout.bottom_font, TEXT_ALIGN_LEFT,
              color_white, "R=%d", modified_color.r);
    gfx_print(layout.bottom_pos, layout.bottom_font, TEXT_ALIGN_CENTER,
              color_white, "G=%d", modified_color.g);
    gfx_print(layout.bottom_pos, layout.bottom_font, TEXT_ALIGN_RIGHT,
              color_white, "B=%d", modified_color.b);

    uint16_t rect_width = SCREEN_WIDTH - (layout.horizontal_pad * 2);
    uint16_t rect_height = (SCREEN_HEIGHT - (layout.top_h + layout.bottom_h))/2;
    point_t rect_origin = {(SCREEN_WIDTH - rect_width) / 2,
                            (SCREEN_HEIGHT - rect_height) / 3 };
    
    switch (current_column) {
        case COLUMN_RED:
            gfx_print(layout.line3_pos, layout.bottom_font, TEXT_ALIGN_CENTER,
            color_white, "Adjust Red Value", color_white);
            break;
        case COLUMN_GREEN:
            gfx_print(layout.line3_pos, layout.bottom_font, TEXT_ALIGN_CENTER,
            color_white, "Adjust Green Value", color_white);
            break;
        case COLUMN_BLUE:
            gfx_print(layout.line3_pos, layout.bottom_font, TEXT_ALIGN_CENTER,
            color_white, "Adjust Blue Value", color_white);
            break;
        case COLUMN_ALPHA:
            gfx_print(layout.line3_pos, layout.bottom_font, TEXT_ALIGN_CENTER,
            color_white, "Adjust Alpha Value", color_white);
            break;
    }
    gfx_drawRect(rect_origin, rect_width, rect_height, modified_color, true);
}

void color_picker_init(color_t * dstColor) {
    if (!active) {
        original = dstColor;
        modified_color = *dstColor;
        current_column = COLUMN_RED;
        active = true;
        color_picker_draw();
    }
}

void color_picker_deinit() {
    current_column = COLUMN_RED;
    active = false;
}

uint8_t color_picker_back() {
    switch (current_column) {
        case COLUMN_RED:
            color_picker_deinit();
            return 1;
        case COLUMN_GREEN:
            current_column = COLUMN_RED;
            break;
        case COLUMN_BLUE:
            current_column = COLUMN_GREEN;
            break;
        case COLUMN_ALPHA:
            current_column = COLUMN_BLUE;
            break;
    }
    color_picker_draw();
    return 0;
}

uint8_t color_picker_next() {
    switch (current_column) {
        case COLUMN_RED:
            current_column = COLUMN_GREEN;
            break;
        case COLUMN_GREEN:
            current_column = COLUMN_BLUE;
            break;
        case COLUMN_BLUE:
            current_column = COLUMN_ALPHA;
            break;
        case COLUMN_ALPHA:
            // set init-ed color to our modified one
            *original = modified_color;
            color_picker_deinit();
            return 1;
    }
    color_picker_draw();
    return 0;
}

void color_picker_key(kbd_msg_t* msg) {
    if (msg->keys & KEY_UP || msg->keys & KEY_DOWN) {
        switch (current_column) {
            case COLUMN_RED:
                modified_color.r += (msg->keys & KEY_UP) ? 5:-5;
                break;
            case COLUMN_GREEN:
                modified_color.g += (msg->keys & KEY_UP) ? 5:-5;
                break;
            case COLUMN_BLUE:
                modified_color.b += (msg->keys & KEY_UP) ? 5:-5;
                break;
            case COLUMN_ALPHA:
                modified_color.alpha += (msg->keys & KEY_UP) ? 5:-5;
                break;
        }
    }
    color_picker_draw();
}
