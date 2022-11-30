#include "Score.h"

Score::Score(std::string filename) : location(0)
{
	file.open(filename);

	std::string line;
	if (file.is_open())
	{
		for (int i = 0; getline(file, line); i++)
		{
			raw.push_back(std::vector<int>());
			std::stringstream stream(line);
			// std::cout << "Found ";
			for (int n; stream >> n; )
			{
				raw[i].push_back(n);
				// std::cout << n << " ";
			}
		}

		file.close();
	}
}

void Score::print()
{
	for (auto a : raw)
	{
		for (auto n : a)
		{
			std::cout << n << " ";
		}
		std::cout << std::endl;
	}
}

std::vector<int> Score::read()
{
	std::vector<int> out = raw[location];
	location++;
	location %= raw.size();
	return out;
}