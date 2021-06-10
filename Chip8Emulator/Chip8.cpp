#include "Chip8.h"
#include "Constants.h"
#include "AsyncBeeper.h"
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <stdexcept>

Chip8::Chip8() : m_stack(Constants::DEFAULT_STACK_SIZE)
{
	reset();
}

bool Chip8::load(const std::wstring& filename)
{
	std::ifstream file(filename, std::ios::binary);
	if (!file)
	{
		std::wcerr << "Could not open file: \"" << filename << "\".\n";
		return false;
	}

	file.seekg(0, std::ios::end);
	auto size = file.tellg();
	if (size > (Constants::MEMORY_SIZE - Constants::PC_START_ADDR))
	{
		std::wcerr << "The file is too big (" << size << " bytes), not enough memory.\n";
		return false;
	}

	file.seekg(0, std::ios::beg);
	file.read(reinterpret_cast<char*>(&m_memory[Constants::PC_START_ADDR]), size);
	std::wcout << "The file \"" << filename << "\" was loaded successfully!\n";
	return true;
}


void Chip8::reset()
{
	memset(m_memory, 0, sizeof(m_memory));
	// Load fontset
	for (int i = 0; i < sizeof(Constants::FONTSET); ++i)
	{
		m_memory[i] = Constants::FONTSET[i];
	}

	memset(m_V, 0, sizeof(m_V));
	m_I = 0;
	m_pc = Constants::PC_START_ADDR;
	m_stack.clear();

	m_delayTimer = 0;
	m_soundTimer = 0;
	m_lastTimersTick = {};
	m_lastCycleTime = {};
	m_cycleRate = Constants::DEFAULT_CYCLE_RATE;

	memset(m_keys, 0, sizeof(m_keys));
	m_waitingForKeyPress = false;
	m_inputRegIndex = 0;

	memset(m_screen, 0, sizeof(m_screen));
	m_screenChanged = false;
}
	
void Chip8::cycle()
{
	m_screenChanged = false;
	if (m_waitingForKeyPress || !m_cycleRate)
	{
		return;
	}

	if (m_pc >= Constants::MEMORY_SIZE - 1)
	{
		throw std::runtime_error("program counter is out of range!");
	}

	auto currentTime = std::chrono::high_resolution_clock::now();
	auto deltaT = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - m_lastCycleTime).count();
	if (deltaT >= (1000.0 / m_cycleRate))
	{
		WORD opcode = m_memory[m_pc] << 8 | m_memory[m_pc + 1];
		//std::cout << std::hex << m_pc << ' ' << opcode << '\n';
		m_pc += 2;
		executeOpcode(opcode);
		m_lastCycleTime = currentTime;
	}

	deltaT = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - m_lastTimersTick).count();
	if (deltaT >= (1000.0 / Constants::TIMERS_RATE))
	{
		// Update timers
		if (m_delayTimer > 0)
		{
			m_delayTimer--;
		}
		if (m_soundTimer > 0)
		{
			m_beeper.beep();
			m_soundTimer--;
			if (!m_soundTimer)
			{
				// Play all the collected beeps together, so the sound will be continuous.
				m_beeper.play(); 
			}
		}
		m_lastTimersTick = currentTime;
	}
}

void Chip8::setKeyState(BYTE key, bool isPressed)
{
	m_keys[key] = isPressed;
	if (m_waitingForKeyPress && isPressed)
	{
		m_V[m_inputRegIndex] = key;
		m_waitingForKeyPress = false;
		std::cout << "Received key: " << static_cast<int>(key) << "\n";
	}
}

