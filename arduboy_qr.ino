#include <Arduboy2.h>
#include "qrcodegen.h"

Arduboy2 arduboy;

const uint8_t FRAMERATE = 60;

const uint8_t MAXVERSION = 9;

//Just preallocate the maximum supported buffer size (version 11)
uint8_t qrbuffer[qrcodegen_BUFFER_LEN_FOR_VERSION(MAXVERSION)];
uint8_t tempbuffer[qrcodegen_BUFFER_LEN_FOR_VERSION(MAXVERSION)];


void setup()
{
    // Initialize the Arduboy
    arduboy.begin();
    arduboy.setFrameRate(FRAMERATE);
}


void loop()
{
    if(!arduboy.nextFrame()) return;
    arduboy.pollButtons();

    qrbuffer[arduboy.frameCount % qrcodegen_BUFFER_LEN_MAX] = arduboy.frameCount;
    tempbuffer[arduboy.frameCount % qrcodegen_BUFFER_LEN_MAX] = arduboy.frameCount;

    bool ok = qrcodegen_encodeText("https://github.com/nayuki/QR-Code-generator/tree/master/c",
        tempbuffer, qrbuffer, qrcodegen_Ecc_MEDIUM,
        qrcodegen_VERSION_MIN, MAXVERSION,
        qrcodegen_Mask_AUTO, true);
    
    int size = qrcodegen_getSize(qrbuffer);
    arduboy.fillRect(0, 0, size + 4, size + 4, WHITE);
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            arduboy.drawPixel(x + 2, y + 2, qrcodegen_getModule(qrbuffer, x, y) ? BLACK : WHITE);
        }
    }

    //arduboy.setCursor(0,0);
    //arduboy.print("Max bytes: ");
    //arduboy.print(qrcodegen_BUFFER_LEN_MAX);
    //arduboy.setCursor(0,1);
    //arduboy.print(qrbuffer[69]);
    //arduboy.print(tempbuffer[69]);

    arduboy.display();
}
