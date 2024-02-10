#ifndef SH110X_MOD17_H
#define SH110X_MOD17_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void SH110x_init();

void SH110x_terminate();

void SH110x_renderRows(uint8_t startRow, uint8_t endRow, void *fb);

void SH110x_render(void *fb);

void SH110x_setContrast(uint8_t contrast);

#ifdef __cplusplus
}
#endif

#endif /* SH110X_MOD17_H */