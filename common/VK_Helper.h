#pragma once

#include <SystemTools.h>
#include <Application.h>
#include <fstream>


// from Vulkan demo..

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <signal.h>
#ifdef __linux__
#include <X11/Xutil.h>
#endif

#ifdef _WIN32
#pragma comment(linker, "/subsystem:windows")
#define APP_NAME_STR_LEN 80
#endif // _WIN32

#ifdef ANDROID
#include "vulkan_wrapper.h"
#else
#include <vulkan/vulkan.h>
#endif

#include <vulkan/vk_sdk_platform.h>

#if defined(NDEBUG) && defined(__GNUC__)
#define U_ASSERT_ONLY __attribute__((unused))
#else
#define U_ASSERT_ONLY
#endif






#define GET_INSTANCE_PROC_ADDR(inst, entrypoint)                               \
    {                                                                          \
        fp##entrypoint =                                                 \
            (PFN_vk##entrypoint)vkGetInstanceProcAddr(inst, "vk" #entrypoint); \
        if (fp##entrypoint == NULL) {                                    \
            ERR_EXIT("vkGetInstanceProcAddr failed to find vk" #entrypoint,    \
                     "vkGetInstanceProcAddr Failure");                         \
        }                                                                      \
    }

static PFN_vkGetDeviceProcAddr g_gdpa = NULL;

#define GET_DEVICE_PROC_ADDR(dev, entrypoint)                                  \
    {                                                                          \
        if (!g_gdpa)                                                           \
            g_gdpa = (PFN_vkGetDeviceProcAddr)vkGetInstanceProcAddr(           \
                context.inst, "vkGetDeviceProcAddr");                            \
        fp##entrypoint =                                                 \
            (PFN_vk##entrypoint)g_gdpa(dev, "vk" #entrypoint);                 \
        if (fp##entrypoint == NULL) {                                    \
            ERR_EXIT("vkGetDeviceProcAddr failed to find vk" #entrypoint,      \
                     "vkGetDeviceProcAddr Failure");                           \
        }                                                                      \
    }

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))



namespace VK_Helper
{
	static VKAPI_ATTR VkBool32 VKAPI_CALL
	dbgFunc (VkFlags msgFlags, VkDebugReportObjectTypeEXT objType,
			uint64_t srcObject, size_t location, int32_t msgCode,
			const char *pLayerPrefix, const char *pMsg, void *pUserData) 
	{
		char *message = (char *)malloc(strlen(pMsg) + 100);

		assert(message);

		if (msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT) 
		{
			sprintf(message, "ERROR: [%s] Code %d : %s", pLayerPrefix, msgCode,
					pMsg);
			//validation_error = 1;
		} 
		else if (msgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT) 
		{
			// We know that we're submitting queues without fences, ignore this
			// warning
			if (strstr(pMsg, "vkQueueSubmit parameter, VkFence fence, is null pointer")) 
			{
				return false;
			}
			sprintf(message, "WARNING: [%s] Code %d : %s", pLayerPrefix, msgCode, pMsg);
			//validation_error = 1;
		} 
		else 
		{
			//validation_error = 1;
			return false;
		}

		SystemTools::Log(message);

#ifdef _WIN32
		MessageBox(NULL, message, "Alert", MB_OK);
#endif
		free(message);

		/*
		 * false indicates that layer should not bail-out of an
		 * API call that had validation failures. This may mean that the
		 * app dies inside the driver due to invalid parameter(s).
		 * That's what would happen without validation layers, so we'll
		 * keep that behavior here.
		 */
		return false;
	}




	static VKAPI_ATTR VkBool32 VKAPI_CALL
	BreakCallback (VkFlags msgFlags, VkDebugReportObjectTypeEXT objType,
				  uint64_t srcObject, size_t location, int32_t msgCode,
				  const char *pLayerPrefix, const char *pMsg,
				  void *pUserData) 
	{
		// todo: how to get rid of this?
		if (!strcmp(pMsg, "loader_get_json: Failed to open JSON file C:\\VulkanSDK\\RenderDoc_2016_07_23_33ed86a4_64\\renderdoc.json")) return false;
		if (!strcmp(pMsg, "loader_get_json: Failed to open JSON file C:\\Program Files (x86)\\Steam\\SteamOverlayVulkanLayer64.json")) return false;
		if (!strcmp(pMsg, "loader_get_json: Failed to open JSON file C:\\WINDOWS\\system32\\nv-vk64.json")) return false;
		
		SystemTools::Log (pMsg);
		SystemTools::Log ("\n");
		DebugBreak();
		return false;
	}



#if 1

	//Return 1 (true) if all layer names specified in check_names
	//can be found in given layer properties.	
	static bool CheckLayers (const uint32_t check_count, const char *const *check_names,
								 const uint32_t layer_count,  const VkLayerProperties *layers) 
	{
		for (uint32_t i = 0; i < check_count; i++) 
		{
			bool found = 0;
			for (uint32_t j = 0; j < layer_count; j++) 
			{
				if (!strcmp(check_names[i], layers[j].layerName)) 
				{
					found = 1;
					break;
				}
			}
			if (!found) 
			{
				SystemTools::Log ("Cannot find layer: %s\n", check_names[i]);
				return 0;
			}
		}
		return 1;
	}





	static char *instance_validation_layers_alt1[] = 
		{
			"VK_LAYER_LUNARG_standard_validation",
		};

	static char *instance_validation_layers_alt2[] = 
		{
			"VK_LAYER_GOOGLE_threading",     "VK_LAYER_LUNARG_parameter_validation",
			"VK_LAYER_LUNARG_device_limits", "VK_LAYER_LUNARG_object_tracker",
			"VK_LAYER_LUNARG_image",         "VK_LAYER_LUNARG_core_validation",
			"VK_LAYER_LUNARG_swapchain",     "VK_LAYER_GOOGLE_unique_objects"
		};




	static VkBool32 FindInstanceValidationLayers (
		char **instance_validation_layers,
		uint32_t &instance_validation_layer_count)
	{
		VkBool32 validation_found = 0;

		uint32_t instance_layer_count = 0;
		VkResult err = vkEnumerateInstanceLayerProperties(&instance_layer_count, NULL);
		assert(!err);

		char **desired_layers = instance_validation_layers_alt1;
		if (instance_layer_count > 0) 
		{
			VkLayerProperties *instance_layers = (VkLayerProperties*) malloc(sizeof (VkLayerProperties) * instance_layer_count);
			err = vkEnumerateInstanceLayerProperties(&instance_layer_count, instance_layers);
			assert(!err);


			validation_found = CheckLayers(ARRAY_SIZE(instance_validation_layers_alt1), desired_layers, 
				instance_layer_count, instance_layers);

			if (1 && validation_found) 
			{
				instance_validation_layer_count = 1;
				instance_validation_layers[0] = (desired_layers)[0];
			} 
			else // use alternative set of validation layers
			{
				desired_layers = instance_validation_layers_alt2;
				
				validation_found = CheckLayers(ARRAY_SIZE(instance_validation_layers_alt2), desired_layers, 
					instance_layer_count, instance_layers);

				instance_validation_layer_count = ARRAY_SIZE(instance_validation_layers_alt2);
				for (uint32_t i = 0; i < instance_validation_layer_count; i++) 
					instance_validation_layers[i] = (desired_layers)[i];
			}
			free(instance_layers);
		}

		for (uint32_t i = 0; i < instance_validation_layer_count; i++) 
			SystemTools::Log ("Available instance validation layer: %s\n", instance_validation_layers[i]);
		
		if (!validation_found) 
		{
			ERR_EXIT("vkEnumerateInstanceLayerProperties failed to find "
					"required validation layer.\n\n"
					"Please look at the Getting Started guide for additional "
					"information.\n",
					"vkCreateInstance Failure");
		}

		return validation_found;
	}

