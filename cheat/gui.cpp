#include "gui.h"
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_dx9.h"
#include "../imgui/imgui_impl_win32.h"
#include "hacks.h"
#include <vector>
#include "globals.h"
#include <string>
#include "memory.h"



extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
	HWND window,
	UINT message,
	WPARAM wideParameter,
	LPARAM longParameter
);

long __stdcall WindowProcess(
	HWND window,
	UINT message,
	WPARAM wideParameter,
	LPARAM longParameter)
{
	if (ImGui_ImplWin32_WndProcHandler(window, message, wideParameter, longParameter))
		return true;

	switch (message)
	{
	case WM_SIZE: {
		if (gui::device && wideParameter != SIZE_MINIMIZED)
		{
			gui::presentParameters.BackBufferWidth = LOWORD(longParameter);
			gui::presentParameters.BackBufferHeight = HIWORD(longParameter);
			gui::ResetDevice();
		}
	}return 0;

	case WM_SYSCOMMAND: {
		if ((wideParameter & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
	}break;

	case WM_DESTROY: {
		PostQuitMessage(0);
	}return 0;

	case WM_LBUTTONDOWN: {
		gui::position = MAKEPOINTS(longParameter); // set click points
	}return 0;

	case WM_MOUSEMOVE: {
		if (wideParameter == MK_LBUTTON)
		{
			const auto points = MAKEPOINTS(longParameter);
			auto rect = ::RECT{ };

			GetWindowRect(gui::window, &rect);

			rect.left += points.x - gui::position.x;
			rect.top += points.y - gui::position.y;

			if (gui::position.x >= 0 &&
				gui::position.x <= gui::WIDTH &&
				gui::position.y >= 0 && gui::position.y <= 19)
				SetWindowPos(
					gui::window,
					HWND_TOPMOST,
					rect.left,
					rect.top,
					0, 0,
					SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOZORDER
				);
		}

	}return 0;

	}

	return DefWindowProc(window, message, wideParameter, longParameter);
}

void gui::CreateHWindow(const char* windowName) noexcept
{
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_CLASSDC;
	windowClass.lpfnWndProc = WindowProcess;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = GetModuleHandleA(0);
	windowClass.hIcon = 0;
	windowClass.hCursor = 0;
	windowClass.hbrBackground = 0;
	windowClass.lpszMenuName = 0;
	windowClass.lpszClassName = "class001";
	windowClass.hIconSm = 0;

	RegisterClassEx(&windowClass);

	window = CreateWindowEx(
		0,
		"class001",
		windowName,
		WS_POPUP,
		100,
		100,
		WIDTH,
		HEIGHT,
		0,
		0,
		windowClass.hInstance,
		0
	);

	ShowWindow(window, SW_SHOWDEFAULT);
	UpdateWindow(window);
}

void gui::DestroyHWindow() noexcept
{
	DestroyWindow(window);
	UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
}

bool gui::CreateDevice() noexcept
{
	d3d = Direct3DCreate9(D3D_SDK_VERSION);

	if (!d3d)
		return false;

	ZeroMemory(&presentParameters, sizeof(presentParameters));

	presentParameters.Windowed = TRUE;
	presentParameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
	presentParameters.BackBufferFormat = D3DFMT_UNKNOWN;
	presentParameters.EnableAutoDepthStencil = TRUE;
	presentParameters.AutoDepthStencilFormat = D3DFMT_D16;
	presentParameters.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

	if (d3d->CreateDevice(
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		window,
		D3DCREATE_HARDWARE_VERTEXPROCESSING,
		&presentParameters,
		&device) < 0)
		return false;

	return true;
}

void gui::ResetDevice() noexcept
{
	ImGui_ImplDX9_InvalidateDeviceObjects();

	const auto result = device->Reset(&presentParameters);

	if (result == D3DERR_INVALIDCALL)
		IM_ASSERT(0);

	ImGui_ImplDX9_CreateDeviceObjects();
}

void gui::DestroyDevice() noexcept
{
	if (device)
	{
		device->Release();
		device = nullptr;
	}

	if (d3d)
	{
		d3d->Release();
		d3d = nullptr;
	}
}

void Colors() {
	ImGuiStyle& style = ImGui::GetStyle();

	ImGuiIO& io = ImGui::GetIO(); (void)io;

	style.Colors[ImGuiCol_WindowBg] = ImColor(16, 16, 16);
	style.Colors[ImGuiCol_ChildBg] = ImColor(24, 24, 24);
	style.WindowMinSize = ImVec2(540, 295);
	style.Colors[ImGuiCol_Text] = ImColor(255, 255, 255);
	style.Colors[ImGuiCol_CheckMark] = ImColor(0, 255, 247);
	style.Colors[ImGuiCol_ButtonHovered] = ImColor(75, 75, 75);
	style.Colors[ImGuiCol_Button] = ImColor(35, 35, 35);
	style.Colors[ImGuiCol_SliderGrab] = ImColor(45, 45, 45);
	style.Colors[ImGuiCol_SliderGrabActive] = ImColor(75, 75, 75);
	style.Colors[ImGuiCol_FrameBg] = ImColor(35, 35, 35);
	style.Colors[ImGuiCol_FrameBgHovered] = ImColor(40, 40, 40);
	style.Colors[ImGuiCol_Separator] = ImColor(0, 255, 247);
}


void gui::CreateImGui() noexcept
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ::ImGui::GetIO();

	io.IniFilename = NULL;

	Colors();

	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX9_Init(device);
}

void gui::DestroyImGui() noexcept
{
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void gui::BeginRender() noexcept
{
	MSG message;
	while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&message);
		DispatchMessage(&message);

		if (message.message == WM_QUIT)
		{
			isRunning = !isRunning;
			return;
		}
	}

	// Start the Dear ImGui frame
	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void gui::EndRender() noexcept
{
	ImGui::EndFrame();

	device->SetRenderState(D3DRS_ZENABLE, FALSE);
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

	device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_RGBA(0, 0, 0, 255), 1.0f, 0);

	if (device->BeginScene() >= 0)
	{
		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
		device->EndScene();
	}

	const auto result = device->Present(0, 0, 0, 0);

	// Handle loss of D3D9 device
	if (result == D3DERR_DEVICELOST && device->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
		ResetDevice();
}


void gui::Render() noexcept
{
	static const char* aimpart[]
	{
		"head",
		"neck",
		"chest",
		"pelvis"
	};

	

	static int tabb = 0;

	ImGui::SetNextWindowPos({ 0, 0 });
	ImGui::SetNextWindowSize({ 540, 295 });
	ImGui::Begin(
		"Demo Cheat",
		&isRunning,
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoTitleBar
	);
	//ImGui::BeginGroup();


	ImGui::PushStyleColor(ImGuiCol_Border, ImColor(0, 0, 0, 255).Value);
	ImGui::BeginChild("##LeftSide", ImVec2(120, ImGui::GetContentRegionAvail().y), true);
	{
		ImGui::Text("Tv0123\nExternal");
		ImGui::Separator();
		ImGui::Dummy(ImVec2(0.0f, 5.0f));
		if (ImGui::Button("Aim", ImVec2(ImGui::GetContentRegionAvail().x, 30)))
		{
			tabb = 0;
		}
		ImGui::Dummy(ImVec2(0.0f, 5.0f));
		if (ImGui::Button("Visual", ImVec2(ImGui::GetContentRegionAvail().x, 30)))
		{
			tabb = 1;
		}
		ImGui::Dummy(ImVec2(0.0f, 5.0f));
		if (ImGui::Button("Misc", ImVec2(ImGui::GetContentRegionAvail().x, 30)))
		{
			tabb = 2;
		}
		ImGui::Dummy(ImVec2(0.0f, 5.0f));
		if (ImGui::Button("Skin", ImVec2(ImGui::GetContentRegionAvail().x, 30)))
		{
			tabb = 3;
		}
		ImGui::Dummy(ImVec2(0.0f, 5.0f));
		if (ImGui::Button("Exit", ImVec2(ImGui::GetContentRegionAvail().x, 30)))
		{
			exit(1);
		}

	}
	ImGui::EndChild();

	{
		ImGui::SameLine(0);
		ImGui::SameLine();
		ImGui::BeginChild("##RightSide", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y), true);
		if (tabb == 0)
		{

			ImGui::Text("Aim Settings");
			ImGui::Separator();
			ImGui::Dummy(ImVec2(0.0f, 10.0f));
			ImGui::Checkbox("Aimbot", &globals::aimbot);
			ImGui::Dummy(ImVec2(0.0f, 5.0f));
			ImGui::ListBox("Aim Part", &globals::selected, aimpart, 4, 1);
			ImGui::Dummy(ImVec2(0.0f, 5.0f));
			ImGui::SliderFloat("FOV", &globals::fov, 5.f, 100.f);
			ImGui::Dummy(ImVec2(0.0f, 5.0f));
			ImGui::SliderFloat("Smoothness", &globals::smooth, 1.f, 100.f);
			ImGui::Separator();
			ImGui::Text("THIS IS A FREE CHEAT \nMADE BY Tv0123");
		}
		if (tabb == 1)
		{
			ImGui::Text("Visual Settings");
			ImGui::Separator();
			ImGui::Dummy(ImVec2(0.0f, 10.0f));
			ImGui::Checkbox("ESP (Super Buggy)", &globals::esp);
			ImGui::Checkbox("Box", &globals::espbox);
			ImGui::Checkbox("SnapLines", &globals::snaplines);
			ImGui::Checkbox("EnemyGlow", &globals::enemyglow);
			ImGui::ColorEdit4("Enamy Glow Color", globals::enemyglowColor);

			ImGui::Checkbox("TeamGlow", &globals::teamglow);
			ImGui::ColorEdit4("Team Glow Color", globals::teamglowColor);
			ImGui::Separator();
			ImGui::Text("THIS IS A FREE CHEAT \nMADE BY Tv0123");
		}
		if (tabb == 2)
		{
			ImGui::Text("Misc Settings");
			ImGui::Separator();
			ImGui::Dummy(ImVec2(0.0f, 10.0f));
			ImGui::Checkbox("Bhop", &globals::bhop);
			ImGui::Checkbox("No Flash", &globals::noflash);
			ImGui::SliderFloat("Flash percent", &globals::flash, 0.00, 1.00);
			ImGui::Checkbox("Radar", &globals::radar);
			ImGui::Checkbox("Custom FOV (FUCKS WITH ESP)", &globals::usefov);
			ImGui::SliderInt("Player FOV", &globals::viewfov, 1, 200);
			ImGui::Separator();
			ImGui::Text("THIS IS A FREE CHEAT \nMADE BY Tv0123");
		}
		if (tabb == 3)
		{
			ImGui::Text("Skin Changer");
			ImGui::Separator();
			ImGui::Dummy(ImVec2(0.0f, 10.0f));
			ImGui::Checkbox("SkinChanger", &globals::skinchanger);
			ImGui::SliderFloat("Wear", &globals::skinwear, 0.0f, 1.f);
			ImGui::SliderInt("Seed", &globals::seed, 1, 100);
			ImGui::Checkbox("StatTrack", &globals::stattrack);
			ImGui::SliderInt("Kills", &globals::killcount, 1, 3000);
			if (ImGui::Button("Update"))
				globals::update = true;
			ImGui::Separator();
			ImGui::Text("THIS IS A FREE CHEAT \nMADE BY Tv0123");
		}
		ImGui::EndChild();
	}

	ImGui::PopStyleColor();
	ImGui::End();
}
