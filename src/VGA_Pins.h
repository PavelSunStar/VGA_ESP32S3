//LCD connect pins ----------------------------------------------------------------------------------------------------

//R
#define VGA_PIN_NUM_DATA0          4
#define VGA_PIN_NUM_DATA1          5
#define VGA_PIN_NUM_DATA2          6
#define VGA_PIN_NUM_DATA3          7
#define VGA_PIN_NUM_DATA4          15

//G
#define VGA_PIN_NUM_DATA5          9
#define VGA_PIN_NUM_DATA6          10
#define VGA_PIN_NUM_DATA7          11
#define VGA_PIN_NUM_DATA8          12
#define VGA_PIN_NUM_DATA9          13
#define VGA_PIN_NUM_DATA10         14

//B
#define VGA_PIN_NUM_DATA11         16
#define VGA_PIN_NUM_DATA12         17
#define VGA_PIN_NUM_DATA13         18
#define VGA_PIN_NUM_DATA14         8
#define VGA_PIN_NUM_DATA15         3

//Other
#define VGA_PIN_NUM_HSYNC          1
#define VGA_PIN_NUM_VSYNC          2
#define VGA_PIN_NUM_DISP           -1
#define VGA_PIN_NUM_DISP_EN        -1
#define VGA_PIN_NUM_DE             -1

// note: PCLK pin is not needed for VGA output.
// however, the current version of the esp lcd rgb driver requires this to be set
// to keep this pin unused and available for something else, you need a patched version
// of the driver (for now)
#if PATCHED_LCD_DRIVER
#define VGA_PIN_NUM_PCLK           -1
#else
#define VGA_PIN_NUM_PCLK           -1//21
#endif
//End LCD connect pins ----------------------------------------------------------------------------------------------------
/*
//R
#define VGA_PIN_NUM_DATA0          4
#define VGA_PIN_NUM_DATA1          5
#define VGA_PIN_NUM_DATA2          6
#define VGA_PIN_NUM_DATA3          7
#define VGA_PIN_NUM_DATA4          8

//G
#define VGA_PIN_NUM_DATA5          9
#define VGA_PIN_NUM_DATA6          10
#define VGA_PIN_NUM_DATA7          11
#define VGA_PIN_NUM_DATA8          12
#define VGA_PIN_NUM_DATA9          13
#define VGA_PIN_NUM_DATA10         14

//B
#define VGA_PIN_NUM_DATA11         15
#define VGA_PIN_NUM_DATA12         16
#define VGA_PIN_NUM_DATA13         17
#define VGA_PIN_NUM_DATA14         18
#define VGA_PIN_NUM_DATA15         21
*/