	static VkBool32 CheckDeviceValidationLayers (
		uint32_t &device_enabled_layer_count, 
		const VkPhysicalDevice physicalDevice, 
		const uint32_t instance_validation_layer_count, const char *const *instance_validation_layers, 
		const bool enforceValidation)
	{
		// Look for validation layers
		VkBool32 validation_found = 0;
		device_enabled_layer_count = 0;
		VkResult err = vkEnumerateDeviceLayerProperties (physicalDevice, &device_enabled_layer_count, NULL);
		assert(!err);

		if (device_enabled_layer_count > 0) 
		{
			VkLayerProperties *device_layers = (VkLayerProperties*) malloc(sizeof(VkLayerProperties) * device_enabled_layer_count);
			err = vkEnumerateDeviceLayerProperties (physicalDevice, &device_enabled_layer_count, device_layers);
			//err = vkEnumerateDeviceLayerProperties (physicalDevice, &device_enabled_layer_count, NULL);
			assert(!err);

			for (uint32_t i = 0; i < device_enabled_layer_count; i++) 
				SystemTools::Log ("Available device validation layer: %s %s\n", device_layers[i].layerName, device_layers[i].description);

			if (enforceValidation) 
			{
				validation_found = CheckLayers (instance_validation_layer_count, instance_validation_layers,
												device_enabled_layer_count, device_layers);
			}

			free(device_layers);
		}
		
		return VK_TRUE; // todo: check below does not work anymore (problem: VK_LAYER_LUNARG_standard_validation vs. seperate layers)
		/*
		if (enforceValidation && !validation_found) 
		{
			ERR_EXIT("vkEnumerateDeviceLayerProperties failed to find "
						"a required validation layer.\n\n"
						"Please look at the Getting Started guide for additional "
						"information.\n",
						"vkCreateDevice Failure");
		}
		return validation_found;
		*/
	}

#endif



	static VkBool32 FindInstanceExtensions (
		VkExtensionProperties* &instance_extensions,  // will contain string data, free it after instance creation!
		uint32_t &enabled_extension_count, 
		char **extension_names, // pointers to instance_extensions strings 	
		const uint32_t maxExtensions, const bool validate, const bool enforceSurface, const bool useAll = false)
	{
		uint32_t instance_extension_count = 0;
		enabled_extension_count = 0;
		VkBool32 surfaceExtFound = 0;
		VkBool32 platformSurfaceExtFound = 0;
		memset(extension_names, 0, sizeof(extension_names));

		VkResult err = vkEnumerateInstanceExtensionProperties(NULL, &instance_extension_count, NULL);
		assert(!err);

		if (instance_extension_count > 0) 
		{
			/*VkExtensionProperties **/instance_extensions = (VkExtensionProperties*)
				malloc(sizeof(VkExtensionProperties) * instance_extension_count);
			err = vkEnumerateInstanceExtensionProperties(NULL, &instance_extension_count, instance_extensions);
			assert(!err);

			for (uint32_t i = 0; i < instance_extension_count; i++) 
			{

				if (!strcmp(VK_KHR_SURFACE_EXTENSION_NAME, instance_extensions[i].extensionName)) 
				{
					surfaceExtFound = 1;
					extension_names[enabled_extension_count++] = VK_KHR_SURFACE_EXTENSION_NAME;
				}
				else if (!strcmp(VK_KHR_WIN32_SURFACE_EXTENSION_NAME, instance_extensions[i].extensionName)) 
				{
					platformSurfaceExtFound = 1;
					extension_names[enabled_extension_count++] = VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
				}
				else if (!strcmp(VK_EXT_DEBUG_REPORT_EXTENSION_NAME, instance_extensions[i].extensionName)) 
				{
					if (validate) 
					{
						extension_names[enabled_extension_count++] = instance_extensions[i].extensionName;
					}
				}
				else if (useAll) 
				{
					extension_names[enabled_extension_count++] = instance_extensions[i].extensionName;
				}
				else
				{
					SystemTools::Log ("Ignored instance extension: %s\n", instance_extensions[i].extensionName);
				}
				assert(enabled_extension_count < maxExtensions);
			}
			for (uint32_t i = 0; i < enabled_extension_count; i++)
				SystemTools::Log ("Instance extension: %s\n", extension_names[i]);
			//free(instance_extensions);
		}

		if (enforceSurface)
		{
			if (!surfaceExtFound) 
			{
				ERR_EXIT("vkEnumerateInstanceExtensionProperties failed to find "
							"the " VK_KHR_SURFACE_EXTENSION_NAME
							" extension.\n\nDo you have a compatible "
							"Vulkan installable client driver (ICD) installed?\nPlease "
							"look at the Getting Started guide for additional "
							"information.\n",
							"vkCreateInstance Failure");
			}

			if (!platformSurfaceExtFound) 
			{
				ERR_EXIT("vkEnumerateInstanceExtensionProperties failed to find "
							"the " VK_KHR_WIN32_SURFACE_EXTENSION_NAME
							" extension.\n\nDo you have a compatible "
							"Vulkan installable client driver (ICD) installed?\nPlease "
							"look at the Getting Started guide for additional "
							"information.\n",
							"vkCreateInstance Failure");
			}
		}

		return surfaceExtFound && platformSurfaceExtFound;
	}


	static VkBool32 LoadDeviceExtensions (
		VkExtensionProperties* &device_extensions,  // will contain string data, free it after device creation!
		uint32_t &enabled_device_extension_count, 
		char **device_extension_names, // pointers to device_extensions strings 	
		const VkPhysicalDevice physicalDevice, const uint32_t maxExtensions, const bool enforceSwapchain, const bool loadAllExtensions = true)
	{
		//VkExtensionProperties *device_extensions = NULL;

		uint32_t device_extension_count = 0;
		VkBool32 swapchainExtFound = 0;
		enabled_device_extension_count = 0;
		//memset(device_extension_names, 0, sizeof(extension_names));

		VkResult err = vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &device_extension_count, NULL);
		assert(!err);

		if (device_extension_count > 0) 
		{
			device_extensions = (VkExtensionProperties*)
				malloc(sizeof(VkExtensionProperties) * device_extension_count);
			err = vkEnumerateDeviceExtensionProperties(
				physicalDevice, NULL, &device_extension_count, device_extensions);
			assert(!err);

			for (uint32_t i = 0; i < device_extension_count; i++) 
			{
				if (!strcmp(VK_KHR_SWAPCHAIN_EXTENSION_NAME, device_extensions[i].extensionName)) 
				{
					swapchainExtFound = 1;
					device_extension_names[enabled_device_extension_count++] = device_extensions[i].extensionName;
				}
				else if (loadAllExtensions)
				{
					device_extension_names[enabled_device_extension_count++] = device_extensions[i].extensionName;
				}

				assert(enabled_device_extension_count < maxExtensions);
			}
			for (uint32_t i = 0; i < enabled_device_extension_count; i++)
				SystemTools::Log ("Device extension: %s\n", device_extension_names[i]);
		}

		if (enforceSwapchain && !swapchainExtFound) 
		{
			ERR_EXIT("vkEnumerateDeviceExtensionProperties failed to find "
						"the " VK_KHR_SWAPCHAIN_EXTENSION_NAME
						" extension.\n\nDo you have a compatible "
						"Vulkan installable client driver (ICD) installed?\nPlease "
						"look at the Getting Started guide for additional "
						"information.\n",
						"vkCreateInstance Failure");
		}

