#include "VK_Helper.h"
#include "Application.h"
//#include "VK_SimpleVis.h"
//#include "VK_Vis.h"
#include "ImGuiImpl.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale, glm::perspective
#include <glm/gtx/euler_angles.hpp>
using namespace glm;



#include "../AsyncComputeTest_VK.h"

//#define USE_UB
#define USE_SB

#define USE_GUI
#define RENDER_GUI
//#define USE_VIS
#define USE_ASYNC_TEST

#define DEMO_TEXTURE_COUNT 1
#define DEMO_OBJ_COUNT 1
#define VERTEX_BUFFER_BIND_ID 0

using namespace VK_Helper;


namespace Renderer
{



struct Camera
{
	mat4 world;
	mat4 worldInverse;
	mat4 projection;
	vec3 euler;

	float fov;
	float firstPersonSpeed;
	float firstPersonSensibility;
	float nearClip;
	float farClip;
	float aspect;
	

	void Init ()
	{
		fov = 60.0f;
		firstPersonSpeed = 2.0f;
		firstPersonSensibility = 0.1f;
		aspect = 1.0f;
		nearClip = 0.1f;
		farClip = 1000.0f;
		
		world = mat4();
		worldInverse = mat4(); 
		euler = vec3();
		UpdateProjection ();
	}
	
	void UpdateWorld (const vec3 position, const vec3 orientation)
	{
		vec3 pos = position;
		euler = orientation;
		world  = eulerAngleZ(orientation[2]);
		world *= eulerAngleY(orientation[1]);
		world *= eulerAngleX(orientation[0]);
		world[3] = vec4 (pos, 1.0f);

		worldInverse = glm::inverse (world);
	}

	void MoveFirstPerson (const vec3 translation, vec3 rotation)
	{
		UpdateWorld (vec3(world[3] + world * vec4(translation * firstPersonSpeed, 0.0f)), euler + rotation * firstPersonSensibility * 0.01f);
	}

	void UpdateProjection ()
	{
		float fovY = fov / 180.0f * float(M_PI);
		if (fov > 0.0f)
			projection = perspective (fovY, aspect, nearClip, farClip);
		else
			projection = glm::ortho (fov*aspect, -fov*aspect, fov, -fov, -nearClip, farClip);
		projection[1] *= -1.f;
	}
};

struct FrameTimer 
{
	uint64_t prevTime;
	float avgDeltaTime;
	float realTime;

	void Reset (float expectedFps = 60.0f)
	{
		prevTime = SystemTools::GetTimeNS();
		avgDeltaTime = 1.0f / expectedFps;
	}

	float Update ()
	{
		uint64_t time = SystemTools::GetTimeNS();
		uint64_t delta = time - prevTime;
		float deltaTime = ((float)delta) / 1000000000.0f;
		float weight = min (1.0f, (avgDeltaTime + 0.000001f) / 0.05f); 
		//ImGui::Text("weight %f", weight);
		avgDeltaTime = max (0.0000000001f, avgDeltaTime * (1.0f-weight) + deltaTime * weight);
		prevTime = time;
		return avgDeltaTime;
	}

	float AverageMS () {return avgDeltaTime * 1000.0f;}
	float AverageFPS () {return 1.0f / avgDeltaTime;}
};










struct VertexPosTex
{
    float     posX, posY, posZ, posW;    // Position data
    float     u, v, s, t;                // Texcoord
};

static const float g_vertex_buffer_data[] = 
{
    -1.0f,-1.0f,-1.0f, -1.0f,-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f,-1.0f, -1.0f,-1.0f,-1.0f,  // -X side
    -1.0f,-1.0f,-1.0f,  1.0f, 1.0f,-1.0f,  1.0f,-1.0f,-1.0f, -1.0f,-1.0f,-1.0f, -1.0f, 1.0f,-1.0f,  1.0f, 1.0f,-1.0f,  // -Z side
    -1.0f,-1.0f,-1.0f,  1.0f,-1.0f,-1.0f,  1.0f,-1.0f, 1.0f, -1.0f,-1.0f,-1.0f,  1.0f,-1.0f, 1.0f, -1.0f,-1.0f, 1.0f,  // -Y side
    -1.0f, 1.0f,-1.0f, -1.0f, 1.0f, 1.0f,  1.0f, 1.0f, 1.0f, -1.0f, 1.0f,-1.0f,  1.0f, 1.0f, 1.0f,  1.0f, 1.0f,-1.0f,  // +Y side
     1.0f, 1.0f,-1.0f,  1.0f, 1.0f, 1.0f,  1.0f,-1.0f, 1.0f,  1.0f,-1.0f, 1.0f,  1.0f,-1.0f,-1.0f,  1.0f, 1.0f,-1.0f,  // +X side
    -1.0f, 1.0f, 1.0f, -1.0f,-1.0f, 1.0f,  1.0f, 1.0f, 1.0f, -1.0f,-1.0f, 1.0f,  1.0f,-1.0f, 1.0f,  1.0f, 1.0f, 1.0f,  // +Z side
};

static const float g_uv_buffer_data[] = 
{
    0.0f, 0.0f,  1.0f, 0.0f,  1.0f, 1.0f,  1.0f, 1.0f,  0.0f, 1.0f,  0.0f, 0.0f,  // -X side
    1.0f, 0.0f,  0.0f, 1.0f,  0.0f, 0.0f,  1.0f, 0.0f,  1.0f, 1.0f,  0.0f, 1.0f,  // -Z side
    1.0f, 1.0f,  1.0f, 0.0f,  0.0f, 0.0f,  1.0f, 1.0f,  0.0f, 0.0f,  0.0f, 1.0f,  // -Y side
    1.0f, 1.0f,  0.0f, 1.0f,  0.0f, 0.0f,  1.0f, 1.0f,  0.0f, 0.0f,  1.0f, 0.0f,  // +Y side
    1.0f, 1.0f,  0.0f, 1.0f,  0.0f, 0.0f,  0.0f, 0.0f,  1.0f, 0.0f,  1.0f, 1.0f,  // +X side
	0.0f, 1.0f,  0.0f, 0.0f,  1.0f, 1.0f,  0.0f, 0.0f,  1.0f, 0.0f,  1.0f, 1.0f,  // +Z side
};

struct VertexShaderInfo
{
	VkVertexInputBindingDescription bindingDescription;
	VkVertexInputAttributeDescription attributeDescriptions[2];
	VkPipelineVertexInputStateCreateInfo inputState;
};





struct Demo 
{
	Context context;
	Swapchain swapchain;
	Application::Application *application;

