/* OpenArcanum ArtViewer main app code */

#include "app/ArtViewer.h"

#include <stdio.h>

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "artviewer_vulkan.h"

#include <string>
#include <vector>

ArtViewer::ArtViewer(int argC, char** argV)
{
	ParseInputParams(argC, argV);

	// Setup SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
	{
		printf("Error: %s\n", SDL_GetError());
		return;
	}

	// Setup window
	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	SdlWindow = SDL_CreateWindow("OpenArcanum ArtViewer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		windowWidth, windowHeight, window_flags);

	// Setup Vulkan
	uint32_t extensions_count = 0;
	SDL_Vulkan_GetInstanceExtensions(SdlWindow, &extensions_count, NULL);
	const char** extensions = new const char*[extensions_count];
	SDL_Vulkan_GetInstanceExtensions(SdlWindow, &extensions_count, extensions);

	vkCtrl = new VulkanController(extensions, extensions_count);
	delete[] extensions;

	// Create Window Surface
	VkSurfaceKHR surface;
	if (SDL_Vulkan_CreateSurface(SdlWindow, vkCtrl->GetVkInstance(), &surface) == 0)
	{
		printf("Failed to create Vulkan surface.\n");
		return;
	}

	// Create Framebuffers
	int w, h;
	SDL_GetWindowSize(SdlWindow, &w, &h);
	vkCtrl->SetupVulkanWindow(&ImGuiVulkanWindow, surface, w, h, mMinImageCount);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	//ImGuiIO& io = ImGui::GetIO();
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer bindings
	ImGui_ImplSDL2_InitForVulkan(SdlWindow);

	ImGui_ImplVulkan_InitInfo init_info = vkCtrl->GetImGuiInitInfo();
	init_info.ImageCount = ImGuiVulkanWindow.ImageCount;
	init_info.MinImageCount = mMinImageCount;
	ImGui_ImplVulkan_Init(&init_info, ImGuiVulkanWindow.RenderPass);

	vkCtrl->UploadFonts(&ImGuiVulkanWindow);
}


ArtViewer::~ArtViewer()
{
	// Cleanup
	vkCtrl->VulkanError(vkDeviceWaitIdle(vkCtrl->GetVkDevice()));

	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
	ImGui_ImplVulkanH_DestroyWindow(vkCtrl->GetVkInstance(), vkCtrl->GetVkDevice(), &ImGuiVulkanWindow, vkCtrl->GetVkAllocationCallbacks());

	delete vkCtrl;

	SDL_DestroyWindow(SdlWindow);
	SDL_Quit();
}


void ArtViewer::ParseInputParams(int argC, char** argV)
{
	for (int i = 0; i < argC; i++)
		fprintf(stdout, argV[i]);
}


int ArtViewer::Run()
{
	// Our state
	bool show_demo_window = true;
	bool show_another_window = false;
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	// Main loop
	bool done = false;
	while (!done)
	{
		// Poll and handle events (inputs, window resize, etc.)
		// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
		// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
		// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
		// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			ImGui_ImplSDL2_ProcessEvent(&event);
			if (event.type == SDL_QUIT)
				done = true;
			if (event.type == SDL_WINDOWEVENT &&
				event.window.event == SDL_WINDOWEVENT_RESIZED && event.window.windowID == SDL_GetWindowID(SdlWindow))
			{
				// Note: your own application may rely on SDL_WINDOWEVENT_MINIMIZED/SDL_WINDOWEVENT_RESTORED to skip updating all-together.
				// Here ImGui_ImplSDL2_NewFrame() will set io.DisplaySize to zero which will disable rendering but let application run.
				// Please note that you can't Present into a minimized window.
				mSwapChainResizeWidth = (int)event.window.data1;
				mSwapChainResizeHeight = (int)event.window.data2;
				mSwapChainRebuild = true;
			}
		}

		// Resize swap chain?
		if (mSwapChainRebuild && mSwapChainResizeWidth > 0 && mSwapChainResizeHeight > 0)
		{
			mSwapChainRebuild = false;
			ImGui_ImplVulkan_SetMinImageCount(mMinImageCount);
			ImGui_ImplVulkanH_CreateOrResizeWindow(
				vkCtrl->GetVkInstance(), vkCtrl->GetVkPhysicalDevice(), vkCtrl->GetVkDevice(), &ImGuiVulkanWindow, vkCtrl->GetVkQueueFamily(),
				vkCtrl->GetVkAllocationCallbacks(), mSwapChainResizeWidth, mSwapChainResizeHeight, mMinImageCount);
			ImGuiVulkanWindow.FrameIndex = 0;
		}

		// Start the Dear ImGui frame
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplSDL2_NewFrame(SdlWindow);
		ImGui::NewFrame();

		// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
		if (show_demo_window)
			ImGui::ShowDemoWindow(&show_demo_window);

		// 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
		{
			static float f = 0.0f;
			static int counter = 0;

			ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

			ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
			ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
			ImGui::Checkbox("Another Window", &show_another_window);

			ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
			ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

			if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
				counter++;
			ImGui::SameLine();
			ImGui::Text("counter = %d", counter);

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			ImGui::End();
		}

		// 3. Show another simple window.
		if (show_another_window)
		{
			ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
			ImGui::Text("Hello from another window!");
			if (ImGui::Button("Close Me"))
				show_another_window = false;
			ImGui::End();
		}

		// Rendering
		ImGui::Render();
		ImDrawData* draw_data = ImGui::GetDrawData();
		const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);

		if (!is_minimized)
		{
			vkCtrl->FrameRender(&ImGuiVulkanWindow, draw_data);
			vkCtrl->FramePresent(&ImGuiVulkanWindow);
		}
	}

	return 0;
}
