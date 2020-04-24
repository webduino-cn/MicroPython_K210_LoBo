/*
 * This file is part of the MicroPython K210 project, https://github.com/loboris/MicroPython_K210_LoBo
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 LoBo (https://github.com/loboris)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
/*
 * 
 * HIGH SPEED LOW LEVEL DISPLAY FUNCTIONS USING DIRECT TRANSFER MODE
 *
*/

#ifndef _TFTSPI_H_
#define _TFTSPI_H_

#include "mpconfigport.h"
#include "modmachine.h"

#if MICROPY_USE_DISPLAY

typedef struct {
    uint32_t	speed;		// SPI clock in Hz
    uint8_t		type;		// Display type, use one of the defined DISP_TYPE_* values
    uint8_t		gamma;		// Gamma curve used
    uint16_t	width;		// Display width (smaller dimension)
    uint16_t	height;		// Display height (larger dimension)
    uint8_t		invrot;		// rotation
    uint8_t		bgr;		// SET TO 0X00 FOR DISPLAYS WITH RGB MATRIX, 0x08 FOR DISPLAYS WITH BGR MATRIX
} display_config_t;

/*
// 24-bit color type structure
typedef struct __attribute__((__packed__)) {
	uint8_t r;
	uint8_t g;
	uint8_t b;
} color_t ;
*/
typedef uint16_t color_t;

#define EPD_TYPE_2_9        0
#define EPD_TYPE_4_2        1
#define EPD_TYPE_4_2_C      2

#define DISP_COLOR_BITS_24	0x66
#define DISP_COLOR_BITS_16	0x55
#define DISP_COLOR_BITS		DISP_COLOR_BITS_16

#define TOUCH_TYPE_NONE		0
#define TOUCH_TYPE_XPT2046	1
#define TOUCH_TYPE_STMPE610	2

#define TP_CALX_XPT2046		7472920
#define TP_CALY_XPT2046		122224794

#define TP_CALX_STMPE610	21368532
#define TP_CALY_STMPE610	11800144

// === Screen orientation constants ===
// Do not change !!
#define PORTRAIT	0
#define LANDSCAPE	1
#define PORTRAIT_FLIP	2
#define LANDSCAPE_FLIP	3

#define DISP_TYPE_ILI9341	0
#define DISP_TYPE_ILI9488	1
#define DISP_TYPE_ST7789V	2
#define DISP_TYPE_ST7735	3
#define DISP_TYPE_ST7735R	4
#define DISP_TYPE_ST7735B	5
#define DISP_TYPE_M5STACK	6
#define DISP_TYPE_GENERIC	7
#define DISP_TYPE_ILI9342   8
// EPD types
#define DISP_TYPE_EPD_2_9_GDEH  8
#define DISP_TYPE_EPD_2_9_DEPG  9
#define DISP_TYPE_EPD_4_2       10
#define DISP_TYPE_EPD_4_2_C     11
#define DISP_TYPE_MAX		    12

#define DEFAULT_TFT_DISPLAY_WIDTH  320
#define DEFAULT_TFT_DISPLAY_HEIGHT 240
#define DEFAULT_DISP_TYPE   DISP_TYPE_ILI9342

#define SPI_DFS8_SPEED      4000000
#define SPI_DEFAULT_SPEED   10000000

