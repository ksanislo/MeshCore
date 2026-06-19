#pragma once

// HAS_BUZZER is set when any buzzer backend is available.
// UITask uses HAS_BUZZER as the single guard; backend is selected below.
#if defined(PIN_BUZZER)
  #define HAS_BUZZER 1
#elif defined(PIN_I2S_BCK)
  #define HAS_BUZZER 1
  #define BUZZER_IS_I2S 1
#endif

#if defined(PIN_BUZZER)
#include <Arduino.h>
#include <NonBlockingRtttl.h>

/* class abstracts underlying RTTTL library 

    Just a simple implementation to start.  At the moment use same
    melody for message and discovery
    Suggest enum type for different sounds
    - on message
    - on discovery

    TODO
    - make message ring tone configurable

*/

class genericBuzzer
{
    public:
        void begin();  // set up buzzer port
        void play(const char *melody); // Generic play function
        void loop();  // loop driven-nonblocking
        void startup();  // play startup sound
        void shutdown();  // play shutdown sound
        bool isPlaying();  // returns true if a sound is still playing else false
        void quiet(bool buzzer_state);  // enables or disables the buzzer
        bool isQuiet();  // get buzzer state on/off

    private:
        // gemini's picks:
        const char *startup_song = "Startup:d=4,o=5,b=160:16c6,16e6,8g6";
        const char *shutdown_song = "Shutdown:d=4,o=5,b=100:8g5,16e5,16c5";

        bool _is_quiet = true;
};
#endif // PIN_BUZZER
