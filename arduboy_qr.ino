// haloopdy - 2024-01-04

// Uncomment to use the arduboy FX version
//#define FXVERSION

#include <Arduboy2.h>
#include "qrcodegen.h"

Arduboy2 arduboy;

constexpr uint8_t FRAMERATE = 60;
constexpr uint8_t QRFRAMERATE = 5;

// Constants for QR code generation. You generally don't want to change these
const uint8_t QR_MAXVERSION = 9;
const qrcodegen_Ecc QR_MINECC = qrcodegen_Ecc_MEDIUM;
const bool QR_BOOSTECL = true;
const uint8_t QR_OVERDRAW = 2; //Amount on each side to overdraw the qr rect

// Dimensions of screen in characters (for normal font)
constexpr uint8_t FONT_WIDTH = 6;
constexpr uint8_t FONT_HEIGHT = 8;
constexpr uint8_t SCREENCHARWIDTH = WIDTH / FONT_WIDTH;
constexpr uint8_t SCREENCHARHEIGHT = HEIGHT / FONT_HEIGHT;

// Input and keyboard dimensions
constexpr uint8_t MAXLINEWIDTH = SCREENCHARWIDTH;
constexpr uint8_t MAXINPUT = MAXLINEWIDTH * (128 / MAXLINEWIDTH); //Arbitrarily chosen to fit within 128
constexpr uint8_t KEYBOARD_WIDTH = SCREENCHARWIDTH;
constexpr uint8_t KEYBOARD_HEIGHT = 5;
constexpr uint8_t KEYBOARD_STARTY = SCREENCHARHEIGHT - KEYBOARD_HEIGHT; //In characters
constexpr uint8_t KEYBOARD_STARTX = 0;
constexpr uint8_t INPUT_PIXELHEIGHT = (SCREENCHARHEIGHT - KEYBOARD_HEIGHT) * FONT_HEIGHT;

// The keyboard itself
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

#define MENUWRAP(var, max, mov) { var = (var + mov + max) % max; }

// States the "game" could be in (idk, I just always call it that)
enum GameState {
    About,
    Entry,
    FullInput,
    Display
};


// Our few little global variables
char input[MAXINPUT + 1];

//Just preallocate the maximum supported buffer size
uint8_t qrbuffer[qrcodegen_BUFFER_LEN_FOR_VERSION(QR_MAXVERSION)];
uint8_t tempbuffer[qrcodegen_BUFFER_LEN_FOR_VERSION(QR_MAXVERSION)];

uint8_t kbx, kby, okbx, okby;
GameState state;

unsigned long buttonRepeats[BUTTONCOUNT] = {0};



void setup()
{
    // Initialize the Arduboy
    arduboy.begin();
    arduboy.setFrameRate(FRAMERATE);
    setStateAbout();
}


void setTextCursor(int x, int y) {
    arduboy.setCursor(x * FONT_WIDTH, y * FONT_HEIGHT);
}

void setTextInvert(bool inverted) {
    arduboy.setTextColor(inverted ? BLACK : WHITE);
    arduboy.setTextBackground(inverted ? WHITE : BLACK);
}

void printCentered(char * text) {
    arduboy.setCursor((WIDTH - strlen(text) * FONT_WIDTH) / 2, (HEIGHT - FONT_HEIGHT) / 2);
    arduboy.print(text);
}


// Return the character on the keyboard at the given location
void resetKeyboard()
{
    // There are several values related to the keyboard. button repetition
    // isn't necessarily just for the keyboard, but we need it reset upon start
    kbx = 0; kby = 0;
    okbx = -1; okby = -1;
    memset(buttonRepeats, 0, BUTTONCOUNT);
}

char getKeyboardAt(uint8_t x, uint8_t y)
{
    return pgm_read_byte(KEYLINES[y] + x);
}

// Print a single key. It's up to you to invert it or not
void printKeyboardKey(uint8_t x, uint8_t y)
{
    setTextCursor(KEYBOARD_STARTX + x, KEYBOARD_STARTY + y);
    arduboy.print(getKeyboardAt(x, y));
}