/* clang-format off */
#define NO_OPERATION            0x00
#define SOFTWARE_RESET          0x01
#define TFT_READ_ID             0x04
#define READ_STATUS             0x09
#define READ_POWER_MODE         0x0A
#define READ_MADCTL             0x0B
#define READ_PIXEL_FORMAT       0x0C
#define READ_IMAGE_FORMAT       0x0D
#define READ_SIGNAL_MODE        0x0E
#define READ_SELT_DIAG_RESULT   0x0F
#define SLEEP_ON                0x10
#define SLEEP_OFF               0x11
#define PARTIAL_DISPALY_ON      0x12
#define NORMAL_DISPALY_ON       0x13
#define INVERSION_DISPALY_OFF   0x20
#define INVERSION_DISPALY_ON    0x21
#define GAMMA_SET               0x26
#define DISPALY_OFF             0x28
#define DISPALY_ON              0x29
#define HORIZONTAL_ADDRESS_SET  0x2A
#define VERTICAL_ADDRESS_SET    0x2B
#define MEMORY_WRITE            0x2C
#define COLOR_SET               0x2D
#define MEMORY_READ             0x2E
#define PARTIAL_AREA            0x30
#define VERTICAL_SCROL_DEFINE   0x33
#define TEAR_EFFECT_LINE_OFF    0x34
#define TEAR_EFFECT_LINE_ON     0x35
#define MEMORY_ACCESS_CTL       0x36
#define VERTICAL_SCROL_S_ADD    0x37
#define IDLE_MODE_OFF           0x38
#define IDLE_MODE_ON            0x39
#define PIXEL_FORMAT_SET        0x3A
#define WRITE_MEMORY_CONTINUE   0x3C
#define READ_MEMORY_CONTINUE    0x3E
#define SET_TEAR_SCANLINE       0x44
#define GET_SCANLINE            0x45
#define WRITE_BRIGHTNESS        0x51
#define READ_BRIGHTNESS         0x52
#define WRITE_CTRL_DISPALY      0x53
#define READ_CTRL_DISPALY       0x54
#define WRITE_BRIGHTNESS_CTL    0x55
#define READ_BRIGHTNESS_CTL     0x56
#define WRITE_MIN_BRIGHTNESS    0x5E
#define READ_MIN_BRIGHTNESS     0x5F
#define READ_ID1                0xDA
#define READ_ID2                0xDB
#define READ_ID3                0xDC
#define RGB_IF_SIGNAL_CTL       0xB0
#define NORMAL_FRAME_CTL        0xB1
#define IDLE_FRAME_CTL          0xB2
#define PARTIAL_FRAME_CTL       0xB3
#define INVERSION_CTL           0xB4
#define BLANK_PORCH_CTL         0xB5
#define DISPALY_FUNCTION_CTL    0xB6
#define ENTRY_MODE_SET          0xB7
#define BACKLIGHT_CTL1          0xB8
#define BACKLIGHT_CTL2          0xB9
#define BACKLIGHT_CTL3          0xBA
#define BACKLIGHT_CTL4          0xBB
#define BACKLIGHT_CTL5          0xBC
#define BACKLIGHT_CTL7          0xBE
#define BACKLIGHT_CTL8          0xBF
#define POWER_CTL1              0xC0
#define POWER_CTL2              0xC1
#define VCOM_CTL1               0xC5
#define VCOM_CTL2               0xC7
#define NV_MEMORY_WRITE         0xD0
#define NV_MEMORY_PROTECT_KEY   0xD1
#define NV_MEMORY_STATUS_READ   0xD2
#define READ_ID4                0xD3
#define POSITIVE_GAMMA_CORRECT  0xE0
#define NEGATIVE_GAMMA_CORRECT  0xE1
#define DIGITAL_GAMMA_CTL1      0xE2
#define DIGITAL_GAMMA_CTL2      0xE3
#define INTERFACE_CTL           0xF6

#define MADCTL_MY               0x80
#define MADCTL_MX               0x40
#define MADCTL_MV               0x20
#define MADCTL_ML               0x10
#define MADCTL_MH               0x04

/* clang-format on */

#define TFT_CMD_DELAY	0x80


// ==== Public functions =========================================================

// == Low level functions; usually not used directly ==
void disp_spi_transfer_cmd(int8_t cmd);
void disp_spi_transfer_cmd_data(int8_t cmd, uint8_t *data, uint32_t len);
void drawPixel(int16_t x, int16_t y, color_t color);
void send_data(int x1, int y1, int x2, int y2, uint32_t len, color_t *buf);
void send_data_scale(int x1, int y1, int width, int height, color_t *buf, int scale);
void TFT_pushColorRep(int x1, int y1, int x2, int y2, color_t data, uint32_t len);
void send_frame_buffer();
void TFT_display_setvars(display_config_t *dconfig);
void tft_set_speed(uint32_t speed);
uint32_t tft_get_speed();

// Change the screen rotation.
// Input: m new rotation value (0 to 3)
//=================================
void _tft_setRotation(uint8_t rot);

// Perform display initialization sequence
// Sets orientation to landscape; clears the screen
//============================================================
int  TFT_display_init(display_config_t *display_config);

// ===============================================================================

#endif // MICROPY_USE_DISPLAY

#endif
