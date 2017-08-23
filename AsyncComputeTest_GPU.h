#pragma once

#include "SystemTools.h"
#include <imgui.h>

#define USE_GPU_PROFILER






struct AsyncComputeTest_GPU
{
	enum // buffers
	{
		bTEST0,
		bTEST1,
		bTEST2,
		bTEST3,
		bDISPATCH,
		bDBG,
		bNUM,
	};

	
	enum // tasks
	{
		tTEST0,
		tTEST1,
		tTEST2,
		tTEST3,
		tNUM,
	};


	enum
	{
		TS_FULL,
		TS_NUM
	};

	size_t bufferSize[bNUM];
	size_t totalBufferSize;
	size_t maxBufferSize;
	
	SystemTools::StopWatch sw[TS_NUM];

	void InitBase ()
	{
		
		bufferSize[bTEST0] = sizeof(float) * 0x4000;
		bufferSize[bTEST1] = sizeof(float) * 0x4000;
		bufferSize[bTEST2] = sizeof(float) * 0x4000;
		bufferSize[bTEST3] = sizeof(float) * 0x4000;
		bufferSize[bDISPATCH] = 1024 * 3 * sizeof(uint32_t);
		bufferSize[bDBG] = 512 * sizeof(uint32_t);

		size_t maxSize = 0;
		size_t totalSize = 0;

		for (int i=0; i<bNUM; i++)
		{
			totalSize += bufferSize[i];
			maxSize = max (maxSize, bufferSize[i]);
		}

		totalBufferSize = totalSize;
		maxBufferSize = maxSize;
		SystemTools::Log ("GI buffers size: %i \n",  totalSize);

		sw[TS_FULL].SetNumberOfSamplesToAverage(8);
	}

	void DataToBuffer (void *bufferData, const uint32_t bufferIndex, const int* wavefrontsPerDispatch, int maxDispatches)
	{
		if (bufferIndex == bDISPATCH)
		{ 
			uint32_t *dst = (uint32_t*) bufferData;

			for (int i=0; i<1024; i++)
			{ 
				dst[i*3+0] = uint32_t (wavefrontsPerDispatch[min(tNUM-1, i/maxDispatches)]);
				dst[i*3+1] = 1;
				dst[i*3+2] = 1;
			}
		}

		if (bufferIndex == bDBG)
		{
			int32_t *dst = (int32_t*) bufferData;
			for (int i=0; i<512; i++)
			{
				dst[i] = 0;
			}
		}

		if (bufferIndex >= bTEST0 && bufferIndex <= bTEST3)
		{
			float *dst = (float*) bufferData;
			for (int i=0; i<bufferSize[bufferIndex] / sizeof(uint32_t); i++) dst[i] = 1.0f;
		}
	}


	void BufferToData( void *bufferData, const uint32_t bufferIndex)
	{
		if (bufferIndex == bDBG)
		{
			uint32_t *src = (uint32_t*) bufferData;			
			ImGui::Text ("\nExecuted Wavefront count: task 0: %i task 1: %i task 2: %i task 3: %i\n\n", src[0], src[1], src[2], src[3]);
		}
	}


};






