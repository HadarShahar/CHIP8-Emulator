#include "Chip8Window.h"
#include "Constants.h"
#include <stdexcept>

Chip8Window::keysMap_t Chip8Window::keysMap = initKeysMap();

Chip8Window::Chip8Window(Chip8 *pChip8) : m_pChip8(pChip8)
{
	if (!glfwInit())
	{
		throw std::runtime_error("GLFW initialization failed.");
	}

	m_pGlfwWindow = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "", nullptr, nullptr);
	if (!m_pGlfwWindow)
	{
		glfwTerminate();
		throw std::runtime_error("Window or OpenGL context creation failed.");
	}
	updateTitle();

	// needed for glfwGetWindowUserPointer to work
	glfwSetWindowUserPointer(m_pGlfwWindow, this);
	glfwSetKeyCallback(m_pGlfwWindow, keyCallback);
	glfwSetFramebufferSizeCallback(m_pGlfwWindow, framebufferSizeCallback);

	glfwMakeContextCurrent(m_pGlfwWindow);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);	// Set background color to black
	glColor3f(1.0f, 1.0f, 1.0f);			// White
}

Chip8Window::~Chip8Window()
{
	if (m_pGlfwWindow)
	{
		glfwDestroyWindow(m_pGlfwWindow);
	}
	glfwTerminate();
}

void Chip8Window::mainLoop()
{
	while (!glfwWindowShouldClose(m_pGlfwWindow))
	{
		m_pChip8->cycle();
		if (m_pChip8->getScreenChanged())
		{
			glClear(GL_COLOR_BUFFER_BIT);
			drawScreen();
			glfwSwapBuffers(m_pGlfwWindow);
		}
		glfwPollEvents();
	}
}

void Chip8Window::drawScreen()
{
	//g_pChip8->printScreen();
	//std::cout << "\n\n";
	
	for (int y = 0; y < Constants::SCREEN_HEIGHT; y++)
	{
		float rectY = 1 - y * GL_Y_UNIT;
		for (int x = 0; x < Constants::SCREEN_WIDTH; x++)
		{
			float rectX = -1 + x * GL_X_UNIT;
			if (m_pChip8->getScreen()[y][x])
			{
				glRectf(rectX, rectY, rectX + GL_X_UNIT, rectY - GL_Y_UNIT);
			}
		}
	}
}

void Chip8Window::updateTitle()
{
	std::string title{ "CHIP-8 Emulator,  cycles per second: " };
	title += std::to_string(m_pChip8->getCycleRate());
	glfwSetWindowTitle(m_pGlfwWindow, title.c_str());
}

void Chip8Window::handleKeyEvent(int key, int scancode, int action, int mods)
{
	//std::cout << key << ' ' << scancode << '\n';
	bool isPressed = (action == GLFW_PRESS || action == GLFW_REPEAT);

	// Handle control keys. 
	switch (key)
	{
	case GLFW_KEY_ESCAPE:
		if (isPressed)
			glfwSetWindowShouldClose(m_pGlfwWindow, GLFW_TRUE);
		return;
	case GLFW_KEY_KP_SUBTRACT:
		if (isPressed)
		{
			m_pChip8->slowDown();
			updateTitle();
		}
		return;
	case GLFW_KEY_KP_ADD:
		if (isPressed)
		{
			m_pChip8->speedUp();
			updateTitle();
		}
		return;
	}

	if (keysMap.count(key))  // if the key exists
	{
		m_pChip8->setKeyState(keysMap[key], isPressed); 
		//std::cout << "Mapped key " << static_cast<int>(keysMap[key]) << " is " << (isPressed ? "pressed" : "released") << ".\n";
	}
	else
	{
		std::cout << "The key " << key << " does not exist in my keyboard.\n";
	}


	//// If the key is a hex letter, convert it to its hex value.
	//BYTE hexKey = 0;
	//if (key >= '0' && key <= '9')
	//{
	//	hexKey = key - '0';
	//}
	//else if (key >= 320 && key <= 329)  // keycodes for the numeric keypad
	//{
	//	hexKey = key - 320;
	//}
	//else if (key >= 'A' && key <= 'F')
	//{
	//	hexKey = key - 'A';
	//}
	//else
	//{
	//	std::cout << "The key " << key << " is not in the hex keyboard.\n";
	//	return;
	//}
	//m_pChip8->setKeyState(hexKey, isPressed);
}

void Chip8Window::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	Chip8Window *win = static_cast<Chip8Window*>(glfwGetWindowUserPointer(window));
	win->handleKeyEvent(key, scancode, action, mods);
}


void Chip8Window::framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions
	glViewport(0, 0, width, height);

	Chip8Window *win = static_cast<Chip8Window*>(glfwGetWindowUserPointer(window));
	win->drawScreen();
}