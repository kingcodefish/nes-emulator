#include "FileHandle.h"

#include <iostream>


FileHandle::FileHandle(std::string filename)
{
	fileInput.open(filename);
}

FileHandle::~FileHandle()
{
	fileInput.close();
}

std::string FileHandle::getLine()
{
	std::ifstream t("file.txt");
	std::stringstream buffer;
	buffer << fileInput.rdbuf();

	std::cout << buffer.str();

	return buffer.str();
}