	StagingBufferObject matrix_data; //mat4* obj_matrices;
	StagingBufferObject vertex_data;
	StagingBufferObject index_data;
	uint32_t drawVertexCount;
	uint32_t drawMatrixCount;

	VertexShaderInfo vertexShaderInfo;//

#ifdef USE_VIS
	//VK_SimpleVis vis;
	VkDescriptorSet desc_set_vis;//
#endif

#ifdef USE_GUI
	ImGuiImpl gui;
#endif

	VkPipelineLayout pipeline_layout;
	VkDescriptorSetLayout desc_layout;
	VkPipelineCache pipelineCache;

	VkRenderPass render_pass;
	VkRenderPass renderPassGui;

	VkPipeline pipeline;

	VkShaderModule vert_shader_module;//
	VkShaderModule frag_shader_module;//









	VkDescriptorPool desc_pool;
	VkDescriptorSet desc_set;//

	TextureObject textures[DEMO_TEXTURE_COUNT];

	VkSemaphore presentCompleteSemaphore;
    VkSemaphore renderCompleteSemaphore;
    VkSemaphore guiCompleteSemaphore;


	Camera camera;
    mat4 model_matrix;

    float spin_angle;

	int framesDone;
	int quitAtFrame;

	bool prepared;
	bool use_staging_buffer;
	bool pause;
	bool quit;

	bool hideGui;

	FrameTimer timer;
	
	Demo ()
	{
		memset(this, 0, sizeof(Demo));
	};

	void UserInput (float deltaTime)
	{
		vec3 move (0,0,0);
		vec3 look (0,0,0);

		if (application->KeyDown('W') || application->KeyDown(Application::KEY_UP))		move += vec3 (0.0f, 0.0f, -1.0f);
		if (application->KeyDown('S') || application->KeyDown(Application::KEY_DOWN))	move += vec3 (0.0f, 0.0f,  1.0f);
		if (application->KeyDown('A') || application->KeyDown(Application::KEY_LEFT))	move += vec3 (-1.0f, 0.0f, 0.0f);
		if (application->KeyDown('D') || application->KeyDown(Application::KEY_RIGHT))	move += vec3 ( 1.0f, 0.0f, 0.0f);
		if (application->KeyDown('E') || application->KeyDown(Application::KEY_PGUP))	move += vec3 (0.0f, -1.0f, 0.0f);
		if (application->KeyDown('C') || application->KeyDown(Application::KEY_PGDOWN)) move += vec3 (0.0f,  1.0f, 0.0f);

		if (application->KeyDown(Application::KEY_RBUTTON) || application->forceMouseCapture) 
		{
			look[1] = -float(application->mousePosX - application->mousePrevX);
			look[0] = -float(application->mousePosY - application->mousePrevY);
		}
		//ImGui::Text("look[0] %f", look[0]);
		if (application->KeyDown(Application::KEY_CTRL)) move *= 0.1f;
		if (application->KeyDown(Application::KEY_SHIFT)) move *= 10.0f;

		
		camera.MoveFirstPerson (move * deltaTime, look);
	}

	void DestroyOnResize () 
	{
SystemTools::Log("DestroyOnResize\n");
		vkDeviceWaitIdle(context.gpus[0].device);

		vkDestroySemaphore(context.gpus[0].device, presentCompleteSemaphore, NULL);
		vkDestroySemaphore(context.gpus[0].device, renderCompleteSemaphore, NULL);
		vkDestroySemaphore(context.gpus[0].device, guiCompleteSemaphore, NULL);

		// In order to properly resize the window, we must re-create the swapchain
		// AND redo the command buffers, etc.
			
		//prepared = false;

		vkDestroyDescriptorPool(swapchain.gpu->device, desc_pool, NULL);

		vkDestroyPipeline(swapchain.gpu->device, pipeline, NULL);
		vkDestroyPipelineCache(swapchain.gpu->device, pipelineCache, NULL);

		vkDestroyRenderPass(swapchain.gpu->device, render_pass, NULL);
		vkDestroyPipelineLayout(swapchain.gpu->device, pipeline_layout, NULL);		
		vkDestroyDescriptorSetLayout(swapchain.gpu->device, desc_layout, NULL);

		uint32_t i;
		for (i = 0; i < DEMO_TEXTURE_COUNT; i++) 
		{
			textures[i].Destroy (swapchain.gpu->device);
		}

		matrix_data.Destroy(context.gpus[0]); //delete [] obj_matrices;
		vertex_data.Destroy(context.gpus[0]);
		index_data.Destroy(context.gpus[0]);
		
#ifdef USE_GUI
		gui.fontTexture.Destroy (swapchain.gpu->device);
#endif
#ifdef RENDER_GUI
		vkDestroyRenderPass(swapchain.gpu->device, renderPassGui, NULL);
		gui.DestroyDeviceObjects (*swapchain.gpu);
#endif

#ifdef USE_VIS
		simpleVis.DestroyOnResize (*swapchain.gpu, swapchain.frameCount);
#endif

#ifdef USE_ASYNC_TEST
		giGPU_VK.Destroy();
#endif
	}










#ifdef USE_ASYNC_TEST
	
	AsyncComputeTest_VK giGPU_VK;

	void InitScene ()
	{
	}

	void VisScene (const uint32_t channel = 0)
	{
	}

