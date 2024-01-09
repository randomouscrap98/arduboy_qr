#pragma ONCE

#include <Arduboy2.h>

constexpr uint8_t FONT_WIDTH = 6;
constexpr uint8_t FONT_HEIGHT = 8;
constexpr uint8_t SCREENCHARWIDTH = WIDTH / FONT_WIDTH;
constexpr uint8_t SCREENCHARHEIGHT = HEIGHT / FONT_HEIGHT;

constexpr uint8_t KEYBOARD_WIDTH = SCREENCHARWIDTH;
constexpr uint8_t KEYBOARD_HEIGHT = 5;
constexpr uint8_t KEYBOARD_STARTY = SCREENCHARHEIGHT - KEYBOARD_HEIGHT; //In characters
constexpr uint8_t KEYBOARD_STARTX = 0;

// The default keyboard
constexpr char KEYLINE1[] PROGMEM = "ABCDEFGHIJKLM[01234]?";
constexpr char KEYLINE2[] PROGMEM = "NOPQRSTUVWXYZ(56789)!";
constexpr char KEYLINE3[] PROGMEM = "abcdefghijklm{./:@&}#";
constexpr char KEYLINE4[] PROGMEM = "nopqrstuvwxyz<+-=*%>$";
constexpr char KEYLINE5[] PROGMEM = "     |^_\\;,~`\"'      ";
const char * KEYLINES[KEYBOARD_HEIGHT] = {
    KEYLINE1, KEYLINE2, KEYLINE3, KEYLINE4, KEYLINE5
};

// Button / repetition constants (greatly changes the feel of the keyboard)
constexpr uint16_t REPEATDELAY = 250;
constexpr uint16_t REPEATREPEAT = 50;
constexpr uint8_t BUTTONCOUNT = 6;
constexpr uint8_t DIR_BUTTONS = UP_BUTTON | DOWN_BUTTON | LEFT_BUTTON | RIGHT_BUTTON;
constexpr uint8_t COMBO_SHOWQR = DOWN_BUTTON | RIGHT_BUTTON | B_BUTTON;
constexpr uint8_t COMBO_HIDEQR = UP_BUTTON | RIGHT_BUTTON | B_BUTTON;
constexpr uint8_t COMBO_SHOWINPUT = DOWN_BUTTON | B_BUTTON;

// Basic menu wrapping for any kind of menu
#define MENUWRAP(var, max, mov) { var = (var + mov + max) % max; }


void setTextCursor(Arduboy2 * arduboy, uint8_t x, uint8_t y) {
    arduboy->setCursor(x * FONT_WIDTH, y * FONT_HEIGHT);
}

void setTextInvert(Arduboy2 * arduboy, bool inverted) {
    arduboy->setTextColor(inverted ? BLACK : WHITE);
    arduboy->setTextBackground(inverted ? WHITE : BLACK);
}

void setCentered(Arduboy2 * arduboy, uint8_t len) {
    arduboy->setCursor((WIDTH - len * FONT_WIDTH) / 2, (HEIGHT - FONT_HEIGHT) / 2);
}


class ButtonRepeater
{
    private:
        Arduboy2 * arduboy;
        unsigned long buttonRepeats[BUTTONCOUNT] = {0};

    public:
        ButtonRepeater(Arduboy2 * arduboy)
        {
            this->arduboy = arduboy;
            this->reset();
        }

        void reset() {
            memset(buttonRepeats, 0, BUTTONCOUNT);
        }

        // Function to enable simple button repetition. Returns whether a button
        // is considered pressed for this frame. ONLY CALL THIS ONCE PER PER BUTTON!
        // Later we may make it safe to call as often as you wish
        bool repeat(uint8_t button)
        {
            uint8_t i = 0;

            switch(button)
            {
                case A_BUTTON: i = 0; break;
                case B_BUTTON: i = 1; break;
                case UP_BUTTON: i = 2; break;
                case DOWN_BUTTON: i = 3; break;
                case LEFT_BUTTON: i = 4; break;
                case RIGHT_BUTTON: i = 5; break;
            }

            bool pressed = false;

            if (arduboy->pressed(button))
            {
                if(millis() > buttonRepeats[i])
                {
                    buttonRepeats[i] = millis() + (buttonRepeats[i] ? REPEATREPEAT : REPEATDELAY);
                    pressed = true;
                }
            }
            else
            {
                buttonRepeats[i] = 0;
            }

            return pressed;
        }
};


class BasicKeyboard
{
    //Not sure yet if I want these exposed...
    private:
        // Keyboard state
        uint8_t kbx, kby, okbx, okby;
        Arduboy2 * arduboy;

        // Print a single key. It's up to you to invert it or not
        void printKeyboardKey(uint8_t x, uint8_t y)
        {
            setTextCursor(arduboy, KEYBOARD_STARTX + x, KEYBOARD_STARTY + y);
            arduboy->print(getKeyboardAt(x, y));
        }

        // Return the character at a given x/y
        char getKeyboardAt(uint8_t x, uint8_t y)
        {
            return pgm_read_byte(KEYLINES[y] + x);
        }


    public:
        BasicKeyboard(Arduboy2 * arduboy) {
            this->arduboy = arduboy;
            reset();
        }

        inline char getCurrentChar() {
            return getKeyboardAt(kbx, kby);
        }

        // Return everything to defaults
        void reset()
        {
            // There are several values related to the keyboard. button repetition
            // isn't necessarily just for the keyboard, but we need it reset upon start
            kbx = 0; kby = 0;
            okbx = -1; okby = -1;
            //memset(buttonRepeats, 0, BUTTONCOUNT);
        }

        void runFrame(ButtonRepeater * repeater)
        {
            if (repeater->repeat(UP_BUTTON))
                MENUWRAP(kby, KEYBOARD_HEIGHT, -1);
            if (repeater->repeat(DOWN_BUTTON))
                MENUWRAP(kby, KEYBOARD_HEIGHT, 1);
            if (repeater->repeat(LEFT_BUTTON))
                MENUWRAP(kbx, KEYBOARD_WIDTH, -1);
            if (repeater->repeat(RIGHT_BUTTON))
                MENUWRAP(kbx, KEYBOARD_WIDTH, 1);

            print(false); // false = not the full keyboard
        }

        void print(bool full)
        {
            char line[KEYBOARD_WIDTH + 1];
            line[KEYBOARD_WIDTH] = 0;

            setTextInvert(arduboy, false);

            //Printing the keyboard is a bit funny. We don't really need to print the full
            //keyboard most of the time. Tracking when that happens kind of sucks, but our 
            //program is generally simple enough that it doesn't matter.
            if(full)
            {
                for (uint8_t i = 0; i < KEYBOARD_HEIGHT; i++)
                {
                    memcpy_P(line, KEYLINES[i], KEYBOARD_WIDTH);
                    setTextCursor(arduboy, KEYBOARD_STARTX, KEYBOARD_STARTY + i);
                    arduboy->print(line);
                }
            }
            else if (okbx != kbx || okby != kby)
            {
                // Reset the old location
                printKeyboardKey(okbx, okby);
            }

            // Print the new location inverted
            setTextInvert(arduboy, true);
            printKeyboardKey(kbx, kby);
            setTextInvert(arduboy, false);
            
            // Save the last printed location.
            okbx = kbx;
            okby = kby;
        }
};

