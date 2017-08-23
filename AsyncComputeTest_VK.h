#pragma once

#include "AsyncComputeTest_GPU.h"

#include "VK_Helper.h"
using namespace VK_Helper;

//#define FORCE_WAIT

#define TASK_MAX_DISPATCHES 200 

#define PROFILE_TASKS 1 // 1: per task, 2: per task dispatch (does not work with iterations > 1), 0: off

#define USE_UNIQUE_BUFFER_PER_TASK 1 // no effect on performance





struct AsyncComputeTest_VK : public AsyncComputeTest_GPU
{
	enum
	{
		MAX_PIPELINES = 1024,
		ASYNC_QUEUES = 3,
		MAX_ITERATIONS = 10,
		SEMAPHORE_COUNT = MAX_ITERATIONS * 3,
		EVENT_COUNT = MAX_ITERATIONS * 2,
	};

	enum
	{
		TS_UPLOAD,
		TS_DOWNLOAD,

		TS_GRAPHICS_QUEUE,

		TS_COMPUTE_QUEUE_0,
		TS_COMPUTE_QUEUE_1,
		TS_COMPUTE_QUEUE_2,
		

		TS_TEST0, 
		TS_TEST1	= TS_TEST0 + MAX_ITERATIONS,
		TS_TEST2	= TS_TEST1 + MAX_ITERATIONS,
		TS_TEST3	= TS_TEST2 + MAX_ITERATIONS,

		TS_NUM		= TS_TEST3 + MAX_ITERATIONS
	};

	

	
	
	
	
	Gpu *gpu;

	uint32_t gfxQueueFamilyIndex;
	VkCommandPool gfxCommandPool;
	VkQueue gfxQueue;
	VkCommandBuffer gfxCommandBuffers[tNUM*MAX_ITERATIONS];

	VkCommandPool computeCommandPools[ASYNC_QUEUES]; 
	VkQueue computeQueues[ASYNC_QUEUES];
	VkCommandBuffer computeCommandBuffers[ASYNC_QUEUES * tNUM*MAX_ITERATIONS];

	VkSemaphore semaphores[SEMAPHORE_COUNT];
	VkEvent events[EVENT_COUNT];

	uint32_t asyncQueueCount;

	VkDescriptorPool descriptorPool;

	StagingBufferObject buffers[bNUM];

	VkDescriptorSetLayout descriptorSetLayouts[tNUM];
	VkDescriptorSet descriptorSets[tNUM];
	VkPipelineLayout pipelineLayouts[tNUM]; 

	VkPipeline pipelines[MAX_PIPELINES];
	VkShaderModule shaderModules[MAX_PIPELINES];

	int32_t taskToPipeline[tNUM];

	SystemTools::StopWatch stopWatch;

	





	struct Settings
	{
		bool barrier[tNUM];
		int dispatches[tNUM];
		int wavefrontsPerDispatch[tNUM];

		bool interleave0and1;
		bool interleave3withAny;
		bool useMultipleQueues;
		bool syncByEvent;
		bool syncIterations;
		bool sync0;
		bool sync1;

		int gfxQueueMapping;
		int iterations;

		void Init()
		{
			barrier[0] = 1; dispatches[0] = 50; wavefrontsPerDispatch[0] = 10; 
			barrier[1] = 1; dispatches[1] = 50; wavefrontsPerDispatch[1] = 10; 
			barrier[2] = 1; dispatches[2] = 50; wavefrontsPerDispatch[2] = 10;
			barrier[3] = 0; dispatches[3] = 15; wavefrontsPerDispatch[3] = 100000;
		
			interleave0and1 = 0;
			interleave3withAny = false;
			useMultipleQueues = true;
			syncByEvent = true;
			syncIterations = true;
			sync0 = true;
			sync1 = false;
			gfxQueueMapping = 2;//-1;//
			iterations = 1;	
		}

	} settings;





	inline int CmdToQueue (int cmdIndex)
	{
		const int map[4] = {0,1,1,2};
		return map[cmdIndex];
	}
	VkQueue GetQueue (int queueIndex)
	{
		assert (queueIndex>=0 && queueIndex<3);
		if (queueIndex == settings.gfxQueueMapping) return gfxQueue;
		return computeQueues[queueIndex];
	}
	VkCommandBuffer* GetCommandBuffer (int taskIndex, int interation = 0)
	{
//return &computeCommandBuffers[ (interation * tNUM) + taskIndex ];
		assert (taskIndex>=0 && taskIndex<4);
		int queueIndex = CmdToQueue(taskIndex);
		if (queueIndex == settings.gfxQueueMapping) return &gfxCommandBuffers[ (interation * tNUM) + taskIndex ];
		return &computeCommandBuffers[ (queueIndex * tNUM * MAX_ITERATIONS) + (interation * tNUM) + taskIndex ];
	}
	int GetProfilerQueueID (int queueIndex)
	{
		assert (queueIndex>=0 && queueIndex<3);
		if (queueIndex == settings.gfxQueueMapping) return TS_GRAPHICS_QUEUE;
		return TS_COMPUTE_QUEUE_0 + queueIndex;
	}








#ifdef USE_GPU_PROFILER


	struct Profiler 
	{
		enum
		{
			MAX_TIMERS = 2048,
			IGNORE_INITIAL_FRAMES = 3,
		};

		VkQueryPool queryPool;

		int numTimers;
		int tick;
		uint64_t usedFlags [(MAX_TIMERS*2+63) / 64];
		uint64_t timestamps [MAX_TIMERS*2];
		uint64_t startTime;
		uint64_t endTime;
		double averageDuration [MAX_TIMERS];
		double totalAverageDuration;

		void SetUsed (const int id)
		{
			usedFlags[id>>6] |= (uint64_t(1)<<uint64_t(id&63)); 
		}

		bool IsUsed (const int id)
		{
			return (usedFlags[id>>6] & (uint64_t(1)<<uint64_t(id&63))) != 0;
		}




		void ResetAveraging ()
		{
			for (int i=0; i < MAX_TIMERS*2; i++) timestamps[i] = 0;
			for (int i=0; i < MAX_TIMERS; i++) averageDuration[i] = 0;
			totalAverageDuration = 0;
			tick = 0;
		}

		void ClearFlags ()
		{
			for (int i=0; i < (MAX_TIMERS*2+63) / 64; i++) usedFlags[i] = 0;
		}

		void Init (Gpu *gpu, uint32_t numTimers)
		{
			this->numTimers = min((int)MAX_TIMERS, (int)numTimers);

			ClearFlags ();
			ResetAveraging ();

			VkQueryPoolCreateInfo poolCI = {};
			poolCI.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
			poolCI.queryType = VK_QUERY_TYPE_TIMESTAMP;
			poolCI.queryCount = this->numTimers*2;

			VkResult err = vkCreateQueryPool (gpu->device, &poolCI, nullptr, &queryPool);
			assert (!err);
		}