void printKeyboard(bool full)
{
    char line[KEYBOARD_WIDTH + 1];
    line[KEYBOARD_WIDTH] = 0;

    setTextInvert(false);

    //Printing the keyboard is a bit funny. We don't really need to print the full
    //keyboard most of the time. Tracking when that happens kind of sucks, but our 
    //program is generally simple enough that it doesn't matter.
    if(full)
    {
        for (uint8_t i = 0; i < KEYBOARD_HEIGHT; i++)
        {
            memcpy_P(line, KEYLINES[i], KEYBOARD_WIDTH);
            setTextCursor(KEYBOARD_STARTX, KEYBOARD_STARTY + i);
            arduboy.print(line);
        }
    }
    else if (okbx != kbx || okby != kby)
    {
        // Reset the old location
        printKeyboardKey(okbx, okby);
    }

    // Print the new location inverted
    setTextInvert(true);
    printKeyboardKey(kbx, kby);
    setTextInvert(false);
    
    // Save the last printed location.
    okbx = kbx;
    okby = kby;
}

// Function to enable simple button repetition. Returns whether a button
// is considered pressed for this frame. ONLY CALL THIS ONCE PER PER BUTTON!
// Later we may make it safe to call as often as you wish
bool doRepeat(uint8_t button)
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

    if (arduboy.pressed(button))
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


// Print the input portion of the screen. The input includes the horizontal line
// above the keyboard, since it's used to display certain stats
void printInput()
{
    // Clear entire input area
    arduboy.fillRect(0, 0, WIDTH, INPUT_PIXELHEIGHT, BLACK);

    uint8_t inlen = strlen(input);

    // Draw the rect for the buffer fullness. This rect only works if the input 
    // is less than 128!! IDK, maybe have an assert somewhere? Those don't really work...
    arduboy.drawRect(WIDTH - MAXINPUT - 2, INPUT_PIXELHEIGHT - 4, WIDTH, 3, WHITE);
    arduboy.drawFastHLine(WIDTH - MAXINPUT - 1, INPUT_PIXELHEIGHT - 3, inlen, WHITE);

    setTextCursor(0,0);
    // We display two lines at a time, but the math works out to just - MAXLINEWIDTH
    uint8_t offset = min(max(0, MAXLINEWIDTH * ((inlen - MAXLINEWIDTH) / MAXLINEWIDTH)), MAXINPUT - 2 * MAXLINEWIDTH);
    uint8_t ooffset = offset;
    char line[MAXLINEWIDTH + 1];

    while(offset < inlen && offset < MAXINPUT)
    {
        memcpy(line, input + offset, MAXLINEWIDTH);
        line[MAXLINEWIDTH] = 0;
        arduboy.println(line);
        offset += MAXLINEWIDTH;
    }

    if(inlen < MAXINPUT)
    {
        // Print a cursor (could just make a rect...)
        setTextCursor((inlen - ooffset) % MAXLINEWIDTH, (inlen - ooffset) / MAXLINEWIDTH);
        setTextInvert(true);
        arduboy.print(" ");
        setTextInvert(false);
    }
}

void printFullInput()
{
    uint8_t inlen = strlen(input);

    setTextCursor(0,0);

    // We can display the full window
    uint8_t offset = 0;
    char line[MAXLINEWIDTH + 1];

    while(offset < inlen && offset < MAXINPUT)
    {
        memcpy(line, input + offset, MAXLINEWIDTH);
        line[MAXLINEWIDTH] = 0;
        arduboy.println(line);
        offset += MAXLINEWIDTH;
    }
}

void tryAddInput()
{
    input[min(strlen(input), MAXINPUT - 1)] = getKeyboardAt(kbx, kby);
}

void tryRemoveInput()
{
    input[max(0, strlen(input) - 1)] = 0;
}

bool generateQr(char * text)
{
    //"https://github.com/nayuki/QR-Code-generator/tree/master/c",
    return qrcodegen_encodeText(
        text, tempbuffer, qrbuffer, 
        QR_MINECC, qrcodegen_VERSION_MIN, QR_MAXVERSION, qrcodegen_Mask_AUTO, QR_BOOSTECL);
}

