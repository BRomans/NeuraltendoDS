#include <nds.h>
#include <stdio.h>
#include <fat.h>    
#include <string.h>

int currentColorIndex = 0;
u16 colors[] = {
    RGB15(0, 0, 0),
    RGB15(31, 0, 0),
    RGB15(0, 31, 0),
    RGB15(0, 0, 31),
    RGB15(31, 31, 0),
    RGB15(0, 31, 31),
    RGB15(31, 0, 31),
    RGB15(31, 31, 31)
};

void pressKeysAndTouch(int keys, touchPosition touch) {
    iprintf("\x1b[2J"); // Clear screen
    iprintf("Hello World!\n\n");
    iprintf("Pressed Buttons:\n\n");

    if (keys & KEY_A) iprintf("A\n");
    if (keys & KEY_B) iprintf("B\n");
    if (keys & KEY_X) iprintf("X\n");
    if (keys & KEY_Y) iprintf("Y\n");
    if (keys & KEY_START) iprintf("START\n");
    if (keys & KEY_SELECT) iprintf("SELECT\n");
    if (keys & KEY_UP) iprintf("UP\n");
    if (keys & KEY_DOWN) iprintf("DOWN\n");
    if (keys & KEY_LEFT) iprintf("LEFT\n");
    if (keys & KEY_RIGHT) iprintf("RIGHT\n");
    if (keys & KEY_L) iprintf("L\n");
    if (keys & KEY_R) iprintf("R\n");
    if (keys & KEY_TOUCH) {
        iprintf("TOUCH Stylus\n");
        iprintf("Touch: X=%d Y=%d\n", touch.px, touch.py);
    }
}

void draw(int keys, touchPosition touch, touchPosition &lastTouch, bool &hasLastTouch, u16* videoBuffer) {
     if (keys & KEY_TOUCH) {
        int x = touch.px;
        int y = touch.py;

        // Clamp to screen bounds
        if (x >= 0 && x < 256 && y >= 0 && y < 192) {
            // Draw a line from last touch point to current
            if (lastTouch.px >= 0 && lastTouch.py >= 0) {
                // Simple Bresenham's line
                int dx = abs(x - lastTouch.px), sx = lastTouch.px < x ? 1 : -1;
                int dy = -abs(y - lastTouch.py), sy = lastTouch.py < y ? 1 : -1;
                int err = dx + dy, e2;

                int cx = lastTouch.px, cy = lastTouch.py;
                while (true) {
                    if (cx >= 0 && cx < 256 && cy >= 0 && cy < 192)
                        videoBuffer[cx + cy * 256] = RGB15(31, 31, 31) | BIT(15);

                    if (cx == x && cy == y) break;
                    e2 = 2 * err;
                    if (e2 >= dy) { err += dy; cx += sx; }
                    if (e2 <= dx) { err += dx; cy += sy; }
                }
            }
            lastTouch = touch;
            hasLastTouch = true;
        }
    } else {
        hasLastTouch = false;
    }
}

void clearScreen(u16* videoBuffer) {
    for (int i = 0; i < 256 * 192; ++i) {
        videoBuffer[i] = RGB15(31, 31, 31) | BIT(15);  // White background
    }
}

int main(void) {
    // Set top screen (main) to display a text console
    videoSetMode(MODE_0_2D);
    vramSetBankA(VRAM_A_MAIN_BG);
    consoleInit(NULL, 0, BgType_Text4bpp, BgSize_T_256x256, 31, 0, true, true);

    // --- Set up bottom screen for drawing ---
    videoSetModeSub(MODE_5_2D);
    vramSetBankC(VRAM_C_SUB_BG);
    REG_BG3CNT = BG_BMP16_256x256 | BG_BMP_BASE(0);
    REG_DISPCNT_SUB = DISPLAY_BG3_ACTIVE;

    u16* videoBuffer = (u16*)BG_BMP_RAM_SUB(0);

    clearScreen(videoBuffer); // Fill with white

    
    touchPosition touch;
    touchPosition lastTouch = { 0, 0 };
    bool hasLastTouch = false;

    while (1) {
        scanKeys();
        int keys = keysHeld();
        int kDown = keysDown();


        touchRead(&touch);

        // --- Top screen console ---
        pressKeysAndTouch(keys, touch);

        // Color picker
        if (kDown & KEY_L) {
            iprintf("L:Color Index: %d\n", currentColorIndex);
            currentColorIndex = (currentColorIndex - 1 + (sizeof(colors) / sizeof(colors[0]))) % (sizeof(colors) / sizeof(colors[0]));
        }
        if (kDown & KEY_R) {
            iprintf("R:Color Index: %d\n", currentColorIndex);
            currentColorIndex = (currentColorIndex + 1) % (sizeof(colors) / sizeof(colors[0]));
        }

        // Clear screen
        if (kDown & KEY_SELECT) {
            iprintf("Select: Clear Screen\n");
            clearScreen(videoBuffer);
        }

        // Save
        if (kDown & KEY_START) {
            iprintf("Start: Save Screen\n");
            //saveBMP("draw.bmp", videoBuffer);
        }

        // Top screen
        pressKeysAndTouch(keys, touch);

        // --- Bottom screen drawing ---
        draw(keys, touch, lastTouch, hasLastTouch, videoBuffer);
        // Wait for VBlank to avoid tearing
        swiWaitForVBlank();
    }

    return 0;
}
