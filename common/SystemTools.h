#pragma once

#include <windows.h>
#include <Mmsystem.h>
#include <stdio.h>
#include <stdint.h>
//#include <vector>
//#include <string>

namespace SystemTools
{
	static uint64_t GetTimeNS ()
	{
		static LARGE_INTEGER ticksPerSec;
		static bool initialized = false;

		if (!initialized) 
		{
			QueryPerformanceFrequency(&ticksPerSec); 
			initialized = true;
		}
		LARGE_INTEGER now;
		QueryPerformanceCounter(&now);
		return (uint64_t) ((1e9 * now.QuadPart) / ticksPerSec.QuadPart);
	}

	static void SetLogFile (char *filename)
	{
		freopen (filename, "w", stdout);
	}

	static void Log (const char *text, ...)
	{
		va_list vl;
		va_start (vl, text);
		vprintf (text, vl);
		va_end (vl);
		fflush (stdout);
	}

	class StopWatch
	{
		float durations [256];

		uint64_t startTime;
		int numSamples;
		int curPos;
			
		double accumulatedDuration;
		int tick;

	public:

		StopWatch (int numSamples = 64)
		{
			Init ();
			SetNumberOfSamplesToAverage (numSamples);
		}
		void Init ()
		{
			for (int i=0; i<256; i++) durations[i] = 0;
			curPos = 0;

			accumulatedDuration = 0;
			tick = 0;
		}
		void SetNumberOfSamplesToAverage (int numSamples)
		{
			this->numSamples = min (numSamples, 255);
		}
		void Start ()
		{
			startTime = GetTimeNS();
		}
		float AddDuration (float dur)
		{
			durations [curPos] = dur;
			curPos = (curPos+1)&0xFF;
			return AverageTime ();
		}
		float Stop ()
		{
			uint64_t dur = GetTimeNS() - startTime;
			accumulatedDuration += double (dur) / 1000000.0;
			tick++;
			return AddDuration (float (dur) / 1000000.0f);
		}
		float AverageTime ()
		{
			float averageTime = 0;
			for (int i=0; i<numSamples; i++)
			{
				int index = (curPos-1 - i) & 0xFF;
				averageTime += durations[index];
			}
			return averageTime / float (numSamples);
		}
		float AccumulatedTime ()
		{
			double act = accumulatedDuration / double(tick);
			return float (act);
		}
		float CurrentTime ()
		{
			int index = (curPos-1) & 0xFF;
			return durations[index];
		}
	};

/*
	static void GetDirectoryContent (const char *directory, std::vector<std::string> *names, std::vector<std::string> *subDirectories)
	{
		if (names) names->clear();
		if (subDirectories) subDirectories->clear();

		char path[260];
		sprintf (path, "%s*.*", directory);
		WIN32_FIND_DATA fd; 
		HANDLE hFind = FindFirstFile (path, &fd); 
		if (hFind != INVALID_HANDLE_VALUE)
		{ 
			do 
			{ 
				if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) 
				{
					if (names) names->push_back (fd.cFileName);
				}
				else if (fd.cFileName[0] != '.') 
				{
					if (subDirectories) subDirectories->push_back (fd.cFileName);
				}
			} while (FindNextFile (hFind, &fd)); 
			FindClose (hFind); 
		} 
	}
*/
}