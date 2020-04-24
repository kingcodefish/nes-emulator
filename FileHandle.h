#pragma once

#include <fstream>
#include <string>
#include <sstream>

class FileHandle
{
private:
	std::ifstream fileInput;
	unsigned int line;
public:
	FileHandle(std::string filename);
	~FileHandle();

	std::string getLine();
};

