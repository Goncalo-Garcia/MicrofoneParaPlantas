#pragma once

/******************************************************************************
 * Definitions                                                                *
 ******************************************************************************/
 // 128 * 16 Yellow zone
 // 128 * 48 Blue zone

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)

#define XX  40				 // Face x coordinate
#define YY  16				 // Face y coordinate
#define FRAME_DELAY  150     // Face delay between frames
#define FRAME_BLINK  1000    // Face delay between blinks at neutral state

#define FRAME_PIXELS       288   // Number of pixels in a frame 191
#define FRAME_NUMBER       7     // Number of frames

enum
{
	KAWAI = 0,
	SAD		 ,
	JOKE	 ,
	NEUTRAL0 ,
	NEUTRAL  ,
	TOTAL
};

 /******************************************************************************
  * Symbols Declaration                                                        *
  ******************************************************************************/

extern const uint8_t PROGMEM drops   [];
extern const uint8_t PROGMEM noDrops [];
extern const uint8_t PROGMEM noSun   [];
extern const uint8_t PROGMEM sun     [];

extern const uint8_t PROGMEM joke    [][FRAME_PIXELS];
extern const uint8_t PROGMEM kawaii  [][FRAME_PIXELS];
extern const uint8_t PROGMEM sad     [][FRAME_PIXELS];
extern const uint8_t PROGMEM neutral [][FRAME_PIXELS];