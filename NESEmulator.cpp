#include "SDL.h"
#include <stdio.h>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <iomanip>

#include "APU.h"
#include "PPU.h"
#include "CPU.h"
#include "Cartridge.h"

#define WIDTH 256
#define HEIGHT 240

int main(int argc, char* argv[])
{
	SDL_Event evt;
	bool running = true;

	int* pixels = new int[WIDTH * HEIGHT];
	int pitch = WIDTH * 4;

	SDL_Init(SDL_INIT_VIDEO);

	SDL_Window* window = SDL_CreateWindow
	("NES Emulator", // window's title
		100, 100, // coordinates on the screen, in pixels, of the window's upper left corner
		WIDTH * 2, HEIGHT * 2, // window's length and height in pixels  
		SDL_WINDOW_OPENGL);

	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

	// Pixel manipulation through texture of the surface.
	SDL_Texture* buffer = SDL_CreateTexture(renderer,
		SDL_PIXELFORMAT_RGBA8888,
		SDL_TEXTUREACCESS_STREAMING,
		WIDTH,
		HEIGHT);
	SDL_LockTexture(buffer,
		NULL,      // NULL means the *whole texture* here.
		(void**)(&pixels),
		&pitch);

	// Manipulate pixels here.

	SDL_UnlockTexture(buffer);

	//std::string filename("C:\\MyWork\\Super_mario_brothers.nes");
	std::string filename("C:\\MyWork\\ex1.dasm.rom");

	Cartridge::load(filename.c_str());

	APU::initialize();
	PPU::initialize();
	CPU::power();

	while(running)
	{
		SDL_PollEvent(&evt);
		switch(evt.type)
		{
		case SDL_QUIT:
			running = false;
			break;
		}
		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, buffer, NULL, NULL);
		SDL_RenderPresent(renderer);

		if (Cartridge::loaded())
		{
			for (int i = 0; i < 3; ++i)
				PPU::execute();
			CPU::execute();
		}
	}

	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}