	void UpdateScene ()
	{
		giGPU_VK.Update();
	}

#endif





};

// Forward declaration:
static void demo_resize(Application::Application *app, int width, int height);












///////////////////////////
//
// Command Buffers
//
///////////////////////////


static void demo_draw_build_cmd(Demo *demo, VkCommandBuffer cmd_buf, int frameIndex) 
{
SystemTools::Log("demo_draw_build_cmd\n");
	CommandBuffer::Begin (cmd_buf);

	VkClearValue clear_values[2] = {{},{}};
    clear_values[0].color.float32[0] = clear_values[0].color.float32[1] = clear_values[0].color.float32[2] = clear_values[0].color.float32[3] = 0.2f;
    clear_values[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo rp_begin = {};
    rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
    rp_begin.pNext = NULL,
    rp_begin.renderPass = demo->render_pass,
    rp_begin.framebuffer = demo->swapchain.frames[demo->swapchain.currentFrame].framebuffer,
    rp_begin.renderArea.offset.x = 0,
    rp_begin.renderArea.offset.y = 0,
    rp_begin.renderArea.extent.width = demo->swapchain.width,
    rp_begin.renderArea.extent.height = demo->swapchain.height,
    rp_begin.clearValueCount = 2,
    rp_begin.pClearValues = clear_values;
    

    VkViewport viewport;
    memset(&viewport, 0, sizeof(viewport));
    viewport.height = (float)demo->swapchain.height;
    viewport.width = (float)demo->swapchain.width;
    viewport.minDepth = (float)0.0f;
    viewport.maxDepth = (float)1.0f;
    vkCmdSetViewport(cmd_buf, 0, 1, &viewport);

    VkRect2D scissor;
    memset(&scissor, 0, sizeof(scissor));
    scissor.extent.width = demo->swapchain.width;
    scissor.extent.height = demo->swapchain.height;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    vkCmdSetScissor(cmd_buf, 0, 1, &scissor);
    	
	vkCmdBeginRenderPass(cmd_buf, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, demo->pipeline);

	// Bind triangle vertex buffer (contains position and colors)
	VkDeviceSize offsets[1] = { 0 };
	vkCmdBindVertexBuffers(cmd_buf, VERTEX_BUFFER_BIND_ID, 1, &demo->vertex_data.deviceBuffer, offsets);
	// Bind triangle index buffer
	vkCmdBindIndexBuffer(cmd_buf, demo->index_data.deviceBuffer, 0, VK_INDEX_TYPE_UINT32);
	
	
	{
		vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            demo->pipeline_layout, 0, 1, &demo->desc_set, 0,
                            NULL);

		vkCmdDrawIndexed(cmd_buf, demo->drawVertexCount, 1, 0, 0, 1);
	}


	// vis
#ifdef USE_VIS
    vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            demo->pipeline_layout, 0, 1, &demo->desc_set_vis, 0,
                            NULL);
	simpleVis.DrawCommands (cmd_buf, frameIndex);
#endif

    vkCmdEndRenderPass(cmd_buf);

    CommandBuffer::End(cmd_buf);
}




static void demo_draw(Demo *demo) 
{
	float deltaTime = demo->timer.Update();
#ifdef USE_GUI
	demo->gui.NewFrame(*demo->application, demo->swapchain.width, demo->swapchain.height, deltaTime);
#endif

	demo->UserInput(deltaTime);
#ifdef USE_VIS
	(vec4&)simpleVisEye[0] = demo->camera.world * vec4(0,0,1,0);
#endif	









	VkResult U_ASSERT_ONLY err;

    VkFence nullFence = VK_NULL_HANDLE;


    VkSemaphore finalSemaphore = demo->renderCompleteSemaphore;

    // Get the index of the next available swapchain image:
    err = demo->swapchain.AcquireNextFrame (demo->presentCompleteSemaphore);
    assert(!err);

	demo->swapchain.PrepareFrame ();



	// some debug vis

	{
#ifdef USE_ASYNC_TEST
		demo->UpdateScene ();
#endif
	}



    VkPipelineStageFlags pipe_stage_flags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

	VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = NULL;
		submitInfo.pWaitDstStageMask = &pipe_stage_flags;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &demo->presentCompleteSemaphore;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &demo->renderCompleteSemaphore;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &demo->swapchain.frames[demo->swapchain.currentFrame].drawCommandBuffer;// commandBuffers.data();
		

	// Submit to queue
	err = vkQueueSubmit(demo->swapchain.gfxPresentQueue, 1, &submitInfo, nullFence);
	assert(!err);










#ifdef USE_GUI

	if (demo->application->KeyPressed ('1')) 
	{
		demo->hideGui = !demo->hideGui; 
	}
	if (!demo->hideGui)//gui
	{

		//demo->gui.NewFrame(*demo->application, demo->swapchain.width, demo->swapchain.height, deltaTime);
		
		{
            ImGui::Text("ms %f fps %.1f", demo->timer.AverageMS(), demo->timer.AverageFPS());
        }
		
 		static bool show_camera_window = 0;
        if (show_camera_window)
        {
            ImGui::SetNextWindowSize(ImVec2(200,300), ImGuiSetCond_FirstUseEver);
            ImGui::Begin("Camera Options", &show_camera_window);
            ImGui::SliderFloat("Fov", &demo->camera.fov, -200.0f, 160.0f, "%.3f", 3);
            ImGui::SliderFloat("near", &demo->camera.nearClip, 0.0001f, 10.0f, "%.3f", 3);
            ImGui::SliderFloat("far", &demo->camera.farClip, 0.0002f, 10000.0f, "%.3f", 10);
            ImGui::SliderFloat("sens", &demo->camera.firstPersonSensibility, 0.01f, 1.00f, "%.3f", 3);
            ImGui::SliderFloat("speed", &demo->camera.firstPersonSpeed, 0.0f, 100.f, "%.3f", 3);
            ImGui::InputFloat3("pos", &demo->camera.world[3][0]);
            ImGui::InputFloat3("orn", &demo->camera.euler[0]);
            ImGui::End();
        }


        // Rendering
        ImGui::Render();


#ifdef RENDER_GUI
		demo->gui.Draw (finalSemaphore, 
			demo->swapchain, demo->renderCompleteSemaphore, demo->guiCompleteSemaphore, demo->renderPassGui);
#endif
	}


#endif
	demo->swapchain.SubmitFrame(finalSemaphore);
	
vkQueueWaitIdle(demo->swapchain.gfxPresentQueue);
vkQueueWaitIdle(demo->swapchain.uploadQueue);


}


















