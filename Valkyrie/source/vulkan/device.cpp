#include "valkyrie/vulkan/physical_device.h"
#include "valkyrie/vulkan/device.h"
#include "common.h"
using namespace Vulkan;

const std::vector<const char*> g_device_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

void WriteDeviceQueueCreates(std::vector<VkDeviceQueueCreateInfo>& device_queue_creates) {
	for (uint32_t queue_family_index = 0; queue_family_index < device_queue_creates.size(); ++queue_family_index) {
		VkDeviceQueueCreateInfo& qi = device_queue_creates[queue_family_index];
		qi = {};
		qi.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		qi.queueCount = 1;

		float queue_priorities[] = { 1.0 };
		qi.pQueuePriorities = queue_priorities;
		qi.queueFamilyIndex = queue_family_index;
	}
}

VkResult Vulkan::CreateDevice(Device& device) {
	VkResult result;
	uint32_t number_of_queue_family = PhysicalDevice::queueFamilyProperties.size();

	const uint32_t queue_count = PhysicalDevice::queueFamilyProperties.size();
	std::vector<VkDeviceQueueCreateInfo> device_queue_creates(queue_count);
	WriteDeviceQueueCreates(device_queue_creates);

	VkDeviceCreateInfo device_create = {};
	device_create.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_create.queueCreateInfoCount = device_queue_creates.size();
	device_create.pQueueCreateInfos = device_queue_creates.data();
	device_create.enabledExtensionCount = g_device_extensions.size();
	device_create.ppEnabledExtensionNames = g_device_extensions.data();

	result = vkCreateDevice(g_physical_device_handle, &device_create, nullptr, &device.handle);
	return result;
}

void Vulkan::DestroyDevice(Device& device) {
	vkDestroyDevice(g_device_handle, nullptr);
}