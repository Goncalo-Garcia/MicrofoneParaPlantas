#include <SD.h>               // need to include the SD library
#include <TMRpcm.h>           // Lib to play wav file
#include <SPI.h>

#define HOURS_TO_MILLIS(h) (h*360000)
#define MINUT_TO_MILLIS(m) (m*6000)

#define SD_ChipSelectPin 10   // Pin10 for Arduino Nano
#define SPEAKER_PIN      9    // Pin9 for Arduino Nano

#define ON  1
#define OFF 0

#define YES 1
#define NO  0

#define FASTMODE          ON
#define LOGGER            OFF

#define SOIL_SENSOR_ADC   A0    // Analog sensor input pin
#define SOIL_SENSOR_PWR   A1    // Power pin of sensor 
#define LIGHT_SENSOR_ADC  A2    // Power pin of sensor

#define SAMPLES_NUMBER    32    // Number of samples read for the moister sensor

#define SOIL_THRESHOLD    400   // Threshold value to be define
#define LIGHT_THRESHOLD   500   // Threshold value to be define

#if FASTMODE == ON

#define SAMPLES_INTERVAL  250                   // Time elapsed between samples 5 seg
#define TIME_WITHOUT_LIGHT  3000                // Max min that the plant should be without light

#else

#define SAMPLES_INTERVAL  6000                  // Time elapsed between samples 6000 seg
#define TIME_WITHOUT_LIGHT  HOURS_TO_MILLIS(16) // Max hours that the plant should be without light

#endif

// Possible states for flower state machine

#define OK          1 // :) Plant is happy, do nothing
#define LACK        2 // :| Plant needs something
#define ALERT       3 // :( Plant needs something, do sound alert

// Possible sound alerts 

#define WATER_ALERT 0b00000001 // Request for water
#define WATER_THNKS 0b00000010 // Thanks for the water
#define LIGHT_ALERT 0b00000100 // Request for light
#define LIGHT_THNKS 0b00001000 // Thanks for the light
#define RADOM_ALERT 0b00010000 // Say something funny & random

typedef struct
{
    volatile uint8_t     read          = OFF;
             uint8_t     sampleCounter = 0;
             uint16_t    value         = 0;
             uint32_t    sumSamples    = 0;
}
Sensor_t;

typedef struct
{
    volatile uint8_t  speak    = ON; // Flag to speak
             uint8_t  water    = OK; // water state of the plant soil
             uint8_t  light    = OK; // light state of the plant
             unsigned long  lightRef = 0;  // millis time reference of last good light value
}
FlowerState_t;

volatile uint32_t timerMillis = 0;      // This represents 100 milliseconds
volatile uint32_t lastTimeRef = 0;      // This represents 100 milliseconds

TMRpcm speakerDriver;                   // create an object for use in this sketch

Sensor_t        sensorSoil;
Sensor_t        sensorLight;

FlowerState_t flowerState;

uint8_t speakAlert = 0;

void setup() 
{
    Serial.begin(115200);

    randomSeed(A3);
    
    init_TimerSensors();                    // Initiates the 1 millisecond timer for the sensors

    Serial.println("Timers Init");

    speakerDriver.speakerPin = SPEAKER_PIN; // pin 9 for Aduino Nano  

    if (!SD.begin(SD_ChipSelectPin))        // see if the card is present and can be initialized:
    {
        Serial.println("SD fail");
        return;
    }
    else
    {
        Serial.println("SD ok");
    }

    speakerDriver.volume(2);
}

void loop() 
{
    read_Sensor(&sensorSoil);
    read_Sensor(&sensorLight);

    if (flowerState.speak == ON)
    {
        flowerState.speak = OFF;

        speakMachine();
    }

    stateMachine();

    Serial.println(speakAlert);
}

//+++++++++++++++++++++++++++++++++++++
// FUNCTIONS
//+++++++++++++++++++++++++++++++++++++

