#include <Arduboy2.h>

Arduboy2 arduboy;

const uint8_t FRAMERATE = 60;

void setup()
{
    // Initialize the Arduboy
    arduboy.boot();
    arduboy.flashlight();
    arduboy.setFrameRate(FRAMERATE);
}


void loop()
{
    if(!arduboy.nextFrame()) return;
    arduboy.pollButtons();

    arduboy.display();
}
