#pragma once
#include <Windows.h>

BOOL RealTimePriorityClass()
{
	return SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
}

BOOL RealTimeThreadPriority()
{
	return SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
}