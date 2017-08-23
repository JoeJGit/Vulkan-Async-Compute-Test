#include "VK_Helper.h"

namespace VK_Helper
{
	void Context::InitVK (char* APP_SHORT_NAME, bool validate, bool use_break)
	{
		SystemTools::Log ("InitVK...\n");

		this->validate = validate;
		this->use_break = use_break;

		for (int i=0; i<MAX_GPUS; i++) gpus[i].Init();
		gpuCount = 0;

		int const MAX_LAYERS = 128;
		int const MAX_EXTENSIONS = 1024;
			
		uint32_t instance_validation_layer_count = 0;
		//@char *instance_validation_layers[MAX_LAYERS] = {};
		const char *instance_validation_layers[] = {"VK_LAYER_LUNARG_standard_validation"};
			
		uint32_t enabled_instance_extension_count = 0;
		char *instance_extension_names[MAX_EXTENSIONS] = {};

		VkResult err;



		// Look for instance layers 
		if (validate) 
		{
			//@VkBool32 validation_found = FindInstanceValidationLayers (instance_validation_layers, instance_validation_layer_count);
			instance_validation_layer_count = 1;
		}	

		// Look for instance extensions 
		VkExtensionProperties* instance_extensions = NULL; 
		FindInstanceExtensions (instance_extensions, 
			enabled_instance_extension_count, instance_extension_names, 	
			MAX_EXTENSIONS, validate, true);

		inst = CreateInstance (APP_SHORT_NAME, 
			instance_validation_layer_count,
			instance_validation_layers,
			enabled_instance_extension_count,
			instance_extension_names,
			validate, use_break);

		if(instance_extensions) free(instance_extensions);



		// Make initial call to query gpu_count, then second call for physicalDevice info
		err = vkEnumeratePhysicalDevices(inst, &gpuCount, NULL);
		assert(!err && gpuCount > 0);

		SystemTools::Log ("\nfound GPUs: %i\n", gpuCount);

		if (gpuCount > 0) 
		{
			VkPhysicalDevice *physical_devices = (VkPhysicalDevice*) malloc(sizeof(VkPhysicalDevice) * gpuCount);
			err = vkEnumeratePhysicalDevices(inst, &gpuCount, physical_devices);
			assert(!err);

			for (uint32_t i=0; i<min(MAX_GPUS, gpuCount); i++)
			{
				gpus[i].physicalDevice = physical_devices[i]; //@@@
				vkGetPhysicalDeviceProperties(gpus[i].physicalDevice, &gpus[i].physicalDeviceProps);
				vkGetPhysicalDeviceFeatures(gpus[i].physicalDevice, &gpus[i].features);
				vkGetPhysicalDeviceMemoryProperties(gpus[i].physicalDevice, &gpus[i].memoryProperties);

				SystemTools::Log ("deviceName: %s\n", gpus[i].physicalDeviceProps.deviceName);
				SystemTools::Log ("apiVersion: %i\n", gpus[i].physicalDeviceProps.apiVersion);
				SystemTools::Log ("driverVersion: %i\n", gpus[i].physicalDeviceProps.driverVersion);
#if 0
				// Look for validation layers
				if (validate) 
				{
						uint32_t device_enabled_layer_count;
						VkBool32 validation_found = CheckDeviceValidationLayers (device_enabled_layer_count,
							gpus[i].physicalDevice, instance_validation_layer_count, instance_validation_layers, validate);
				}
#endif

				// Look for device extensions

				uint32_t enabled_device_extension_count = 0;
				char *device_extension_names[MAX_EXTENSIONS] = {};

				VkExtensionProperties* device_extensions = NULL;

				LoadDeviceExtensions (device_extensions, enabled_device_extension_count, device_extension_names, 	
					gpus[i].physicalDevice, MAX_EXTENSIONS, true, /*true*/false); // todo: loading all Extensions causes vkCreateDevice to fail on AMD
			



				gpus[i].device = CreateDevice (
					gpus[i].queueFamilyProps,	
					gpus[i].queueFamilyCount,
					gpus[i].physicalDevice,
					instance_validation_layer_count,
					(const char *const *)((validate) ? instance_validation_layers : NULL),
					enabled_device_extension_count,
					device_extension_names);

				if (device_extensions) free(device_extensions); // should we keep them?

				/*VkDevice device2 = 0;
				device2 = CreateDevice (
					gpus[i].queueFamilyProps,	
					gpus[i].queueFamilyCount,
					gpus[i].physicalDevice,
					instance_validation_layer_count,
					(const char *const *)((validate) ? instance_validation_layers : NULL),
					enabled_extension_count,
					extension_names);*/

				VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
				pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
				err = vkCreatePipelineCache (gpus[i].device, &pipelineCacheCreateInfo, nullptr, &gpus[i].pipelineCache);
				assert(!err);

				SystemTools::Log ("\n");

			}
			free(physical_devices);
		} 
		else 
		{
			ERR_EXIT("vkEnumeratePhysicalDevices reported zero accessible devices.\n\n"
						"Do you have a compatible Vulkan installable client driver (ICD) "
						"installed?\nPlease look at the Getting Started guide for "
						"additional information.\n",
						"vkEnumeratePhysicalDevices Failure");
		}

//*
		if (validate) 
		{
			CreateDebugReportCallback = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(inst, "vkCreateDebugReportCallbackEXT");
			if (!CreateDebugReportCallback) ERR_EXIT("GetProcAddr: Unable to find vkCreateDebugReportCallbackEXT\n", "vkGetProcAddr Failure");
			DestroyDebugReportCallback = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(inst, "vkDestroyDebugReportCallbackEXT");
			if (!DestroyDebugReportCallback) ERR_EXIT("GetProcAddr: Unable to find vkDestroyDebugReportCallbackEXT\n", "vkGetProcAddr Failure");
			DebugReportMessage = (PFN_vkDebugReportMessageEXT)vkGetInstanceProcAddr(inst, "vkDebugReportMessageEXT");
			if (!DebugReportMessage) ERR_EXIT("GetProcAddr: Unable to find vkDebugReportMessageEXT\n", "vkGetProcAddr Failure");
				
			PFN_vkDebugReportCallbackEXT callback = use_break ? BreakCallback : dbgFunc;
			VkDebugReportCallbackCreateInfoEXT dbgCreateInfo = {};
				dbgCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
				dbgCreateInfo.pNext = NULL;
				dbgCreateInfo.pfnCallback = callback;
				dbgCreateInfo.pUserData = NULL;
				dbgCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
			err = CreateDebugReportCallback(inst, &dbgCreateInfo, NULL, &msg_callback);
			switch (err) 
			{
			case VK_SUCCESS:
				break;
			case VK_ERROR_OUT_OF_HOST_MEMORY:
				ERR_EXIT("CreateDebugReportCallback: out of host memory\n", "CreateDebugReportCallback Failure");
				break;
			default:
				ERR_EXIT("CreateDebugReportCallback: unknown failure\n", "CreateDebugReportCallback Failure");
				break;
			}
		}
//*/	


	}

		
	