		void Destroy (Gpu *gpu)
		{
			vkDestroyQueryPool (gpu->device, queryPool, nullptr);
		}

		void RecordReset (VkCommandBuffer commandBuffer)
		{
			VkCommandBufferBeginInfo commandBufferBeginInfo = {};
			commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			commandBufferBeginInfo.flags = 0;
			vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
			vkCmdResetQueryPool (commandBuffer, queryPool, 0, MAX_TIMERS*2);
			vkEndCommandBuffer(commandBuffer);
		}

		void DownloadResults (Gpu *gpu)
		{
			bool oldUsed = false;
			uint32_t oldId = 0;
			for (uint32_t id = 0; id <= uint32_t(numTimers)*2; id++)
			{
				bool used = (id<MAX_TIMERS ? IsUsed (id) : false);
				if (!used && oldUsed)
				{
					VkResult err = vkGetQueryPoolResults(gpu->device, queryPool, oldId, id-oldId,
						sizeof(timestamps), (void*)&timestamps[oldId], sizeof(uint64_t), VK_QUERY_RESULT_64_BIT);
				}
				if (used && !oldUsed)
				{
					oldId = id;
				}
				oldUsed = used;
			}
			
			tick++;
			double f = double(gpu->physicalDeviceProps.limits.timestampPeriod) / 1000000.0;

			uint64_t start = 0xFFFFFFFFFFFFFFFFUL;
			uint64_t end = 0;

			for (int i=0; i<numTimers; i++)
			{
				double dur = f * double(GetDuration(i));
				if (tick > IGNORE_INITIAL_FRAMES) averageDuration[i] += dur;
			}
			
			{
				if (IsUsed(TS_GRAPHICS_QUEUE*2)) start = min (start, GetStartTime (TS_GRAPHICS_QUEUE));
				if (IsUsed(TS_GRAPHICS_QUEUE*2+1)) end = max (end, GetStopTime (TS_GRAPHICS_QUEUE));
				if (IsUsed(TS_COMPUTE_QUEUE_0*2)) start = min (start, GetStartTime (TS_COMPUTE_QUEUE_0));
				if (IsUsed(TS_COMPUTE_QUEUE_0*2+1)) end = max (end, GetStopTime (TS_COMPUTE_QUEUE_0));
				if (IsUsed(TS_COMPUTE_QUEUE_1*2)) start = min (start, GetStartTime (TS_COMPUTE_QUEUE_1));
				if (IsUsed(TS_COMPUTE_QUEUE_1*2+1)) end = max (end, GetStopTime (TS_COMPUTE_QUEUE_1));
				if (IsUsed(TS_COMPUTE_QUEUE_2*2)) start = min (start, GetStartTime (TS_COMPUTE_QUEUE_2));
				if (IsUsed(TS_COMPUTE_QUEUE_2*2+1)) end = max (end, GetStopTime (TS_COMPUTE_QUEUE_2));
			}

			startTime = start;
			endTime = end;
			double dur = f * double(end - start);
			if (tick > IGNORE_INITIAL_FRAMES) totalAverageDuration += dur;


			ImGui::Text("Current duration: %f ms", f * float(end-start));
		}


		void Start (int id, VkCommandBuffer cmd, VkPipelineStageFlagBits pipelineStage)
		{
			id = (id*2) & (MAX_TIMERS*2-1);
			SetUsed (id);
			vkCmdWriteTimestamp (cmd, pipelineStage, queryPool, id);
		}

		void Stop (int id, VkCommandBuffer cmd, VkPipelineStageFlagBits pipelineStage)
		{
			id = (id*2+1) & (MAX_TIMERS*2-1);
			SetUsed (id);
			vkCmdWriteTimestamp (cmd, pipelineStage, queryPool, id);
		}


		uint64_t GetDuration (int id) { return timestamps[id*2+1] - timestamps[id*2]; }
		uint64_t GetStartTime (int id) { return timestamps[id*2]; }
		uint64_t GetStopTime (int id) { return timestamps[id*2+1]; }
		float GetAverageDuration (int id) 
		{
			double d = averageDuration[id] / double(tick - IGNORE_INITIAL_FRAMES);
			return float(d); 
		}
		float GetAverageTotalDuration () 
		{
			double d = totalAverageDuration / double(tick - IGNORE_INITIAL_FRAMES);
			return float(d); 
		}

	};

	Profiler profiler;

#endif


	void CreateCommandPoolAndBuffer()
	{
		// create command pool and buffers for graphics queue

		{
			VkCommandPoolCreateInfo commandPoolCreateInfo = {};
				commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
				commandPoolCreateInfo.queueFamilyIndex = gfxQueueFamilyIndex;
				commandPoolCreateInfo.flags = 0;

			vkCreateCommandPool(gpu->device, &commandPoolCreateInfo, nullptr, &gfxCommandPool);
		}

		{
			VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
				commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				commandBufferAllocateInfo.commandBufferCount = tNUM * MAX_ITERATIONS;
				commandBufferAllocateInfo.commandPool = gfxCommandPool;
				commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

			vkAllocateCommandBuffers(gpu->device, &commandBufferAllocateInfo, gfxCommandBuffers);
		}


		// AMD: pick 3 queues from compute / transfer family and ignure graphics queue

		uint32_t const desiredCount = ASYNC_QUEUES;
		uint32_t foundFamilies[desiredCount];
		gpu->SearchQueues (asyncQueueCount, foundFamilies, computeQueues, desiredCount, 
			VK_QUEUE_COMPUTE_BIT, VK_QUEUE_GRAPHICS_BIT);
			//VK_QUEUE_COMPUTE_BIT, 0); // 1st from graphics, 2 & 3 from other family - makes no difference on performance

		assert (asyncQueueCount==ASYNC_QUEUES);

		// create command pool and buffer for each compute queue
		for (uint32_t i=0; i<asyncQueueCount; i++)
		{
			VkCommandPoolCreateInfo commandPoolCreateInfo = {};
				commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
				commandPoolCreateInfo.queueFamilyIndex = foundFamilies[i];
				commandPoolCreateInfo.flags = 0;

			vkCreateCommandPool(gpu->device, &commandPoolCreateInfo, nullptr, &computeCommandPools[i]);
		}

		for (int i=0; i<ASYNC_QUEUES; i++)
		{
			VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
				commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				commandBufferAllocateInfo.commandBufferCount = tNUM * MAX_ITERATIONS;
				commandBufferAllocateInfo.commandPool = computeCommandPools[i];
				commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

			vkAllocateCommandBuffers(gpu->device, &commandBufferAllocateInfo, &computeCommandBuffers[i * tNUM * MAX_ITERATIONS]);
		}
	}



