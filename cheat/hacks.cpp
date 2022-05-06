#include "hacks.h"
#include "gui.h"
#include "globals.h"
#include "vector.h"
#include <iostream>
#include <Windows.h>
#include <array>
#include <thread>

#define EnemyPen 0x000000FF
HBRUSH EnemyBrush = CreateSolidBrush(0x000000FF);

int screenX = GetSystemMetrics(SM_CXSCREEN);
int screenY = GetSystemMetrics(SM_CYSCREEN);
HDC hdc = GetDC(FindWindowA(NULL, "Counter-Strike: Global Offensive - Direct3D 9"));


struct view_matrix_t {
	float* operator[ ](int index) {
		return matrix[index];
	}
	float matrix[4][4];
};

struct Vectorr3
{
	float x, y, z;
};

Vectorr3 WorldToScreen(const Vectorr3 pos, view_matrix_t matrix) {
	float _x = matrix[0][0] * pos.x + matrix[0][1] * pos.y + matrix[0][2] * pos.z + matrix[0][3];
	float _y = matrix[1][0] * pos.x + matrix[1][1] * pos.y + matrix[1][2] * pos.z + matrix[1][3];

	float w = matrix[3][0] * pos.x + matrix[3][1] * pos.y + matrix[3][2] * pos.z + matrix[3][3];

	float inv_w = 1.f / w;
	_x *= inv_w;
	_y *= inv_w;

	float x = screenX * .5f;
	float y = screenY * .5f;

	x += 0.5f * _x * screenX + 0.5f;
	y -= 0.5f * _y * screenY + 0.5f;

	return { x,y,w };
}

void DrawFilledRect(int x, int y, int w, int h)
{
	RECT rect = { x, y, x + w, y + h };
	FillRect(hdc, &rect, EnemyBrush);
}

void DrawBorderBox(int x, int y, int w, int h, int thickness)
{
	DrawFilledRect(x, y, w, thickness); //Top horiz line
	DrawFilledRect(x, y, thickness, h); //Left vertical line
	DrawFilledRect((x + w), y, thickness, h); //right vertical line
	DrawFilledRect(x, y + h, w + thickness, thickness); //bottom horiz line

}

void DrawLinee(float StartX, float StartY, float EndX, float EndY)
{
	int a, b = 0;
	HPEN hOPen;
	HPEN hNPen = CreatePen(PS_SOLID, 2, EnemyPen);// penstyle, width, color
	hOPen = (HPEN)SelectObject(hdc, hNPen);
	MoveToEx(hdc, StartX, StartY, NULL); //start
	a = LineTo(hdc, EndX, EndY); //end
	DeleteObject(SelectObject(hdc, hOPen));
}

void hacks::EnemyGlowThread(const Memory& mem) noexcept
{

	while (gui::isRunning)
	{

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		
		struct Color
		{
			std::uint8_t r{ }, g{ }, b{ };
		};

		constexpr const auto EnemyColor = Color{ 255, 0, 0 };
		constexpr const auto TeamColor = Color{ 0, 0, 255 };
		

		const auto localPlayer = mem.Read<std::uintptr_t>(globals::clientAddress + hazedumper::signatures::dwLocalPlayer);
		if (!localPlayer)
			continue;

		const auto glowManager = mem.Read<std::uintptr_t>(globals::clientAddress + hazedumper::signatures::dwGlowObjectManager);
		if (!glowManager)
			continue;

		const auto localTeam = mem.Read<std::int32_t>(localPlayer + hazedumper::netvars::m_iTeamNum);

		for (auto i = 1; i <= 32; ++i)
		{


			const auto player = mem.Read<std::uintptr_t>(globals::clientAddress + hazedumper::signatures::dwEntityList + i * 0x10);
			if (!player)
				continue;

			const auto team = mem.Read<std::int32_t>(player + hazedumper::netvars::m_iTeamNum);

			if (team == localTeam)
				continue;

			const auto lifeState = mem.Read<std::int32_t>(player + hazedumper::netvars::m_lifeState);

			if (lifeState != 0)
				continue;


			if (globals::enemyglow)
			{
				const auto glowIndex = mem.Read<std::int32_t>(player + hazedumper::netvars::m_iGlowIndex);

				mem.Write(glowManager + (glowIndex * 0x38) + 0x8, globals::enemyglowColor[0]);
				mem.Write(glowManager + (glowIndex * 0x38) + 0xC, globals::enemyglowColor[1]);
				mem.Write(glowManager + (glowIndex * 0x38) + 0x10, globals::enemyglowColor[2]);
				mem.Write(glowManager + (glowIndex * 0x38) + 0x14, globals::enemyglowColor[3]);

				mem.Write(glowManager + (glowIndex * 0x38) + 0x28, true);
				mem.Write(glowManager + (glowIndex * 0x38) + 0x29, false);
			}

			if (globals::radar)
				mem.Write(player + hazedumper::netvars::m_bSpotted, true);

		}

	}
}

