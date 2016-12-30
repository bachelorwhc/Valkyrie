#include "valkyrie/vulkan/instance.h"
#include "utility.h"
using namespace Vulkan;
//const std::vector<const char*> g_instance_extensions = { VK_KHR_SURFACE_EXTENSION_NAME , SURFACE_EXTENSION_NAME, VK_EXT_DEBUG_REPORT_EXTENSION_NAME };
const std::vector<const char*> g_instance_extensions = { VK_KHR_SURFACE_EXTENSION_NAME , SURFACE_EXTENSION_NAME };
//const std::vector<const char*> g_instance_layers = { "VK_LAYER_LUNARG_standard_validation" };
const std::vector<const char*> g_instance_layers = {  };

VkResult Vulkan::CreateInstance(const char* application_name, Instance& instance) {
	VkApplicationInfo application = {};
	application.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	application.pApplicationName = application_name;
	application.applicationVersion = 1;
	application.pEngineName = "Valkyrie";
	application.engineVersion = 1;
	application.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo instance_create = {};
	instance_create.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_create.pApplicationInfo = &application;
	instance_create.enabledExtensionCount = g_instance_extensions.size();
	instance_create.ppEnabledExtensionNames = g_instance_extensions.data();
	instance_create.enabledLayerCount = g_instance_layers.size();
	instance_create.ppEnabledLayerNames = g_instance_layers.data();

	VkResult result;
	result = vkCreateInstance(&instance_create, NULL, &instance.handle);
	return result;
}

void Vulkan::DestroyInstance(Instance& instance)
{
	vkDestroyInstance(instance.handle, nullptr);
}