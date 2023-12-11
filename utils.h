#pragma once

#include <Windows.h>
#include <iostream>

namespace
{
    LARGE_INTEGER StartingTime, EndingTime, Frequency, ElapsedMicroseconds;

    void StartTimer()
    {
        QueryPerformanceFrequency(&Frequency);
        QueryPerformanceCounter(&StartingTime);
    }
    void StopTimer(bool display = true)
    {
        QueryPerformanceCounter(&EndingTime);
        ElapsedMicroseconds.QuadPart = EndingTime.QuadPart - StartingTime.QuadPart;
        ElapsedMicroseconds.QuadPart *= 1000000;
        ElapsedMicroseconds.QuadPart /= Frequency.QuadPart;
        if (display)
            std::cout << ElapsedMicroseconds.QuadPart << std::endl;
    }
    LONGLONG GetElapsedTime() { return ElapsedMicroseconds.QuadPart; }

}
