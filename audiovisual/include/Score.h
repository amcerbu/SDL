#pragma once

#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <iostream>

class Score
{
public:
	Score(std::string filename);
	
	void print();

	std::vector<int> read();

private:
	std::string filename;
	std::ifstream file;
	std::vector<std::vector<int>> raw;

	int location;
};