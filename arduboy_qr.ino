// Uncomment to use the arduboy FX version
//#define FXVERSION

#include <Arduboy2.h>
#include "qrcodegen.h"

Arduboy2 arduboy;

const uint8_t FRAMERATE = 60;

const uint8_t MAXLINEWIDTH = 21;
const uint8_t MAXINPUT = MAXLINEWIDTH * 3;
char input[MAXINPUT + 1];

const uint8_t MAXVERSION = 9;
const qrcodegen_Ecc MINECC = qrcodegen_Ecc_MEDIUM;
const bool BOOSTECL = true;

//Just preallocate the maximum supported buffer size
uint8_t qrbuffer[qrcodegen_BUFFER_LEN_FOR_VERSION(MAXVERSION)];
uint8_t tempbuffer[qrcodegen_BUFFER_LEN_FOR_VERSION(MAXVERSION)];

const uint8_t KEYBOARD_LINELENGTH = 21;
const uint8_t KEYBOARD_LINECOUNT = 5;
const uint8_t KEYBOARD_STARTY = 3;
const uint8_t KEYBOARD_STARTX = 0;

const char KEYLINE1[] PROGMEM = "ABCDEFGHIJKLM[01234]?";
const char KEYLINE2[] PROGMEM = "NOPQRSTUVWXYZ(56789)!";
const char KEYLINE3[] PROGMEM = "abcdefghijklm{./:@&}#";
const char KEYLINE4[] PROGMEM = "nopqrstuvwxyz<+-=*%>$";
const char KEYLINE5[] PROGMEM = "     |^_\\;,~`\"'      ";

const char * KEYLINES[KEYBOARD_LINECOUNT];

enum GameState {
    About,
    Entry,
    Display
};

uint8_t kbx, kby;
GameState state;

void setup()
{
    //Really kinda dumb lol
    KEYLINES[0] = KEYLINE1;
    KEYLINES[1] = KEYLINE2;
    KEYLINES[2] = KEYLINE3;
    KEYLINES[3] = KEYLINE4;
    KEYLINES[4] = KEYLINE5;

    // Initialize the Arduboy
    arduboy.begin();
    arduboy.setFrameRate(FRAMERATE);

    setStateAbout();
}

void resetKeyboard()
{
    kbx = 0;
    kby = 0;
}

void setTextCursor(int x, int y) {
    arduboy.setCursor(x * 6, y * 8);
}
void setTextInvert(bool inverted) {
    arduboy.setTextColor(inverted ? BLACK : WHITE);
    arduboy.setTextBackground(inverted ? WHITE : BLACK);
}

void printKeyboard(uint8_t cx, uint8_t cy)
{
    char line[KEYBOARD_LINELENGTH + 1];
    line[KEYBOARD_LINELENGTH] = 0;

    setTextInvert(false);

    for(uint8_t i = 0; i < KEYBOARD_LINECOUNT; i++)
    {
        memcpy_P(line, KEYLINES[i], KEYBOARD_LINELENGTH);
        setTextCursor(KEYBOARD_STARTX, KEYBOARD_STARTY + i);
        arduboy.print(line);
    }

    setTextInvert(true);
    setTextCursor(KEYBOARD_STARTX + cx, KEYBOARD_STARTY + cy);
    arduboy.print((char)pgm_read_byte(KEYLINES[cy] + cx));

    setTextInvert(false);
}

void printInput()
{
    arduboy.fillRect(0, 0, 128, 24, BLACK);
    arduboy.drawFastHLine(0, 22, 128, WHITE);
    setTextCursor(0,0);
    uint8_t offset = 0;
    char line[MAXLINEWIDTH + 1];
    while(offset < strlen(input) && offset < MAXINPUT)
    {
        memcpy(line, input + offset, MAXLINEWIDTH);
        line[MAXLINEWIDTH] = 0;
        arduboy.println(line);
        offset += MAXLINEWIDTH;
    }
}

char getKeyboardAt(uint8_t cx, uint8_t cy)
{
    return pgm_read_byte(KEYLINES[cy] + cx);
}

bool generateQr(char * text)
{
    //"https://github.com/nayuki/QR-Code-generator/tree/master/c",
    return qrcodegen_encodeText(
        text, tempbuffer, qrbuffer, 
        MINECC, qrcodegen_VERSION_MIN, MAXVERSION, qrcodegen_Mask_AUTO, BOOSTECL);
}

