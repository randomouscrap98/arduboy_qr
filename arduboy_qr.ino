// Uncomment to use the arduboy FX version
//#define FXVERSION

#include <Arduboy2.h>
#include "qrcodegen.h"

//#include "qr_ardspecial.h"

Arduboy2 arduboy;

const uint8_t FRAMERATE = 60;

const uint8_t MAXVERSION = 9;
const qrcodegen_Ecc MINECC = qrcodegen_Ecc_MEDIUM;
const bool BOOSTECL = true;

//Just preallocate the maximum supported buffer size
uint8_t qrbuffer[qrcodegen_BUFFER_LEN_FOR_VERSION(MAXVERSION)];
uint8_t tempbuffer[qrcodegen_BUFFER_LEN_FOR_VERSION(MAXVERSION)];

const uint8_t KEYBOARD_LINELENGTH = 20;
const uint8_t KEYBOARD_LINECOUNT = 5;
const uint8_t KEYBOARD_STARTY = 3;
const uint8_t KEYBOARD_STARTX = 0;

const char KEYLINE1[] PROGMEM = "ABCDEFGHIJKLM[01234]";
const char KEYLINE2[] PROGMEM = "NOPQRSTUVWXYZ(56789)";
const char KEYLINE3[] PROGMEM = "abcdefghijklm{./@&=}";
const char KEYLINE4[] PROGMEM = "nopqrstuvwxyz<?!,'\">";
const char KEYLINE5[] PROGMEM = "#$%^*+-_=|\\:;~`     ";

const char * KEYLINES[KEYBOARD_LINECOUNT]; // = { KEYLINE1, KEYLINE2, KEYLINE3, KEYLINE4, KEYLINE5 };


uint8_t kbx, kby;

void setup()
{
    //getEccCodewordsPerBlock = special_getEccCodewordsPerBlock;
    //getNumErrorCorrectionBlocks = special_getNumErrorCorrectionBlocks;

    //Really kinda dumb lol
    KEYLINES[0] = KEYLINE1;
    KEYLINES[1] = KEYLINE2;
    KEYLINES[2] = KEYLINE3;
    KEYLINES[3] = KEYLINE4;
    KEYLINES[4] = KEYLINE5;

    // Initialize the Arduboy
    arduboy.begin();
    arduboy.setFrameRate(FRAMERATE);

    resetKeyboard();
}

void resetKeyboard()
{
    kbx = 0;
    kby = 0;
}

void printKeyboard(uint8_t cx, uint8_t cy)
{
    char line[KEYBOARD_LINELENGTH + 1];
    line[KEYBOARD_LINELENGTH] = 0;

    for(uint8_t i = 0; i < KEYBOARD_LINECOUNT; i++)
    {
        arduboy.setCursor(KEYBOARD_STARTX, KEYBOARD_STARTY + i);
        arduboy.print(KEYLINES[i]);
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


void loop()
{
    if(!arduboy.nextFrame()) return;
    arduboy.pollButtons();
    arduboy.clear();

    //if(arduboy.anyPressed(UP_BUTTON | DOWN_BUTTON | LEFT_BUTTON | RIGHT_BUTTON))
    //printKeyboard(kbx, kby);

    generateQr("https://github.com/nayuki/QR-Code-generator/tree/master/c");
    int size = qrcodegen_getSize(qrbuffer);
    arduboy.fillRect(0, 0, size + 4, size + 4, WHITE);
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            arduboy.drawPixel(x + 2, y + 2, qrcodegen_getModule(qrbuffer, x, y) ? BLACK : WHITE);
        }
    }

    //arduboy.print("A");
    //arduboy.setCursor(0,0);
    //arduboy.print("Max bytes: ");
    //arduboy.print(qrcodegen_BUFFER_LEN_MAX);
    //arduboy.setCursor(0,1);
    //arduboy.print(qrbuffer[69]);
    //arduboy.print(tempbuffer[69]);

    arduboy.display();
}