///////////////////////////
//
// Textures
//
///////////////////////////



static void demo_destroy_texture_image(Demo *demo,
                                       TextureObject *tex_objs) 
{
SystemTools::Log("demo_destroy_texture_image\n");
    /* clean up staging resources */
    vkFreeMemory(demo->context.gpus[0].device, tex_objs->deviceMemory, NULL);
    vkDestroyImage(demo->context.gpus[0].device, tex_objs->image, NULL);
}

static void demo_prepare_textures(Demo *demo) 
{
SystemTools::Log("demo_prepare_textures\n");
	Gpu &gpu = *demo->swapchain.gpu;

	const VkFormat tex_format = VK_FORMAT_R8G8B8A8_UNORM;
 
	int tex_width = 64;
	int tex_height = 64;
	int size = (tex_width * tex_height * 4);
	uint8_t* data = new uint8_t [size];

	for (int y=0; y<tex_height; y++) for (int x=0; x<tex_width; x++)
	{
		uint8_t* pixel = &data[(y * tex_width + x) * 4];
		pixel[0] = uint8_t(float(y) / float(tex_height) * 255 + .5f);//(x&7) * 36;
		pixel[1] = uint8_t(float(x) / float(tex_width) * 255 + .5f);//(y&7) * 36;
		pixel[2] = y^x;
		pixel[3] = 128;
	}

	TextureObject &texture = demo->textures[0];
	TextureObject::LoadTexture (texture, 
		gpu, demo->swapchain.commandPool, demo->swapchain.gfxPresentQueue,
		data, tex_width, tex_height,
		tex_format, false);

	delete [] data;
		
	TextureObject::CreateSamplerAndView (texture, 
		gpu, tex_format);

#ifdef USE_GUI
//#ifdef RENDER_GUI
	demo->gui.CreateFontTexture (gpu, demo->swapchain.commandPool, demo->swapchain.gfxPresentQueue);
#endif 
}












///////////////////////////
//
// BufferObject
//
///////////////////////////



void demo_prepare_matrices_buffer(Demo *demo) 
{
SystemTools::Log("demo_prepare_cube_data_buffer\n");

	//demo->obj_matrices = new mat4 [DEMO_OBJ_COUNT];
    demo->matrix_data.Prepare (demo->context.gpus[0], 0, sizeof(float)*16 * demo->drawMatrixCount, 
#ifdef USE_UB
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT); 
#endif
#ifdef USE_SB
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT); 
#endif


#ifdef USE_VIS
	mat4 MVP = mat4();
    simpleVis.uniform_data_vis.Prepare (demo->context.gpus[0], &MVP, sizeof(MVP), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);//VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT); 
#endif
}

