#include <stdio.h>
#include <stdint.h>
#include "interfaces/keyboard.h"
#include "emulator/sdl_engine.h"
#include "emulator/emulator.h"

void kbd_init()
{
}

keyboard_t kbd_getKeys()
{
    keyboard_t keys = 0;

    //this pulls in emulated keypresses from the command shell
    keys |= emulator_getKeys();
    keys |= sdlEngine_getKeys();

    return keys;
}

