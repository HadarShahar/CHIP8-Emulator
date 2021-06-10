#include "AsyncBeeper.h"
#include <iostream>
#include <thread>
#include <condition_variable>
#include <Windows.h>

AsyncBeeper::AsyncBeeper(DWORD frequency, DWORD duration)
	: m_frequency(frequency),
	m_duration(duration),
	m_beeperThread(&AsyncBeeper::playBeepsLoop, this)
{
	m_beeperThread.detach();
}

void AsyncBeeper::playBeepsLoop()
{
	while (true)
	{
		std::unique_lock<std::mutex> ul(m_lock);
		
		// Atomically unlocks the lock and blocks until the condition variable is notified.
		// When unblocked, the lock is reacquired and wait exits.
		m_conditionVar.wait(ul); 
			
		int count = m_beepsCounter;
		m_beepsCounter = 0;
		ul.unlock();

		//std::cout << count << '\n';
		if (count)
		{
			Beep(m_frequency, m_duration * count);
		}
	}
}

void AsyncBeeper::beep()
{
	{
		auto ul = std::scoped_lock<std::mutex>(m_lock);
		m_beepsCounter++;
	}
	//m_conditionVar.notify_one();
}