static bool thirdpersonon = false;

void hacks::EspThread(const Memory& mem) noexcept
{
	while (gui::isRunning)
	{

		std::this_thread::sleep_for(std::chrono::milliseconds(1));

		if (globals::esp)
		{
			while (globals::esp)
			{
				for (int i = 1; i < 1000; i++) //NO FUCKING CLUE WHY IF I DONT GO THIS HIGH IT, IT DOESNT WORK
				{
					const auto localPlayer = mem.Read<std::uintptr_t>(globals::clientAddress + hazedumper::signatures::dwLocalPlayer);
					const auto localTeam = mem.Read<std::int32_t>(localPlayer + hazedumper::netvars::m_iTeamNum);
					const auto entity = mem.Read<std::uintptr_t>(globals::clientAddress + hazedumper::signatures::dwEntityList + i + 0x10);
					const auto boneMatrix = mem.Read<std::uintptr_t>(entity + hazedumper::netvars::m_dwBoneMatrix);
					const auto health = mem.Read<std::uintptr_t>(entity + hazedumper::netvars::m_iHealth);
					if (!entity)
						continue;

					const auto team = mem.Read<std::int32_t>(entity + hazedumper::netvars::m_iTeamNum);
					if (team == localTeam)
						continue;

					const auto lifeState = mem.Read<std::int32_t>(entity + hazedumper::netvars::m_lifeState);
					if (lifeState != 0)
						continue;

					if (mem.Read<bool>(entity + hazedumper::signatures::m_bDormant))
						continue;


					view_matrix_t vm = mem.Read<view_matrix_t>(globals::clientAddress + hazedumper::signatures::dwViewMatrix);

					Vectorr3 pos = mem.Read<Vectorr3>(entity + hazedumper::netvars::m_vecOrigin);

					Vectorr3 head;
					head.x = pos.x;
					head.y = pos.y;
					head.z = pos.z + 72.f;
					Vectorr3 screenpos = WorldToScreen(pos, vm);
					Vectorr3 screenhead = WorldToScreen(head, vm);
					float height = screenhead.y - screenpos.y;
					float width = height / 2.4f;

					if (screenpos.z >= 0.01f && team != localTeam && health > 0 && health < 101)
					{
						if (globals::espbox)
							DrawBorderBox(screenpos.x - (width / 2), screenpos.y, width, height, 1);
							DrawTextA(hdc, "Joe", NULL, NULL, NULL);
						if(globals::snaplines)
							DrawLinee(screenX / 2, screenY, screenpos.x, screenpos.y);

					}

				}

			}
		}

	}
}

