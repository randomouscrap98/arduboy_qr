// haloopdy - 2024-01-04

// Comment out to generate the standard arduboy version (no saving)
#define FXVERSION

#include <Arduboy2.h>
#include "qrcodegen.h"
#include "keyboard.h"


constexpr uint8_t FRAMERATE = 60;
constexpr uint8_t QRFRAMERATE = 5;

// Constants for QR code generation. You generally don't want to change these
const uint8_t QR_MAXVERSION = 10;
const qrcodegen_Ecc QR_MINECC = qrcodegen_Ecc_MEDIUM;
const bool QR_BOOSTECL = true;
const uint8_t QR_OVERDRAW = 2; //Amount on each side to overdraw the qr rect
const uint16_t QR_BUFSIZE = qrcodegen_BUFFER_LEN_FOR_VERSION(QR_MAXVERSION);

// Input and keyboard dimensions
constexpr uint8_t MAXLINEWIDTH = SCREENCHARWIDTH;
constexpr uint8_t MAXINPUT = MAXLINEWIDTH * (128 / MAXLINEWIDTH); //Arbitrarily chosen to fit within 128
constexpr uint8_t INPUT_PIXELHEIGHT = (SCREENCHARHEIGHT - KEYBOARD_HEIGHT) * FONT_HEIGHT;


// States the "game" could be in (idk, I just always call it that)
enum GameState {
    About,
    Entry,
    SlotSelect, // This is an FX exclusive, but it's fine to exist
    FullInput,
    Display
};


// Our few little global variables
Arduboy2 arduboy;
GameState state;
BasicKeyboard keyboard(&arduboy);
ButtonRepeater repeater(&arduboy);

// Buffers (could probably get rid of input with clever badness)
char input[MAXINPUT + 1];
uint8_t allbuffer[2 * QR_BUFSIZE];


#ifdef FXVERSION
#include <ArduboyFX.h>      // required to access the FX external flash
#include "fx/fxdata.h"

constexpr uint8_t DISPLAYSLOTS = SCREENCHARHEIGHT - 1;
constexpr uint8_t FX_SLOTCOUNT = (2 * QR_BUFSIZE) / sizeof(input);

typedef struct 
{
    char inputs[FX_SLOTCOUNT][sizeof(input)];
} SaveData;

// Add these together to get the real slot
uint8_t slot_cursor = 0;

// Load all the slots out of FX into the giant allbuffer
void loadSlots()
{
    SaveData& savedata = *(SaveData*)allbuffer;
    if(!FX::loadGameState(savedata))
    {
        //This is a new state, initialize all to 0
        for(uint8_t i = 0; i < FX_SLOTCOUNT; i++)
            savedata.inputs[i][0] = 0;
    }
}

// Save the slots (must be valid at point of writing of course)
void saveSlots()
{
    SaveData& savedata = *(SaveData*)allbuffer;
    FX::saveGameState(savedata);
}

void displayCurrentSlot()
{
    arduboy.clear();
    arduboy.setCursor(0,0);
    arduboy.print(F("   --< Slot "));
    arduboy.print(slot_cursor + 1); //char('A' + slot_cursor));
    arduboy.print(F(": >--   \n\n"));
    
    SaveData * data = (SaveData *)allbuffer;
    uint8_t dlen = strlen(data->inputs[slot_cursor]);
    char line[SCREENCHARWIDTH + 1] = {0};

    for(uint8_t offset = 0; offset < dlen; offset += SCREENCHARWIDTH)
    {
        memcpy(line, data->inputs[slot_cursor] + offset, SCREENCHARWIDTH);
        arduboy.println(line);
        arduboy.print("\n");
    }
}

void copySlotToInput()
{
    SaveData * data = (SaveData *)allbuffer;
    memcpy(input, data->inputs[slot_cursor], sizeof(input));
}

void copyInputToSlot()
{
    SaveData * data = (SaveData *)allbuffer;
    memcpy(data->inputs[slot_cursor], input, sizeof(input));
}

#endif



void setup()
{
    // Initialize the Arduboy
    arduboy.begin();
    arduboy.setFrameRate(FRAMERATE);
    #ifdef FXVERSION
    FX::begin(FX_DATA_PAGE, FX_SAVE_PAGE);    // initialise FX chip
    #endif
    setStateAbout();
}


void doDisplay()
{
    #ifdef FXVERSION
    FX::display(false);
    #else
    arduboy.display();
    #endif
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

    setTextCursor(&arduboy, 0,0);
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
        setTextCursor(&arduboy, (inlen - ooffset) % MAXLINEWIDTH, (inlen - ooffset) / MAXLINEWIDTH);
        setTextInvert(&arduboy, true);
        arduboy.print(" ");
        setTextInvert(&arduboy, false);
    }
}

