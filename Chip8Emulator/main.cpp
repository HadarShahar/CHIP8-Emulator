#include "Chip8.h"
#include "Chip8Window.h"
#include <iostream>
#include <memory>
#include <filesystem>
#include <string>
#include <limits>
#include <Windows.h>

const std::wstring PATH_TO_GAMES = L"..\\Chip8Emulator\\games\\";
std::wstring chooseGame();
std::wstring stringToWstring(const std::string& str);

int main(int argc, char *argv[]) 
{
	auto pChip8 = std::make_unique<Chip8>();
	auto gameName = argc > 1 ? stringToWstring(argv[1]) : chooseGame();
	while (!gameName.empty())
	{
		if (pChip8->load(gameName))
		{
			Chip8Window window(pChip8.get());
			window.mainLoop();
		}
		pChip8->reset();
		gameName = chooseGame();
	}
	return EXIT_SUCCESS;
}


std::wstring chooseGame()
{
	// Create a list of all the files in the directory at PATH_TO_GAMES.
	std::vector<std::wstring> gamesNames;
	for (const auto& entry : std::filesystem::directory_iterator(PATH_TO_GAMES))
	{
		const std::wstring& path = entry.path().native();
		std::wstring filename{ path.substr(path.find_last_of(L"/\\") + 1) };
		gamesNames.push_back(filename);
	}

	std::cout << "=================== Choose a game ===================\n";
	for (auto i = 0; i < gamesNames.size(); ++i)
	{
		std::wcout << '[' << i << "] " << gamesNames[i] << '\n';
	}

	int gameIndex = 0;
	while (true)
	{
		std::cout << "Enter the game index (-1 to quit): ";
		std::cin >> gameIndex;
		if (gameIndex == -1)
		{
			return L"";
		}
		if (!std::cin.fail() && gameIndex >= 0 && gameIndex < gamesNames.size())
		{
			return PATH_TO_GAMES + gamesNames[gameIndex];
		}
		std::cout << "Invalid index. ";
		std::cin.clear();
		// wrap the 'max' function name in parenthesis, so it won't collide with max macro in Windows.h
		std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
	}
}


std::wstring stringToWstring(const std::string& str)
{
	int wstrSize = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
	std::wstring wstr(wstrSize, 0);
	MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstr[0], wstrSize);
	return wstr;
}