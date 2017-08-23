#pragma once

#include <imgui.h>
#include "VK_Helper.h"
#include "Application.h"
#include "SystemTools.h"

struct ImGuiImpl
{
	VK_Helper::TextureObject fontTexture; 

	VkDescriptorSetLayout	descriptorSetLayout;
	VkPipelineLayout		pipelineLayout;
	VkDescriptorSet			descriptorSet;
	VkPipeline				pipeline;
	VkPipelineCache			pipelineCache;

	VkCommandPool			commandPool;
	VkCommandBuffer			commandBuffer; 

	VK_Helper::BufferObject vertexBuffer;
	VK_Helper::BufferObject indexBuffer;

	//long prevTime;




	bool CreateFontTexture (VK_Helper::Gpu &gpu, VkCommandPool commandPool, VkQueue queue)
	{
SystemTools::Log("CreateFontTexture\n");
		ImGuiIO& io = ImGui::GetIO();

		uint8_t* pixels;
		int width, height;
		io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
		size_t upload_size = width*height*4*sizeof(char);

		fontTexture.LoadTexture (fontTexture, gpu, commandPool, queue, pixels, width, height, VK_FORMAT_R8G8B8A8_UNORM, false);
		fontTexture.CreateSamplerAndView (fontTexture, gpu, VK_FORMAT_R8G8B8A8_UNORM);

		// Store our identifier
		io.Fonts->TexID = (void *)(intptr_t)fontTexture.image;

		return true;
	}

