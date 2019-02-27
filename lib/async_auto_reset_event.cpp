///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Lewis Baker
// Licenced under MIT license. See LICENSE.txt for details.
///////////////////////////////////////////////////////////////////////////////

#include <cppcoro/async_auto_reset_event.hpp>

#include <cppcoro/config.hpp>

#include <cassert>
#include <algorithm>

cppcoro::async_auto_reset_event::async_auto_reset_event(bool initiallySet) noexcept
	: async_semaphore(initiallySet ? 1 : 0)
{
}

void cppcoro::async_auto_reset_event::set() noexcept
{
	std::uint64_t oldState = m_state.load(std::memory_order_relaxed);
	do
	{
		if (async_semaphore_detail::get_set_count(oldState) > async_semaphore_detail::get_waiter_count(oldState))
		{
			// Already set.
			return;
		}

		// Increment the set-count
	} while (!m_state.compare_exchange_weak(
		oldState,
		oldState + async_semaphore_detail::set_increment,
		std::memory_order_acq_rel,
		std::memory_order_acquire));

	resume_waiters_if_locked(oldState, 1);
}

void cppcoro::async_auto_reset_event::reset() noexcept
{
	std::uint64_t oldState = m_state.load(std::memory_order_relaxed);
	while (async_semaphore_detail::get_set_count(oldState) > async_semaphore_detail::get_waiter_count(oldState))
	{
		if (m_state.compare_exchange_weak(
			oldState,
			oldState - async_semaphore_detail::set_increment,
			std::memory_order_relaxed))
		{
			// Successfully reset.
			return;
		}
	}

	// Not set. Nothing to do.
}