constexpr const int GetWeaponPaint(const short& itemDefinition)
{
	switch (itemDefinition)
	{
	case 1: return 37; //Deagle - Blaze
	case 3: return 464; //Fiveseven - Neon Kimono
	case 4: return 353; //Glock - Water Elemental
	case 7: return 1141; //Ak47 - NightWish
	case 8: return 455; //Aug - Akihabara Accept
	case 9: return 51; //Awp - Lightning strike
	case 10: return 919; //FAMAS | Commemoration
	case 11: return 493; //G3SG1 | Flux
	case 13: return 1013; //Galil AR | Phoenix Blacklight
	case 14: return 401; //M249 | System Lock
	case 16: return 309; //m4a1 - howl
	case 40: return 899; //SSG 08 | Bloodshot 
	case 74: return 507;
	case 60: return 984; // M4a41s -  printstream (best skin... no ones probably even reading this :( )
	case 61: return 504; //usps - kill confirmed 
	case 64: return 522; //r8 fade
	default: return 0;
	}
}



void hacks::SkinChangerThread(const Memory& mem) noexcept
{
	while (gui::isRunning)
	{
		if (globals::skinchanger)
		{
			const auto& localPlayer = mem.Read<std::uintptr_t>(globals::clientAddress + hazedumper::signatures::dwLocalPlayer);
			const auto& weapons = mem.Read<std::array<unsigned long, 8>>(localPlayer + hazedumper::netvars::m_hMyWeapons);

			for (const auto& handle : weapons)
			{
				const auto& weapon = mem.Read<std::uintptr_t>((globals::clientAddress + hazedumper::signatures::dwEntityList + (handle & 0xFFF) * 0x10) - 0x10);

				if (!weapon)
					continue;

				if (const auto paint = GetWeaponPaint(mem.Read<short>(weapon + hazedumper::netvars::m_iItemDefinitionIndex)))
				{
					const bool shouldUpdate = mem.Read<std::int32_t>(weapon + hazedumper::netvars::m_nFallbackPaintKit) != paint;

					mem.Write<std::int32_t>(weapon + hazedumper::netvars::m_iItemIDHigh, -1);
					mem.Write<std::int32_t>(weapon + hazedumper::netvars::m_nFallbackPaintKit, paint);
					mem.Write<float>(weapon + hazedumper::netvars::m_flFallbackWear, globals::skinwear);

					mem.Write<std::int32_t>(weapon + hazedumper::netvars::m_nFallbackSeed, globals::seed);
					if (globals::stattrack)
					{
						mem.Write<std::int32_t>(weapon + hazedumper::netvars::m_nFallbackStatTrak, globals::killcount);
						mem.Write<std::int32_t>(weapon + hazedumper::netvars::m_iAccountID, mem.Read< std::int32_t >(weapon + hazedumper::netvars::m_OriginalOwnerXuidLow));
					}

					if (shouldUpdate)
						mem.Write<std::int32_t>(mem.Read<std::uintptr_t>(globals::engineAddress + hazedumper::signatures::dwClientState) + 0x174, -1);
				}
			}


		}
		if (globals::update)
		{
			mem.Write<std::int32_t>(mem.Read<std::uintptr_t>(globals::engineAddress + hazedumper::signatures::dwClientState) + 0x174, -1);
			globals::update = false;
		}
	}

}