	bool CreateDeviceObjects (VK_Helper::Swapchain &swapchain, VkDescriptorPool g_DescriptorPool, VkRenderPass renderPass)
	{
		VkDevice device = swapchain.gpu->device;

		VkResult U_ASSERT_ONLY err;
		VkShaderModule vert_module;
		VkShaderModule frag_module;

		VkPipelineCreateFlags  g_PipelineCreateFlags = 0;

		// Create The Shader Modules:
		{
			vert_module = VK_Helper::LoadSPV (device, "..\\shaders\\imgui-vert.spv");
			frag_module = VK_Helper::LoadSPV (device, "..\\shaders\\imgui-frag.spv");
		}

		//(create sampler)

		//if (!descriptorSetLayout)
		{
			VkSampler sampler[1] = {fontTexture.sampler};
			VkDescriptorSetLayoutBinding binding[1] = {};
			binding[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			binding[0].descriptorCount = 1;
			binding[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			binding[0].pImmutableSamplers = sampler;
			VkDescriptorSetLayoutCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			info.bindingCount = 1;
			info.pBindings = binding;
			err = vkCreateDescriptorSetLayout(device, &info, NULL, &descriptorSetLayout);
			assert (!err);
		}

		// Create Descriptor Set:
		{
			VkDescriptorSetAllocateInfo alloc_info = {};
			alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			alloc_info.descriptorPool = g_DescriptorPool;
			alloc_info.descriptorSetCount = 1;
			alloc_info.pSetLayouts = &descriptorSetLayout;
			err = vkAllocateDescriptorSets(device, &alloc_info, &descriptorSet);
			assert (!err);
		}

// Update the Descriptor Set:
{
    VkDescriptorImageInfo desc_image[1] = {};
    desc_image[0].sampler = fontTexture.sampler;
    desc_image[0].imageView = fontTexture.view;
    desc_image[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    VkWriteDescriptorSet write_desc[1] = {};
    write_desc[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_desc[0].dstSet = descriptorSet;
    write_desc[0].descriptorCount = 1;
    write_desc[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write_desc[0].pImageInfo = desc_image;
    vkUpdateDescriptorSets(device, 1, write_desc, 0, NULL);
}

		//if (!pipelineLayout)
		{
			VkPushConstantRange push_constants[1] = {};
			
			push_constants[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			push_constants[0].offset = sizeof(float) * 0;
			push_constants[0].size = sizeof(float) * 4;
			
			VkDescriptorSetLayout set_layout[1] = {descriptorSetLayout};

			VkPipelineLayoutCreateInfo layout_info = {};
			layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			layout_info.setLayoutCount = 1;
			layout_info.pSetLayouts = set_layout;
			layout_info.pushConstantRangeCount = 1;//2;
			layout_info.pPushConstantRanges = push_constants;
			err = vkCreatePipelineLayout(device, &layout_info, NULL, &pipelineLayout);
			assert (!err);
		}

		VkPipelineShaderStageCreateInfo stage[2] = {};
		stage[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		stage[0].module = vert_module;
		stage[0].pName = "main";
		stage[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		stage[1].module = frag_module;
		stage[1].pName = "main";

		VkVertexInputBindingDescription binding_desc[1] = {};
			binding_desc[0].binding = 0;
			binding_desc[0].stride = sizeof(ImDrawVert);
			binding_desc[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;


		VkVertexInputAttributeDescription attribute_desc[3] = {};

			attribute_desc[0].binding = binding_desc[0].binding;
			attribute_desc[0].location = 0;
			attribute_desc[0].format = VK_FORMAT_R32G32_SFLOAT;
			attribute_desc[0].offset = offsetof(ImDrawVert, pos);//(size_t)(&((ImDrawVert*)0)->pos);

			attribute_desc[1].binding = binding_desc[0].binding;
			attribute_desc[1].location = 1;
			attribute_desc[1].format = VK_FORMAT_R32G32_SFLOAT;
			attribute_desc[1].offset = offsetof(ImDrawVert, uv);//(size_t)(&((ImDrawVert*)0)->uv);

			attribute_desc[2].binding = binding_desc[0].binding;
			attribute_desc[2].location = 2;
			attribute_desc[2].format = VK_FORMAT_R8G8B8A8_UNORM;
			attribute_desc[2].offset = offsetof(ImDrawVert, col);//(size_t)(&((ImDrawVert*)0)->col);


		VkPipelineVertexInputStateCreateInfo vertex_info = {};
		vertex_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertex_info.vertexBindingDescriptionCount = 1;
		vertex_info.pVertexBindingDescriptions = binding_desc;
		vertex_info.vertexAttributeDescriptionCount = 3;
		vertex_info.pVertexAttributeDescriptions = attribute_desc;

		VkPipelineInputAssemblyStateCreateInfo ia_info = {};
		ia_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		ia_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;//VK_PRIMITIVE_TOPOLOGY_POINT_LIST;//

		VkPipelineViewportStateCreateInfo viewport_info = {};
		viewport_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewport_info.viewportCount = 1;
		viewport_info.scissorCount = 1;

		VkPipelineRasterizationStateCreateInfo raster_info = {};
		raster_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		raster_info.polygonMode = VK_POLYGON_MODE_FILL;//VK_POLYGON_MODE_LINE;//
		raster_info.cullMode = VK_CULL_MODE_NONE;
		raster_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		raster_info.lineWidth = 1.0f;
//raster_info.rasterizerDiscardEnable = true;

		VkPipelineMultisampleStateCreateInfo ms_info = {};
		ms_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		ms_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineColorBlendAttachmentState color_attachment[1] = {};
		color_attachment[0].blendEnable = VK_TRUE;
		color_attachment[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		color_attachment[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		color_attachment[0].colorBlendOp = VK_BLEND_OP_ADD;
		color_attachment[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		color_attachment[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		color_attachment[0].alphaBlendOp = VK_BLEND_OP_ADD;
		color_attachment[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		//color_attachment[0].colorWriteMask = 0xf;
		//color_attachment[0].blendEnable = VK_FALSE;

		VkPipelineColorBlendStateCreateInfo blend_info = {};
		blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		blend_info.attachmentCount = 1;
		blend_info.pAttachments = color_attachment;

		VkDynamicState dynamic_states[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamic_state = {};
		dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamic_state.dynamicStateCount = 2;
		dynamic_state.pDynamicStates = dynamic_states;

		VkGraphicsPipelineCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		info.flags = g_PipelineCreateFlags;
		info.stageCount = 2;
		info.pStages = stage;
		info.pVertexInputState = &vertex_info;
		info.pInputAssemblyState = &ia_info;
		info.pViewportState = &viewport_info;
		info.pRasterizationState = &raster_info;
		info.pMultisampleState = &ms_info;
		info.pColorBlendState = &blend_info;
		info.pDynamicState = &dynamic_state;
		info.pDepthStencilState = 0;
		info.layout = pipelineLayout;
		info.renderPass = renderPass;

		err = vkCreateGraphicsPipelines(device, pipelineCache, 1, &info, NULL, &pipeline);
		assert (!err);
//Attempt to set lineWidth to 0.000000 but physical device wideLines feature not supported/enabled so lineWidth must be 1.0f!

		vkDestroyShaderModule(device, vert_module, NULL);
		vkDestroyShaderModule(device, frag_module, NULL);





		VkCommandPoolCreateInfo commandPoolCI = {};
			commandPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			commandPoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;//VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;//
			commandPoolCI.queueFamilyIndex = swapchain.gfxPresentFamily;
		err = vkCreateCommandPool (swapchain.gpu->device, &commandPoolCI, NULL, &commandPool);
		assert(!err);

		VK_Helper::CommandBuffer::Create (&commandBuffer, swapchain.gpu->device, commandPool);
		vertexBuffer.Prepare (*swapchain.gpu, 0, 1024*8, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
		indexBuffer.Prepare (*swapchain.gpu, 0, 1024, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);


		return true;
	}

	bool CreateDeviceObjectsDBG (VK_Helper::Swapchain &swapchain, VkDescriptorPool g_DescriptorPool, VkRenderPass renderPass)
	{
		VkDevice device = swapchain.gpu->device;

		VkResult U_ASSERT_ONLY err;
		VkShaderModule vert_module;
		VkShaderModule frag_module;

		VkPipelineCreateFlags  g_PipelineCreateFlags = 0;

		// Create The Shader Modules:
		{
			vert_module = VK_Helper::LoadSPV (device, "..\\shaders\\imgui-vert.spv");
			frag_module = VK_Helper::LoadSPV (device, "..\\shaders\\imgui-frag.spv");
		}

		//(create sampler)

		//if (!descriptorSetLayout)
		{
			VkSampler sampler[1] = {fontTexture.sampler};
			VkDescriptorSetLayoutBinding binding[1] = {};
			binding[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			binding[0].descriptorCount = 1;
			binding[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			binding[0].pImmutableSamplers = sampler;
			VkDescriptorSetLayoutCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			info.bindingCount = 1;
			info.pBindings = binding;
			err = vkCreateDescriptorSetLayout(device, &info, NULL, &descriptorSetLayout);
			assert (!err);
		}

		// Create Descriptor Set:
		{
			VkDescriptorSetAllocateInfo alloc_info = {};
			alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			alloc_info.descriptorPool = g_DescriptorPool;
			alloc_info.descriptorSetCount = 1;
			alloc_info.pSetLayouts = &descriptorSetLayout;
			err = vkAllocateDescriptorSets(device, &alloc_info, &descriptorSet);
			assert (!err);
		}

// Update the Descriptor Set:
{
    VkDescriptorImageInfo desc_image[1] = {};
    desc_image[0].sampler = fontTexture.sampler;
    desc_image[0].imageView = fontTexture.view;
    desc_image[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    VkWriteDescriptorSet write_desc[1] = {};
    write_desc[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_desc[0].dstSet = descriptorSet;
    write_desc[0].descriptorCount = 1;
    write_desc[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write_desc[0].pImageInfo = desc_image;
    vkUpdateDescriptorSets(device, 1, write_desc, 0, NULL);
}

		//if (!pipelineLayout)
		{
			VkPushConstantRange push_constants[2] = {};

			push_constants[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			push_constants[0].offset = sizeof(float) * 0;
			push_constants[0].size = sizeof(float) * 2;

			push_constants[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			push_constants[1].offset = sizeof(float) * 2;
			push_constants[1].size = sizeof(float) * 2;

			VkDescriptorSetLayout set_layout[1] = {descriptorSetLayout};

			VkPipelineLayoutCreateInfo layout_info = {};
			layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			layout_info.setLayoutCount = 1;
			layout_info.pSetLayouts = set_layout;
#ifdef USE_PUSH_CONSTANTS
			layout_info.pushConstantRangeCount = 2;
			layout_info.pPushConstantRanges = push_constants;
#endif
			err = vkCreatePipelineLayout(device, &layout_info, NULL, &pipelineLayout);
			assert (!err);
		}

		VkPipelineShaderStageCreateInfo stage[2] = {};
		stage[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		stage[0].module = vert_module;
		stage[0].pName = "main";
		stage[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		stage[1].module = frag_module;
		stage[1].pName = "main";

		VkVertexInputBindingDescription binding_desc[1] = {};
			binding_desc[0].binding = 0;
			binding_desc[0].stride = sizeof(ImDrawVert);
			binding_desc[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;


		VkVertexInputAttributeDescription attribute_desc[3] = {};

			attribute_desc[0].binding = binding_desc[0].binding;
			attribute_desc[0].location = 0;
			attribute_desc[0].format = VK_FORMAT_R32G32_SFLOAT;
			attribute_desc[0].offset = offsetof(ImDrawVert, pos);//(size_t)(&((ImDrawVert*)0)->pos);

			attribute_desc[1].binding = binding_desc[0].binding;
			attribute_desc[1].location = 1;
			attribute_desc[1].format = VK_FORMAT_R32G32_SFLOAT;
			attribute_desc[1].offset = offsetof(ImDrawVert, uv);//(size_t)(&((ImDrawVert*)0)->uv);

			attribute_desc[2].binding = binding_desc[0].binding;
			attribute_desc[2].location = 2;
			attribute_desc[2].format = VK_FORMAT_R8G8B8A8_UNORM;
			attribute_desc[2].offset = offsetof(ImDrawVert, col);//(size_t)(&((ImDrawVert*)0)->col);


		VkPipelineVertexInputStateCreateInfo vertex_info = {};
		vertex_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertex_info.vertexBindingDescriptionCount = 1;
		vertex_info.pVertexBindingDescriptions = binding_desc;
		vertex_info.vertexAttributeDescriptionCount = 3;
		vertex_info.pVertexAttributeDescriptions = attribute_desc;

		VkPipelineInputAssemblyStateCreateInfo ia_info = {};
		ia_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		ia_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;//VK_PRIMITIVE_TOPOLOGY_POINT_LIST;//

		VkPipelineViewportStateCreateInfo viewport_info = {};
		viewport_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewport_info.viewportCount = 1;
		viewport_info.scissorCount = 1;

		VkPipelineRasterizationStateCreateInfo raster_info = {};
		raster_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		raster_info.polygonMode = VK_POLYGON_MODE_FILL;//VK_POLYGON_MODE_LINE;//
		raster_info.cullMode = VK_CULL_MODE_NONE;
		raster_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		raster_info.lineWidth = 1.0f;
raster_info.rasterizerDiscardEnable = true;

		VkPipelineMultisampleStateCreateInfo ms_info = {};
		ms_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		ms_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineColorBlendAttachmentState color_attachment[1] = {};
		color_attachment[0].blendEnable = VK_TRUE;
		color_attachment[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		color_attachment[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		color_attachment[0].colorBlendOp = VK_BLEND_OP_ADD;
		color_attachment[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		color_attachment[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		color_attachment[0].alphaBlendOp = VK_BLEND_OP_ADD;
		color_attachment[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		//color_attachment[0].colorWriteMask = 0xf;
		//color_attachment[0].blendEnable = VK_FALSE;

		VkPipelineColorBlendStateCreateInfo blend_info = {};
		blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		blend_info.attachmentCount = 1;
		blend_info.pAttachments = color_attachment;

		VkDynamicState dynamic_states[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamic_state = {};
		dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamic_state.dynamicStateCount = 2;
		dynamic_state.pDynamicStates = dynamic_states;

		VkGraphicsPipelineCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		info.flags = g_PipelineCreateFlags;
		info.stageCount = 2;
		info.pStages = stage;
		info.pVertexInputState = &vertex_info;
		info.pInputAssemblyState = &ia_info;
		info.pViewportState = &viewport_info;
		info.pRasterizationState = &raster_info;
		info.pMultisampleState = &ms_info;
		info.pColorBlendState = &blend_info;
		info.pDynamicState = &dynamic_state;
		info.pDepthStencilState = 0;
		info.layout = pipelineLayout;
		info.renderPass = renderPass;
VkGraphicsPipelineCreateInfo info2 = {};
info2.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
info2.flags = g_PipelineCreateFlags;
info2.stageCount = 2;
info2.pStages = stage;
info2.pVertexInputState = &vertex_info;
info2.pInputAssemblyState = &ia_info;
info2.pViewportState = 0;//&viewport_info;
info2.pRasterizationState = &raster_info;
info2.pMultisampleState = 0;//&ms_info;
info2.pColorBlendState = 0;//&blend_info;
info2.pDynamicState = 0;//&dynamic_state;
info2.pDepthStencilState = 0;
info2.layout = pipelineLayout;
info2.renderPass = renderPass;
		err = vkCreateGraphicsPipelines(device, pipelineCache, 1, &info2, NULL, &pipeline);
		assert (!err);
//Attempt to set lineWidth to 0.000000 but physical device wideLines feature not supported/enabled so lineWidth must be 1.0f!

		vkDestroyShaderModule(device, vert_module, NULL);
		vkDestroyShaderModule(device, frag_module, NULL);





		VkCommandPoolCreateInfo commandPoolCI = {};
			commandPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			commandPoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;//VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;//
			commandPoolCI.queueFamilyIndex = swapchain.gfxPresentFamily;
		err = vkCreateCommandPool (swapchain.gpu->device, &commandPoolCI, NULL, &commandPool);
		assert(!err);

		VK_Helper::CommandBuffer::Create (&commandBuffer, swapchain.gpu->device, commandPool);
		vertexBuffer.Prepare (*swapchain.gpu, 0, 1024*8, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
		indexBuffer.Prepare (*swapchain.gpu, 0, 1024, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);


		return true;
	}

	void DestroyDeviceObjects (VK_Helper::Gpu &gpu)
	{
		// todo: why branches?
		if (pipeline) 
			vkDestroyPipeline(gpu.device, pipeline, NULL);
		if (pipelineCache) 
			vkDestroyPipelineCache(gpu.device, pipelineCache, NULL);

		if (pipelineLayout) 
			vkDestroyPipelineLayout(gpu.device, pipelineLayout, NULL);		
		if (descriptorSetLayout) 
			vkDestroyDescriptorSetLayout(gpu.device, descriptorSetLayout, NULL);

		vertexBuffer.Destroy (gpu);
		indexBuffer.Destroy (gpu);

		vkDestroyCommandPool (gpu.device, commandPool, NULL);
	}
	
	void UpdateVertexBuffers(VK_Helper::Gpu &gpu)
	{
		ImDrawData* draw_data = ImGui::GetDrawData ();
		if (!draw_data) return;
		size_t vertex_size = draw_data->TotalVtxCount * sizeof(ImDrawVert);
		size_t index_size = draw_data->TotalIdxCount * sizeof(ImDrawIdx);
		
		if (vertex_size > vertexBuffer.size)
		{
			if (vertexBuffer.buffer) vertexBuffer.Destroy (gpu);
			vertexBuffer.Prepare (gpu, 0, vertex_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
		}

		if (index_size > indexBuffer.size)
		{
			if (indexBuffer.buffer) indexBuffer.Destroy (gpu);
			indexBuffer.Prepare (gpu, 0, index_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
		}
		
		ImDrawVert* vtx_dst = (ImDrawVert*) vertexBuffer.Map (gpu);
		ImDrawIdx* idx_dst = (ImDrawIdx*) indexBuffer.Map (gpu);
			
		for (int n = 0; n < draw_data->CmdListsCount; n++)
		{
			const ImDrawList* cmd_list = draw_data->CmdLists[n];
			memcpy(vtx_dst, &cmd_list->VtxBuffer[0], cmd_list->VtxBuffer.size() * sizeof(ImDrawVert));
			memcpy(idx_dst, &cmd_list->IdxBuffer[0], cmd_list->IdxBuffer.size() * sizeof(ImDrawIdx));
			vtx_dst += cmd_list->VtxBuffer.size();
			idx_dst += cmd_list->IdxBuffer.size();
		}
			
		vertexBuffer.Unmap (gpu);
		indexBuffer.Unmap (gpu);
/*
		VkCommandBuffer copyCmd = bufferData[frameIndex].uploadCmd;
		//VK_Helper::CommandBuffer::Create (&copyCmd, gpu.device, uploadCommandPool);
		VK_Helper::CommandBuffer::Begin (copyCmd);
		vertexBuffer.CmdUpload (copyCmd);
		indexBuffer.CmdUpload (copyCmd);
		VK_Helper::CommandBuffer::End (copyCmd);
		VK_Helper::CommandBuffer::Submit (&copyCmd, copyQueue);
		//vkQueueWaitIdle(copyQueue); // todo: nec?
*/
	}

	void BuildDrawCommandBuffer (const VK_Helper::Swapchain &swapchain, VkRenderPass renderPass)
	{
		VkCommandBuffer cmd_buf = commandBuffer;

		VK_Helper::CommandBuffer::Begin (cmd_buf, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

		VkRenderPassBeginInfo rp_begin = {};
		rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		rp_begin.pNext = NULL;
		rp_begin.renderPass = renderPass;
		rp_begin.framebuffer = swapchain.frames[swapchain.currentFrame].framebuffer;
		rp_begin.renderArea.offset.x = 0;
		rp_begin.renderArea.offset.y = 0;
		rp_begin.renderArea.extent.width = swapchain.width;
		rp_begin.renderArea.extent.height = swapchain.height;
		rp_begin.clearValueCount = 0;
		//rp_begin.pClearValues = clear_values;


		vkCmdBeginRenderPass (cmd_buf, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);

		// Setup viewport:
		{
			VkViewport viewport;
			viewport.x = 0;
			viewport.y = 0;
			viewport.width = (float)swapchain.width;
			viewport.height = (float)swapchain.height;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;
			vkCmdSetViewport(cmd_buf, 0, 1, &viewport);
		}

		// Setup scale and translation:
		{
			float scaleTranslate[4];
			scaleTranslate[0] = 2.0f/swapchain.width;
			scaleTranslate[1] = 2.0f/swapchain.height;
			scaleTranslate[2] = -1.0f;
			scaleTranslate[3] = -1.0f;
			vkCmdPushConstants(cmd_buf, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(float) * 0, sizeof(float) * 4, scaleTranslate);
			//vkCmdPushConstants(cmd_buf, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(float) * 2, sizeof(float) * 2, translate);
		}

		// Render the command lists:
		ImDrawData* draw_data = ImGui::GetDrawData ();
		if (draw_data)
		{
			// Bind Vertex And Index Buffer:
			{
				VkBuffer vertex_buffers[1] = {vertexBuffer.buffer};
				VkDeviceSize vertex_offset[1] = {0};
				vkCmdBindVertexBuffers(cmd_buf, 0, 1, vertex_buffers, vertex_offset);
				vkCmdBindIndexBuffer(cmd_buf, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);
			}

			int vtx_offset = 0;
			int idx_offset = 0;
			for (int n = 0; n < draw_data->CmdListsCount; n++)
			{
				const ImDrawList* cmd_list = draw_data->CmdLists[n];
				for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.size(); cmd_i++)
				{
					const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
					if (pcmd->UserCallback)
					{
						pcmd->UserCallback(cmd_list, pcmd);
					}
					else
					{
						VkRect2D scissor;
						scissor.offset.x = static_cast<int32_t>(pcmd->ClipRect.x);
						scissor.offset.y = static_cast<int32_t>(pcmd->ClipRect.y);
						scissor.extent.width = static_cast<uint32_t>(pcmd->ClipRect.z - pcmd->ClipRect.x);
						scissor.extent.height = static_cast<uint32_t>(pcmd->ClipRect.w - pcmd->ClipRect.y + 1); // TODO: + 1??????
						vkCmdSetScissor(cmd_buf, 0, 1, &scissor);
						vkCmdDrawIndexed(cmd_buf, pcmd->ElemCount, 1, idx_offset, vtx_offset, 0);
					}
					idx_offset += pcmd->ElemCount;
				}
				vtx_offset += cmd_list->VtxBuffer.size();
			}
		}

		vkCmdEndRenderPass (cmd_buf);

		VK_Helper::CommandBuffer::End (cmd_buf);
		
	}

	void Draw (VkSemaphore &finalSemaphore, 
		const VK_Helper::Swapchain &swapchain, const VkSemaphore waitSemaphore, const VkSemaphore signalSemaphore,
		const VkRenderPass renderPass)
	{
		UpdateVertexBuffers(*swapchain.gpu);
		BuildDrawCommandBuffer (swapchain, renderPass);
		
		VkPipelineStageFlags pipe_stage_flags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		VkSubmitInfo submit_info = {};
			submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submit_info.pNext = NULL;
			submit_info.pWaitDstStageMask = &pipe_stage_flags;
			submit_info.waitSemaphoreCount = 1;
			submit_info.pWaitSemaphores = &waitSemaphore;//
			submit_info.signalSemaphoreCount = 1;
			submit_info.pSignalSemaphores = &signalSemaphore;//;		
			submit_info.commandBufferCount = 1;
			submit_info.pCommandBuffers = &commandBuffer;
		
		VkResult U_ASSERT_ONLY err = vkQueueSubmit(swapchain.gfxPresentQueue, 1, &submit_info, VK_NULL_HANDLE);
		assert(!err);
		finalSemaphore = signalSemaphore;
	}

	void NewFrame (Application::Application &app, int width, int height, float deltaTime)
	{
		ImGuiIO& io = ImGui::GetIO();

		int window_w = width, window_h = height;
		int display_w = width, display_h = height;

		io.DisplaySize = ImVec2((float)window_w, (float)window_h);
		io.DisplayFramebufferScale = ImVec2(window_w > 0 ? ((float)display_w / window_w) : 0, window_h > 0 ? ((float)display_h / window_h) : 0);

		// time

		//long time = SystemTools::GetTimeMS();
		//long delta = time - prevTime;
		////io.DeltaTime = ((float)(time - prevTime)) / 1000.0f;
		//io.DeltaTime = ((float)delta) / 1000.0f;
		//prevTime = time;
		io.DeltaTime = deltaTime;

		// keyboard

		while (char c = app.PopChar())
			io.AddInputCharacter((unsigned short)c);

		io.KeyCtrl = app.KeyDown (Application::KEY_CTRL);
		io.KeyShift = app.KeyDown (Application::KEY_SHIFT);
		io.KeyAlt = app.KeyDown (Application::KEY_ALT);
		

		static char keyMap[] = {
			ImGuiKey_Tab,			Application::KEY_TAB,
			ImGuiKey_LeftArrow,		Application::KEY_LEFT,
			ImGuiKey_RightArrow,	Application::KEY_RIGHT,
			ImGuiKey_UpArrow,		Application::KEY_UP,
			ImGuiKey_DownArrow,		Application::KEY_DOWN,
			ImGuiKey_PageUp,		Application::KEY_PGUP,
			ImGuiKey_PageDown,		Application::KEY_PGDOWN,
			ImGuiKey_Home,			Application::KEY_HOME,
			ImGuiKey_End,			Application::KEY_END,
			ImGuiKey_Delete,		Application::KEY_DELETE,
			ImGuiKey_Backspace,		Application::KEY_BACK,
			ImGuiKey_Enter,			Application::KEY_RETURN,
			ImGuiKey_Escape,		Application::KEY_ESCAPE,
			ImGuiKey_A,				'A',
			ImGuiKey_C,				'C',
			ImGuiKey_V,				'V',
			ImGuiKey_X,				'X',
			ImGuiKey_Y,				'Y',
			ImGuiKey_Z,				'Z'
		};
		
		for (int i=0; i<ImGuiKey_COUNT; i++)
		{
			io.KeyMap[keyMap[i*2]] = keyMap[i*2+1];
			io.KeysDown[keyMap[i*2+1]] = app.KeyDown (keyMap[i*2+1]);
		}

		// mouse
		
		io.MousePos = ImVec2((float)app.mousePosX, (float)app.mousePosY);
		
		for (int i=0; i<3; i++)
		{
			io.MouseDown[i] = app.KeyDown (Application::KEY_LBUTTON + i);
			io.MouseReleased[i] = app.KeyReleased (Application::KEY_LBUTTON+1);
		}

		io.MouseWheel = (float) app.MouseWheelDelta();//g_MouseWheel;
		
		
		ImGui::NewFrame();
	}
};