// Executes a given opcode and increments the program counter.
void Chip8::executeOpcode(WORD opcode)
{
	// Vx and Vy are part of many opcodes (in the same positions).
	BYTE &Vx = m_V[(opcode & 0x0F00) >> 8];
	BYTE &Vy = m_V[(opcode & 0x00F0) >> 4];

	switch (opcode & 0xF000)
	{
	case 0x0000:
		handleOpcodeStartsWith0(opcode);
		break;

	case 0x1000: // 1NNN: goto NNN;
		m_pc = opcode & 0x0FFF;
		break;

	case 0x2000: // 2NNN: *(0xNNN)()
		m_stack.push_back(m_pc);
		m_pc = opcode & 0x0FFF;
		break;

	case 0x3000: // 3XNN: Skips the next instruction if VX equals NN.
		if (Vx == (opcode & 0x00FF))
			m_pc += 2;
		break;

	case 0x4000: // 4XNN: Skips the next instruction if VX does not equal NN.
		if (Vx != (opcode & 0x00FF))
			m_pc += 2;
		break;

	case 0x5000: // 5XY0: Skips the next instruction if VX equals VY.
		if (Vx == Vy)
			m_pc += 2;
		break;

	case 0x6000: // 6XNN: Vx = NN
		Vx = opcode & 0x00FF;
		break;

	case 0x7000: // 7XNN: Vx += NN
		Vx += opcode & 0x00FF;
		break;

	case 0x8000:
		handleOpcodeStartsWith8(opcode, Vx, Vy);
		break;

	case 0x9000: // 9XY0: Skips the next instruction if VX does not equal VY. 
		if (Vx != Vy)
			m_pc += 2;
		break;

	case 0xA000: // ANNN: I = NNN
		m_I = opcode & 0x0FFF;
		break;

	case 0xB000: // BNNN: PC=V0+NNN
		m_pc = m_V[0] + (opcode & 0x0FFF);
		break;

	case 0xC000: // CXNN: Vx=rand()&NN
		Vx = rand() & (opcode & 0x00FF);
		break;

	case 0xD000: // DXYN
		drawSprite(Vx, Vy, opcode & 0xF);
		break;

	case 0xE000:
		handleOpcodeStartsWithE(opcode, Vx);
		break;

	case 0xF000:
		handleOpcodeStartsWithF(opcode, Vx);
		break;

	default:
		unknownOpcode(opcode);
	}
}

void Chip8::handleOpcodeStartsWith0(WORD opcode)
{
	switch (opcode & 0x0FFF)
	{
	case 0x00E0: // 00E0: Clears the screen.
		memset(m_screen, 0, Constants::SCREEN_HEIGHT * Constants::SCREEN_WIDTH);
		m_screenChanged = true;
		break;
	case 0x00EE: // 00EE: Returns from a subroutine.
		m_pc = m_stack.back();
		m_stack.pop_back();
		break;
	default:
		// The instruction 0NNN is ignored by modern interpreters.
		//// 0NNN: Jump to a machine code routine at nnn.
		//pc = opcode & 0x0FFF;
		unknownOpcode(opcode);
	}
}

void Chip8::handleOpcodeStartsWith8(WORD opcode, BYTE &Vx, BYTE &Vy)
{
	switch (opcode & 0xF)
	{
	case 0x0:
		Vx = Vy;
		break;
	case 0x1:
		Vx = Vx | Vy;
		break;
	case 0x2:
		Vx = Vx & Vy;
		break;
	case 0x3:
		Vx = Vx ^ Vy;
		break;
	case 0x4:
		// Adds VY to VX. VF is set to 1 when there's a carry, and to 0 when there is not.
		m_V[0xF] = Vx > (0xFF - Vy);
		Vx += Vy;
		break;
	case 0x5:
		// VY is subtracted from VX. VF is set to 0 when there's a borrow, and 1 when there is not.
		m_V[0xF] = Vx >= Vy;
		Vx -= Vy;
		break;
	case 0x6:
		// Stores the least significant bit of VX in VF and then shifts VX to the right by 1.
		m_V[0xF] = Vx & 1;
		Vx >>= 1;
		break;
	case 0x7:
		// Sets VX to VY minus VX. VF is set to 0 when there's a borrow, and 1 when there is not.
		m_V[0xF] = Vy >= Vx;
		Vx = Vy - Vx;
		break;
	case 0xE:
		// Stores the most significant bit of VX in VF and then shifts VX to the left by 1
		m_V[0xF] = Vx & (1 << 7);
		Vx <<= 1;
		break;
	default:
		unknownOpcode(opcode);
	}
}