		return swapchainExtFound;
	}





	static VkInstance CreateInstance (
		char* name, 
		const uint32_t enabled_layer_count,
		const char *const *instance_validation_layers,
		const uint32_t enabled_extension_count,
		const char *const *extension_names,
		const bool validate, const bool use_break)
	{
		VkApplicationInfo app = {};
		app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		app.pNext = NULL,
		app.pApplicationName = name,
		app.applicationVersion = 0,
		app.pEngineName = name,
		app.engineVersion = 0,
		app.apiVersion = VK_API_VERSION_1_0;
    
		VkInstanceCreateInfo inst_info = {};
		inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		inst_info.pNext = NULL,
		inst_info.pApplicationInfo = &app,
		inst_info.enabledLayerCount = enabled_layer_count,
		inst_info.ppEnabledLayerNames = instance_validation_layers,
		inst_info.enabledExtensionCount = enabled_extension_count,
		inst_info.ppEnabledExtensionNames = (const char *const *)extension_names;
    

		//
		//This is info for a temp callback to use during CreateInstance.
		//After the instance is created, we use the instance-based
		//function to register the final callback.
		//
		VkDebugReportCallbackCreateInfoEXT dbgCreateInfo;
		if (validate) 
		{
			dbgCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
			dbgCreateInfo.pNext = NULL;
			dbgCreateInfo.pfnCallback = use_break ? BreakCallback : dbgFunc;
			dbgCreateInfo.pUserData = NULL;
			dbgCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
			inst_info.pNext = &dbgCreateInfo;
		}

		VkInstance inst = NULL;

		VkResult err = vkCreateInstance(&inst_info, NULL, &inst);
		if (err == VK_ERROR_INCOMPATIBLE_DRIVER) 
		{
			ERR_EXIT("Cannot find a compatible Vulkan installable client driver "
						"(ICD).\n\nPlease look at the Getting Started guide for "
						"additional information.\n",
						"vkCreateInstance Failure");
		} 
		else if (err == VK_ERROR_EXTENSION_NOT_PRESENT) 
		{
			ERR_EXIT("Cannot find a specified extension library"
						".\nMake sure your layers path is set appropriately.\n",
						"vkCreateInstance Failure");
		} 
		else if (err) 
		{
			ERR_EXIT("vkCreateInstance failed.\n\nDo you have a compatible Vulkan "
						"installable client driver (ICD) installed?\nPlease look at "
						"the Getting Started guide for additional information.\n",
						"vkCreateInstance Failure");
		}

		return inst;
	}


	static VkDevice CreateDevice (
		VkQueueFamilyProperties* &queueProps,
		uint32_t &queueFamilyCount,
		const VkPhysicalDevice physicalDevice,
		const uint32_t enabled_layer_count,
		const char *const *device_validation_layers,
		const uint32_t enabled_extension_count,
		const char *const *extension_names) 
	{
		//uint32_t queueFamilyCount;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, NULL);
		assert(queueFamilyCount >= 1);

		queueProps = (VkQueueFamilyProperties *)malloc(queueFamilyCount * sizeof(VkQueueFamilyProperties));
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueProps);
		
		VkDeviceQueueCreateInfo *queueInfos = (VkDeviceQueueCreateInfo *)malloc(queueFamilyCount * sizeof(VkDeviceQueueCreateInfo));
		memset(queueInfos, 0, queueFamilyCount * sizeof(VkDeviceQueueCreateInfo));

		float queue_priorities[32] = {1,1,1,1, 1,1,1,1,  1,1,1,1, 1,1,1,1,  1,1,1,1, 1,1,1,1,  1,1,1,1, 1,1,1,1}; // todo: how to give this by function param?
		int totalQueueCount = 0;

		for (uint32_t queueFamily = 0; queueFamily < queueFamilyCount; queueFamily++) 
		{
			bool hasGraphics = (queueProps[queueFamily].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
			bool hasCompute =  (queueProps[queueFamily].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0;
			bool hasTransfer = (queueProps[queueFamily].queueFlags & VK_QUEUE_TRANSFER_BIT) != 0;
			bool hasSparse =   (queueProps[queueFamily].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) != 0;

			if (hasGraphics) queue_priorities[0] = 0.0f; // AMD: try to make graphics queue less dominant so other queues can kick in more likely, but has no effect

			queueInfos[queueFamily].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			queueInfos[queueFamily].pNext = NULL,
			queueInfos[queueFamily].queueFamilyIndex = queueFamily,
			queueInfos[queueFamily].queueCount = queueProps[queueFamily].queueCount,
			queueInfos[queueFamily].pQueuePriorities = &queue_priorities[totalQueueCount];

			totalQueueCount += queueProps[queueFamily].queueCount;
			assert (totalQueueCount <= 32);

				
			SystemTools::Log ("Queue family %i (%i queues): graphics: %i compute: %i transfer: %i sparse: %i | ", 
				queueFamily, queueProps[queueFamily].queueCount,
				hasGraphics, hasCompute, hasTransfer, hasSparse);

			SystemTools::Log ("Priorities: ");
			for (uint32_t i=0; i<queueProps[queueFamily].queueCount; i++) SystemTools::Log ("%f ", queue_priorities[totalQueueCount - queueProps[queueFamily].queueCount + i]);
			SystemTools::Log ("\n");

		}

		VkPhysicalDeviceFeatures features = {};

		features.fillModeNonSolid = true;
		//features.shaderInt64 = true; // freezes NV !
		//features.shaderClipDistance = true; 
		//features.shaderCullDistance = true; 

		VkDeviceCreateInfo deviceInfo = {};
			deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			deviceInfo.pNext = NULL,
			deviceInfo.queueCreateInfoCount = queueFamilyCount,
			deviceInfo.pQueueCreateInfos = queueInfos,
			deviceInfo.enabledLayerCount = enabled_layer_count,
			deviceInfo.ppEnabledLayerNames = device_validation_layers,
			deviceInfo.enabledExtensionCount = enabled_extension_count,
			deviceInfo.ppEnabledExtensionNames = extension_names,
			deviceInfo.pEnabledFeatures = &features; // If specific features are required, pass them in here    

		VkDevice device;
		VkResult U_ASSERT_ONLY err = vkCreateDevice (physicalDevice, &deviceInfo, NULL, &device);
		assert(!err);

		free(queueInfos);

		return device;
	}













	static uint32_t MemoryTypeFromProperties ( const uint32_t typeBits, const VkFlags propertiesMask, const VkPhysicalDeviceMemoryProperties memoryProperties )
	{
		// Search memtypes to find first index with those properties
		for (uint32_t typeIndex = 0; typeIndex < /*VK_MAX_MEMORY_TYPES*/memoryProperties.memoryTypeCount; typeIndex++) 
		{
			if ((typeBits>>typeIndex) & 1)
			{
				// Type is available, does it match user properties?
				if ((memoryProperties.memoryTypes[typeIndex].propertyFlags & propertiesMask) == propertiesMask)
				{
					return typeIndex;
				}
			}
		}
		// No memory types matched, return failure
		assert (0);
		return UINT32_MAX;
	}




	namespace CommandBuffer
	{
		inline void Create (
			VkCommandBuffer *cmdBuffers,
			const VkDevice device, 
			const VkCommandPool commandPool, 
			const VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY, 
			const uint32_t bufferCount = 1 )
		{
			VkCommandBufferAllocateInfo cmdBufAllocateInfo = {};
				cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				cmdBufAllocateInfo.pNext = NULL,
				cmdBufAllocateInfo.commandPool = commandPool;
				cmdBufAllocateInfo.level = level;
				cmdBufAllocateInfo.commandBufferCount = bufferCount;

			VkResult U_ASSERT_ONLY err = vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, cmdBuffers);
			assert(!err);
		}

		inline void Begin ( VkCommandBuffer cmdBuffer, VkCommandBufferUsageFlags flags = 0)
		{
			VkCommandBufferBeginInfo cmd_buf_info = {};
				cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
				cmd_buf_info.pNext = NULL,
				cmd_buf_info.flags = flags,
				cmd_buf_info.pInheritanceInfo = NULL;//&cmd_buf_hinfo;
        
			VkResult U_ASSERT_ONLY err = vkBeginCommandBuffer(cmdBuffer, &cmd_buf_info);
			assert(!err);
		}


		inline void End ( VkCommandBuffer cmdBuffer)
		{
			VkResult U_ASSERT_ONLY err = vkEndCommandBuffer(cmdBuffer);
			assert(!err);
		}

		inline void Submit ( VkCommandBuffer *cmdBuffers, VkQueue queue, const uint32_t bufferCount = 1 )
		{
			VkSubmitInfo submitInfo = {};
				submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				submitInfo.commandBufferCount = bufferCount;
				submitInfo.pCommandBuffers = cmdBuffers;
			
			VkResult U_ASSERT_ONLY err = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
			assert(!err);
		}

		inline void Free ( 
			VkCommandBuffer *cmdBuffers, 
			const VkDevice device, 
			const VkCommandPool commandPool, 
			const uint32_t bufferCount = 1 )
		{
			vkFreeCommandBuffers(device, commandPool, bufferCount, cmdBuffers);
		}
	};

	namespace Buffer
	{
		inline void* Map (const VkDevice device, const VkDeviceMemory deviceMemory, const VkDeviceSize size, const VkDeviceSize offset = 0)
		{
			void *pData;
			VkResult U_ASSERT_ONLY err = vkMapMemory(device, deviceMemory, offset, size, 0, &pData);
			assert(!err);
			return pData;
		}

		inline void Unmap (const VkDevice device, const VkDeviceMemory deviceMemory)
		{
			vkUnmapMemory(device, deviceMemory);
		}

		inline void Update (const void *data, const VkDevice device, const VkDeviceMemory deviceMemory, const VkDeviceSize size, const VkDeviceSize offset = 0)
		{
			void *pData = Map (device, deviceMemory, size, offset);
			memcpy(pData, data, size);
			Unmap (device, deviceMemory);
		}

		inline void Prepare (
			VkBuffer &buffer, VkDeviceMemory &deviceMemory, 
			const void *data, const VkDeviceSize size, 
			const VkDevice device, const VkPhysicalDeviceMemoryProperties deviceMemoryProperties,
			const VkBufferUsageFlags usage, // e.g. VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT or VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
			const VkFlags memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			const VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE) 			 
		{
			VkBufferCreateInfo bufferCreateInfo = {};
				bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				bufferCreateInfo.usage = usage;
				bufferCreateInfo.size = size;
				bufferCreateInfo.sharingMode = sharingMode;
			VkResult U_ASSERT_ONLY err = vkCreateBuffer(device, &bufferCreateInfo, NULL, &buffer);
			assert(!err);

			VkMemoryRequirements memoryRequirements;
			vkGetBufferMemoryRequirements(device, buffer, &memoryRequirements);

			VkMemoryAllocateInfo memoryAllocateInfo;
				memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
				memoryAllocateInfo.pNext = NULL;
				memoryAllocateInfo.allocationSize = memoryRequirements.size;

			memoryAllocateInfo.memoryTypeIndex = MemoryTypeFromProperties(
				memoryRequirements.memoryTypeBits, 
				memoryProperties,
				deviceMemoryProperties);

			err = vkAllocateMemory(device, &memoryAllocateInfo, NULL, &(deviceMemory));
			assert(!err);

			if (data) Update (data, device, deviceMemory, size);

			err = vkBindBufferMemory(device, buffer, deviceMemory, 0);
			assert(!err);
		}

		inline void Destroy (const VkBuffer buffer, const VkDevice device, const VkDeviceMemory deviceMemory)
		{
			vkDestroyBuffer(device, buffer, NULL);
			vkFreeMemory(device, deviceMemory, NULL);
		}

		inline void GetDescriptorBufferInfo (VkDescriptorBufferInfo &descriptorBufferInfo,
			const VkBuffer buffer, const VkDeviceSize size, const VkDeviceSize offset = 0)
		{
			descriptorBufferInfo = {};
			descriptorBufferInfo.buffer = buffer;
			descriptorBufferInfo.offset = offset;
			descriptorBufferInfo.range = size;//memoryAllocateInfo.allocationSize;//?
		}



	};

	namespace Image
	{
		inline VkDeviceSize Prepare ( // todo: allocate multiple images at once
			VkImage &image,
			VkDeviceMemory &deviceMemory,
			const VkDevice device, const VkPhysicalDeviceMemoryProperties deviceMemoryProperties,
			const VkImageCreateInfo &imageCreateInfo,
			const VkFlags memoryPropertiesMask = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
			)
		{
			VkResult U_ASSERT_ONLY err;

			err = vkCreateImage (device, &imageCreateInfo, nullptr, &image);
			assert(!err);

			VkMemoryRequirements memReqs = {};
			vkGetImageMemoryRequirements(device, image, &memReqs);

			VkMemoryAllocateInfo memAllocInfo = {};
			memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			memAllocInfo.pNext = NULL;
			memAllocInfo.allocationSize = memReqs.size;
			memAllocInfo.memoryTypeIndex = MemoryTypeFromProperties (
				memReqs.memoryTypeBits, memoryPropertiesMask, deviceMemoryProperties);

			err = vkAllocateMemory(device, &memAllocInfo, nullptr, &deviceMemory);
			assert(!err);
			err = vkBindImageMemory(device, image, deviceMemory, 0);
			assert(!err);

			return memReqs.size;
		}

		inline void ChangeLayoutBarrier( // by Sascha Willems
									VkCommandBuffer cmdBuffer, 
									VkImage image, 
									VkImageAspectFlags aspectMask, 
									VkImageLayout oldImageLayout, 
									VkImageLayout newImageLayout, 
									VkImageSubresourceRange subresourceRange)
		{
			// Create an image barrier object
			VkImageMemoryBarrier imageMemoryBarrier = {};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.pNext = NULL;
			imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemoryBarrier.oldLayout = oldImageLayout;
			imageMemoryBarrier.newLayout = newImageLayout;
			imageMemoryBarrier.image = image;
			imageMemoryBarrier.subresourceRange = subresourceRange;

			// Source layouts (old)
			// Source access mask controls actions that have to be finished on the old layout
			// before it will be transitioned to the new layout
			switch (oldImageLayout)
			{
			case VK_IMAGE_LAYOUT_UNDEFINED:
					// Image layout is undefined (or does not matter)
					// Only valid as initial layout
					// No flags required, listed only for completeness
					imageMemoryBarrier.srcAccessMask = 0;
					break;

			case VK_IMAGE_LAYOUT_PREINITIALIZED:
					// Image is preinitialized
					// Only valid as initial layout for linear images, preserves memory contents
					// Make sure host writes have been finished
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
					break;

			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
					// Image is a color attachment
					// Make sure any writes to the color buffer have been finished
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
					break;

			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
					// Image is a depth/stencil attachment
					// Make sure any writes to the depth/stencil buffer have been finished
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
					break;

			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
					// Image is a transfer source 
					// Make sure any reads from the image have been finished
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
					break;

			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
					// Image is a transfer destination
					// Make sure any writes to the image have been finished
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					break;

			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
					// Image is read by a shader
					// Make sure any shader reads from the image have been finished
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
					break;
			}

			// Target layouts (new)
			// Destination access mask controls the dependency for the new image layout
			switch (newImageLayout)
			{
			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				// Image will be used as a transfer destination
				// Make sure any writes to the image have been finished
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
				// Image will be used as a transfer source
				// Make sure any reads from and writes to the image have been finished
				imageMemoryBarrier.srcAccessMask = imageMemoryBarrier.srcAccessMask | VK_ACCESS_TRANSFER_READ_BIT;
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				break;

			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
				// Image will be used as a color attachment
				// Make sure any writes to the color buffer have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
				// Image layout will be used as a depth/stencil attachment
				// Make sure any writes to depth/stencil buffer have been finished
				imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				// Image will be read in a shader (sampler, input attachment)
				// Make sure any writes to the image have been finished
				if (imageMemoryBarrier.srcAccessMask == 0)
				{
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
				}
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				break;
			}

			// Put barrier on top
			VkPipelineStageFlags srcStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			VkPipelineStageFlags destStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

//TODO:
srcStageFlags = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
destStageFlags = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

			// Put barrier inside setup command buffer
			vkCmdPipelineBarrier(
				cmdBuffer, 
				srcStageFlags, 
				destStageFlags, 
				0, 
				0, nullptr,
				0, nullptr,
				1, &imageMemoryBarrier);

		}
	};




	struct Gpu
	{
		VkPhysicalDevice physicalDevice; // Physical device (GPU) that Vulkan will use
		VkDevice device; // Logical device, application's view of the physical device (GPU)
		
		VkPhysicalDeviceProperties physicalDeviceProps; // Stores physical device properties (for e.g. checking device limits)
		VkPhysicalDeviceMemoryProperties memoryProperties; // Stores all available memory (type) properties for the physical device
		VkPhysicalDeviceFeatures features; // Stores the features available on the selected physical device (for e.g. checking if a feature is available)
	
		VkQueueFamilyProperties *queueFamilyProps;
		uint32_t queueFamilyCount;

		VkPipelineCache pipelineCache;

		void SearchQueues (uint32_t &foundCount, uint32_t *foundFamilies, VkQueue *foundQeues, 
			const uint32_t desiredCount, 
			const VkQueueFlags desiredFlags, // e.g. VK_QUEUE_TRANSFER_BIT in case we want an independent transfer queue
			const VkQueueFlags avoidFlags) const // e.g. VK_QUEUE_GRAPHICS_BIT in case we want a queue independent of graphics
		{
			// todo: store bits for used queues and avoid them if possible

			foundCount = 0;
			for (int run=0; run<3 && foundCount<desiredCount; run++)
			{
				for (uint32_t family=0; family < queueFamilyCount && foundCount<desiredCount; family++) 
				{
					VkQueueFlags flags = queueFamilyProps[family].queueFlags;

					bool runPassed = false;
				
					if (run == 0) runPassed = ((flags & avoidFlags) == 0) && ((flags & desiredFlags) == desiredFlags);
					if (run == 1) runPassed = (flags & desiredFlags) == desiredFlags;
	
					if (runPassed) 
					{
						uint32_t count = queueFamilyProps[family].queueCount;
						for (uint32_t index = 0; index < count && foundCount<desiredCount; index++)
						{
							foundFamilies[foundCount] = family;
							vkGetDeviceQueue (device, family, index, &foundQeues[foundCount]);
							foundCount++;
						}
					}
				}
			}

			// todo: option to duplicate queues to enforce desiredCount if at least one has been found?
		}

		void Init ()
		{
			physicalDevice = NULL;
			device = NULL;
			queueFamilyCount = 0;
			pipelineCache = NULL;
		}
		
		void Destroy ()
		{
			vkDeviceWaitIdle (device);
			vkDestroyPipelineCache (device, pipelineCache, NULL);
			free (queueFamilyProps);
			vkDestroyDevice (device, NULL);
		}
	};


	struct Context
	{
		enum 
		{
			MAX_GPUS = 8,
		};

		VkInstance inst; // Vulkan instance, stores all per-application states

		struct Gpu gpus[MAX_GPUS];
		uint32_t gpuCount;

		bool validate;
		bool use_break;
		
		PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallback;
		PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallback;
		VkDebugReportCallbackEXT msg_callback;
		PFN_vkDebugReportMessageEXT DebugReportMessage;

		void InitVK (char* APP_SHORT_NAME, bool validate = false, bool use_break = false);
		
		void Destroy ()
		{
			for (uint32_t i=0; i<gpuCount; i++) gpus[i].Destroy();
			if (validate) DestroyDebugReportCallback (inst, msg_callback, NULL);
			vkDestroyInstance (inst, NULL);
		}
	};

	
	struct Swapchain 
	{
		enum
		{
			MAX_FRAMES = 4, // todo: enforce
		};


		struct Frame 
		{
			VkImage image;
			VkImageView view;
			VkCommandBuffer postPresentCommandBuffer; // todo: allocate all cmd buffers as array
			VkCommandBuffer drawCommandBuffer;
			VkCommandBuffer prePresentCommandBuffer;
			VkFramebuffer framebuffer;
		};

		struct 
		{
			VkFormat format;
			VkImage image;
			VkDeviceMemory mem;
			VkImageView view;

		} depth;

		Gpu *gpu;

		VkSwapchainKHR swapchain;
		VkSurfaceKHR surface;

		// queues & families - todo: family to Gpu?
		VkQueue gfxPresentQueue; // graphics AND present queue	// todo: may need to seperate
		uint32_t gfxPresentFamily; // graphics AND present queue family
		
		VkQueue uploadQueue;
		uint32_t uploadFamily;

		VkQueue downloadQueue;
		uint32_t downloadFamily;

		VkCommandPool commandPool; // Command buffer pool to allocate init and swapchain frames command buffers
		VkCommandBuffer initCommandBuffer; // Buffer for initialization commands


		Frame *frames;
		uint32_t frameCount;
		uint32_t currentFrame;

		int width, height;
		VkFormat format;
		VkColorSpaceKHR colorSpace;

		// os
		HINSTANCE connection;        // hInstance - Windows Instance
		HWND window;                 // hWnd - window handle


		PFN_vkGetPhysicalDeviceSurfaceSupportKHR fpGetPhysicalDeviceSurfaceSupportKHR;
		PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR fpGetPhysicalDeviceSurfaceCapabilitiesKHR;
		PFN_vkGetPhysicalDeviceSurfaceFormatsKHR fpGetPhysicalDeviceSurfaceFormatsKHR;
		PFN_vkGetPhysicalDeviceSurfacePresentModesKHR fpGetPhysicalDeviceSurfacePresentModesKHR;
		PFN_vkCreateSwapchainKHR fpCreateSwapchainKHR;
		PFN_vkDestroySwapchainKHR fpDestroySwapchainKHR;
		PFN_vkGetSwapchainImagesKHR fpGetSwapchainImagesKHR;
		PFN_vkAcquireNextImageKHR fpAcquireNextImageKHR;
		PFN_vkQueuePresentKHR fpQueuePresentKHR;



		


		
		void Init (Context &context, HINSTANCE connection, HWND window, int width, int height, int gpuIndex); // Init surface, format, queue
		

		void PrepareDepth ();
		void PrepareSwapchain (); // Vsync, create swapchain & frames command buffers
		void PrepareFramebuffers (VkRenderPass renderPass); // classic forward color and depth buffer - replace for deferred etc.
		void Destroy (/*VkInstance inst*/)
		{
			vkDeviceWaitIdle (gpu->device);

			//fpDestroySwapchainKHR (gpu->device, swapchain, NULL);

			for (uint32_t i = 0; i < frameCount; i++) 
			{
				vkDestroyFramebuffer(gpu->device, frames[i].framebuffer, NULL);
				vkDestroyImageView(gpu->device, frames[i].view, NULL); // todo: sometimes crashes here
				vkFreeCommandBuffers(gpu->device, commandPool, 1, &frames[i].drawCommandBuffer);
				vkFreeCommandBuffers(gpu->device, commandPool, 1, &frames[i].postPresentCommandBuffer);
				vkFreeCommandBuffers(gpu->device, commandPool, 1, &frames[i].prePresentCommandBuffer);
			}
			free(frames);
			frames = nullptr;

			vkDestroyImageView(gpu->device, depth.view, NULL);
			vkDestroyImage(gpu->device, depth.image, NULL);
			vkFreeMemory(gpu->device, depth.mem, NULL);

			vkDestroyCommandPool (gpu->device, commandPool, NULL);
			commandPool = VK_NULL_HANDLE;

			//vkDestroySurfaceKHR (inst, surface, NULL);
			
		}


		void CreateCommandPoolAndInitCommandBuffer ()
		{
			// create command pool

			VkCommandPoolCreateInfo cmd_pool_info = {};
			cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			cmd_pool_info.pNext = NULL,
			cmd_pool_info.queueFamilyIndex = gfxPresentFamily,
			cmd_pool_info.flags = 0;
    
			VkResult U_ASSERT_ONLY err = vkCreateCommandPool(gpu->device, &cmd_pool_info, NULL, &commandPool);
			assert(!err);

			CommandBuffer::Create (&initCommandBuffer, gpu->device, commandPool);
			CommandBuffer::Begin (initCommandBuffer);
		}
		void FlushInitCommandBuffer () 
		{
			if (initCommandBuffer == VK_NULL_HANDLE) return;

			CommandBuffer::End (initCommandBuffer);
			CommandBuffer::Submit (&initCommandBuffer, gfxPresentQueue);

			VkResult U_ASSERT_ONLY err = vkQueueWaitIdle(gfxPresentQueue);
			assert(!err);

			CommandBuffer::Free (&initCommandBuffer, gpu->device, commandPool);
			initCommandBuffer = VK_NULL_HANDLE;
		}
		
		VkResult AcquireNextFrame (VkSemaphore presentCompleteSemaphore)
		{
			return fpAcquireNextImageKHR (gpu->device, swapchain, UINT64_MAX, presentCompleteSemaphore, (VkFence)0, &currentFrame);
			//vkAcquireNextImageKHR: Semaphore must not be currently signaled or in a wait state
		}

		void PrepareFrame()
		{
			//VK_CHECK_RESULT(swapChain.acquireNextImage(semaphores.presentComplete, &currentBuffer));

			// Submit post present image barrier to transform the image back to a color attachment that our render pass can write to
			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.pNext = NULL;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &frames[currentFrame].postPresentCommandBuffer;
			VkResult U_ASSERT_ONLY err = vkQueueSubmit(gfxPresentQueue, 1, &submitInfo, NULL);
			assert (!err);
		}

		void SubmitFrame (VkSemaphore waitSemaphore)
		{/*
			bool submitTextOverlay = enableTextOverlay && textOverlay->visible;

			if (submitTextOverlay)
			{
				// Wait for color attachment output to finish before rendering the text overlay
				VkPipelineStageFlags stageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				submitInfo.pWaitDstStageMask = &stageFlags;

				// Set semaphores
				// Wait for render complete semaphore
				submitInfo.waitSemaphoreCount = 1;
				submitInfo.pWaitSemaphores = &semaphores.renderComplete;
				// Signal ready with text overlay complete semaphpre
				submitInfo.signalSemaphoreCount = 1;
				submitInfo.pSignalSemaphores = &semaphores.textOverlayComplete;

				// Submit current text overlay command buffer
				submitInfo.commandBufferCount = 1;
				submitInfo.pCommandBuffers = &textOverlay->cmdBuffers[currentBuffer];
				VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

				// Reset stage mask
				submitInfo.pWaitDstStageMask = &submitPipelineStages;
				// Reset wait and signal semaphores for rendering next frame
				// Wait for swap chain presentation to finish
				submitInfo.waitSemaphoreCount = 1;
				submitInfo.pWaitSemaphores = &semaphores.presentComplete;
				// Signal ready with offscreen semaphore
				submitInfo.signalSemaphoreCount = 1;
				submitInfo.pSignalSemaphores = &semaphores.renderComplete;
			}*/

			// Submit pre present image barrier to transform the image from color attachment to present(khr) for presenting to the swap chain
			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.pNext = NULL;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &frames[currentFrame].prePresentCommandBuffer;
			VkResult U_ASSERT_ONLY err = vkQueueSubmit(gfxPresentQueue, 1, &submitInfo, NULL);

			
			VkPresentInfoKHR presentInfo = {};
			presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
			presentInfo.pNext = NULL;
			presentInfo.swapchainCount = 1;
			presentInfo.pSwapchains = &swapchain;
			presentInfo.pImageIndices = &currentFrame;
			if (waitSemaphore != VK_NULL_HANDLE)
			{
				presentInfo.pWaitSemaphores = &waitSemaphore;
				presentInfo.waitSemaphoreCount = 1;
			}
			
			err = fpQueuePresentKHR(gfxPresentQueue, &presentInfo);
			assert(!err);

			//err = vkQueueWaitIdle(gfxPresentQueue); // todo: nec?
			//assert(!err);	
		}
	};




	struct BufferObject
	{
		VkBuffer buffer;
		VkDeviceMemory deviceMemory;
		VkDeviceSize size;
			

		void* Map (Gpu &gpu) 
			{ return Buffer::Map (gpu.device, deviceMemory, size); }

		void* MapRange (Gpu &gpu, const VkDeviceSize size, const VkDeviceSize offset)
			{ return Buffer::Map (gpu.device, deviceMemory, size, offset); }

		void Unmap (Gpu &gpu)
			{ Buffer::Unmap (gpu.device, deviceMemory); }

		void Update (Gpu &gpu, const void *data, const VkDeviceSize size, const VkDeviceSize offset = 0)
			{ Buffer::Update (data, gpu.device, deviceMemory, size, offset); }

		void Prepare (Gpu &gpu, const void *data, const VkDeviceSize size,
			const VkBufferUsageFlags usage, // e.g. VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT or VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
			const VkFlags memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			const VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE
			) 			 
		{
			this->size = size;
			Buffer::Prepare (buffer, deviceMemory, data, size, gpu.device, gpu.memoryProperties, usage, memoryProperties, sharingMode);
		}

		void Destroy (Gpu &gpu)
		{ 
			Buffer::Destroy (buffer, gpu.device, deviceMemory); 
			buffer = 0;
			size = 0;
		}

		inline void GetDescriptorBufferInfo (VkDescriptorBufferInfo &descriptorBufferInfo,
			const VkDeviceSize size = 0, const VkDeviceSize offset = 0)
		{
			Buffer::GetDescriptorBufferInfo (descriptorBufferInfo, buffer,
				(size == 0 ? this->size : size), offset);
		}
	};

	struct StagingBufferObject
	{
		VkBuffer deviceBuffer;
		VkDeviceMemory deviceDeviceMemory;

		VkBuffer hostBuffer;
		VkDeviceMemory hostDeviceMemory;
		
		VkDeviceSize size;

			

		void* Map (Gpu &gpu) 
			{ return Buffer::Map (gpu.device, hostDeviceMemory, size); }

		void* MapRange (Gpu &gpu, const VkDeviceSize size, const VkDeviceSize offset)
			{ return Buffer::Map (gpu.device, hostDeviceMemory, size, offset); }

		void Unmap (Gpu &gpu)
			{ Buffer::Unmap (gpu.device, hostDeviceMemory); }

		void Update (Gpu &gpu, const void *data, const VkDeviceSize size, const VkDeviceSize offset = 0)
			{ Buffer::Update (data, gpu.device, hostDeviceMemory, size, offset); }

		void CmdUpload (VkCommandBuffer copyCmd, const VkDeviceSize size = 0, const VkDeviceSize srcOffset = 0, const VkDeviceSize dstOffset = 0)
		{
			VkBufferCopy copyRegion = {};
				copyRegion.size = (size == 0 ? this->size : size);
				copyRegion.srcOffset = srcOffset;
				copyRegion.dstOffset = dstOffset;
			vkCmdCopyBuffer (copyCmd, hostBuffer, deviceBuffer, 1, &copyRegion);
		}

		void CmdDownload (VkCommandBuffer copyCmd, const VkDeviceSize size = 0, const VkDeviceSize srcOffset = 0, const VkDeviceSize dstOffset = 0)
		{
			VkBufferCopy copyRegion = {};
				copyRegion.size = (size == 0 ? this->size : size);
				copyRegion.srcOffset = srcOffset;
				copyRegion.dstOffset = dstOffset;
			vkCmdCopyBuffer (copyCmd, deviceBuffer, hostBuffer, 1, &copyRegion);
		}

		void Prepare (Gpu &gpu, const void *data, const VkDeviceSize size,

			const VkBufferUsageFlags deviceUsage	= VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			const VkBufferUsageFlags hostUsage		= VK_BUFFER_USAGE_TRANSFER_SRC_BIT,

			const VkFlags deviceMemoryProperties	= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			const VkFlags hostMemoryProperties		= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,

			const VkSharingMode deviceSharingMode	= VK_SHARING_MODE_EXCLUSIVE,
			const VkSharingMode hostSharingMode		= VK_SHARING_MODE_EXCLUSIVE
			) 			 
		{
			this->size = size;
			Buffer::Prepare (deviceBuffer, deviceDeviceMemory, 0, size, gpu.device, gpu.memoryProperties, deviceUsage, deviceMemoryProperties, deviceSharingMode);
			Buffer::Prepare (hostBuffer, hostDeviceMemory, data, size, gpu.device, gpu.memoryProperties, hostUsage, hostMemoryProperties, hostSharingMode);

		}

		void DestroyOnHost (Gpu &gpu)
		{ 
			Buffer::Destroy (hostBuffer, gpu.device, hostDeviceMemory); 
			hostBuffer = NULL;
		}

		void DestroyOnDevice (Gpu &gpu)
		{ 
			Buffer::Destroy (deviceBuffer, gpu.device, deviceDeviceMemory); 
			deviceBuffer = NULL;
		}

		void Destroy (Gpu &gpu)
		{ 
			if (hostBuffer) DestroyOnHost (gpu);
			if (deviceBuffer) DestroyOnDevice (gpu);
		}

		inline void GetDescriptorBufferInfo (VkDescriptorBufferInfo &descriptorBufferInfo,
			const VkDeviceSize size = 0, const VkDeviceSize offset = 0)
		{
			Buffer::GetDescriptorBufferInfo (descriptorBufferInfo, deviceBuffer,
				(size == 0 ? this->size : size), offset);
		}
	};

	


	struct TextureObject // from Sascha Willems
	{
		VkSampler sampler;
		VkImage image;
		VkImageLayout imageLayout;
		VkDeviceMemory deviceMemory;
		VkImageView view;
		//VkMemoryAllocateInfo memAllocInfo;

		uint32_t width, height;
		uint32_t mipLevels;

		void Destroy (VkDevice device)
		{
			vkDestroyImageView(device, view, NULL);
			vkDestroyImage(device, image, NULL);
			vkFreeMemory(device, deviceMemory, NULL);
			vkDestroySampler(device, sampler, NULL);
		}
	
		static void LoadTexture (TextureObject &texture, 
			Gpu &gpu, VkCommandPool commandPool, VkQueue queue, 
			uint8_t* tex_data, uint32_t tex_width, uint32_t tex_height, 
			VkFormat format, bool forceLinearTiling)
		{
			VkResult U_ASSERT_ONLY err;

			VkFormatProperties formatProperties;

			texture.width = tex_width;
			texture.height = tex_height;
			texture.mipLevels = 1;
			int size = tex_width * tex_height * 4; // todo: assuming RGBA!

			// Get device properites for the requested texture format
			vkGetPhysicalDeviceFormatProperties(gpu.physicalDevice, format, &formatProperties);

			// Only use linear tiling if requested (and supported by the device)
			// Support for linear tiling is mostly limited, so prefer to use
			// optimal tiling instead
			// On most implementations linear tiling will only support a very
			// limited amount of formats and features (mip maps, cubemaps, arrays, etc.)
			VkBool32 useStaging = true;

			// Only use linear tiling if forced
			if (forceLinearTiling)
			{
				// Don't use linear if format is not supported for (linear) shader sampling
				useStaging = !(formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);
			}



			if (useStaging)//0)//
			{

				// Setup buffer copy regions for each mip level
				/*std::vector<VkBufferImageCopy> bufferCopyRegions;
				uint32_t offset = 0;

				for (uint32_t i = 0; i < texture.mipLevels; i++)
				{
					VkBufferImageCopy bufferCopyRegion = {};
					bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					bufferCopyRegion.imageSubresource.mipLevel = i;
					bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
					bufferCopyRegion.imageSubresource.layerCount = 1;
					bufferCopyRegion.imageExtent.width = tex2D[i].dimensions().x;
					bufferCopyRegion.imageExtent.height = tex2D[i].dimensions().y;
					bufferCopyRegion.imageExtent.depth = 1;
					bufferCopyRegion.bufferOffset = offset;

					bufferCopyRegions.push_back(bufferCopyRegion);

					offset += tex2D[i].size();
				}*/

				VkBufferImageCopy bufferCopyRegion = {}; // no mips for now
				{
					bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					bufferCopyRegion.imageSubresource.mipLevel = 0;
					bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
					bufferCopyRegion.imageSubresource.layerCount = 1;
					bufferCopyRegion.imageExtent.width = tex_width;
					bufferCopyRegion.imageExtent.height = tex_height;
					bufferCopyRegion.imageExtent.depth = 1;
					bufferCopyRegion.bufferOffset = 0;
				}

				// Create optimal tiled target image
				VkImageCreateInfo imageCreateInfo = {};
				imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
				imageCreateInfo.pNext = NULL;
				imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
				imageCreateInfo.format = format;
				imageCreateInfo.mipLevels = texture.mipLevels;
				imageCreateInfo.arrayLayers = 1;
				imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
				imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
				imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
				imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
				// Set initial layout of the image to undefined
				imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				imageCreateInfo.extent = { texture.width, texture.height, 1 };
				imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
				
				Image::Prepare (texture.image, texture.deviceMemory, gpu.device, gpu.memoryProperties, imageCreateInfo);




				BufferObject stagingBuffer;
				stagingBuffer.Prepare (gpu, tex_data, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

				VkCommandBuffer copyCmd;
				CommandBuffer::Create (&copyCmd, gpu.device, commandPool);
				CommandBuffer::Begin (copyCmd);

				// Image barrier for optimal image
				VkImageSubresourceRange subresourceRange = {};// The sub resource range describes the regions of the image we will be transition				
				subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;// Image only contains color data
				subresourceRange.baseMipLevel = 0;// Start at first mip level
				subresourceRange.levelCount = texture.mipLevels;// We will transition on all mip levels
				subresourceRange.layerCount = 1;// The 2D texture only has one layer

				// Optimal image will be used as destination for the copy, so we must transfer from our
				// initial undefined image layout to the transfer destination layout
				Image::ChangeLayoutBarrier(
					copyCmd,
					texture.image,
					VK_IMAGE_ASPECT_COLOR_BIT,
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					subresourceRange);

				// Copy mip levels from staging buffer
				vkCmdCopyBufferToImage(
					copyCmd,
					stagingBuffer.buffer,
					texture.image,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1,//bufferCopyRegions.size(),
					&bufferCopyRegion);//bufferCopyRegions.data());

				// Change texture image layout to shader read after all mip levels have been copied
				texture.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				Image::ChangeLayoutBarrier(
					copyCmd,
					texture.image,
					VK_IMAGE_ASPECT_COLOR_BIT,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					texture.imageLayout,
					subresourceRange);

				CommandBuffer::End (copyCmd);
				CommandBuffer::Submit (&copyCmd, queue);
				err = vkQueueWaitIdle (queue);
				assert(!err);
				CommandBuffer::Free (&copyCmd, gpu.device, commandPool);
				stagingBuffer.Destroy(gpu);
			}
			else
			{
				VkMemoryAllocateInfo memAllocInfo = {};
					memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
					memAllocInfo.pNext = NULL;
					memAllocInfo.allocationSize = 0;
					memAllocInfo.memoryTypeIndex = 0;
				//VkMemoryRequirements memReqs = {};


				// Prefer using optimal tiling, as linear tiling 
				// may support only a small set of features 
				// depending on implementation (e.g. no mip maps, only one layer, etc.)

				//VkImage mappableImage;
				//VkDeviceMemory mappableMemory;

				// Load mip map level 0 to linear tiling image
				VkImageCreateInfo imageCreateInfo = {};
					imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
					imageCreateInfo.pNext = NULL;
					imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
					imageCreateInfo.format = format;
					imageCreateInfo.mipLevels = 1;
					imageCreateInfo.arrayLayers = 1;
					imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
					imageCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
					imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
					imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
					imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
					imageCreateInfo.extent = { texture.width, texture.height, 1 };
				
				VkDeviceSize deviceSize = Image::Prepare (texture.image, texture.deviceMemory, gpu.device, gpu.memoryProperties, imageCreateInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

				// Get sub resource layout
				// Mip map count, array layer, etc.
				VkImageSubresource subRes = {};
				subRes.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

				VkSubresourceLayout subResLayout;
				uint8_t *data;

				// Get sub resources layout 
				// Includes row pitch, size offsets, etc.
				vkGetImageSubresourceLayout(gpu.device, texture.image, &subRes, &subResLayout);

				// Map image memory
				err = vkMapMemory(gpu.device, texture.deviceMemory, 0, deviceSize, 0, (void**) &data);

				// Copy image data into memory
				memcpy(data, tex_data, size);

				vkUnmapMemory(gpu.device, texture.deviceMemory);

				// Linear tiled images don't need to be staged
				// and can be directly used as textures
				//texture.image = mappableImage;
				//texture.deviceMemory = mappableMemory;
				texture.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			
//VkCommandBuffer copyCmd = VulkanExampleBase::createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
				VkCommandBuffer copyCmd;
				CommandBuffer::Create (&copyCmd, gpu.device, commandPool);
				CommandBuffer::Begin (copyCmd);

				// Setup image memory barrier transfer image to shader read layout

				// The sub resource range describes the regions of the image we will be transition
				VkImageSubresourceRange subresourceRange = {};
				// Image only contains color data
				subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				// Start at first mip level
				subresourceRange.baseMipLevel = 0;
				// Only one mip level, most implementations won't support more for linear tiled images
				subresourceRange.levelCount = 1;
				// The 2D texture only has one layer
				subresourceRange.layerCount = 1;

				Image::ChangeLayoutBarrier(
					copyCmd, 
					texture.image,
					VK_IMAGE_ASPECT_COLOR_BIT, 
					VK_IMAGE_LAYOUT_PREINITIALIZED,
					texture.imageLayout,
					subresourceRange);

//VulkanExampleBase::flushCommandBuffer(copyCmd, queue, true);
				CommandBuffer::End (copyCmd);
				CommandBuffer::Submit (&copyCmd, queue);
				err = vkQueueWaitIdle (queue);
				assert(!err);
				CommandBuffer::Free (&copyCmd, gpu.device, commandPool);
			}
/*
			// Create sampler
			// In Vulkan textures are accessed by samplers
			// This separates all the sampling information from the 
			// texture data
			// This means you could have multiple sampler objects
			// for the same texture with different settings
			// Similar to the samplers available with OpenGL 3.3
			VkSamplerCreateInfo sampler = vkTools::initializers::samplerCreateInfo();
			sampler.magFilter = VK_FILTER_LINEAR;
			sampler.minFilter = VK_FILTER_LINEAR;
			sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			sampler.mipLodBias = 0.0f;
			sampler.compareOp = VK_COMPARE_OP_NEVER;
			sampler.minLod = 0.0f;
			// Max level-of-detail should match mip level count
			sampler.maxLod = (useStaging) ? (float)texture.mipLevels : 0.0f;
			// Enable anisotropic filtering
			sampler.maxAnisotropy = 8;
			sampler.anisotropyEnable = VK_TRUE;
			sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
			VK_CHECK_RESULT(vkCreateSampler(device, &sampler, nullptr, &texture.sampler));

			// Create image view
			// Textures are not directly accessed by the shaders and
			// are abstracted by image views containing additional
			// information and sub resource ranges
			VkImageViewCreateInfo view = vkTools::initializers::imageViewCreateInfo();
			view.image = VK_NULL_HANDLE;
			view.viewType = VK_IMAGE_VIEW_TYPE_2D;
			view.format = format;
			view.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
			view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			view.subresourceRange.baseMipLevel = 0;
			view.subresourceRange.baseArrayLayer = 0;
			view.subresourceRange.layerCount = 1;
			// Linear tiling usually won't support mip maps
			// Only set mip map count if optimal tiling is used
			view.subresourceRange.levelCount = (useStaging) ? texture.mipLevels : 1;
			view.image = texture.image;
			VK_CHECK_RESULT(vkCreateImageView(device, &view, nullptr, &texture.view));
*/
		}
		
		static void CreateSamplerAndView (TextureObject &texture, 
			Gpu &gpu, VkFormat format)
		{
			

			// Create sampler
			// In Vulkan textures are accessed by samplers
			// This separates all the sampling information from the 
			// texture data
			// This means you could have multiple sampler objects
			// for the same texture with different settings
			// Similar to the samplers available with OpenGL 3.3
			VkSamplerCreateInfo sampler = {};
				sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
				sampler.pNext = NULL,
				//sampler.magFilter = VK_FILTER_NEAREST,
				//sampler.minFilter = VK_FILTER_NEAREST,
				//sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
			sampler.magFilter = VK_FILTER_LINEAR;
			sampler.minFilter = VK_FILTER_LINEAR;
			sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
				sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
				sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
				sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
				sampler.mipLodBias = 0.0f,
				sampler.anisotropyEnable = VK_FALSE,
				sampler.maxAnisotropy = 1,
				sampler.compareOp = VK_COMPARE_OP_NEVER,
				sampler.minLod = 0.0f,
				//sampler.maxLod = 0.0f,
			sampler.maxLod = (float)texture.mipLevels;
				sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
				sampler.unnormalizedCoordinates = VK_FALSE;

			
			VkResult U_ASSERT_ONLY err = vkCreateSampler(gpu.device, &sampler, nullptr, &texture.sampler);
			assert(!err);

			// Create image view
			// Textures are not directly accessed by the shaders and
			// are abstracted by image views containing additional
			// information and sub resource ranges
			VkImageViewCreateInfo view = {};
				view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				view.pNext = NULL,
				view.viewType = VK_IMAGE_VIEW_TYPE_2D,
				view.format = format,
				view.components =
						{
						 VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G,
						 VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A,
						},
				view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				view.subresourceRange.baseMipLevel = 0;
				view.subresourceRange.baseArrayLayer = 0;
				view.subresourceRange.layerCount = 1;
				view.flags = 0;
				// Linear tiling usually won't support mip maps
				// Only set mip map count if optimal tiling is used
				view.subresourceRange.levelCount = texture.mipLevels;
				view.image = texture.image;
			
			err = vkCreateImageView(gpu.device, &view, nullptr, &texture.view);

		}

	};

	



	static VkShaderModule LoadSPV (VkDevice device, const char *filename) 
	{
		long int size;
		size_t U_ASSERT_ONLY retval;
		void *shader_code;

		FILE *fp = fopen(filename, "rb");	
		assert (fp);

		if (!fp) 
		{
			SystemTools::Log ("Error: VK_Helper::LoadSPV(): Can't load file %s \n", filename);
			return NULL;
		}

		fseek(fp, 0L, SEEK_END);
		size = ftell(fp);

		fseek(fp, 0L, SEEK_SET);

		shader_code = malloc(size);
		retval = fread(shader_code, size, 1, fp);
		assert(retval == 1);

		//*psize = size;

		fclose(fp);
		//return (char *) shader_code;

		VkResult U_ASSERT_ONLY err;

		VkShaderModuleCreateInfo moduleCreateInfo = {};
		moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		moduleCreateInfo.pNext = NULL;
		moduleCreateInfo.codeSize = size;
		moduleCreateInfo.pCode = (const uint32_t*) shader_code;
		moduleCreateInfo.flags = 0;

		VkShaderModule module;
		err = vkCreateShaderModule (device, &moduleCreateInfo, NULL, &module);
		assert(!err);

		free(shader_code);
		return module;
	}

}