void printFullInput()
{
    uint8_t inlen = strlen(input);

    setTextCursor(&arduboy, 0,0);

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
    uint8_t inlen = strlen(input);
    input[min(inlen, MAXINPUT - 1)] = keyboard.getCurrentChar();
    input[min(inlen + 1, MAXINPUT)] = 0;
}

void tryRemoveInput()
{
    input[max(0, strlen(input) - 1)] = 0;
}

bool generateQr(char * text)
{
    //"https://github.com/nayuki/QR-Code-generator/tree/master/c",
    return qrcodegen_encodeText(
        text, allbuffer + QR_BUFSIZE, allbuffer, 
        QR_MINECC, qrcodegen_VERSION_MIN, QR_MAXVERSION, qrcodegen_Mask_AUTO, QR_BOOSTECL);
}

void drawQr()
{
    int size = qrcodegen_getSize(allbuffer);
    int sizemod = floor((float)(HEIGHT - 2 * QR_OVERDRAW) / size);
    int realsize = sizemod * size;
    int startx = (WIDTH - realsize - 2 * QR_OVERDRAW) / 2;
    int starty = (HEIGHT - realsize - 2 * QR_OVERDRAW) / 2;

    // White rect for qr code background.
    arduboy.fillRect(startx, starty, realsize + 2 * QR_OVERDRAW, realsize + 2 * QR_OVERDRAW, WHITE);

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            // Silly, but we technically only need to draw the black pixels. It's slightly less code too?
            if(qrcodegen_getModule(allbuffer, x, y)) {
                arduboy.fillRect(startx + x * sizemod + QR_OVERDRAW, starty + y * sizemod + QR_OVERDRAW, sizemod, sizemod, BLACK);
            }
        }
    }
}


//For the various states, we display portions of the static screen to 
//reduce battery usage. I think...
void setStateAbout()
{
    arduboy.setFrameRate(FRAMERATE);
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
    memset(input, 0, sizeof(input));
}

void setStateSlotSelect()
{
    arduboy.setFrameRate(FRAMERATE);
    state = GameState::SlotSelect;
    keyboard.reset();
    repeater.reset();
    #ifdef FXVERSION
    loadSlots();
    displayCurrentSlot();
    #else
    setStateEntry(); // If you accidentally call this, jump right into text entry
    #endif
}

void setStateEntry()
{
    arduboy.setFrameRate(FRAMERATE);
    // Notice that we don't reset the input here. This lets you exit the
    // qr and still have your input there, so don't clear it here!
    state = GameState::Entry;
    arduboy.clear();
    keyboard.reset();
    repeater.reset();
    keyboard.print(true);
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
    setCentered(&arduboy, 10); //10 chars
    arduboy.print(F("Generating"));
    doDisplay();
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
            setStateSlotSelect();
    }
    else if(state == GameState::SlotSelect)
    {
        #ifdef FXVERSION
        uint8_t osc = slot_cursor;
        if(!arduboy.pressed(B_BUTTON))
        {
            if (repeater.repeat(LEFT_BUTTON))
                MENUWRAP(slot_cursor, FX_SLOTCOUNT, -1);
            if (repeater.repeat(RIGHT_BUTTON))
                MENUWRAP(slot_cursor, FX_SLOTCOUNT, 1);
        }
        if(arduboy.justPressed(A_BUTTON))
        {
            copySlotToInput();
            setStateEntry();
        }
        if(osc != slot_cursor)
        {
            displayCurrentSlot();
        }
        #else
        setStateEntry(); // You're not supposed to be here!
        #endif
    }
    else if(state == GameState::Entry)
    {
        keyboard.runFrame(&repeater);

        if (arduboy.pressed(COMBO_SHOWINPUT))
            setStateFullInput();

        if (arduboy.justPressed(A_BUTTON))
        {
            tryAddInput();
            printInput();
        }
        if (repeater.repeat(B_BUTTON) && !arduboy.anyPressed(DIR_BUTTONS))
        {
            tryRemoveInput();
            printInput();
        }

        // Special combo to generate qr
        if (arduboy.pressed(COMBO_SHOWQR))
        {
            #ifdef FXVERSION
            copyInputToSlot();
            saveSlots();
            #endif
            setStateDisplay();
        }
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
            setStateSlotSelect();
    }

    doDisplay();
}
