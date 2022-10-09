#ifndef PLATFORM_FT70
#error "Blinky test is only available for FT-70D"
#endif

#include "iodefine.h"
#include "typedefine.h"

#define GREEN1_PIN (1 << 1)// N15
#define GREEN1_PORT PI

#define RED1_PIN (1 << 0)// M13
#define RED1_PORT PI

void main() {
  RED1_PORT.DR.BYTE = 0;
  PI.DDR = RED1_PIN;
  RED1_PORT.DR.BYTE |= RED1_PIN;
}