void stateMachine(void)
{
    // Serial.println(sensorSoil.value);
    // Serial.println(sensorLight.value);

    switch (flowerState.water)
    {
        case(OK):
        {
            Serial.println("waterON");
            while (Serial.available() < 0); // Wait for ACK

            if (soilDry() == YES)
            {
                flowerState.water = ALERT;
            }
        }
        break;

        case(ALERT):
        {
            Serial.println("waterOFF");
            while (Serial.available() < 0); // Wait for ACK
            
            setAlert(WATER_ALERT); // Request water alert

            if (soilDry() == NO)
            {
                flowerState.water = OK;
                setAlert(WATER_THNKS); // Thanks sound alert
                
                speakAlert ^= WATER_ALERT; // Clear the flag
            }
        }
        break;
    }

    switch (flowerState.light)
    {
        case(OK):
        {
            Serial.println("lightON");
            while (Serial.available() < 0); // Wait for ACK

            if (light() == OFF)
            {
                flowerState.light    = LACK;
                flowerState.lightRef = timerMillis;
            }
        }
        break;

        case(LACK):
        {
            Serial.println("lightOFF");
            while (Serial.available() < 0); // Wait for ACK

            if (timeDelta(timerMillis, flowerState.lightRef) >= TIME_WITHOUT_LIGHT)
            {
                flowerState.light = ALERT;
            }

            if (light() == ON)
            {
                flowerState.light = OK;
                setAlert(LIGHT_THNKS); // Thanks sound alert
            }
        }
        break;

        case(ALERT):
        {
            Serial.println("lightOFF");
            while (Serial.available() < 0); // Wait for ACK

            setAlert(LIGHT_ALERT); // Request light alert

            if (light() == ON)
            {
                flowerState.light = OK;
                setAlert(LIGHT_THNKS); // Thanks sound alert

                speakAlert ^= LIGHT_ALERT; // Clear the flag
            }
        }
        break;
    }

    if ((flowerState.light != OK) || (flowerState.water != OK))
    {
        if ((flowerState.water == ALERT) || (flowerState.light == ALERT))
        {
            Serial.println("sad");      // Set sad Face
            while (Serial.available() < 0); // Wait for ACK
        }
        else if (flowerState.light == LACK)
        {
            Serial.println("neutral");  // Set neutral Face
            while (Serial.available() < 0); // Wait for ACK
        }
    }

    if ((flowerState.light == OK) && (flowerState.water == OK))
    {
        // Set Happy Face
        Serial.println("kawai");
        while (Serial.available() < 0); // Wait for ACK

        setAlert(RADOM_ALERT); // Random sound alert
    }
}

