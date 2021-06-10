#pragma once

#include <thread>
#include <condition_variable>
#include <Windows.h>

class AsyncBeeper
{
public:
	AsyncBeeper(DWORD frequency = 750, DWORD duration = 30);
	void beep();
	void play() { m_conditionVar.notify_one(); }

private:
	DWORD m_frequency{};
	DWORD m_duration{};
	std::thread m_beeperThread;

	int m_beepsCounter{ 0 };
	std::condition_variable m_conditionVar;
	std::mutex m_lock;

	void playBeepsLoop();
};