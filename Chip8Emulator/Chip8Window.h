#pragma once

#include "Chip8.h"
#include "Constants.h"
#include <memory>
#include <unordered_map>
#include <GLFW/glfw3.h>

constexpr int DISPLAY_FACTOR = 10;
constexpr int WINDOW_WIDTH	 = Constants::SCREEN_WIDTH * DISPLAY_FACTOR;
constexpr int WINDOW_HEIGHT  = Constants::SCREEN_HEIGHT * DISPLAY_FACTOR;

//  OpenGL space uses floating point numbers between -1 and 1.
constexpr float GL_Y_UNIT = 2.0f / Constants::SCREEN_HEIGHT;
constexpr float GL_X_UNIT = 2.0f / Constants::SCREEN_WIDTH;

class Chip8Window
{
public:
	Chip8Window(Chip8 *chip8);
	~Chip8Window();
	void mainLoop();

private:
	GLFWwindow *m_pGlfwWindow = nullptr;
	Chip8 *m_pChip8 = nullptr;

	void drawScreen();
	void updateTitle();
	void handleKeyEvent(int key, int scancode, int action, int mods);

	// Callback can not be member functions.
	static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void framebufferSizeCallback(GLFWwindow* window, int width, int height);

	// Map of each key in Constants::MY_KEYBOARD and the corresponding key in Constants::ORIGINAL_KEYBOARD.
	using keysMap_t = std::unordered_map<int, BYTE>;
	static keysMap_t keysMap;
	static keysMap_t initKeysMap()
	{
		keysMap_t keysMap;
		for (int i = 0; i < Constants::KEYBOARD_SIZE; i++)
		{
			keysMap[Constants::MY_KEYBOARD[i]] = Constants::ORIGINAL_KEYBOARD[i];
		}
		return keysMap;
	}

};