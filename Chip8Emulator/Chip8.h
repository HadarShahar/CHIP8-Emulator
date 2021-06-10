#pragma once

#include "Constants.h"
#include "AsyncBeeper.h"
#include <iostream>
#include <string>
#include <vector>
#include <chrono>

class Chip8
{
public:
	Chip8();
	bool load(const std::wstring& filename);
	void reset();
	void cycle();

	void slowDown() { m_cycleRate -= 10; }
	void speedUp()  { m_cycleRate += 10; }
	unsigned int getCycleRate() const { return m_cycleRate; }

	void setKeyState(BYTE key, bool isPressed);
	bool getScreenChanged() const { return m_screenChanged; }
	void printScreen() const;

	using screen_t = BYTE[Constants::SCREEN_HEIGHT][Constants::SCREEN_WIDTH];
	const screen_t& getScreen() const { return m_screen; }


private:
	BYTE m_memory[Constants::MEMORY_SIZE]{};
	BYTE m_V[16]{};							// Registers
	WORD m_I{};								// Index register
	WORD m_pc = Constants::PC_START_ADDR;	// Program counter

	// Stack
	//WORD stack[STACK_SIZE]{};
	//WORD sp{};
	std::vector<WORD> m_stack;

	// Timers
	BYTE m_delayTimer{};
	BYTE m_soundTimer{};
	std::chrono::steady_clock::time_point m_lastTimersTick{};
	std::chrono::steady_clock::time_point m_lastCycleTime{};
	unsigned int m_cycleRate = Constants::DEFAULT_CYCLE_RATE;

	// Input 
	bool m_keys[Constants::KEYBOARD_SIZE]{};	// The current state of each key in the hex keyboard (1=pressed, 0=released).
	bool m_waitingForKeyPress{ false };			// For the opcode FX0A (blocking key input).
	int m_inputRegIndex{};   					// The index of the register that will receive the blocking input (pressed key).

	// Graphics
	screen_t m_screen{};
	bool m_screenChanged{ false };

	// Sound
	AsyncBeeper m_beeper;

	void executeOpcode(WORD opcode);
	void handleOpcodeStartsWith0(WORD opcode);
	void handleOpcodeStartsWith8(WORD opcode, BYTE &Vx, BYTE &Vy);
	void handleOpcodeStartsWithE(WORD opcode, BYTE &Vx);
	void handleOpcodeStartsWithF(WORD opcode, BYTE &Vx);

	void drawSprite(BYTE Vx, BYTE Vy, BYTE height);
	WORD searchHexDigitSprite(const BYTE *spriteBytes);
	void unknownOpcode(WORD opcode) { std::cout << "Unknown opcode: " << std::hex << opcode << '\n'; }
};

