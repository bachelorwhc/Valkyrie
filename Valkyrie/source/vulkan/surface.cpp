#include "valkyrie/vulkan/surface.h"
#include "valkyrie/vulkan/instance.h"
#include "valkyrie/vulkan/physical_device.h"
#include "valkyrie/UI/window.h"
#include "valkyrie/UI/window_manager.h"
#include "valkyrie/utility/vulkan_manager.h"
using namespace Vulkan;

VkResult Vulkan::setSurface(Surface& surface, const Valkyrie::WindowPtr& window_ptr) {
	const auto& physical_device = Valkyrie::VulkanManager::getPhysicalDevice();
	VkResult result;
#ifdef _WIN32
	VkWin32SurfaceCreateInfoKHR surface_create = {};
	surface_create.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surface_create.hinstance = window_ptr->getWindowInstance();
	surface_create.hwnd = window_ptr->getWindowHandle();
	result = vkCreateWin32SurfaceKHR(Valkyrie::VulkanManager::getInstance(), &surface_create, nullptr, &surface.handle);
#elif __ANDROID__
	
#elif __linux__
	
#endif
	uint32_t format_count = 0;
	result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface.handle, &format_count, NULL);
	
	VkSurfaceFormatKHR* surface_format = NEW_NT VkSurfaceFormatKHR[format_count];
	result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface.handle, &format_count, surface_format);
	
	if (format_count == 1 && surface_format[0].format == VK_FORMAT_UNDEFINED) {
		surface_format[0].format = VK_FORMAT_B8G8R8A8_UNORM;
	}
	assert(format_count >= 1);
	surface.format = surface_format[0];

	for (uint32_t i = 0; i < format_count; ++i) {
		if (surface_format[i].format == VK_FORMAT_B8G8R8A8_UNORM) {
			surface.format = surface_format[i];
			break;
		}
	}
	delete[] surface_format;
	return result;
}

void Vulkan::DestroySurface(Surface& surface) {
	vkDestroySurfaceKHR(Valkyrie::VulkanManager::getInstance(), surface.handle, nullptr);
}

VkAttachmentDescription Surface::getAttachmentDescription() const {
	VkAttachmentDescription attachment_description = {};
	attachment_description.format = format.format;
	attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
	attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachment_description.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	attachment_description.flags = 0;
	return attachment_description;
}