	void Swapchain::Init (Context &context, HINSTANCE connection, HWND window, int width, int height, int gpuIndex)
	{
		SystemTools::Log ("InitSwapchain...\n");

		this->connection = connection;
		this->window = window;
		this->width = width;
		this->height = height;
		this->gpu = &context.gpus[gpuIndex]; // todo: find best gpu with surface support

		GET_INSTANCE_PROC_ADDR(context.inst, GetPhysicalDeviceSurfaceSupportKHR);
		GET_INSTANCE_PROC_ADDR(context.inst, GetPhysicalDeviceSurfaceCapabilitiesKHR);
		GET_INSTANCE_PROC_ADDR(context.inst, GetPhysicalDeviceSurfaceFormatsKHR);
		GET_INSTANCE_PROC_ADDR(context.inst, GetPhysicalDeviceSurfacePresentModesKHR);
		GET_INSTANCE_PROC_ADDR(context.inst, GetSwapchainImagesKHR);

		VkResult U_ASSERT_ONLY err;
		uint32_t i;




	// Create a WSI surface for the window:

		VkWin32SurfaceCreateInfoKHR createInfo;
		createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		createInfo.pNext = NULL;
		createInfo.flags = 0;
		createInfo.hinstance = connection;
		createInfo.hwnd = window;

		err = vkCreateWin32SurfaceKHR(context.inst, &createInfo, NULL, &surface);
		assert(!err);


		// Iterate over each queue to learn whether it supports presenting:
		VkBool32 *supportsPresent = (VkBool32 *)malloc(gpu->queueFamilyCount * sizeof(VkBool32));
		for (i = 0; i < gpu->queueFamilyCount; i++) 
		{
			fpGetPhysicalDeviceSurfaceSupportKHR(gpu->physicalDevice, i, surface, &supportsPresent[i]);
		}

		// Search for a graphics and a present queue in the array of queue
		// families, try to find one that supports both
		uint32_t graphicsQueueNodeIndex = UINT32_MAX;
		uint32_t presentQueueNodeIndex = UINT32_MAX;
		for (i = 0; i < gpu->queueFamilyCount; i++) 
		{
			if ((gpu->queueFamilyProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) 
			{
				if (graphicsQueueNodeIndex == UINT32_MAX) 
				{
					graphicsQueueNodeIndex = i;
				}

				if (supportsPresent[i] == VK_TRUE) 
				{
					graphicsQueueNodeIndex = i;
					presentQueueNodeIndex = i;
					break;
				}
			}
		}
		if (presentQueueNodeIndex == UINT32_MAX) 
		{
			// If didn't find a queue that supports both graphics and present, then
			// find a separate present queue.
			for (uint32_t i = 0; i < gpu->queueFamilyCount; ++i) 
			{
				if (supportsPresent[i] == VK_TRUE) 
				{
					presentQueueNodeIndex = i;
					break;
				}
			}
		}
		free(supportsPresent);

		// Generate error if could not find both a graphics and a present queue
		if (graphicsQueueNodeIndex == UINT32_MAX || presentQueueNodeIndex == UINT32_MAX) 
		{
			ERR_EXIT("Could not find a graphics and a present queue\n",
						"Swapchain Initialization Failure");
		}

		// TODO: Add support for separate queues, including presentation,
		//       synchronization, and appropriate tracking for QueueSubmit.
		// NOTE: While it is possible for an application to use a separate graphics
		//       and a present queues, this demo program assumes it is only using
		//       one:
		if (graphicsQueueNodeIndex != presentQueueNodeIndex) 
		{
			ERR_EXIT("Could not find a common graphics and a present queue\n",
						"Swapchain Initialization Failure");
		}

	// Get queue

		gfxPresentFamily = graphicsQueueNodeIndex;

		vkGetDeviceQueue (gpu->device, gfxPresentFamily, 0, &gfxPresentQueue);



		// search for independent transfer queues // todo: replace with Gpu::SearchQueues

		int foundCount = 0;
		for (int run=0; run<3 && foundCount<2; run++)
		{
			for (i = 0; i < gpu->queueFamilyCount && foundCount<2; i++) 
			{
				VkQueueFlags flags = gpu->queueFamilyProps[i].queueFlags;

				bool runPassed = false;
				
				if (run == 0) runPassed = (flags & (VK_QUEUE_TRANSFER_BIT | VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) == VK_QUEUE_TRANSFER_BIT;
				if (run == 1) runPassed = (flags & (VK_QUEUE_TRANSFER_BIT | VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) == (VK_QUEUE_TRANSFER_BIT | VK_QUEUE_COMPUTE_BIT);
				if (run == 2) runPassed = (flags & VK_QUEUE_TRANSFER_BIT) != 0;

				if (runPassed) 
				{
					if (foundCount == 0)
					{
						uploadFamily = i;
						vkGetDeviceQueue (gpu->device, uploadFamily, 0, &uploadQueue);
						foundCount++;
						if (gpu->queueFamilyProps[i].queueCount >= 2)
						{
							downloadFamily = i;
							vkGetDeviceQueue (gpu->device, downloadFamily, 1, &downloadQueue);
							foundCount++;
						}
					}
					if (foundCount == 1)
					{
						uploadFamily = i;
						vkGetDeviceQueue (gpu->device, downloadFamily, 1, &downloadQueue);
						foundCount++;
					}				
				}
			}
		}

		if (foundCount == 0) 
		{
			ERR_EXIT("Could not find a transfer queue\n",
						"Swapchain Initialization Failure");
		}

		if (foundCount == 1)
		{
			downloadFamily = uploadFamily;
			downloadQueue = uploadQueue;
		}





		GET_DEVICE_PROC_ADDR(gpu->device, CreateSwapchainKHR);
		GET_DEVICE_PROC_ADDR(gpu->device, DestroySwapchainKHR);
		GET_DEVICE_PROC_ADDR(gpu->device, GetSwapchainImagesKHR);
		GET_DEVICE_PROC_ADDR(gpu->device, AcquireNextImageKHR);
		GET_DEVICE_PROC_ADDR(gpu->device, QueuePresentKHR);


		// Get the list of VkFormat's that are supported:
		uint32_t formatCount;
		err = fpGetPhysicalDeviceSurfaceFormatsKHR(gpu->physicalDevice, surface, &formatCount, NULL);
		assert(!err);
		VkSurfaceFormatKHR *surfFormats = (VkSurfaceFormatKHR *)malloc(formatCount * sizeof(VkSurfaceFormatKHR));
		err = fpGetPhysicalDeviceSurfaceFormatsKHR(gpu->physicalDevice, surface, &formatCount, surfFormats);
		assert(!err);
		// If the format list includes just one entry of VK_FORMAT_UNDEFINED,
		// the surface has no preferred format.  Otherwise, at least one
		// supported format will be returned.
		if (formatCount == 1 && surfFormats[0].format == VK_FORMAT_UNDEFINED) 
		{
			format = VK_FORMAT_B8G8R8A8_UNORM;
		} 
		else 
		{
			assert(formatCount >= 1);
			format = surfFormats[0].format;
		}
		colorSpace = surfFormats[0].colorSpace;
	}


	void Swapchain::PrepareSwapchain () 
	{
		SystemTools::Log ("PrepareBuffers...\n");

		VkResult U_ASSERT_ONLY err;
		VkSwapchainKHR oldSwapchain = swapchain;

		// Check the surface capabilities and formats
		VkSurfaceCapabilitiesKHR surfCapabilities;
		err = fpGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu->physicalDevice, surface, &surfCapabilities);
		assert(!err);

		uint32_t presentModeCount;
		err = fpGetPhysicalDeviceSurfacePresentModesKHR(gpu->physicalDevice, surface, &presentModeCount, NULL);
		assert(!err);
		VkPresentModeKHR *presentModes = (VkPresentModeKHR *)malloc(presentModeCount * sizeof(VkPresentModeKHR));
		assert(presentModes);
		err = fpGetPhysicalDeviceSurfacePresentModesKHR(gpu->physicalDevice, surface, &presentModeCount, presentModes);
		assert(!err);

		VkExtent2D swapchainExtent;
		// width and height are either both -1, or both not -1.
		if (surfCapabilities.currentExtent.width == (uint32_t)-1) 
		{
			// If the surface size is undefined, the size is set to
			// the size of the images requested.
			swapchainExtent.width = width;
			swapchainExtent.height = height;
		} 
		else 
		{
			// If the surface size is defined, the swap chain size must match
			swapchainExtent = surfCapabilities.currentExtent;
			width = surfCapabilities.currentExtent.width;
			height = surfCapabilities.currentExtent.height;
		}

		// If mailbox mode is available, use it, as is the lowest-latency non-
		// tearing mode.  If not, try IMMEDIATE which will usually be available,
		// and is fastest (though it tears).  If not, fall back to FIFO which is
		// always available.
		VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
		for (size_t i = 0; i < presentModeCount; i++) 
		{
			if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) 
			{
				swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
				break;
			}
			if ((swapchainPresentMode != VK_PRESENT_MODE_MAILBOX_KHR) &&
				(presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)) 
			{
				swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
			}
		}
//swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
		if (swapchainPresentMode==VK_PRESENT_MODE_FIFO_KHR) SystemTools::Log ("swapchainPresentMode: VK_PRESENT_MODE_FIFO_KHR\n");
		if (swapchainPresentMode==VK_PRESENT_MODE_MAILBOX_KHR) SystemTools::Log ("swapchainPresentMode: VK_PRESENT_MODE_MAILBOX_KHR\n");
		if (swapchainPresentMode==VK_PRESENT_MODE_IMMEDIATE_KHR) SystemTools::Log ("swapchainPresentMode: VK_PRESENT_MODE_IMMEDIATE_KHR\n");

		// Determine the number of VkImage's to use in the swap chain (we desire to
		// own only 1 image at a time, besides the images being displayed and
		// queued for display):
		uint32_t desiredNumberOfSwapchainImages =
			surfCapabilities.minImageCount + 1;
		if ((surfCapabilities.maxImageCount > 0) &&
			(desiredNumberOfSwapchainImages > surfCapabilities.maxImageCount)) {
			// Application must settle for fewer images than desired:
			desiredNumberOfSwapchainImages = surfCapabilities.maxImageCount;
		}

		SystemTools::Log ("desiredNumberOfSwapchainImages: %i\n", desiredNumberOfSwapchainImages);

		VkSurfaceTransformFlagsKHR preTransform;
		if (surfCapabilities.supportedTransforms &
			VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
			preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		} else {
			preTransform = surfCapabilities.currentTransform;
		}

		VkSwapchainCreateInfoKHR swapchainInfo = {};
		swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		swapchainInfo.pNext = NULL,
		swapchainInfo.surface = surface,
		swapchainInfo.minImageCount = desiredNumberOfSwapchainImages,
		swapchainInfo.imageFormat = format,
		swapchainInfo.imageColorSpace = colorSpace,
		swapchainInfo.imageExtent.width = swapchainExtent.width, 
		swapchainInfo.imageExtent.height = swapchainExtent.height,
		swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		swapchainInfo.preTransform = (VkSurfaceTransformFlagBitsKHR) preTransform, // @@@
		swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		swapchainInfo.imageArrayLayers = 1,
		swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		swapchainInfo.queueFamilyIndexCount = 0,
		swapchainInfo.pQueueFamilyIndices = NULL,
		swapchainInfo.presentMode = swapchainPresentMode,
		swapchainInfo.oldSwapchain = oldSwapchain,
		swapchainInfo.clipped = true;
    
		uint32_t i;

		err = fpCreateSwapchainKHR(gpu->device, &swapchainInfo, NULL, &swapchain);
		assert(!err);

		// If we just re-created an existing swapchain, we should destroy the old
		// swapchain at this point.
		// Note: destroying the swapchain also cleans up all its associated
		// presentable images once the platform is done with them.
		if (oldSwapchain != VK_NULL_HANDLE) 
		{
			fpDestroySwapchainKHR(gpu->device, oldSwapchain, NULL);
		}

		err = fpGetSwapchainImagesKHR(gpu->device, swapchain,
											&frameCount, NULL);
		assert(!err);

		VkImage *swapchainImages = (VkImage *)malloc(frameCount * sizeof(VkImage));
		assert(swapchainImages);
		err = fpGetSwapchainImagesKHR(gpu->device, swapchain,
											&frameCount,
											swapchainImages);
		assert(!err);

		frames = (VK_Helper::Swapchain::Frame *)malloc(sizeof(VK_Helper::Swapchain::Frame) * frameCount);
		assert(frames);

		for (i = 0; i < frameCount; i++) 
		{
			VkImageViewCreateInfo color_image_view = {};
			color_image_view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			color_image_view.pNext = NULL,
			color_image_view.format = format,
			color_image_view.components.r = VK_COMPONENT_SWIZZLE_R,
			color_image_view.components.g = VK_COMPONENT_SWIZZLE_G,
			color_image_view.components.b = VK_COMPONENT_SWIZZLE_B,
			color_image_view.components.a = VK_COMPONENT_SWIZZLE_A,
			color_image_view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			color_image_view.subresourceRange.baseMipLevel = 0,
			color_image_view.subresourceRange.levelCount = 1,
			color_image_view.subresourceRange.baseArrayLayer = 0,
			color_image_view.subresourceRange.layerCount = 1,
			color_image_view.viewType = VK_IMAGE_VIEW_TYPE_2D,
			color_image_view.flags = 0;
        

			frames[i].image = swapchainImages[i];

			color_image_view.image = frames[i].image;

			err = vkCreateImageView(gpu->device, &color_image_view, NULL,
									&frames[i].view);
			assert(!err);
		}


		if (NULL != presentModes) {
			free(presentModes);
		}





		// allocate command buffers for each swapchain frame

		VkCommandBufferAllocateInfo cmdAllocInfo = {};
			cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			cmdAllocInfo.pNext = NULL,
			cmdAllocInfo.commandPool = commandPool,
			cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			cmdAllocInfo.commandBufferCount = 1;
    
		VkCommandBufferBeginInfo cmdBufInfo = {};
			cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			cmdBufInfo.pNext = NULL;

		for (uint32_t i = 0; i < frameCount; i++) 
		{
			err = vkAllocateCommandBuffers(gpu->device, &cmdAllocInfo, &frames[i].drawCommandBuffer);
			assert(!err);
			// Command buffers for submitting present barriers
			// One pre and post present buffer per swap chain image
			err = vkAllocateCommandBuffers(gpu->device, &cmdAllocInfo, &frames[i].prePresentCommandBuffer);
			err = vkAllocateCommandBuffers(gpu->device, &cmdAllocInfo, &frames[i].postPresentCommandBuffer);

			// Command buffer for post present barrier

			// Insert a post present image barrier to transform the image back to a
			// color attachment that our render pass can write to
			// We always use undefined image layout as the source as it doesn't actually matter
			// what is done with the previous image contents

			err = vkBeginCommandBuffer (frames[i].postPresentCommandBuffer, &cmdBufInfo);
			assert(!err);

			VkImageMemoryBarrier postPresentBarrier = {};
			postPresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			postPresentBarrier.pNext = NULL;
			postPresentBarrier.srcAccessMask = 0;
			postPresentBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // <-
			postPresentBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			postPresentBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			postPresentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			postPresentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			postPresentBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
			postPresentBarrier.image = frames[i].image;

			vkCmdPipelineBarrier(
				frames[i].postPresentCommandBuffer,
				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				//VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, // <-
//TODO:
VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &postPresentBarrier);



			err = vkEndCommandBuffer(frames[i].postPresentCommandBuffer);
			assert(!err);

			// Command buffers for pre present barrier

			// Submit a pre present image barrier to the queue
			// Transforms the (framebuffer) image layout from color attachment to present(khr) for presenting to the swap chain

			err = vkBeginCommandBuffer(frames[i].prePresentCommandBuffer, &cmdBufInfo);
			assert(!err);

			VkImageMemoryBarrier prePresentBarrier = {};
			prePresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			prePresentBarrier.pNext = NULL;
			prePresentBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			prePresentBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			prePresentBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			prePresentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			prePresentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			prePresentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			prePresentBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
			prePresentBarrier.image = frames[i].image;

			vkCmdPipelineBarrier(
				frames[i].prePresentCommandBuffer,
				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
				0,
				0, nullptr, // No memory barriers,
				0, nullptr, // No buffer barriers,
				1, &prePresentBarrier);

			err = vkEndCommandBuffer(frames[i].prePresentCommandBuffer);
			assert(!err);
		}

		//

	}


	void Swapchain::PrepareFramebuffers (VkRenderPass renderPass) 
	{
		SystemTools::Log ("PrepareFramebuffers...\n");

		VkImageView attachments[2];
		attachments[1] = depth.view;

		VkFramebufferCreateInfo fb_info = {};
		fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		fb_info.pNext = NULL,
		fb_info.renderPass = renderPass,
		fb_info.attachmentCount = 2,
		fb_info.pAttachments = attachments,
		fb_info.width = width,
		fb_info.height = height,
		fb_info.layers = 1;
    
		VkResult U_ASSERT_ONLY err;
		uint32_t i;

		//framebuffers = (VkFramebuffer *)malloc(frameCount * sizeof(VkFramebuffer));
		//assert(framebuffers);

		for (i = 0; i < frameCount; i++) 
		{
			attachments[0] = frames[i].view;
			err = vkCreateFramebuffer(gpu->device, &fb_info, NULL, &frames[i].framebuffer);
			assert(!err);
		}
	}


	void Swapchain::PrepareDepth () 
	{
		SystemTools::Log ("PrepareDepth...\n");

		VkFormat depth_format = VK_FORMAT_D16_UNORM;

		VkImageCreateInfo imageCreateInfo = {};
			imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			imageCreateInfo.pNext = NULL,
			imageCreateInfo.imageType = VK_IMAGE_TYPE_2D,
			imageCreateInfo.format = depth_format,
			imageCreateInfo.extent = {(uint32_t)width, (uint32_t)height, 1},
			imageCreateInfo.mipLevels = 1,
			imageCreateInfo.arrayLayers = 1,
			imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT,
			imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL,
			imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			imageCreateInfo.flags = 0;  
    

		depth.format = depth_format;

		Image::Prepare (depth.image, depth.mem, gpu->device, gpu->memoryProperties, imageCreateInfo);

		Image::ChangeLayoutBarrier (initCommandBuffer, depth.image, 
			VK_IMAGE_ASPECT_DEPTH_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			{VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1});


		VkImageViewCreateInfo viewCreateInfo = {};
			viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			viewCreateInfo.pNext = NULL,
			viewCreateInfo.image = depth.image;
			viewCreateInfo.format = depth_format,
			viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
			viewCreateInfo.subresourceRange.baseMipLevel = 0,
			viewCreateInfo.subresourceRange.levelCount = 1,
			viewCreateInfo.subresourceRange.baseArrayLayer = 0,
			viewCreateInfo.subresourceRange.layerCount = 1,
			viewCreateInfo.flags = 0,
			viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;

		VkResult U_ASSERT_ONLY err = vkCreateImageView(gpu->device, &viewCreateInfo, NULL, &depth.view);
		assert(!err);
	}





};