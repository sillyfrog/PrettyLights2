// Extra functions to add to a build of PrettyLights2
#include <Arduino.h>

#define EXTRAS_VERSION "0.0.1"

void extrasPreSetup()
{
    // Called just before the LED, WiFi etc setup is run. This is also before the
    // PWM LED's are setup, but after the serial port is configured.
}

void extrasPostSetup()
{
    // Called after everything is setup and ready to go. A good place to override
    // GPIO's etc.
}

void extrasLoop()
{
    // Called every main loop, due to the pixel setup, this maybe have large delays
    // depending on the number of LED's.
}

void extrasPostFrameSet(uint16 currentframe)
{
    // Called the moment after the frames have been set, but well before they are applied
    // currentframe in the currentframe counter
}

void extrasPreFrameApply(uint16 currentframe)
{
    // Called the moment before the frames are applied. This must be very fast to
    // minimise lag when applying frames.
    // currentframe in the currentframe counter
}