	void CreateBuffers()
	{
		const VkBufferUsageFlags deviceUsage	= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		const VkBufferUsageFlags hostUsage	= VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		for (uint32_t bufferName = 0; bufferName < bNUM; bufferName++)
		{
			buffers[bufferName].Prepare	(*gpu, 0, bufferSize[bufferName], deviceUsage, hostUsage);
		}
	}

	void UploadBuffers( const uint32_t flags)
	{

		vkDeviceWaitIdle(gpu->device);

		VkCommandBuffer copyCmd;
		CommandBuffer::Create (&copyCmd, gpu->device, gfxCommandPool);
		CommandBuffer::Begin (copyCmd);



		for (uint32_t bufferName = 0; bufferName < bNUM; bufferName++) if (flags & (1<<bufferName))
		{
			void *dst = (void*) buffers[bufferName].Map (*gpu);
			DataToBuffer (dst, bufferName, settings.wavefrontsPerDispatch, TASK_MAX_DISPATCHES);
			buffers[bufferName].Unmap (*gpu);
			buffers[bufferName].CmdUpload (copyCmd);
		}



		CommandBuffer::End (copyCmd);
		CommandBuffer::Submit (&copyCmd, gfxQueue);
		vkQueueWaitIdle(gfxQueue);
		CommandBuffer::Free (&copyCmd, gpu->device, gfxCommandPool);


	}

	void DownloadBuffers( const uint32_t flags = 0xFFFFFFFF)
	{

		vkDeviceWaitIdle(gpu->device);

		VkCommandBuffer copyCmd;
		CommandBuffer::Create (&copyCmd, gpu->device, gfxCommandPool);
		CommandBuffer::Begin (copyCmd);


		if (flags & (1<<bDBG)) buffers[bDBG].CmdDownload(copyCmd, bufferSize[bDBG]);

		if (flags & (1<<bTEST0)) buffers[bTEST0].CmdDownload(copyCmd, bufferSize[bTEST0]);
		if (flags & (1<<bTEST1)) buffers[bTEST1].CmdDownload(copyCmd, bufferSize[bTEST1]);
		if (flags & (1<<bTEST2)) buffers[bTEST2].CmdDownload(copyCmd, bufferSize[bTEST2]);
		if (flags & (1<<bTEST3)) buffers[bTEST3].CmdDownload(copyCmd, bufferSize[bTEST3]);


		CommandBuffer::End (copyCmd);
		CommandBuffer::Submit (&copyCmd, gfxQueue);
		vkQueueWaitIdle(gfxQueue);
		CommandBuffer::Free (&copyCmd, gpu->device, gfxCommandPool);



		if (flags & (1<<bDBG))
		{
			int32_t *src = (int32_t*) buffers[bDBG].Map (*gpu);
			BufferToData (src, bDBG);							
			buffers[bDBG].Unmap (*gpu);
		}
/*
		if (flags & (1<<bTEST0))
		{
			uint32_t buffer = bTEST0;
			void *data = buffers[buffer].Map (*gpu);
			BufferToData (data, buffer);
			buffers[buffer].Unmap (*gpu);
		}
*/
	}