void hacks::TeamGlowThread(const Memory& mem) noexcept
{
	while (gui::isRunning)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));

		const auto localPlayer = mem.Read<std::uintptr_t>(globals::clientAddress + hazedumper::signatures::dwLocalPlayer);
		if (!localPlayer)
			continue;

		const auto glowManager = mem.Read<std::uintptr_t>(globals::clientAddress + hazedumper::signatures::dwGlowObjectManager);
		if (!glowManager)
			continue;

		const auto localTeam = mem.Read<std::int32_t>(localPlayer + hazedumper::netvars::m_iTeamNum);

		for (auto i = 1; i <= 32; ++i)
		{
			const auto player = mem.Read<std::uintptr_t>(globals::clientAddress + hazedumper::signatures::dwEntityList + i * 0x10);
			if (!player)
				continue;

			const auto team = mem.Read<std::int32_t>(player + hazedumper::netvars::m_iTeamNum);

			if (team != localTeam)
				continue;

			const auto lifeState = mem.Read<std::int32_t>(player + hazedumper::netvars::m_lifeState);

			if (lifeState != 0)
				continue;

			if (globals::teamglow)
			{
				const auto glowIndex = mem.Read<std::int32_t>(player + hazedumper::netvars::m_iGlowIndex);

				mem.Write(glowManager + (glowIndex * 0x38) + 0x8, globals::teamglowColor[0]);
				mem.Write(glowManager + (glowIndex * 0x38) + 0xC, globals::teamglowColor[1]);
				mem.Write(glowManager + (glowIndex * 0x38) + 0x10, globals::teamglowColor[2]);
				mem.Write(glowManager + (glowIndex * 0x38) + 0x14, globals::teamglowColor[3]);

				mem.Write(glowManager + (glowIndex * 0x38) + 0x28, true);
				mem.Write(glowManager + (glowIndex * 0x38) + 0x29, false);
			}


		}

	}
}

void hacks::bhopThread(const Memory& mem) noexcept
{
	while (gui::isRunning)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));

		const auto localPlayer = mem.Read<uintptr_t>(globals::clientAddress + hazedumper::signatures::dwLocalPlayer);

		if (globals::bhop)
		{
			if (localPlayer)
			{
				const auto onGround = mem.Read<bool>(localPlayer + hazedumper::netvars::m_fFlags);

				if (GetAsyncKeyState(VK_SPACE) && onGround & (1 << 0))
					mem.Write<BYTE>(globals::clientAddress + hazedumper::signatures::dwForceJump, 6);
			}

		}
		if (globals::usefov)
		{
			if (localPlayer)
			{
				mem.Write(localPlayer + hazedumper::netvars::m_iFOV, globals::viewfov);
			}
		}
			
		if (globals::noflash)
		{
			if (localPlayer)
			{

				mem.Write(localPlayer + hazedumper::netvars::m_flFlashMaxAlpha, 255 * globals::flash);
			}
		}
		if (globals::thirdperson == true)
		{
			if (localPlayer)
			{
				if (thirdpersonon == true)
				{
					continue;
				}
				else
				{
					mem.Write(localPlayer + hazedumper::netvars::m_iObserverMode, 1);
					thirdpersonon = true;
				}
			}
		}
		if (globals::thirdperson == false)
		{
			if (thirdpersonon == true)
			{
				if (localPlayer)
				{

					mem.Write(localPlayer + hazedumper::netvars::m_iObserverMode, 0);
					thirdpersonon = false;
				}
			}
			else
				continue;
		}
	}
}

constexpr Vector3 CalculateAngle(
	const Vector3& localPosition,
	const Vector3& enemyPosition,
	const Vector3& viewAngles) noexcept
{
	return ((enemyPosition - localPosition).ToAngle() - viewAngles);
}

namespace offset
{
	// client
	constexpr ::std::ptrdiff_t dwLocalPlayer = hazedumper::signatures::dwLocalPlayer;
	constexpr ::std::ptrdiff_t dwEntityList = hazedumper::signatures::dwEntityList;

	// engine
	constexpr ::std::ptrdiff_t dwClientState = hazedumper::signatures::dwClientState;
	constexpr ::std::ptrdiff_t dwClientState_ViewAngles = hazedumper::signatures::dwClientState_ViewAngles;
	constexpr ::std::ptrdiff_t dwClientState_GetLocalPlayer = hazedumper::signatures::dwClientState_GetLocalPlayer;

