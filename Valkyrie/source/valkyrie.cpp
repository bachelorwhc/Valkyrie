#include <imgui.h>
#include "valkyrie.h"
#include "valkyrie/vulkan/tool.h"
#include "valkyrie/vulkan/debug.h"
#include "valkyrie/UI/user_input.h"
#include "valkyrie/UI/window.h"
#include "valkyrie/UI/window_manager.h"
#include "valkyrie/utility/sdl_manager.h"

ValkyrieEngine* ValkyrieEngine::gp_valkyrie = nullptr;
bool ValkyrieEngine::SDLInitialized = false;
VkInstance g_instance_handle = VK_NULL_HANDLE;
VkDevice g_device_handle = VK_NULL_HANDLE;
VkPhysicalDevice g_physical_device_handle = VK_NULL_HANDLE;

int ValkyrieEngine::initializeValkyrieEngine() {
	if (gp_valkyrie != nullptr)
		return 0;
	gp_valkyrie = NEW_NT ValkyrieEngine("Valkyrie");
	if (gp_valkyrie == nullptr)
		return 1;
	int result_tm = Valkyrie::ThreadManager::initialize();
	int result_am = Valkyrie::AssetManager::initialize();
	int result_sm = Valkyrie::SDLManager::initialize();
	int result_wm = Valkyrie::WindowManager::initialize();
	if (result_tm != 0)
		return 2;
	if (result_am != 0)
		return 3;
	if (result_sm != 0)
		return 4;
	if (result_wm != 0)
		return 5;
	return 0;
}

void ValkyrieEngine::closeValkyrieEngine() {
	Valkyrie::WindowManager::close();
	Valkyrie::SDLManager::close();
	Valkyrie::AssetManager::close();
	Valkyrie::ThreadManager::close();
	if(gp_valkyrie != nullptr)
		delete gp_valkyrie;
	gp_valkyrie = nullptr;
}

ValkyrieEngine::ValkyrieEngine(std::string application_name) :
	m_application_name(application_name),
	m_render_pfns() {
	
}

ValkyrieEngine::~ValkyrieEngine() {
	
}

void ValkyrieEngine::initializeImGuiInput() {
	auto& imgui_io = ImGui::GetIO();
	imgui_io.KeyMap[ImGuiKey_Enter] = SDL_SCANCODE_RETURN;
	imgui_io.KeyMap[ImGuiKey_Backspace] = SDL_SCANCODE_BACKSPACE;
	imgui_io.KeyMap[ImGuiKey_Delete] = SDL_SCANCODE_DELETE;
	imgui_io.KeyMap[ImGuiKey_Escape] = SDL_SCANCODE_ESCAPE;
	imgui_io.KeyMap[ImGuiKey_Tab] = SDL_SCANCODE_TAB;
	imgui_io.KeyMap[ImGuiKey_LeftArrow] = SDL_SCANCODE_LEFT;
	imgui_io.KeyMap[ImGuiKey_RightArrow] = SDL_SCANCODE_RIGHT;
	imgui_io.KeyMap[ImGuiKey_UpArrow] = SDL_SCANCODE_UP;
	imgui_io.KeyMap[ImGuiKey_DownArrow] = SDL_SCANCODE_DOWN;
	imgui_io.KeyMap[ImGuiKey_PageUp] = SDL_SCANCODE_PAGEUP;
	imgui_io.KeyMap[ImGuiKey_PageDown] = SDL_SCANCODE_PAGEDOWN;
	imgui_io.KeyMap[ImGuiKey_Home] = SDL_SCANCODE_HOME;
	imgui_io.KeyMap[ImGuiKey_End] = SDL_SCANCODE_END;
	imgui_io.KeyMap[ImGuiKey_A] = SDL_SCANCODE_A;
	imgui_io.KeyMap[ImGuiKey_C] = SDL_SCANCODE_C;
	imgui_io.KeyMap[ImGuiKey_V] = SDL_SCANCODE_V;
	imgui_io.KeyMap[ImGuiKey_X] = SDL_SCANCODE_X;
	imgui_io.KeyMap[ImGuiKey_Y] = SDL_SCANCODE_Y;
	imgui_io.KeyMap[ImGuiKey_Z] = SDL_SCANCODE_Z;
}

void ValkyrieEngine::updateUserInput(const SDL_Event& s_event) {
	auto& imgui_io = ImGui::GetIO();
	if (s_event.type == SDL_MOUSEMOTION) {
		imgui_io.MousePos.x = s_event.motion.x;
		imgui_io.MousePos.y = s_event.motion.y;
	}
	memcpy(imgui_io.MouseDown, userInput.mousePressed, 3);
	imgui_io.MouseWheel = userInput.mouseWheel;
	userInput.mouseWheel = 0.0f;
}

void ValkyrieEngine::updateTime() {
	double time = (double)SDL_GetTicks() / 1000.0;
	m_current_timestamp = time;
	m_deltatime = m_current_timestamp - m_previous_timestamp;
	m_previous_timestamp = m_current_timestamp;
	auto& imgui_io = ImGui::GetIO();
	imgui_io.DeltaTime = (float)m_deltatime;
}

bool ValkyrieEngine::execute() {
	static SDL_Event s_event;
	while (SDL_PollEvent(&s_event)) {
		switch (s_event.type) {
		case SDL_QUIT:
			return false;
		case SDL_TEXTINPUT:
			userInput.handleSDLCharEvent(s_event);
			break;
		case SDL_KEYDOWN:
		case SDL_KEYUP:
			userInput.handleSDLKeyBoardEvent(s_event);
			break;
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			userInput.handleSDLMouseButtonEvent(s_event);
			break;
		case SDL_MOUSEWHEEL:
			userInput.handleSDLScrollEvent(s_event);
			break;
		}
		updateUserInput(s_event);
	}
	updateTime();
	render();
	return true;
}

VkResult ValkyrieEngine::initialize() {
	VkResult result;

	SDL_StartTextInput();

	// VULKAN MANAGER
	// RENDER CONTEXT
	
	initializeImGuiInput();

	

	return VK_SUCCESS;
}

VkResult ValkyrieEngine::render() {
	VkResult result;

	result = mp_swapchain->acquireNextImage(UINT64_MAX, m_present_semaphore, m_present_fence);
	assert(result == VK_SUCCESS);

	VkSubmitInfo submit = {};
	submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit.waitSemaphoreCount = 1;
	submit.pWaitSemaphores = &m_present_semaphore;
	submit.commandBufferCount = 1;
	submit.pCommandBuffers = &renderCommands[mp_swapchain->getCurrent()].handle;

	result = vkQueueSubmit(m_graphics_queue.handle, 1, &submit, m_present_fence);
	assert(result == VK_SUCCESS);

	do {
		result = vkWaitForFences(m_device.handle, 1, &m_present_fence, VK_TRUE, 100000000);
	} while (result == VK_TIMEOUT);
	assert(result == VK_SUCCESS);
	vkResetFences(m_device.handle, 1, &m_present_fence);

	result = mp_swapchain->queuePresent(m_graphics_queue);
	assert(result == VK_SUCCESS);

	return VK_SUCCESS;
}

void ValkyrieEngine::createPipelineModule(const std::string & pipename_name) {
	pipelines[pipename_name] = MAKE_SHARED(Vulkan::PipelineModule)();
	vertexInputs[pipename_name] = MAKE_SHARED(Vulkan::VertexInput)();
}