//For the various states, we display portions of the static screen to 
//reduce battery usage. I think...
void setStateAbout()
{
    state = GameState::About;
    arduboy.clear();
    arduboy.print(F(" -[haloopdy - 2023]-\n"));
    arduboy.print(F(" -------------------\n"));
    arduboy.print(F("  QR Code Displayer \n\n"));
    arduboy.print(F(" * Enter text\n"));
    arduboy.print(F(" * On: Down+Right+B\n"));
    arduboy.print(F(" * Off: Up+Right+B\n"));
    arduboy.print(F(" * Limit 100 chars\n"));

    // This way, it's only reset if you go all the way to the title screen
    memset(input, 0, MAXINPUT + 1);
}

void setStateEntry()
{
    state = GameState::Entry;
    resetKeyboard();
    arduboy.clear();
    printKeyboard(kbx, kby);
    printInput();
}

void setStateDisplay()
{
    state = GameState::Display;

    //generateQr("https://github.com/nayuki/QR-Code-generator/tree/master/c");
    arduboy.clear();
    generateQr(input);

    int size = qrcodegen_getSize(qrbuffer);
    int sizemod = floor(60.0 / size);
    int realsize = sizemod * size;
    int startx = (128 - realsize - 4) / 2;
    int starty = (64 - realsize - 4) / 2;

    arduboy.fillRect(startx, starty, realsize + 4, realsize + 4, WHITE);
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            arduboy.fillRect(startx + x * sizemod + 2, starty + y * sizemod + 2, sizemod, sizemod, qrcodegen_getModule(qrbuffer, x, y) ? BLACK : WHITE);
        }
    }
}

#define MENUWRAP(var, max, mov) { var = (var + mov + max) % max; }

unsigned long lastInputStart = 0;
unsigned long lastInputRepeat = 0;
const uint8_t DIR_BUTTONS = UP_BUTTON | DOWN_BUTTON | LEFT_BUTTON | RIGHT_BUTTON;
const uint16_t REPEATDELAY = 250;
const uint16_t REPEATREPEAT = 50;

void loop()
{
    if(!arduboy.nextFrame()) return;
    arduboy.pollButtons();

    if(state == GameState::About)
    {
        if(arduboy.justPressed(A_BUTTON))
            setStateEntry();
    }
    else if(state == GameState::Entry)
    {
        if (arduboy.anyPressed(DIR_BUTTONS))
        {
            unsigned long ms = millis();

            if(ms - lastInputRepeat > REPEATREPEAT && ms - lastInputStart > REPEATDELAY)
            {
                if(!lastInputStart) lastInputStart = ms;
                lastInputRepeat = ms;

                if (arduboy.pressed(UP_BUTTON))
                    MENUWRAP(kby, KEYBOARD_LINECOUNT, -1);
                if (arduboy.pressed(DOWN_BUTTON))
                    MENUWRAP(kby, KEYBOARD_LINECOUNT, 1);
                if (arduboy.pressed(LEFT_BUTTON))
                    MENUWRAP(kbx, KEYBOARD_LINELENGTH, -1);
                if (arduboy.pressed(RIGHT_BUTTON))
                    MENUWRAP(kbx, KEYBOARD_LINELENGTH, 1);

                printKeyboard(kbx, kby);
            }
        }
        else
        {
            //When they let go of the direction, don't delay their inputs anymore
            lastInputRepeat = 0;
            lastInputStart = 0;
        }

        if (arduboy.justPressed(A_BUTTON))
        {
            input[strlen(input)] = pgm_read_byte(KEYLINES[kby] + kbx);
            printInput();
        }
        if (arduboy.justPressed(B_BUTTON) && !arduboy.anyPressed(DIR_BUTTONS))
        {
            input[max(0, strlen(input) - 1)] = 0;
            printInput();
        }

        //Special combo to generate qr
        if (arduboy.pressed(DOWN_BUTTON | RIGHT_BUTTON | B_BUTTON))
            setStateDisplay();
    }
    else if(state == GameState::Display)
    {
        if (arduboy.pressed(UP_BUTTON | RIGHT_BUTTON | B_BUTTON))
            setStateEntry();
    }

    arduboy.display();
}
