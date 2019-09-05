#pragma once

#ifndef PPU_H
#define PPU_H

void setupScreen();
void drawToScreen();

void updateGraphics(int cycles);
void drawScanLine();
void renderSprites();
void renderTiles();
bool isLcdEnabled();
void setLcdStatus();

#endif