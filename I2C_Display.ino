#include <Adafruit_SSD1306.h>
#include "Faces.h"

#define LF              0x0A 
#define MESSAGE_SIZE    10 

#define FRAME_SQUARE    false

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

volatile bool light = true;
volatile bool water = true;

uint8_t     face        = 0; // Face to paint on lcd
uint32_t    lastRef     = 0; // Last millis reference
uint32_t    frameDelay  = FRAME_DELAY;
 
uint8_t index = 0;
char message[MESSAGE_SIZE];

void drawFace(uint8_t faceIndex);
void readCommand();
uint32_t timeDelta(const uint32_t t1, const uint32_t reference);

void setup() 
{
    Serial.begin(115200);

    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    
    display.setCursor(0, 0);

    display.clearDisplay();

    display.setTextColor(WHITE);

    lastRef = millis(); // Get fisrt millis reference
}

void loop() 
{
    while (true)
    {
        readCommand();
        drawFace(face);
    }
}

void drawFace(uint8_t faceIndex)//const uint8_t face[FRAME_NUMBER][FRAME_PIXELS])
{
    static uint8_t frame = 0;

    if (timeDelta(millis(),lastRef) >= frameDelay)
    {
        lastRef = millis();

        display.clearDisplay();

        printText();

        if(((faceIndex != NEUTRAL0) || (faceIndex != NEUTRAL)) && (frameDelay == FRAME_BLINK)) frameDelay = FRAME_DELAY;

        switch (faceIndex)
        {
            case KAWAI: display.drawBitmap(XX, YY, kawaii[frame], 48, 48, 1);   break;
            case SAD:   display.drawBitmap(XX, YY, sad[frame], 48, 48, 1);      break;
            case JOKE:  display.drawBitmap(XX, YY, joke[frame], 48, 48, 1);     break;
            case NEUTRAL0: 
            {
                frameDelay = FRAME_BLINK;
                face       = NEUTRAL;

                display.drawBitmap(XX, YY, neutral[0], 48, 48, 1); break;
            }
            case NEUTRAL:
            {
                frameDelay = FRAME_DELAY;
                if ((frame + 1) >= FRAME_NUMBER) face = NEUTRAL0; // Start blink again

                display.drawBitmap(XX, YY, neutral[frame], 48, 48, 1); break;

            }
        }

        display.display();

        ((frame + 1) >= FRAME_NUMBER) ? (frame = 0) : (frame += 1);
    }
}

void printText(void)
{    
    static bool frameUpdate = false;

    (water == true) ? (display.drawBitmap(0, 0, drops, 16, 16, 1))   : (display.drawBitmap(0, 0, noDrops, 16, 16, 1));

    (light == true) ? (display.drawBitmap(112, 0, sun, 16, 16, 1))   : (display.drawBitmap(112, 0, noSun, 16, 16, 1));

#if(FRAME_SQUARE)

    (frameUpdate == true) ? (display.fillRect(0, 54, 10, 10, WHITE))   : (display.drawRect(0, 54, 10, 10, WHITE));

    frameUpdate = !frameUpdate;

#endif
}

/**
 *
 */
uint32_t timeDelta(const uint32_t t1, const uint32_t reference)
{
    return (t1 >= reference) ? (t1 - reference) : ((0xFFFFFFFF - reference) + t1);
}

void readCommand(void)
{
    if (Serial.available() > 0) {

        message[index] = Serial.read();

        if (message[index] == LF) {

            message[index - 1] = 0; // Clear line feed caracter

            if (!strcmp(message, "kawai"))      face = KAWAI;
                                                
            if (!strcmp(message, "sad"))        face = SAD;
                                                
            if (!strcmp(message, "joke"))       face = JOKE;
                                                
            if (!strcmp(message, "neutral"))    face = NEUTRAL0;

            if (!strcmp(message, "lightON"))    light = true;

            if (!strcmp(message, "lightOFF"))   light = false;

            if (!strcmp(message, "waterON"))    water = true;

            if (!strcmp(message, "waterOFF"))   water = false;

            Serial.println("ACK");

            memset(message, 0, sizeof(message));

            index = MESSAGE_SIZE;
        }

        if ((index += 1) >= MESSAGE_SIZE) index = 0;
    }
}