void demo_prepare_vertex_buffers(Demo *demo) 
{
SystemTools::Log("demo_prepare_cube_data_buffer2\n");

	demo->drawMatrixCount = DEMO_OBJ_COUNT;
	demo->drawVertexCount = demo->drawMatrixCount * 12*3;

	//uint32_t const numIndices = 12*3*DEMO_OBJ_COUNT;
	
	uint32_t *indexData = new uint32_t[demo->drawVertexCount];
	float *vertexData = new float[demo->drawVertexCount * 8];

	uint32_t *ptrI = indexData;
	float *ptr = vertexData;

    uint32_t index = 0;
    for (int o = 0; o < DEMO_OBJ_COUNT; o++) 
	{
		float const scale = 0.1;//4.0f;
		float const offset = 0;//4.0f;
		int const shift = 7;
		int const mask = (1<<shift)-1;
		int x = o & mask;
		int y = (o>>shift) & mask;
		int z = (o>>(shift*2));
		for (int i = 0; i < 12 * 3; i++) 
		{
			(*ptrI++) = index++;
			(*ptr++) = float(x)*offset + scale*g_vertex_buffer_data[i * 3];
			(*ptr++) = float(y)*offset + scale*g_vertex_buffer_data[i * 3 + 1];
			(*ptr++) = float(z)*offset + scale*g_vertex_buffer_data[i * 3 + 2];
			*((uint32_t*)(ptr++)) = o;
			(*ptr++) = g_uv_buffer_data[2 * i];
			(*ptr++) = g_uv_buffer_data[2 * i + 1];
			(*ptr++) = 0;
			(*ptr++) = 0;
		}
	}

	// upload vertex & index data

	VkCommandBuffer copyCmd;

	demo->index_data.Prepare (demo->context.gpus[0], indexData, demo->drawVertexCount * sizeof(uint32_t), 
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	demo->vertex_data.Prepare (demo->context.gpus[0], vertexData, demo->drawVertexCount*8 * sizeof(float), 
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

	CommandBuffer::Create (&copyCmd, demo->swapchain.gpu->device, demo->swapchain.commandPool);
	CommandBuffer::Begin (copyCmd);
	demo->vertex_data.CmdUpload (copyCmd);
	demo->index_data.CmdUpload (copyCmd);
	CommandBuffer::End (copyCmd);
	CommandBuffer::Submit (&copyCmd, demo->swapchain.gfxPresentQueue);
	vkQueueWaitIdle(demo->swapchain.gfxPresentQueue);
	CommandBuffer::Free (&copyCmd, demo->swapchain.gpu->device, demo->swapchain.commandPool);
	demo->vertex_data.DestroyOnHost (demo->context.gpus[0]);
	demo->index_data.DestroyOnHost (demo->context.gpus[0]);

	delete [] indexData;
	delete [] vertexData;


	// Binding description
	VkVertexInputBindingDescription &bindingDescription = demo->vertexShaderInfo.bindingDescription;
		bindingDescription.binding = VERTEX_BUFFER_BIND_ID;
		bindingDescription.stride = sizeof(float)*8;
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	// Attribute descriptions
	// Describes memory layout and shader attribute locations
	// Location 0 : Position
	VkVertexInputAttributeDescription *attributeDescriptions = demo->vertexShaderInfo.attributeDescriptions;
		attributeDescriptions[0].binding = VERTEX_BUFFER_BIND_ID;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[0].offset = 0;
	// Location 1 : Color
		attributeDescriptions[1].binding = VERTEX_BUFFER_BIND_ID;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[1].offset = sizeof(float)*4;

		// Assign to vertex input state
	VkPipelineVertexInputStateCreateInfo &inputState = demo->vertexShaderInfo.inputState;
		inputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		inputState.pNext = NULL;
		inputState.flags = NULL;
		inputState.vertexBindingDescriptionCount = 1;
		inputState.pVertexBindingDescriptions = &bindingDescription;
		inputState.vertexAttributeDescriptionCount = 2;
		inputState.pVertexAttributeDescriptions = attributeDescriptions;

#ifdef USE_VIS
	simpleVis.InitDrawBufferData (*demo->swapchain.gpu, demo->swapchain.frameCount);
#endif

}

void demo_update_matrices(Demo *demo) 
{
	if (demo->swapchain.height) demo->camera.aspect = float(demo->swapchain.width) / float(demo->swapchain.height);
	demo->camera.UpdateProjection ();

    mat4 VP;
    size_t const matrixSize = sizeof(mat4);
 
    VP = demo->camera.projection * demo->camera.worldInverse;

#ifdef USE_VIS
	simpleVis.uniform_data_vis.Update (demo->context.gpus[0], (const void *)&VP[0][0], matrixSize, 0);
#endif

    // Rotate 22.5 degrees around the Y axis
	
	demo->spin_angle += demo->timer.avgDeltaTime * 0.5f;
	demo->model_matrix = eulerAngleY(demo->spin_angle);
    
	VkCommandBuffer copyCmd;

	//demo->uniform_data.Update (demo->context.gpus[0], (const void *)&MVP[0][0], matrixSize, 0);

	mat4 *pData = (mat4*) demo->matrix_data.Map (demo->context.gpus[0]);
	
	mat4 Model;
    for (uint32_t o = 0; o < demo->drawMatrixCount; o++) 
	{
		float const offset = 4.0f;
		int const shift = 7;
		int const mask = (1<<shift)-1;
		int x = o & mask;
		int y = (o>>shift) & mask;
		int z = (o>>(shift*2));
		Model = eulerAngleY(float (o) / float(DEMO_OBJ_COUNT) * 100.1f + demo->spin_angle);
		Model *= eulerAngleX(float (DEMO_OBJ_COUNT-o) / float(DEMO_OBJ_COUNT) * 100.1f + demo->spin_angle);
		Model[3] = vec4(
			float(x)*offset,
			float(y)*offset,
			float(z)*offset,
			1.0f);
		//demo->obj_matrices[o] = VP * Model;
		(*pData++) = VP * Model;
	}
	demo->matrix_data.Unmap (demo->context.gpus[0]);

	CommandBuffer::Create (&copyCmd, demo->swapchain.gpu->device, demo->swapchain.commandPool);
	CommandBuffer::Begin (copyCmd);
	demo->matrix_data.CmdUpload (copyCmd);
	CommandBuffer::End (copyCmd);
	CommandBuffer::Submit (&copyCmd, demo->swapchain.gfxPresentQueue);
	vkQueueWaitIdle(demo->swapchain.gfxPresentQueue);
	CommandBuffer::Free (&copyCmd, demo->swapchain.gpu->device, demo->swapchain.commandPool);
 }











///////////////////////////
//
// Descriptor
//
///////////////////////////





static void demo_prepare_descriptor_layout(Demo *demo) 
{
SystemTools::Log("demo_prepare_descriptor_layout\n");
	VkDescriptorSetLayoutBinding layout_bindings[3] = {{},{},{}};

	// matrices
    layout_bindings[0].binding = 0,
#ifdef USE_UB
    layout_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
#endif
#ifdef USE_SB
    layout_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
#endif
    layout_bindings[0].descriptorCount = 1,
    layout_bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    layout_bindings[0].pImmutableSamplers = NULL;

	// vertices
    layout_bindings[1].binding = 1,
    layout_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
    layout_bindings[1].descriptorCount = 1,
    layout_bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    layout_bindings[1].pImmutableSamplers = NULL;

	// texture
    layout_bindings[2].binding = 2,
    layout_bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    layout_bindings[2].descriptorCount = DEMO_TEXTURE_COUNT,
    layout_bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
    layout_bindings[2].pImmutableSamplers = NULL;
    
    VkDescriptorSetLayoutCreateInfo descriptor_layout = {};
    descriptor_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    descriptor_layout.pNext = NULL,
    descriptor_layout.bindingCount = 3,
    descriptor_layout.pBindings = layout_bindings;
    
    VkResult U_ASSERT_ONLY err;

    err = vkCreateDescriptorSetLayout(demo->context.gpus[0].device, &descriptor_layout, NULL,
                                      &demo->desc_layout);
    assert(!err);

    VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
    pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    pPipelineLayoutCreateInfo.pNext = NULL,
    pPipelineLayoutCreateInfo.setLayoutCount = 1,
    pPipelineLayoutCreateInfo.pSetLayouts = &demo->desc_layout;
    

    err = vkCreatePipelineLayout(demo->context.gpus[0].device, &pPipelineLayoutCreateInfo, NULL,
                                 &demo->pipeline_layout);
    assert(!err);
}

static void demo_prepare_descriptor_pool(Demo *demo) 
{
SystemTools::Log("demo_prepare_descriptor_pool\n");
	// Create Descriptor Pool
	if (1) // todo: how to determinate exact needs? - move to VK_Helper!
    {
        VkDescriptorPoolSize pool_size[11] = 
        {
            { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
        };
        VkDescriptorPoolCreateInfo pool_info = {};
			pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
			pool_info.maxSets = 1000 * 11;
			pool_info.poolSizeCount = 11;
			pool_info.pPoolSizes = pool_size;

        VkResult U_ASSERT_ONLY err = vkCreateDescriptorPool(demo->context.gpus[0].device, &pool_info, NULL,
									 &demo->desc_pool);
        assert(!err);
    }
	else
	{


		VkDescriptorPoolSize type_counts[3] = {{},{},{}}; // 2
		type_counts[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		type_counts[0].descriptorCount = 1;//1;

		type_counts[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		type_counts[1].descriptorCount = 1;//1;

		type_counts[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		type_counts[2].descriptorCount = DEMO_TEXTURE_COUNT;
    
		VkDescriptorPoolCreateInfo descriptor_pool = {};
		descriptor_pool.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		descriptor_pool.pNext = NULL,
		descriptor_pool.maxSets = 1,
		descriptor_pool.poolSizeCount = 3,
		descriptor_pool.pPoolSizes = type_counts;
    
		VkResult U_ASSERT_ONLY err;

		err = vkCreateDescriptorPool(demo->context.gpus[0].device, &descriptor_pool, NULL,
									 &demo->desc_pool);
		assert(!err);
	}
}

static void demo_prepare_descriptor_set(Demo *demo) 
{
SystemTools::Log("demo_prepare_descriptor_set\n");

	uint32_t const numWrites = 2;

    VkDescriptorImageInfo tex_descs[DEMO_TEXTURE_COUNT];
    VkWriteDescriptorSet writes[numWrites];
    VkResult U_ASSERT_ONLY err;
    uint32_t i;

    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    alloc_info.pNext = NULL,
    alloc_info.descriptorPool = demo->desc_pool,
    alloc_info.descriptorSetCount = 1,
    alloc_info.pSetLayouts = &demo->desc_layout;

    err = vkAllocateDescriptorSets(demo->context.gpus[0].device, &alloc_info, &demo->desc_set);
    assert(!err);

    memset(&tex_descs, 0, sizeof(tex_descs));
    for (i = 0; i < DEMO_TEXTURE_COUNT; i++) {
        tex_descs[i].sampler = demo->textures[i].sampler;
        tex_descs[i].imageView = demo->textures[i].view;
        tex_descs[i].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    }

    memset(&writes, 0, sizeof(writes));

	VkDescriptorBufferInfo uniformBI; demo->matrix_data.GetDescriptorBufferInfo (uniformBI);
    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet = demo->desc_set;
    writes[0].dstBinding = 0;
    writes[0].descriptorCount = 1;
#ifdef USE_UB
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
#endif
#ifdef USE_SB
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
#endif
    writes[0].pBufferInfo = &uniformBI;

	writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstSet = demo->desc_set;
    writes[1].dstBinding = 2;
    writes[1].descriptorCount = DEMO_TEXTURE_COUNT;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[1].pImageInfo = tex_descs;
	/*
	VkDescriptorBufferInfo uniformBI; demo->uniform_data.GetDescriptorBufferInfo (uniformBI);
	writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[2].dstSet = demo->desc_set;
    writes[2].dstBinding = 1;
    writes[2].descriptorCount = 1;
    writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[2].pBufferInfo = &demo->uniform_data.buffer_info;
	*/


    vkUpdateDescriptorSets(demo->context.gpus[0].device, numWrites, writes, 0, NULL);

#ifdef USE_VIS
	{
		err = vkAllocateDescriptorSets(demo->context.gpus[0].device, &alloc_info, &demo->desc_set_vis);
		assert(!err);

		VkDescriptorBufferInfo uniformBI; simpleVis.uniform_data_vis.GetDescriptorBufferInfo (uniformBI);
		writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[0].dstSet = demo->desc_set_vis;
		writes[0].dstBinding = 0;
		writes[0].descriptorCount = 1;//1;
#ifdef USE_UB
		writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
#endif
#ifdef USE_SB
		writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
#endif
		writes[0].pBufferInfo = &uniformBI;

		vkUpdateDescriptorSets(demo->context.gpus[0].device, 1, writes, 0, NULL);
	}
#endif
}















///////////////////////////
//
// Render Pass
//
///////////////////////////



static void demo_prepare_render_pass(Demo *demo) 
{
SystemTools::Log("demo_prepare_render_pass\n");
	VkAttachmentDescription attachments[2] = {{},{}};
    attachments[0].format = demo->swapchain.format,
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT,
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    attachments[1].format = demo->swapchain.depth.format,
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT,
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    VkAttachmentReference color_reference = {};
    color_reference.attachment = 0, 
	color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkAttachmentReference depth_reference = {};
    depth_reference.attachment = 1,
    depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
    subpass.flags = 0,
    subpass.inputAttachmentCount = 0,
    subpass.pInputAttachments = NULL,
    subpass.colorAttachmentCount = 1,
    subpass.pColorAttachments = &color_reference,
    subpass.pResolveAttachments = NULL,
    subpass.pDepthStencilAttachment = &depth_reference,
    subpass.preserveAttachmentCount = 0,
    subpass.pPreserveAttachments = NULL;
    
    VkRenderPassCreateInfo rp_info = {};
    rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
    rp_info.pNext = NULL,
    rp_info.attachmentCount = 2,
    rp_info.pAttachments = attachments,
    rp_info.subpassCount = 1,
    rp_info.pSubpasses = &subpass,
    rp_info.dependencyCount = 0,
    rp_info.pDependencies = NULL;
    
    VkResult U_ASSERT_ONLY err;

    err = vkCreateRenderPass(demo->context.gpus[0].device, &rp_info, NULL, &demo->render_pass);
    assert(!err);


	// gui render pass

#ifdef RENDER_GUI
    {
        VkAttachmentDescription attachments[2] = {{},{}};
        attachments[0].format = demo->swapchain.format;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;//VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;//VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		attachments[1].format = demo->swapchain.depth.format;
		attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
 		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		subpass.flags = 0,
		subpass.inputAttachmentCount = 0,
		subpass.pInputAttachments = NULL,
		subpass.colorAttachmentCount = 1,
		subpass.pColorAttachments = &color_reference,
		subpass.pResolveAttachments = NULL,
		//subpass.pDepthStencilAttachment = &depth_reference,
		subpass.preserveAttachmentCount = 0,
		subpass.pPreserveAttachments = NULL;

		VkRenderPassCreateInfo rp_info = {};
		rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		rp_info.pNext = NULL,
		rp_info.attachmentCount = 2,
		rp_info.pAttachments = attachments,
		rp_info.subpassCount = 1,
		rp_info.pSubpasses = &subpass,
		rp_info.dependencyCount = 0,
		rp_info.pDependencies = NULL;
        
		err = vkCreateRenderPass(demo->context.gpus[0].device, &rp_info, NULL, &demo->renderPassGui);
        assert(!err);
    }
#endif
}
















///////////////////////////
//
// Pipeline
//
///////////////////////////

static void demo_prepare_pipeline_ALL(Demo *demo) 
{
SystemTools::Log("demo_prepare_pipeline_ALL\n");
    VkGraphicsPipelineCreateInfo pipeline;
    VkPipelineCacheCreateInfo pipelineCache;
    //VkPipelineVertexInputStateCreateInfo vi;
    VkPipelineInputAssemblyStateCreateInfo ia;
    VkPipelineRasterizationStateCreateInfo rs;
    VkPipelineColorBlendStateCreateInfo cb;
    VkPipelineDepthStencilStateCreateInfo ds;
    VkPipelineViewportStateCreateInfo vp;
    VkPipelineMultisampleStateCreateInfo ms;
    VkDynamicState dynamicStateEnables[VK_DYNAMIC_STATE_RANGE_SIZE];
    VkPipelineDynamicStateCreateInfo dynamicState;
    VkResult U_ASSERT_ONLY err;

    memset(dynamicStateEnables, 0, sizeof dynamicStateEnables);
    memset(&dynamicState, 0, sizeof dynamicState);
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.pDynamicStates = dynamicStateEnables;

    memset(&pipeline, 0, sizeof(pipeline));
    pipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline.layout = demo->pipeline_layout;

    //memset(&vi, 0, sizeof(vi));
    //vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    memset(&ia, 0, sizeof(ia));
    ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    memset(&rs, 0, sizeof(rs));
    rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode = VK_CULL_MODE_BACK_BIT;
    rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rs.depthClampEnable = VK_FALSE;
    rs.rasterizerDiscardEnable = VK_FALSE;
    rs.depthBiasEnable = VK_FALSE;
    rs.lineWidth = 1.0f;

    memset(&cb, 0, sizeof(cb));
    cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    VkPipelineColorBlendAttachmentState att_state[1];
    memset(att_state, 0, sizeof(att_state));
    att_state[0].colorWriteMask = 0xf;
    att_state[0].blendEnable = VK_FALSE;
    cb.attachmentCount = 1;
    cb.pAttachments = att_state;

    memset(&vp, 0, sizeof(vp));
    vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp.viewportCount = 1;
    dynamicStateEnables[dynamicState.dynamicStateCount++] = VK_DYNAMIC_STATE_VIEWPORT;
    vp.scissorCount = 1;
    dynamicStateEnables[dynamicState.dynamicStateCount++] = VK_DYNAMIC_STATE_SCISSOR;

    memset(&ds, 0, sizeof(ds));
    ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.depthTestEnable = VK_TRUE;
    ds.depthWriteEnable = VK_TRUE;
    ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    ds.depthBoundsTestEnable = VK_FALSE;
    ds.back.failOp = VK_STENCIL_OP_KEEP;
    ds.back.passOp = VK_STENCIL_OP_KEEP;
    ds.back.compareOp = VK_COMPARE_OP_ALWAYS;
    ds.stencilTestEnable = VK_FALSE;
    ds.front = ds.back;

    memset(&ms, 0, sizeof(ms));
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.pSampleMask = NULL;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Two stages: vs and fs
    pipeline.stageCount = 2;
    VkPipelineShaderStageCreateInfo shaderStages[2];
    memset(&shaderStages, 0, 2 * sizeof(VkPipelineShaderStageCreateInfo));
#ifdef USE_UB
	demo->vert_shader_module = LoadSPV (demo->swapchain.gpu->device, "..\\shaders\\texUB-vert.spv");
#endif
#ifdef USE_SB
	demo->vert_shader_module = LoadSPV (demo->swapchain.gpu->device, "..\\shaders\\texSB-vert.spv");
#endif
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = demo->vert_shader_module;
	shaderStages[0].pName = "main";

	demo->frag_shader_module = LoadSPV (demo->swapchain.gpu->device, "..\\shaders\\tex-frag.spv");
    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = demo->frag_shader_module;
    shaderStages[1].pName = "main";

    memset(&pipelineCache, 0, sizeof(pipelineCache));
    pipelineCache.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

    err = vkCreatePipelineCache(demo->context.gpus[0].device, &pipelineCache, NULL, &demo->pipelineCache);
    assert(!err);

    pipeline.pVertexInputState = &demo->vertexShaderInfo.inputState;
    pipeline.pInputAssemblyState = &ia;
    pipeline.pRasterizationState = &rs;
    pipeline.pColorBlendState = &cb;
    pipeline.pMultisampleState = &ms;
    pipeline.pViewportState = &vp;
    pipeline.pDepthStencilState = &ds;
    pipeline.pStages = shaderStages;
    pipeline.renderPass = demo->render_pass;
    pipeline.pDynamicState = &dynamicState;

    pipeline.renderPass = demo->render_pass;

    err = vkCreateGraphicsPipelines(demo->context.gpus[0].device, demo->pipelineCache, 1,
                                    &pipeline, NULL, &demo->pipeline);
//vkCreateGraphicsPipelines: required parameter pCreateInfos[i].pStages[i].module specified as VK_NULL_HANDLE
    assert(!err);



	vkDestroyShaderModule(demo->context.gpus[0].device, demo->frag_shader_module, NULL);
    vkDestroyShaderModule(demo->context.gpus[0].device, demo->vert_shader_module, NULL);







#ifdef USE_VIS
	simpleVis.InitPipeline (demo->context.gpus[0].device, pipeline);
#endif



}















///////////////////////////
//
// Lifecycle
//
///////////////////////////



static void demo_prepare(Demo *demo) 
{
SystemTools::Log("demo_prepare\n"); 

    demo->swapchain.CreateCommandPoolAndInitCommandBuffer ();
    demo->swapchain.PrepareSwapchain ();
	demo->swapchain.PrepareDepth ();
 
    

	demo_prepare_vertex_buffers(demo);
    demo_prepare_matrices_buffer(demo);
    demo_prepare_textures(demo);

    demo_prepare_descriptor_layout(demo);
    demo_prepare_render_pass(demo);
    demo_prepare_pipeline_ALL(demo);
    //demo_prepare_pipeline_Tex(demo);demo_prepare_pipeline_VisP(demo);
    




    demo_prepare_descriptor_pool(demo);
    demo_prepare_descriptor_set(demo);

    demo->swapchain.PrepareFramebuffers(demo->render_pass);

    for (uint32_t i = 0; i < demo->swapchain.frameCount; i++) 
	{
        demo->swapchain.currentFrame = i;
        demo_draw_build_cmd(demo, demo->swapchain.frames[i].drawCommandBuffer, i);
    }
    
    /*
     * Prepare functions above may generate pipeline commands
     * that need to be flushed before beginning the render loop.
     */
	demo->swapchain.FlushInitCommandBuffer();

    demo->swapchain.currentFrame = 0;
    
#ifdef RENDER_GUI
	demo->gui.CreateDeviceObjects (demo->swapchain, demo->desc_pool, demo->renderPassGui);
	//demo->gui.BuildFrames (demo->swapchain);
#endif

	VkSemaphoreCreateInfo presentCompleteSemaphoreCreateInfo = {};
		presentCompleteSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		presentCompleteSemaphoreCreateInfo.pNext = NULL,
		presentCompleteSemaphoreCreateInfo.flags = 0;  
    VkResult U_ASSERT_ONLY err;
    err = vkCreateSemaphore(demo->context.gpus[0].device, &presentCompleteSemaphoreCreateInfo, NULL, &demo->presentCompleteSemaphore);
    assert(!err);
    err = vkCreateSemaphore(demo->context.gpus[0].device, &presentCompleteSemaphoreCreateInfo, NULL, &demo->renderCompleteSemaphore);
    assert(!err);
    err = vkCreateSemaphore(demo->context.gpus[0].device, &presentCompleteSemaphoreCreateInfo, NULL, &demo->guiCompleteSemaphore);
    assert(!err);

	



#ifdef USE_ASYNC_TEST
	//demo->giGPU_impl.Init (demo->swapchain.gpu, demo->swapchain.gfxPresentFamily, demo->swapchain.gfxPresentQueue, demo->desc_pool);
	demo->giGPU_VK.Init (demo->swapchain.gpu, demo->swapchain.gfxPresentFamily, demo->swapchain.gfxPresentQueue, demo->desc_pool);
#endif

	
	demo->prepared = true;

	demo->timer.Reset();

}

static void demo_cleanup(Demo *demo) 
{
SystemTools::Log("demo_cleanup\n"); 
	Gpu &gpu = *demo->swapchain.gpu;
	vkDeviceWaitIdle(gpu.device);
    demo->prepared = false;
   	demo->DestroyOnResize();
	
	demo->swapchain.fpDestroySwapchainKHR(gpu.device, demo->swapchain.swapchain, NULL);
	demo->swapchain.Destroy();
	vkDestroySurfaceKHR(demo->context.inst, demo->swapchain.surface, NULL);

}

static void demo_resize (Application::Application *app, int width, int height) 
{
SystemTools::Log("demo_resize\n");
	Demo *demo = (Demo*) app->userData;


	demo->swapchain.width = width;
	demo->swapchain.height = height;
					
    // Don't react to resize until after first initialization.
    if (!demo->prepared) return;
 
	// In order to properly resize the window, we must re-create the swapchain
    // AND redo the command buffers, etc.
    //
    // First, perform part of the demo_cleanup() function:
	demo->prepared = false;
	demo->DestroyOnResize();
	demo->swapchain.Destroy();

    // Second, re-perform the demo_prepare() function, which will re-create the
    // swapchain:
    demo_prepare(demo);


}

static void demo_run2 (Application::Application *app) 
{
	Demo *demo = (Demo*) app->userData;
	if (!demo->prepared) return;
    // Wait for work to finish before updating MVP.
    vkDeviceWaitIdle(demo->context.gpus[0].device);

    demo_update_matrices(demo);

    demo_draw(demo);

    demo->framesDone++;
    if (demo->framesDone != INT_MAX && demo->framesDone == demo->quitAtFrame) 
	{
        PostQuitMessage(0);
    }
}

static void demo_init (Demo *demo, 
	Application::Application *app,
    bool use_break = false,
    bool validate = false,
	int quitAtFrame = INT32_MAX) 
{
SystemTools::Log("demo_init\n");
    vec3 eye = {0.0f, 3.0f, 5.0f};
    vec3 origin = {0, 0, 0};
    vec3 up = {0.0f, 1.0f, 0.0};

    memset(demo, 0, sizeof(*demo));

    demo->application = app;
    demo->context.use_break = use_break;
    demo->context.validate = validate;
    demo->framesDone = 0;
	demo->quitAtFrame = quitAtFrame;

    
    
 
   // if (!demo->context.use_xlib) demo_init_connection(demo);

    demo->context.InitVK ("AsyncTest", validate, use_break);


    demo->spin_angle = 0.01f;
    demo->pause = false;

    demo->camera.Init();
    demo->camera.UpdateWorld (vec3(0.f,0.f,5.f), vec3(0.f,0.f,0.f));
	//demo->camera.UpdateWorld (vec3(-9.561529f, 11.470680, 10.680165), vec3(-0.795000, -0.718000, 0.f)); // nice view
    //demo->camera.UpdateWorld (vec3(50.0f, 120.0f, 60.0f), vec3(-1.38, -0.2, 0.f)); // debug view

    demo->model_matrix = mat4();
#ifdef USE_VIS
	simpleVis.Init();
#endif


#ifdef USE_ASYNC_TEST
	demo->InitScene();
#endif
}




}