void speakMachine()
{
    if (!speakerDriver.isPlaying())
    {
        uint8_t randomizer = random(1, 20);

        if (randomizer >= 10)
        {
            randomizer = random(2, 7);

            randomizer /= 2;

            if (speakAlert & WATER_ALERT)
            {
                switch (randomizer)
                {
                case 1:
                {
                    speakerDriver.play("W_Alert1.wav");
                    Serial.println("I need water mother fucker!! 1 :(");
                }
                break;

                case 2:
                {
                    speakerDriver.play("W_Alert2.wav");
                    Serial.println("I need water mother fucker!! 2 :(");
                }
                break;

                case 3:
                {
                    speakerDriver.play("W_Alert3.wav");
                    Serial.println("I need water mother fucker!! 3 :(");
                }
                break;
                }

                while (speakerDriver.isPlaying()); // Wait for the alert to finish
                
                speakAlert ^= WATER_ALERT; // Clear the flag

                return;
            }
            
            if (speakAlert & LIGHT_ALERT)
            {
                switch (randomizer)
                {
                case 1:
                {
                    speakerDriver.play("L_Alert1.wav");
                    Serial.println("I need light sucker!! 1 :(");
                }
                break;

                case 2:
                {
                    speakerDriver.play("L_Alert2.wav");
                    Serial.println("I need light sucker!! 2 :(");
                }
                break;

                case 3:
                {
                    speakerDriver.play("L_Alert3.wav");
                    Serial.println("I need light sucker!! 3 :(");
                }
                break;
                }

                while (speakerDriver.isPlaying()); // Wait for the joke to finish

                speakAlert ^= LIGHT_ALERT; // Clear the flag

                return;
            }
            
            if ((speakAlert & RADOM_ALERT))
            {
                Serial.println("joke");
                while (Serial.available() < 0); // Wait for ACK

                switch (randomizer)
                {
                    case 1:
                    {
                        speakerDriver.play("Random1.wav");
                        Serial.println("Random Shit!? 1 xD");
                    }
                    break;

                    case 2:
                    {
                        speakerDriver.play("Random2.wav");
                        Serial.println("Random Shit!? 2 xD");
                    }
                    break;

                    case 3:
                    {
                        speakerDriver.play("Random3.wav");
                        Serial.println("Random Shit!? 3 xD");
                    }
                    break;
                }

                while (speakerDriver.isPlaying()); // Wait for the joke alert to finish

                speakAlert ^= RADOM_ALERT; // Clear the flag

                return;
            }            
        }

        if (speakAlert & WATER_THNKS)
        {
            switch (randomizer)
            {
            case 1:
            {
                speakerDriver.play("W_Thnks1.wav");
                Serial.println("Thanks for the water 1 :)");
            }
            break;

            case 2:
            {
                speakerDriver.play("W_Thnks2.wav");
                Serial.println("Thanks for the water 2 :)");
            }
            break;

            case 3:
            {
                speakerDriver.play("W_Thnks3.wav");
                Serial.println("Thanks for the water 3 :)");
            }
            break;
            }

            while (speakerDriver.isPlaying()); // Wait for the alert to finish
            
            speakAlert ^= WATER_THNKS; // Clear the flag

            return;
        }

        if (speakAlert & LIGHT_THNKS)
        {
            switch (randomizer)
            {
            case 1:
            {
                speakerDriver.play("L_Thnks1.wav");
                Serial.println("Thanks for the light 1 :)");
            }
            break;

            case 2:
            {
                speakerDriver.play("L_Thnks2.wav");
                Serial.println("Thanks for the light 2 :)");
            }
            break;

            case 3:
            {
                speakerDriver.play("L_Thnks3.wav");
                Serial.println("Thanks for the light 3 :)");
            }
            break;
            }

            while (speakerDriver.isPlaying()); // Wait for the alert to finish
            
            speakAlert ^= LIGHT_THNKS; // Clear the flag

            return;
        }
    }
}

void setAlert(uint8_t alert)
{
    if ((alert == RADOM_ALERT) && (random(1, 10000) == 7))
    {
        speakAlert |= RADOM_ALERT; // Set random alert flag
        return;
    }

    switch (alert)
    {
        case(WATER_THNKS):
        {
            speakAlert |= WATER_THNKS; // Set water thanks alert flag
        }
        break;

        case(LIGHT_THNKS):
        {
            speakAlert |= LIGHT_THNKS; // Set light thanks alert flag
        }
        break;

        case(WATER_ALERT):
        {
            speakAlert |= WATER_ALERT; // Set water alert flag
        }
        break;

        case(LIGHT_ALERT):
        {
            speakAlert |= LIGHT_ALERT; // Set light alert flag
        }
        break;
    }
}

uint8_t soilDry()
{
    uint8_t state = NO;

    Serial.print("Water ");
    Serial.println(sensorSoil.value);

    if (sensorSoil.value >= SOIL_THRESHOLD) state = YES; // The bigger the value of sensorSoil, the dryer the soil

    return state;
}

uint8_t light()
{
    uint8_t state = ON;

    Serial.print("Light ");
    Serial.println(sensorLight.value);

    if (sensorLight.value <= LIGHT_THRESHOLD) state = OFF; // The lower the value of lightSensor, less light

    return state;
}

