#pragma once
#include "memory.h"

namespace hacks
{
	void EnemyGlowThread(const Memory& mem) noexcept;
	void TeamGlowThread(const Memory& mem) noexcept;
	void bhopThread(const Memory& mem) noexcept;
	void aimbotThread(const Memory& mem) noexcept;
	void EspThread(const Memory& mem) noexcept;
	void SkinChangerThread(const Memory& mem) noexcept;
}