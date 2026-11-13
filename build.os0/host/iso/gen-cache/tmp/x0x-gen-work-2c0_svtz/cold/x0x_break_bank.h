/* Default firmware: no built-in break loops in the app image.
 * The factory loops are assets/factory-breaks/. The installer can put that pack,
 * or your own loops, into the free tail of this image. X0X_FACTORY_BREAKS=1 puts
 * the factory loops back into the image. X0X_BREAK_BANK=DIR uses another bank.
 */
#pragma once
#include <stdint.h>
#define X0X_NBREAKS 0
#define X0X_BREAK_RATE 44100u
typedef struct { const char *name; const uint8_t *adpcm; uint32_t n; uint8_t bars; } x0x_break_t;
static const x0x_break_t X0X_BREAKS[1] = {{0, 0, 0, 0}};