uint32_t timeDelta(const uint32_t t1, const uint32_t reference)
{
    return (t1 >= reference) ? (t1 - reference) : ((0xFFFFFFFF - reference) + t1);
}


void read_Sensor(Sensor_t* sensor)
{
    if (sensor->read == ON)
    {
        //Serial.print("sample ");
        //Serial.println(sensor->sampleCounter);

        sensor->sumSamples    += analogRead((sensor == &sensorSoil) ? (SOIL_SENSOR_ADC) : (LIGHT_SENSOR_ADC));    // Reads the sensor value
        sensor->sampleCounter += 1;                              // Increments the sample variable
    }
    
    if (sensor->sampleCounter >= SAMPLES_NUMBER)
    {
        uint16_t average = (uint16_t)(sensor->sumSamples / SAMPLES_NUMBER);    // Calculate the average

        sensor->value         = average;            // Sets the new sensor average

        #if LOGGER == ON
            sd_Dataloger(sensor);
        #endif

        sensor->sampleCounter = 0;                  // Resets the samples to aquire a new average
        sensor->sumSamples    = 0;                  // Resets the average to aquire a new average
        sensor->read          = OFF;                // Resets the sample flag
    }
}

void sd_Dataloger(Sensor_t* sensor)
{
    File dataFile;

    // open the file. note that only one file can be open at a time,
    // so you have to close this one before opening another.
    if (sensor == &sensorSoil)        dataFile = SD.open("WATERLOG.txt", FILE_WRITE);
    else /*(sensor == &sensorLight)*/ dataFile = SD.open("LIGHTLOG.txt", FILE_WRITE);
    
    // if the file is available, write to it:
    if (dataFile) 
    {
        dataFile.println(sensor->value);
        dataFile.close();

        // Serial.println(sensor->value);
    }
    // if the file isn't open, pop up an error:
    else 
    {
        Serial.println("error opening lightlog.txt");
    }
}

void init_TimerSensors(void)
{
    cli();  // Disable global interrupts

    TCCR0A = 0;     // set entire TCCR0A register to 0

    TCCR0B = 0;     // same for TCCR0B

    TCNT0 = 0;      // initialize counter value to 0

    // set compare match register for 1khz increments
    OCR0A = 150;    // = (16*10^6) / (1000*1024) - 1 (must be <256)

    // turn on CTC mode
    TCCR0A |= (1 << WGM01);

    // Set CS01 and CS00 bits for 1024 prescaler
    TCCR0B |= (1 << CS02) | (1 << CS00);

    // enable timer compare interrupt
    TIMSK0 |= (1 << OCIE0A);

    sei(); // Enable global interrupts

}

ISR(TIMER0_COMPA_vect) //timer0 interrupt 10Hz
{
    timerMillis += 1;  // Counts 100 more millisecond

    if (timeDelta(timerMillis, lastTimeRef) >= (SAMPLES_INTERVAL))
    {
        lastTimeRef       = timerMillis;
        flowerState.speak = ON;     // Enable speaking machine
        sensorSoil.read   = ON;     // Enable soilMoister sensor reading
        sensorLight.read  = ON;     // Enable light sensor reading
    }
}

//TODO: DELETE!?
/*
#if (SPEAKING == ON)
{
    /*
    if (Serial.available())
    {
        switch (Serial.read())
        {
        case 'd': tmrpcm.play("music.wav"); break;

        case 'b': tmrpcm.play("BeatIt.wav"); break;

        case 'r': tmrpcm.play("Race_Car.wav"); break;

        case 'p': tmrpcm.pause(); break;

        case '?': if (tmrpcm.isPlaying()) { Serial.println("A wav file is being played"); } break;

        case 'S': tmrpcm.stopPlayback(); break;

        case '=': tmrpcm.volume(1); break;

        case '-': tmrpcm.volume(0); break;

        case '0': tmrpcm.quality(0); break;

        case '1': tmrpcm.quality(1); break;

        default: break;
        }
    }
}
#endif
    */
