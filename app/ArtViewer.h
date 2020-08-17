/* OpenArcanum ArtViewer main app code */

#pragma once

#include <SDL.h>
#include <SDL_vulkan.h>

#include "artviewer_vulkan.h"

class ArtViewer
{
public:
	explicit ArtViewer(int argC, char** argV);
	~ArtViewer();

	ArtViewer(const ArtViewer&) = delete;
	ArtViewer(ArtViewer&&) = delete;

	bool isReady() { return initOK && pipelineReady; }

	int Run();

protected:
	void ParseInputParams(int argC, char** argV);

protected:
	SDL_Window* SdlWindow = 0;
	ImGui_ImplVulkanH_Window ImGuiVulkanWindow = {};

	VulkanController* vkCtrl = 0;

	bool initOK = false;
	bool pipelineReady = false;

	int windowWidth = 1024;
	int windowHeight = 768;

	bool mSwapChainRebuild = false;
	int  mSwapChainResizeWidth = 0;
	int  mSwapChainResizeHeight = 0;

	uint32_t mMinImageCount = 2;

};