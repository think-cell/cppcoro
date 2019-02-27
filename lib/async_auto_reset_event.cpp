///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Lewis Baker
// Licenced under MIT license. See LICENSE.txt for details.
///////////////////////////////////////////////////////////////////////////////

#include <cppcoro/async_auto_reset_event.hpp>

#include <cppcoro/config.hpp>

#include <cassert>
#include <algorithm>

namespace
{
	namespace local
	{
		// Some helpers for manipulating the 'm_state' value.

		constexpr std::uint64_t set_increment = 1;
		constexpr std::uint64_t waiter_increment = std::uint64_t(1) << 32;

		constexpr std::uint32_t get_set_count(std::uint64_t state)
		{
			return static_cast<std::uint32_t>(state);
		}

		constexpr std::uint32_t get_waiter_count(std::uint64_t state)
		{
			return static_cast<std::uint32_t>(state >> 32);
		}

		constexpr std::uint32_t get_resumable_waiter_count(std::uint64_t state)
		{
			return std::min(get_set_count(state), get_waiter_count(state));
		}
	}
}

cppcoro::async_auto_reset_event::async_auto_reset_event(bool initiallySet) noexcept
	: async_semaphore(initiallySet ? 1 : 0)
{
}

void cppcoro::async_auto_reset_event::set() noexcept
{
	std::uint64_t oldState = m_state.load(std::memory_order_relaxed);
	do
	{
		if (local::get_set_count(oldState) > local::get_waiter_count(oldState))
		{
			// Already set.
			return;
		}

		// Increment the set-count
	} while (!m_state.compare_exchange_weak(
		oldState,
		oldState + local::set_increment,
		std::memory_order_acq_rel,
		std::memory_order_acquire));

	resume_waiters_if_locked(oldState);
}

void cppcoro::async_auto_reset_event::reset() noexcept
{
	std::uint64_t oldState = m_state.load(std::memory_order_relaxed);
	while (local::get_set_count(oldState) > local::get_waiter_count(oldState))
	{
		if (m_state.compare_exchange_weak(
			oldState,
			oldState - local::set_increment,
			std::memory_order_relaxed))
		{
			// Successfully reset.
			return;
		}
	}

	// Not set. Nothing to do.
}
