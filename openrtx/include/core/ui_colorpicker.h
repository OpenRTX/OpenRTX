#ifndef UI_COLORPICKER_H
#define UI_COLORPICKER_H

#include <input.h>
#include <ui.h>

typedef enum {
    COLUMN_RED,
    COLUMN_GREEN,
    COLUMN_BLUE,
    COLUMN_ALPHA,
} current_column_t;

void color_picker_init(color_t * dstColor);
void color_picker_key(kbd_msg_t* msg);
uint8_t color_picker_back();
uint8_t color_picker_next();


#endif