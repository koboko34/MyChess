#pragma once

#include <chrono>

class Timer
{
public:
	Timer(const std::string& name)
	{
		start = std::chrono::high_resolution_clock::now();
		this->name = name;

		printf("%s timer started...\n", name.c_str());
	}

	~Timer()
	{
		end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<float> duration = end - start;

		printf("%s took %f seconds.\n", name.c_str(), duration.count());
	}

private:
	std::string name;
	std::chrono::steady_clock::time_point start;
	std::chrono::steady_clock::time_point end;
};
