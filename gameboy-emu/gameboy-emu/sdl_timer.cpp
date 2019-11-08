#include "SDL.h"



//The clock time when the timer started
int startTicks;

//The timer status
bool isStarted;

void initializeTimer()
{
	startTicks = 0;
	isStarted = false;
}

void startTimer()
{
	isStarted = true;
	startTicks = SDL_GetTicks();
}

void stopTimer()
{
	isStarted = false;
	startTicks = 0;
}

int getTicks()
{
	return isStarted ? (SDL_GetTicks() - startTicks) : 0;
}