void Chip8::handleOpcodeStartsWithE(WORD opcode, BYTE &Vx)
{
	switch (opcode & 0xFF)
	{
	case 0x9E: // EX9E: Skips the next instruction if the key stored in VX is pressed.
		if (m_keys[Vx])
		{
			m_pc += 2; 
			m_keys[Vx] = 0; 
		}
		break;
	case 0xA1: // EXA1: Skips the next instruction if the key stored in VX is not pressed.
		if (!m_keys[Vx])
		{
			m_pc += 2;
		}
		break;
	default:
		unknownOpcode(opcode);
	}
}

void Chip8::handleOpcodeStartsWithF(WORD opcode, BYTE &Vx)
{
	switch (opcode & 0xFF)
	{
	case 0x07:
		Vx = m_delayTimer;
		break;
	case 0x0A: // FX0A: A key press is awaited, and then stored in VX. Blocking Operation.
		m_waitingForKeyPress = true;
		std::cout << "Waiting for key press...\n";
		m_inputRegIndex = (opcode & 0x0F00) >> 8;
		break;
	case 0x15:
		m_delayTimer = Vx;
		break;
	case 0x18:
		m_soundTimer = Vx;
		break;
	case 0x1E:
		m_I += Vx;
		break;
	case 0x29: // FX29: Sets I to the location of the sprite for the character in VX
		m_I = searchHexDigitSprite(&Constants::FONTSET[Vx * Constants::HEX_DIGIT_SPRITE_LEN]);
		break;

	case 0x33:
		// Take the decimal representation of VX, place the hundreds digit in memory at location in I, 
		// the tens digit at location I+1, and the ones digit at location I+2
		m_memory[m_I] = Vx / 100;
		m_memory[m_I + 1] = (Vx / 10) % 10;
		m_memory[m_I + 2] = Vx % 10;
		break;

	case 0x55: // FX55: Stores V0 to VX (including VX) in memory starting at address I.
		for (int i = 0; i <= ((opcode & 0x0F00) >> 8); i++)
		{
			m_memory[m_I + i] = m_V[i];
		}
		break;

	case 0x65: // FX65: Fills V0 to VX (including VX) with values from memory starting at address I. 
		for (int i = 0; i <= ((opcode & 0x0F00) >> 8); i++)
		{
			m_V[i] = m_memory[m_I + i];
		}
		break;

	default:
		unknownOpcode(opcode);
	}
}

// Draws a sprite at coordinate (Vx, Vy) that has a width of 8 pixels and given height.
void Chip8::drawSprite(BYTE Vx, BYTE Vy, BYTE height)
{
	m_V[0xF] = 0;
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < 8; x++)
		{
			BYTE mask = 1 << (7 - x);
			if (m_screen[Vy + y][Vx + x] && (m_memory[m_I + y] & mask))
			{
				// VF is set to 1 if any screen pixels are flipped from set to unset 
				// when the sprite is drawn, and to 0 if that does not happen
				m_V[0xF] = 1;
			}
			m_screen[Vy + y][Vx + x] ^= (m_memory[m_I + y] & mask);
		}
	}
	m_screenChanged = true;
}

// Searches a given sprite in memory.
WORD Chip8::searchHexDigitSprite(const BYTE *spriteBytes)
{
	for (int i = 0; i < Constants::MEMORY_SIZE; i++)
	{
		bool found = true;
		for (int j = 0; j < Constants::HEX_DIGIT_SPRITE_LEN; j++)
		{
			if (m_memory[i + j] != spriteBytes[j])
			{
				found = false;
				break;
			}
		}
		if (found)
			return i;
	}
	return 0; // not found
}

void Chip8::printScreen() const
{
	for (int y = 0; y < Constants::SCREEN_HEIGHT; y++)
	{
		for (int x = 0; x < Constants::SCREEN_WIDTH; x++)
		{
			if (m_screen[y][x])
				std::cout << '*';
			else
				std::cout << '.';
		}
		std::cout << '\n';
	}
}

