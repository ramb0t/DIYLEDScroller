#ifndef _diyledscroller_h_
#define _diyledscroller_h_

#include <stdint.h>

#define ASCII_OFFSET 0x20
#define DIGIT_OFFSET 95

#define WIDTH 90
#define HEIGHT 7


extern void
ledmatrix_setup();

extern void
ledmatrix_draw();

extern void
ledmatrix_test(uint8_t valx, uint8_t valy);

extern void
ledmatrix_test2(uint8_t val);

#endif
