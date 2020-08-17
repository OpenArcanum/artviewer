/* OpenArcanum Vulkan init routines */

#pragma once

#include "imgui_impl_vulkan.h"

class VulkanController
{
public:
	explicit VulkanController(const char** extensions, uint32_t extensions_count);
	~VulkanController();

	VulkanController(const VulkanController&) = delete;
	VulkanController(VulkanController&&) = delete;

	static bool VulkanError(VkResult err);
	static void VulkanErrorStatic(VkResult err);

	void SetupVulkanWindow(ImGui_ImplVulkanH_Window* wd, VkSurfaceKHR surface, int width, int height, int minImageCount);

	void FrameRender(ImGui_ImplVulkanH_Window* wd, ImDrawData* draw_data);
	void FramePresent(ImGui_ImplVulkanH_Window* wd);

	void UploadFonts(ImGui_ImplVulkanH_Window* wd);

	auto GetVkInstance()			-> VkInstance { return g_Instance; }
	auto GetVkDevice()				-> VkDevice { return g_Device; }
	auto GetVkPhysicalDevice()		-> VkPhysicalDevice { return g_PhysicalDevice; }
	auto GetVkQueue()				-> VkQueue { return g_Queue; }
	auto GetVkAllocationCallbacks()	-> VkAllocationCallbacks* { return g_Allocator; }
	auto GetVkQueueFamily()			-> uint32_t { return g_QueueFamily; }

	ImGui_ImplVulkan_InitInfo GetImGuiInitInfo();

protected:
	void CleanupVulkan();

protected:
	VkAllocationCallbacks*   g_Allocator = NULL;
	VkInstance               g_Instance = VK_NULL_HANDLE;
	VkPhysicalDevice         g_PhysicalDevice = VK_NULL_HANDLE;
	VkDevice                 g_Device = VK_NULL_HANDLE;
	uint32_t                 g_QueueFamily = (uint32_t)-1;
	VkQueue                  g_Queue = VK_NULL_HANDLE;
	VkDebugReportCallbackEXT g_DebugReport = VK_NULL_HANDLE;
	VkPipelineCache          g_PipelineCache = VK_NULL_HANDLE;
	VkDescriptorPool         g_DescriptorPool = VK_NULL_HANDLE;
};