	void PrepareDescriptor (VkDescriptorSetLayout* pSetLayout, const int numBindings)
	{
		int const maxBindings = 16;
		assert (numBindings <= maxBindings);

		VkDescriptorSetLayoutBinding descriptorSetLayoutBinding[maxBindings] = {};
		for (int i=0; i<numBindings; i++)
		{
			descriptorSetLayoutBinding[i].binding = i;
			descriptorSetLayoutBinding[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			descriptorSetLayoutBinding[i].descriptorCount = 1;
			descriptorSetLayoutBinding[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		}

		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo[1] = {};
			descriptorSetLayoutCreateInfo[0].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			descriptorSetLayoutCreateInfo[0].bindingCount = numBindings;
			descriptorSetLayoutCreateInfo[0].pBindings = descriptorSetLayoutBinding;

		vkCreateDescriptorSetLayout(gpu->device, descriptorSetLayoutCreateInfo, nullptr, pSetLayout);
	}

	void PrepareDescriptors (const uint32_t taskFlags = 0xFFFFFFFF)
	{
		if (taskFlags & (1<<tTEST0)) PrepareDescriptor (&descriptorSetLayouts[tTEST0], 2);
		if (taskFlags & (1<<tTEST1)) PrepareDescriptor (&descriptorSetLayouts[tTEST1], 2);
		if (taskFlags & (1<<tTEST2)) PrepareDescriptor (&descriptorSetLayouts[tTEST2], 2);
		if (taskFlags & (1<<tTEST3)) PrepareDescriptor (&descriptorSetLayouts[tTEST3], 2);
	}



	void CreatePipelineLayout (const int task, uint32_t pipeline_count)
	{
		taskToPipeline[task] = pipeline_count;

		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
			pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayouts[task];
			pipelineLayoutCreateInfo.setLayoutCount = 1;

		vkCreatePipelineLayout(gpu->device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayouts[task]);
	}

	void FillComputePipelineCI (VkComputePipelineCreateInfo *computePipelineCI, const int task, uint32_t pipeline_count)
	{
		computePipelineCI->sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		computePipelineCI->stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		computePipelineCI->stage.pName = "main";
		computePipelineCI->stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		computePipelineCI->stage.module = shaderModules[pipeline_count];
		computePipelineCI->layout = pipelineLayouts[task];
	}

	void CreatePipelines(const uint32_t taskFlags = 0xFFFFFFFF)
	{
		uint32_t pipeline_count = 0;

		if (taskFlags & (1<<tTEST0))
		{
			CreatePipelineLayout (tTEST0, pipeline_count);

			shaderModules[pipeline_count] = LoadSPV(gpu->device, "..\\shaders\\test0-64-comp.spv");
			
			VkComputePipelineCreateInfo computePipelineCI = {};
			FillComputePipelineCI (&computePipelineCI, tTEST0, pipeline_count);
			
			vkCreateComputePipelines(gpu->device, VK_NULL_HANDLE, 1, &computePipelineCI, nullptr, &pipelines[pipeline_count]);
			pipeline_count++;
			assert (pipeline_count < MAX_PIPELINES);
		}

		if (taskFlags & (1<<tTEST1))
		{
			CreatePipelineLayout (tTEST1, pipeline_count);

			shaderModules[pipeline_count] = LoadSPV(gpu->device, "..\\shaders\\test1-64-comp.spv");
			
			VkComputePipelineCreateInfo computePipelineCI = {};
			FillComputePipelineCI (&computePipelineCI, tTEST1, pipeline_count);
			
			vkCreateComputePipelines(gpu->device, VK_NULL_HANDLE, 1, &computePipelineCI, nullptr, &pipelines[pipeline_count]);
			pipeline_count++;
			assert (pipeline_count < MAX_PIPELINES);
		}

		if (taskFlags & (1<<tTEST2))
		{
			CreatePipelineLayout (tTEST2, pipeline_count);

			shaderModules[pipeline_count] = LoadSPV(gpu->device, "..\\shaders\\test2-64-comp.spv");
			
			VkComputePipelineCreateInfo computePipelineCI = {};
			FillComputePipelineCI (&computePipelineCI, tTEST2, pipeline_count);
			
			vkCreateComputePipelines(gpu->device, VK_NULL_HANDLE, 1, &computePipelineCI, nullptr, &pipelines[pipeline_count]);
			pipeline_count++;
			assert (pipeline_count < MAX_PIPELINES);
		}

		if (taskFlags & (1<<tTEST3))
		{
			CreatePipelineLayout (tTEST3, pipeline_count);

			shaderModules[pipeline_count] = LoadSPV(gpu->device, "..\\shaders\\test3-64-comp.spv");
			
			VkComputePipelineCreateInfo computePipelineCI = {};
			FillComputePipelineCI (&computePipelineCI, tTEST3, pipeline_count);
			
			vkCreateComputePipelines(gpu->device, VK_NULL_HANDLE, 1, &computePipelineCI, nullptr, &pipelines[pipeline_count]);
			pipeline_count++;
			assert (pipeline_count < MAX_PIPELINES);
		}
	}





	void UpdateTaskDescriptorSets (const int task, VkDescriptorBufferInfo *descriptorBufferInfos, int *bufferList, const int numBindings)
	{
		int const maxBindings = 16;
		assert (numBindings <= maxBindings);

		VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
		descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorSetAllocateInfo.pSetLayouts = &descriptorSetLayouts[task];
		descriptorSetAllocateInfo.descriptorSetCount = 1;
		descriptorSetAllocateInfo.descriptorPool = descriptorPool;


		vkAllocateDescriptorSets(gpu->device, &descriptorSetAllocateInfo, &descriptorSets[task]);

		VkWriteDescriptorSet writeDescriptorSets[maxBindings] = {};
		for (int i=0; i<numBindings; i++)
		{
			writeDescriptorSets[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSets[i].dstSet = descriptorSets[task];
			writeDescriptorSets[i].descriptorCount = 1;
			writeDescriptorSets[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			writeDescriptorSets[i].dstBinding = i;
			writeDescriptorSets[i].pBufferInfo = &descriptorBufferInfos[bufferList[i]];
		}

		vkUpdateDescriptorSets(gpu->device, numBindings, writeDescriptorSets, 0, nullptr);
	}
		
	void UploadDescriptors(const uint32_t taskFlags = 0xFFFFFFFF)
	{
		VkDescriptorBufferInfo descriptorBufferInfo[bNUM] = {};
		for (int i=0; i<bNUM; i++)
		{
			buffers[i].GetDescriptorBufferInfo(descriptorBufferInfo[i]);
		}


		if (taskFlags & (1<<tTEST0))
		{
			int bufferList[] = {bTEST0, bDBG};
			UpdateTaskDescriptorSets (tTEST0, descriptorBufferInfo, bufferList, 2);
		}
		if (taskFlags & (1<<tTEST1))
		{
			int bufferList[] = {USE_UNIQUE_BUFFER_PER_TASK ? bTEST1 : bTEST0, bDBG};
			UpdateTaskDescriptorSets (tTEST1, descriptorBufferInfo, bufferList, 2);
		}
		if (taskFlags & (1<<tTEST2))
		{
			int bufferList[] = {USE_UNIQUE_BUFFER_PER_TASK ? bTEST2 : bTEST0, bDBG};
			UpdateTaskDescriptorSets (tTEST2, descriptorBufferInfo, bufferList, 2);
		}
		if (taskFlags & (1<<tTEST3))
		{
			int bufferList[] = {USE_UNIQUE_BUFFER_PER_TASK ? bTEST3 : bTEST0, bDBG};
			UpdateTaskDescriptorSets (tTEST3, descriptorBufferInfo, bufferList, 2);
		}

	}





	void MemoryBarriers (VkCommandBuffer commandBuffer, int *bufferList, const int numBarriers, VkDeviceSize *offset = 0, VkDeviceSize *size = 0 )
	{
		int const maxBarriers = 16;
		assert (numBarriers <= maxBarriers);

		VkBufferMemoryBarrier bufferMemoryBarriers[maxBarriers] = {};
		
		for (int i=0; i<numBarriers; i++)
		{
			bufferMemoryBarriers[i].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			bufferMemoryBarriers[i].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
			bufferMemoryBarriers[i].dstAccessMask = /*VK_ACCESS_INDIRECT_COMMAND_READ_BIT |*/ VK_ACCESS_SHADER_READ_BIT;
			bufferMemoryBarriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			bufferMemoryBarriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			bufferMemoryBarriers[i].buffer = buffers[bufferList[i]].deviceBuffer;
			bufferMemoryBarriers[i].offset = (offset ? offset[i] : 0);
			bufferMemoryBarriers[i].size = (size ? size[i] : VK_WHOLE_SIZE);
		}

		vkCmdPipelineBarrier(
			commandBuffer,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,// | VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
			0,//VkDependencyFlags       
			0, NULL,//numBarriers, memoryBarriers,//
			numBarriers, bufferMemoryBarriers,
			0, NULL);
	}
















	void RecordDispatch (VkCommandBuffer commandBuffer, const uint32_t taskFlags, int *dispatchCounters)
	{
		int profilerIDs[4] = {TS_TEST0, TS_TEST1, TS_TEST2, TS_TEST3};

		VkDeviceSize barrierOffset[4];// = {0, 0x1000, 0x2000, 0x3000};
		VkDeviceSize barrierSize[4];// = {0x1000, 0x1000, 0x1000, 0x1000};
		int barrierBuffers[4];
		int barrierCount = 0;

		for (int task=0; task<tNUM; task++) if (taskFlags & (1<<task))
		{
			int dispatchNumber = dispatchCounters[task]++;
			if (dispatchNumber < settings.dispatches[task])
			{
				int dispatchIndex = TASK_MAX_DISPATCHES*task + dispatchNumber;
#if (PROFILE_TASKS == 2)
				profiler.Start (profilerIDs[taskNumber]+dispatchIndex, commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
#endif
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayouts[tTEST0 + task], 0, 1, &descriptorSets[tTEST0 + task], 0, nullptr);	
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelines[taskToPipeline[tTEST0 + task]]);
				vkCmdDispatchIndirect(commandBuffer, buffers[bDISPATCH].deviceBuffer, sizeof(VkDispatchIndirectCommand) * dispatchIndex );

#if (PROFILE_TASKS == 2)
				profiler.Stop (profilerIDs[taskNumber]+dispatchIndex, commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
#endif

				if (settings.barrier[task]) 
				{
					barrierBuffers[barrierCount] = {USE_UNIQUE_BUFFER_PER_TASK ? bTEST0 + task : bTEST0};
					barrierOffset[barrierCount] = 0x1000 * task;
					barrierSize[barrierCount] = 0x1000;
					barrierCount++;
				}
			}
		}

		if (barrierCount) MemoryBarriers (commandBuffer, barrierBuffers, barrierCount, barrierSize, barrierOffset );
	}


		
	void RecordCommandBuffer (VkCommandBuffer commandBuffer, const uint32_t taskFlags, int *dispatchCounters, bool useEvent, int iteration, int profilerQueueStartID, int profilerQueueStopID)
	{
		

		VkCommandBufferBeginInfo commandBufferBeginInfo = {};
		commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		commandBufferBeginInfo.flags = 0;

		vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);


#ifdef USE_GPU_PROFILER
		if (profilerQueueStartID>=0) profiler.Start (profilerQueueStartID, commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
#endif


		int iterations = settings.useMultipleQueues ? 1 : settings.iterations;

		for (int i=0; i<iterations; i++)
		{

			if (taskFlags & (1<<tTEST0))
			{
		#if (PROFILE_TASKS == 1)
				profiler.Start (TS_TEST0+iteration+i, commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
		#endif
				uint32_t flags = (1<<tTEST0);
				if (settings.interleave0and1) flags |= (1<<tTEST1);
				if (settings.interleave3withAny) flags |= (1<<tTEST3);
				for (int i=0; i<settings.dispatches[0]; i++)
				{
					RecordDispatch (commandBuffer, flags, dispatchCounters);
				}
		#if (PROFILE_TASKS == 1)
				profiler.Stop (TS_TEST0+iteration+i, commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
		#endif
			}



			if (taskFlags & (1<<tTEST1))
			{
				if (settings.useMultipleQueues && iteration)
				{
					vkCmdWaitEvents( commandBuffer, 1, &events[1],
						VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, NULL, 0,	NULL, 0, NULL);	
					if (iteration+1 < settings.iterations) 
						vkCmdResetEvent( commandBuffer, events[1], VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
				}

		#if (PROFILE_TASKS == 1)
				profiler.Start (TS_TEST1+iteration+i, commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
		#endif
				uint32_t flags = (1<<tTEST1);
				if (settings.interleave0and1) flags |= (1<<tTEST0);
				if (settings.interleave3withAny) flags |= (1<<tTEST3);
				for (int i=0; i<settings.dispatches[1]; i++)
				{
					RecordDispatch (commandBuffer, flags, dispatchCounters);
				}
		#if (PROFILE_TASKS == 1)
				profiler.Stop (TS_TEST1+iteration+i, commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
		#endif

				if (useEvent)
				{
					vkCmdSetEvent(commandBuffer, events[0], VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
				}
			}



			if (taskFlags & (1<<tTEST2))
			{
				if (useEvent)
				{
					vkCmdWaitEvents( commandBuffer, 1, &events[0],
						VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, NULL, 0,	NULL, 0, NULL);	
					if (iteration+1 < settings.iterations) 
						vkCmdResetEvent( commandBuffer, events[0], VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
				}

		#if (PROFILE_TASKS == 1)
				profiler.Start (TS_TEST2+iteration+i, commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
		#endif
				uint32_t flags = (1<<tTEST2);
				if (settings.interleave3withAny) flags |= (1<<tTEST3);
				for (int i=0; i<settings.dispatches[2]; i++)
				{
					RecordDispatch (commandBuffer, flags, dispatchCounters);
				}
		#if (PROFILE_TASKS == 1)
				profiler.Stop (TS_TEST2+iteration+i, commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
		#endif

				if (settings.useMultipleQueues && iteration+1 < settings.iterations)
				{
					vkCmdSetEvent(commandBuffer, events[1], VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
				}
			}

			if (iterations > 1) 
			{
				dispatchCounters[0] = 0; dispatchCounters[1] = 0; dispatchCounters[2] = 0; 
			}
		}


		if (taskFlags & (1<<tTEST3))
		{
	#if (PROFILE_TASKS == 1)
			profiler.Start (TS_TEST3+iteration, commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	#endif
			uint32_t flags = (1<<tTEST3);
			for (int i=0; i<settings.dispatches[3]; i++)
			{
				RecordDispatch (commandBuffer, flags, dispatchCounters);
			}
	#if (PROFILE_TASKS == 1)
			profiler.Stop (TS_TEST3+iteration, commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	#endif
		}



#ifdef USE_GPU_PROFILER
		if (profilerQueueStopID>=0) profiler.Stop (profilerQueueStopID, commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
#endif

		vkEndCommandBuffer(commandBuffer);

	}

	void RebuildCommandBuffers ()
	{
#ifdef USE_GPU_PROFILER
		profiler.ClearFlags();
		profiler.ResetAveraging();
#endif

		vkResetCommandPool (gpu->device, gfxCommandPool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
		vkResetCommandPool (gpu->device, computeCommandPools[0], VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
		vkResetCommandPool (gpu->device, computeCommandPools[1], VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
		vkResetCommandPool (gpu->device, computeCommandPools[2], VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);

		int dispatchCounters[4] = {0,0,0,0};

		if (!settings.useMultipleQueues)
		{
			RecordCommandBuffer (*GetCommandBuffer(0,0), (1<<tTEST0) | (1<<tTEST1) | (1<<tTEST2) | (settings.dispatches[3] ? (1<<tTEST3) : 0), dispatchCounters, false, 0, GetProfilerQueueID(0), GetProfilerQueueID(0));
		}
		else
		{
			for (int i=0; i<settings.iterations; i++)
			{
				RecordCommandBuffer (*GetCommandBuffer(0,i), (1<<tTEST0), dispatchCounters, false,					i, (i==0 ? GetProfilerQueueID(0) : -1),	(i+1==settings.iterations ? GetProfilerQueueID(0) : -1));
				RecordCommandBuffer (*GetCommandBuffer(1,i), (1<<tTEST1), dispatchCounters, settings.syncByEvent,	i, (i==0 ? GetProfilerQueueID(1) : -1),	-1);
				RecordCommandBuffer (*GetCommandBuffer(2,i), (1<<tTEST2), dispatchCounters, settings.syncByEvent,	i, -1,									(i+1==settings.iterations ? GetProfilerQueueID(1) : -1));
				dispatchCounters[0] = 0; dispatchCounters[1] = 0; dispatchCounters[2] = 0; 
			}
			if (settings.dispatches[3]) 
				RecordCommandBuffer (*GetCommandBuffer(3,0), (1<<tTEST3), dispatchCounters, false, 0, GetProfilerQueueID(2),	GetProfilerQueueID(2));
		}

	}
	

	void Execute ()
	{

		bool modCB = false;
		modCB |= ImGui::Checkbox("Use multiple queues", &settings.useMultipleQueues);
		modCB |= ImGui::InputInt("Replace given compute queue with faster graphics queue if 0, 1 or 2 (AMD specific)", &settings.gfxQueueMapping); 
		modCB |= ImGui::InputInt("Iterations of tasks 0 to 2", &settings.iterations); 
		modCB |= ImGui::Checkbox("Task 0 has memory barrier after each dispatch", &settings.barrier[0]); modCB |= ImGui::DragInt("task 0 dispatch count", &settings.dispatches[0], 0.1f, 1, TASK_MAX_DISPATCHES);
		modCB |= ImGui::Checkbox("Task 1 has memory barrier after each dispatch", &settings.barrier[1]); modCB |= ImGui::DragInt("task 1 dispatch count", &settings.dispatches[1], 0.1f, 1, TASK_MAX_DISPATCHES);
		modCB |= ImGui::Checkbox("Task 2 has memory barrier after each dispatch", &settings.barrier[2]); modCB |= ImGui::DragInt("task 2 dispatch count", &settings.dispatches[2], 0.1f, 1, TASK_MAX_DISPATCHES);
		modCB |= ImGui::Checkbox("Task 3 has memory barrier after each dispatch", &settings.barrier[3]); modCB |= ImGui::DragInt("task 3 dispatch count (set to zero to disable task)", &settings.dispatches[3], 0.1f, 0, TASK_MAX_DISPATCHES);
		modCB |= ImGui::Checkbox("Interleave task 0 and 1 dispatches so they can process in parallel within a single queue followed by common memory barrier", &settings.interleave0and1); 
		modCB |= ImGui::Checkbox("Interleave task 3 with any other task within a single queue", &settings.interleave3withAny); 

		settings.iterations = max (1, min (int(MAX_ITERATIONS), settings.iterations));

		if (modCB)
		{
			vkDeviceWaitIdle(gpu->device);

			RebuildCommandBuffers ();
#ifdef USE_GPU_PROFILER
			profiler.ResetAveraging();
#endif
			vkDeviceWaitIdle(gpu->device);
		}




#ifdef FORCE_WAIT
vkDeviceWaitIdle(gpu->device);
#endif
		stopWatch.Start();

		if (!settings.useMultipleQueues)
		{
			VkQueue queue = GetQueue(0);
			
			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = GetCommandBuffer(0);

			vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
			vkQueueWaitIdle(queue); 
		}
		else
		{
			// setup SubmitInfos // optimize: no need to do this every frame

			VkSubmitInfo submitInfosTask3[1] = {};

			VkSubmitInfo submitInfosQ1[MAX_ITERATIONS*2] = {};
			VkSubmitInfo submitInfosQ0[MAX_ITERATIONS] = {};
			
			
			
			bool modSM = false;
			modSM |= ImGui::Checkbox("Sync task 0 to finish before task 2 starts (Semaphore necessary)", &settings.sync0);
			modSM |= ImGui::Checkbox("Sync task 1 to finish before task 2 starts by Event (faster)", &settings.syncByEvent);
			modSM |= ImGui::Checkbox("Sync task 1 to finish before task 2 starts by Semaphore (slower)", &settings.sync1);
			modSM |= ImGui::Checkbox("Sync multiple iterations (1 Semaphore and 1 Event between iterations) Note: I'm unsure if it's necessary to ensure precious CBs have completed using Events at all.", &settings.syncIterations);
			
			if (modSM)
			{
				vkDeviceWaitIdle(gpu->device);

				RebuildCommandBuffers ();
	#ifdef USE_GPU_PROFILER
				profiler.ResetAveraging();
	#endif
				vkDeviceWaitIdle(gpu->device);
			}

			submitInfosTask3[0].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfosTask3[0].commandBufferCount = settings.dispatches[3] ? 1 : 0;
			submitInfosTask3[0].pCommandBuffers = GetCommandBuffer(3,0);

			uint32_t submitCount = settings.iterations;

			for (uint32_t i=0; i<submitCount; i++) 
			{
			
				submitInfosQ0[i].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				submitInfosQ0[i].commandBufferCount = 1;
				submitInfosQ0[i].pCommandBuffers = GetCommandBuffer(0,i);
				if (settings.sync0)
				{
					submitInfosQ0[i].signalSemaphoreCount = 1;
					submitInfosQ0[i].pSignalSemaphores = &semaphores[i*3+0];
				}

				submitInfosQ1[i*2+0].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				submitInfosQ1[i*2+0].commandBufferCount = 1;
				submitInfosQ1[i*2+0].pCommandBuffers = GetCommandBuffer(1,i);
				if (settings.sync1)
				{
					submitInfosQ1[i*2+0].signalSemaphoreCount = 1;
					submitInfosQ1[i*2+0].pSignalSemaphores = &semaphores[i*3+1];
				}

				submitInfosQ1[i*2+1].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				submitInfosQ1[i*2+1].commandBufferCount = 1;
				submitInfosQ1[i*2+1].pCommandBuffers = GetCommandBuffer(2,i);
				if (settings.syncIterations && i+1 != submitCount)
				{
					submitInfosQ1[i*2+1].signalSemaphoreCount = 1;
					submitInfosQ1[i*2+1].pSignalSemaphores = &semaphores[i*3+2];
				}

				VkPipelineStageFlags waitFlags[2] = {VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT};

				if (settings.sync0 && settings.sync1)
				{
					submitInfosQ1[i*2+1].waitSemaphoreCount = 2;
					submitInfosQ1[i*2+1].pWaitSemaphores = &semaphores[i*3+0];
					submitInfosQ1[i*2+1].pWaitDstStageMask = waitFlags;
				}
				else if (settings.sync0)
				{
					submitInfosQ1[i*2+1].waitSemaphoreCount = 1;
					submitInfosQ1[i*2+1].pWaitSemaphores = &semaphores[i*3+0];
					submitInfosQ1[i*2+1].pWaitDstStageMask = waitFlags;
				}
				else if (settings.sync1)
				{
					submitInfosQ1[i*2+1].waitSemaphoreCount = 1;
					submitInfosQ1[i*2+1].pWaitSemaphores = &semaphores[i*3+1];
					submitInfosQ1[i*2+1].pWaitDstStageMask = waitFlags;
				}
				if (settings.syncIterations && i != 0)
				{
					submitInfosQ0[i].waitSemaphoreCount = 1;
					submitInfosQ0[i].pWaitSemaphores = &semaphores[(i-1)*3+2];
					submitInfosQ0[i].pWaitDstStageMask = waitFlags;
				}
			}

			// submit
			if (settings.syncByEvent) vkResetEvent(gpu->device, events[0]);
			if (settings.syncIterations && settings.iterations > 1) vkResetEvent(gpu->device, events[1]);

			if (settings.dispatches[3]) vkQueueSubmit(GetQueue(2), 1, &submitInfosTask3[0], VK_NULL_HANDLE);

#if 0
			vkQueueSubmit(GetQueue(0), submitCount, &submitInfosQ0[0], VK_NULL_HANDLE); // Queue 0x000001D7BF7D07D0 (computeQueues[0]) is waiting on semaphore 0x71 (signaled by submitInfosQ1[1]) that has no way to be signaled.
			vkQueueSubmit(GetQueue(1), submitCount*2, &submitInfosQ1[0], VK_NULL_HANDLE);
#else
			for (uint32_t i=0; i<submitCount; i++) // submit task by task to avoid validation errors - are they justified?
			{
				vkQueueSubmit(GetQueue(0), 1, &submitInfosQ0[i], VK_NULL_HANDLE);
				vkQueueSubmit(GetQueue(1), 2, &submitInfosQ1[i*2], VK_NULL_HANDLE);
			}
#endif

			if (settings.dispatches[3]) vkQueueWaitIdle(GetQueue(2));
			vkQueueWaitIdle(GetQueue(0));
			vkQueueWaitIdle(GetQueue(1));
		}

		stopWatch.Stop();

#ifdef FORCE_WAIT
vkDeviceWaitIdle(gpu->device);
#endif
	}



	void Update ()
	{
		ImGui::Text("Example Problem: Low workload tasks 0 and 1 should execute in parallel but both must be done before task 2 starts.");
		ImGui::Text("Optionally we have a high workload task 3 totally independent of others.\n\n");

		static bool updateBuffers = 1;
		ImGui::Checkbox("Up/Download buffers every frame to change dispatch count and view some results", &updateBuffers);
			
		bool mod = false;
		if (updateBuffers)
		{
			mod |= ImGui::DragInt("task 0 Wavefronts per dispatch (click and drag)", &settings.wavefrontsPerDispatch[0], 0.5f, 0, 10000);
			mod |= ImGui::DragInt("task 1 Wavefronts per dispatch (click and drag)", &settings.wavefrontsPerDispatch[1], 0.5f, 0, 10000);
			mod |= ImGui::DragInt("task 2 Wavefronts per dispatch (click and drag)", &settings.wavefrontsPerDispatch[2], 0.5f, 0, 10000);
			mod |= ImGui::DragInt("task 3 Wavefronts per dispatch (click and drag)", &settings.wavefrontsPerDispatch[3], 10.0f, 0, 2000000);
		}
		ImGui::Text("");

	#ifdef USE_GPU_PROFILER
		if (mod) profiler.ResetAveraging();
	#endif

		if (updateBuffers) UploadBuffers (0xFFFFFFFF);
		Execute();
		if (updateBuffers) DownloadBuffers (0xFFFFFFFF);
		
		ShowTimestamps();

		{
			bool mod = false;
			ImGui::SetNextWindowSize(ImVec2(200,300), ImGuiSetCond_FirstUseEver);
            ImGui::Begin("Example Settings");
			ImGui::Text("Show interesting behaviour, good and bad practices...");
			ImGui::Text("All settings and timings from using FuryX");

			ImGui::Text("");


            if (ImGui::Button("Atomic bug")) { mod = true; settings.Init();
				settings.interleave3withAny = true;
				settings.dispatches[3] = 150; settings.wavefrontsPerDispatch[3] = 10000;
			}
			ImGui::Text("Wavefront count of task 3 fluctuates. \nProbably an issue of atomics from multiple queues?");

			ImGui::Text("---------------------");

            if (ImGui::Button("Gfx queue too dominant")) { mod = true; settings.Init();
				settings.iterations = 3;
				settings.dispatches[3] = 150; settings.wavefrontsPerDispatch[3] = 10000;
			}
			ImGui::Text("Other queues kick in much too late and gfx queue is nearly finished.");

            if (ImGui::Button("Solution 1: Less dispatches in gfx queue.")) { mod = true; settings.Init();
				settings.iterations = 3;
			}
			
            if (ImGui::Button("Solution 2: Barriers in gfx queue.")) { mod = true; settings.Init();
				settings.iterations = 3;
				settings.barrier[3] = true;
			}
			
            if (ImGui::Button("Fastest result (2.32ms) with Interleaving.")) { mod = true; settings.Init();
				settings.iterations = 3;
				settings.interleave0and1 = true;
			}
			
			ImGui::Text("---------------------");

			ImGui::Text("Assuming we have no big amount of work we can do in parallel:");
            if (ImGui::Button("Gaps too large to be a win")) { mod = true; settings.Init();
				settings.iterations = 3;
				settings.dispatches[0] = 20; settings.dispatches[1] = 20; settings.dispatches[2] = 20; settings.dispatches[3] = 0; 
			}

            if (ImGui::Button("Compare: Using only one queue is faster")) { mod = true; settings.Init();
				settings.iterations = 3;
				settings.dispatches[0] = 20; settings.dispatches[1] = 20; settings.dispatches[2] = 20; settings.dispatches[3] = 0; 
				settings.gfxQueueMapping = 0;
				settings.useMultipleQueues = 0;
			}

            if (ImGui::Button("Fastest result (0.44ms) with Interleaving and one queue.")) { mod = true; settings.Init();
				settings.iterations = 3;
				settings.dispatches[0] = 20; settings.dispatches[1] = 20; settings.dispatches[2] = 20; settings.dispatches[3] = 0; 
				settings.gfxQueueMapping = 0;
				settings.useMultipleQueues = 0;
				settings.interleave0and1 = true;
			}

			ImGui::Text("---------------------");

            if (ImGui::Button("Good result (2.09ms) with gfx queue doing large work task.")) { mod = true; settings.Init();
				settings.interleave0and1 = true;
			}
            if (ImGui::Button("Bad result (5.74ms) not utilizing gfx queue. Is this intended?")) { mod = true; settings.Init();
				settings.interleave0and1 = true;
				settings.gfxQueueMapping = -1;
			}
			


            ImGui::End();

			if (mod)
			{
				vkDeviceWaitIdle(gpu->device);

				RebuildCommandBuffers ();
	#ifdef USE_GPU_PROFILER
				profiler.ResetAveraging();
	#endif
				vkDeviceWaitIdle(gpu->device);
			}
		}
	}



#ifdef USE_GPU_PROFILER
	void OutputProfilerLine (const char *desc, int id, ImU32 color = 0xAF808080)
	{
		if (!profiler.IsUsed(id*2)) return;
		if (!profiler.IsUsed(id*2+1)) return;
		
		uint64_t s = profiler.GetStartTime(id) - profiler.startTime;
		uint64_t e = profiler.GetStopTime(id) - profiler.startTime;
		uint64_t d = profiler.endTime - profiler.startTime;

		float t0 = float(s) / float(d);
		float t1 = float(e) / float(d);

		ImGui::PushItemWidth(ImGui::GetWindowWidth() - 20.0f);
		ImGui::RangeBar(t0, t1, ImVec2(0.0f,0.0f), color);
		ImGui::PopItemWidth();
		ImGui::SameLine(10.0f);
		ImGui::Text (desc,	(profiler.GetAverageDuration(id)));
	}
#endif

	void ShowTimestamps ()
	{
		ImGui::SetNextWindowSize(ImVec2(300,300), ImGuiSetCond_FirstUseEver);	
		ImGui::Begin("Profiler");
		
		ImGui::Text ("Host average duration over 64 samples: %f ms",	stopWatch.AverageTime ());

#ifdef USE_GPU_PROFILER
		ImGui::Text ("GPU Profiler");

		profiler.DownloadResults(gpu);



		ImGui::Text ("");
		ImGui::Text ("Average duration over %i samples: %f ms", profiler.tick,	profiler.GetAverageTotalDuration() );
		ImGui::Text ("");
		OutputProfilerLine ("GRAPHICS_QUEUE : %f ms", TS_GRAPHICS_QUEUE , 0xAF7F7F7F);
		OutputProfilerLine ("COMPUTE_QUEUE_0: %f ms", TS_COMPUTE_QUEUE_0, 0xAF0000FF);
		OutputProfilerLine ("COMPUTE_QUEUE_1: %f ms", TS_COMPUTE_QUEUE_1, 0xAF00FF00);
		OutputProfilerLine ("COMPUTE_QUEUE_2: %f ms", TS_COMPUTE_QUEUE_2, 0xAFFF7000);
		ImGui::Text ("");

#if (PROFILE_TASKS == 1)
		OutputProfilerLine ("task 3 %f ms",	TS_TEST3, 0xA00F7F7F);
		for (int i=0; i<settings.iterations; i++)
		{
			OutputProfilerLine ("task 0 %f ms",	TS_TEST0+i);
			OutputProfilerLine ("task 1 %f ms",	TS_TEST1+i, 0xAF7F7F00);
			OutputProfilerLine ("task 2 %f ms",	TS_TEST2+i, 0xAF7F007F);
		}
#endif		
#if (PROFILE_TASKS == 2)
		for (int i=0; i<settings.dispatches[0]; i++) OutputProfilerLine ("task 0 %f ms",	TS_TEST0+i);
		for (int i=0; i<settings.dispatches[1]; i++) OutputProfilerLine ("task 1 %f ms",	TS_TEST1+i);
		for (int i=0; i<settings.dispatches[2]; i++) OutputProfilerLine ("task 2 %f ms",	TS_TEST2+i);
		for (int i=0; i<settings.dispatches[3]; i++) OutputProfilerLine ("task 3 %f ms",	TS_TEST3+i);
#endif		
		
#endif
		

		ImGui::End();


	}





	void Init (Gpu *gpu, uint32_t queueFamilyIndex, VkQueue queue, VkDescriptorPool descriptorPool)
	{
		InitBase ();

		uint32_t taskFlags = (1<<tTEST0) | (1<<tTEST1) | (1<<tTEST2) | (1<<tTEST3);

		this->gpu = gpu;
		this->gfxQueueFamilyIndex = queueFamilyIndex;
		this->gfxQueue = queue;
		this->descriptorPool = descriptorPool;

		
		//

		for (int i=0; i<tNUM; i++) 
		{
			pipelineLayouts[i] = 0;
			descriptorSetLayouts[i] = 0;
		}
		
		for (int i=0; i<MAX_PIPELINES; i++)
		{
			pipelines[i] = 0;
			shaderModules[i] = 0;
		}

		CreateCommandPoolAndBuffer();
		CreateBuffers();
		PrepareDescriptors(taskFlags);
		CreatePipelines(taskFlags);
		UploadDescriptors(taskFlags);

#ifdef USE_GPU_PROFILER
		profiler.Init(gpu, TS_NUM);
#endif

		

		VkSemaphoreCreateInfo VkSemaphoreCI = {};
		VkSemaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		for (int i=0; i<SEMAPHORE_COUNT; i++)
			vkCreateSemaphore(gpu->device, &VkSemaphoreCI, NULL, &semaphores[i]);

		VkEventCreateInfo VkEventCI = {};
		VkEventCI.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
		for (int i=0; i<EVENT_COUNT; i++)
			vkCreateEvent(gpu->device, &VkEventCI, NULL, &events[i]);

		stopWatch.SetNumberOfSamplesToAverage(64);



		
		settings.Init();

		RebuildCommandBuffers ();
	}

	void Destroy ()
	{
#ifdef USE_GPU_PROFILER
		profiler.Destroy (gpu);
#endif

		for (int i=0; i<SEMAPHORE_COUNT; i++)
			vkDestroySemaphore(gpu->device, semaphores[i], NULL);
		
		for (int i=0; i<EVENT_COUNT; i++)
			vkDestroyEvent(gpu->device, events[i], NULL);

		for (int i=0; i<tNUM; i++) if (descriptorSetLayouts[i]) vkDestroyDescriptorSetLayout(gpu->device, descriptorSetLayouts[i], nullptr);
		for (int i=0; i<MAX_PIPELINES; i++) if (pipelines[i]) vkDestroyPipeline(gpu->device, pipelines[i], nullptr);
		for (int i=0; i<tNUM; i++) if (pipelineLayouts[i]) vkDestroyPipelineLayout(gpu->device, pipelineLayouts[i], nullptr);
		for (int i=0; i<MAX_PIPELINES; i++) if (shaderModules[i]) vkDestroyShaderModule(gpu->device, shaderModules[i], nullptr);
		for (int i=0; i<bNUM; i++) buffers[i].Destroy (*gpu);
		vkDestroyCommandPool (gpu->device, gfxCommandPool, nullptr);

		for (uint32_t i=0; i<asyncQueueCount; i++) vkDestroyCommandPool (gpu->device, computeCommandPools[i], nullptr);
	}
};




/*
Q:

Why gfx faster? Queue priority no effect
Others can't kick in if many dispatches on gfx.
Faster gfx requires even more commandbuffer fragmentation and semaphores
not sure is sync1 is necessary at all

Issues:

crash on extensions
atomics from multiple queues


*/