	// entity
	constexpr ::std::ptrdiff_t m_dwBoneMatrix = hazedumper::netvars::m_dwBoneMatrix;
	constexpr ::std::ptrdiff_t m_bDormant = hazedumper::signatures::m_bDormant;
	constexpr ::std::ptrdiff_t m_iTeamNum = hazedumper::netvars::m_iTeamNum;
	constexpr ::std::ptrdiff_t m_lifeState = hazedumper::netvars::m_lifeState;
	constexpr ::std::ptrdiff_t m_vecOrigin = hazedumper::netvars::m_vecOrigin;
	constexpr ::std::ptrdiff_t m_vecViewOffset = hazedumper::netvars::m_vecViewOffset;
	constexpr ::std::ptrdiff_t m_aimPunchAngle = hazedumper::netvars::m_aimPunchAngle;
	constexpr ::std::ptrdiff_t m_bSpottedByMask = hazedumper::netvars::m_bSpottedByMask;
}

static int bonechoice = 8;

void hacks::aimbotThread(const Memory& mem) noexcept
{
	const auto engine = mem.GetModuleAddress("engine.dll");
	const auto client = mem.GetModuleAddress("client.dll");
	while (gui::isRunning)
	{
		// infinite hack loop
		while (true)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			if (globals::aimbot)
			{


				std::this_thread::sleep_for(std::chrono::milliseconds(1));

				// aimbot key
				if (!GetAsyncKeyState(VK_XBUTTON1))
					continue;

				// get local player
				const auto localPlayer = mem.Read<std::uintptr_t>(client + offset::dwLocalPlayer);
				const auto localTeam = mem.Read<std::int32_t>(localPlayer + offset::m_iTeamNum);

				// eye position = origin + viewOffset
				const auto localEyePosition = mem.Read<Vector3>(localPlayer + offset::m_vecOrigin) +
					mem.Read<Vector3>(localPlayer + offset::m_vecViewOffset);

				const auto clientState = mem.Read<std::uintptr_t>(engine + offset::dwClientState);

				const auto localPlayerId =
					mem.Read<std::int32_t>(clientState + offset::dwClientState_GetLocalPlayer);

				const auto viewAngles = mem.Read<Vector3>(clientState + offset::dwClientState_ViewAngles);
				const auto aimPunch = mem.Read<Vector3>(localPlayer + offset::m_aimPunchAngle) * 2;

				// aimbot fov
				auto bestFov = globals::fov;
				auto bestAngle = Vector3{ };

				for (auto i = 1; i <= 32; ++i)
				{
					const auto player = mem.Read<std::uintptr_t>(client + offset::dwEntityList + i * 0x10);

					if (mem.Read<std::int32_t>(player + offset::m_iTeamNum) == localTeam)
						continue;

					if (mem.Read<bool>(player + offset::m_bDormant))
						continue;

					if (mem.Read<std::int32_t>(player + offset::m_lifeState))
						continue;

					if (globals::selected == 1) //neck
						bonechoice = 7;
					if (globals::selected == 2) //chest
						bonechoice = 6;
					if (globals::selected == 3) //pelvis
						bonechoice = 0;
					if (globals::selected == 4) //head
						bonechoice = 8;

					if (mem.Read<std::int32_t>(player + offset::m_bSpottedByMask) & (1 << localPlayerId))
					{
						const auto boneMatrix = mem.Read<std::uintptr_t>(player + offset::m_dwBoneMatrix);

						// pos of player head in 3d space
						// 8 is the head bone index :)
						const auto playerHeadPosition = Vector3{
							mem.Read<float>(boneMatrix + 0x30 * bonechoice + 0x0C),
							mem.Read<float>(boneMatrix + 0x30 * bonechoice + 0x1C),
							mem.Read<float>(boneMatrix + 0x30 * bonechoice + 0x2C)
						};

						const auto angle = CalculateAngle(
							localEyePosition,
							playerHeadPosition,
							viewAngles + aimPunch
						);

						const auto fov = std::hypot(angle.x, angle.y);

						if (fov < bestFov)
						{
							bestFov = fov;
							bestAngle = angle;
						}
					}
				}

				// if we have a best angle, do aimbot
				if (!bestAngle.IsZero())
					mem.Write<Vector3>(clientState + offset::dwClientState_ViewAngles, viewAngles + bestAngle / globals::smooth); // smoothing
			}
		}
		
	}

}