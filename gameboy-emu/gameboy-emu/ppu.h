#pragma once

#ifndef PPU_H
#define PPU_H

#define GB_SCREEN_X 160
#define GB_SCREEN_Y 144

void setupScreen();
void drawToScreen();

void initializePPU();
void handleFrameRate();
void updateGraphics(int cycles);
void drawScanLine();
void renderSprites();
void renderTiles();
bool isLcdEnabled();
void setLcdStatus();
#endif