void drawQr()
{
    int size = qrcodegen_getSize(qrbuffer);
    int sizemod = floor((float)(HEIGHT - 2 * QR_OVERDRAW) / size);
    int realsize = sizemod * size;
    int startx = (WIDTH - realsize - 2 * QR_OVERDRAW) / 2;
    int starty = (HEIGHT - realsize - 2 * QR_OVERDRAW) / 2;

    // White rect for qr code background.
    arduboy.fillRect(startx, starty, realsize + 2 * QR_OVERDRAW, realsize + 2 * QR_OVERDRAW, WHITE);

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            // Silly, but we technically only need to draw the black pixels. It's slightly less code too?
            if(qrcodegen_getModule(qrbuffer, x, y)) {
                arduboy.fillRect(startx + x * sizemod + QR_OVERDRAW, starty + y * sizemod + QR_OVERDRAW, sizemod, sizemod, BLACK);
            }
        }
    }
}


//For the various states, we display portions of the static screen to 
//reduce battery usage. I think...
void setStateAbout()
{
    state = GameState::About;
    arduboy.clear();

    // Wasting serious program memory here but whatever,
    // also serves as a full clear if I need it (or does it?)
    arduboy.print(F(" -[haloopdy - 2024]- \n"));
    arduboy.print(F(" ------------------- \n"));
    arduboy.print(F("  QR Code Displayer  \n\n"));
    arduboy.print(F(" * Enter text        \n"));
    arduboy.print(F(" * All text: (\x19\x42) \n"));
    arduboy.print(F(" * Show QR: (\x19\x1A\x42)\n"));
    arduboy.print(F(" * Hide QR: (\x18\x1A\x42)\n"));
    arduboy.print(F(" * Limit "));
    arduboy.print(MAXINPUT);
    arduboy.print(F(" chars    \n"));

    // This way, it's only reset if you go all the way to the title screen
    memset(input, 0, MAXINPUT + 1);
}

void setStateEntry()
{
    arduboy.setFrameRate(FRAMERATE);
    // Notice that we don't reset the input here. This lets you exit the
    // qr and still have your input there, so don't clear it here!
    state = GameState::Entry;
    arduboy.clear();
    resetKeyboard();
    printKeyboard(true);
    printInput();
}

void setStateFullInput()
{
    state = GameState::FullInput;
    arduboy.clear();
    printFullInput();
}

void setStateDisplay()
{
    arduboy.setFrameRate(QRFRAMERATE);
    state = GameState::Display;
    arduboy.clear();
    printCentered("Generating");
    arduboy.display();
    generateQr(input);
    arduboy.clear();
    drawQr();
}



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
        if (doRepeat(UP_BUTTON))
            MENUWRAP(kby, KEYBOARD_HEIGHT, -1);
        if (doRepeat(DOWN_BUTTON))
            MENUWRAP(kby, KEYBOARD_HEIGHT, 1);
        if (doRepeat(LEFT_BUTTON))
            MENUWRAP(kbx, KEYBOARD_WIDTH, -1);
        if (doRepeat(RIGHT_BUTTON))
            MENUWRAP(kbx, KEYBOARD_WIDTH, 1);

        printKeyboard(false); // false = not the full keyboard

        if (arduboy.pressed(COMBO_SHOWINPUT))
            setStateFullInput();

        if (arduboy.justPressed(A_BUTTON))
        {
            tryAddInput();
            printInput();
        }
        if (doRepeat(B_BUTTON) && !arduboy.anyPressed(DIR_BUTTONS))
        {
            tryRemoveInput();
            printInput();
        }

        // Special combo to generate qr
        if (arduboy.pressed(COMBO_SHOWQR))
            setStateDisplay();
    }
    else if(state == GameState::FullInput)
    {
        //A very simple state indeed
        if(!arduboy.pressed(COMBO_SHOWINPUT))
            setStateEntry();

        // Special combo to generate qr
        if (arduboy.pressed(COMBO_SHOWQR))
            setStateDisplay();
    }
    else if(state == GameState::Display)
    {
        // Special combo to exit qr
        if (arduboy.pressed(COMBO_HIDEQR))
            setStateEntry();
    }

    arduboy.display();
}
