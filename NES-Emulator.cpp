#include "pch.h"
#include "SDL.h"
#include <stdio.h>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <iomanip>

#include "CPU.h"
#include "Cartridge.h"

int main(int argc, char* argv[])
{
	SDL_Event evt;
	bool running = true;

	SDL_Init(SDL_INIT_VIDEO);

	SDL_Window* window = SDL_CreateWindow
	("NES Emulator", // window's title
		100, 100, // coordinates on the screen, in pixels, of the window's upper left corner
		256, 240, // window's length and height in pixels  
		SDL_WINDOW_OPENGL);

	std::string filename("C:\\Users\\Lucius\\Documents\\ROMs\\NES\\Super Mario Bros.nes");
	//std::string filename("C:\\Users\\Lucius\\Documents\\ROMs\\NES\\nes-test.nes");

	Cartridge::load(filename.c_str());

	//CPU::power();

	while(running)
	{
		SDL_PollEvent(&evt);
		switch(evt.type)
		{
		case SDL_QUIT:
			running = false;
			break;
		}
	}

	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}