#pragma once
// Sim shadow of include/board_pins.h: the layout code only needs LCD_W/LCD_H
// from here. Taken from build defines so the harness can render any
// resolution (pass -DSIM_LCD_W=800 -DSIM_LCD_H=480). sim/ is first on the
// include path so this shadows the device board_pins.h.

#ifndef SIM_LCD_W
#define SIM_LCD_W 480
#endif
#ifndef SIM_LCD_H
#define SIM_LCD_H 480
#endif

#define LCD_W SIM_LCD_W
#define LCD_H SIM_LCD_H
