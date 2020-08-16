#pragma once

#include <chrono>

class Timer {
public:
	using clock = std::chrono::high_resolution_clock;
	using time_point = clock::time_point;
public:
	void start() {
		m_start = clock::now();
		m_stopped = false;
	}
	void stop() {
		m_stop = clock::now();
		m_stopped = true;
	}
	float elapsed() {
		if( m_stopped ) {
			auto dur = std::chrono::duration<float>( m_stop - m_start );
			m_start = m_stop;
			return dur.count();
		}
		else {
			auto dur = std::chrono::duration<float>( clock::now() - m_start );
			return dur.count();
		}
	}
	float mark()noexcept {
		stop();
		const auto dur = elapsed();
		start();
		return dur;
	}
private:
	time_point m_start = clock::now();
	time_point m_stop;
	bool m_stopped = false;
};

