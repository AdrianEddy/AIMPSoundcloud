#include "Timer.h"

std::map<UINT_PTR, Timer::TimerInfo> Timer::m_timers;
UINT_PTR Timer::m_maxTaskId = 123456;
