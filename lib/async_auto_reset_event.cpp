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

cppcoro::async_semaphore_acquire_operation
cppcoro::async_auto_reset_event::operator co_await() const noexcept
{
	return acquire();
}

void cppcoro::async_auto_reset_event::set() noexcept
{
	std::uint64_t oldState = m_state.load(std::memory_order_relaxed);
	async_semaphore_detail::decomposed_state oldStateDecomposed;
	async_semaphore_detail::decomposed_state newStateDecomposed;
	do
	{
		oldStateDecomposed = async_semaphore_detail::decomposed_state(oldState);
		if (oldStateDecomposed.m_resources > oldStateDecomposed.m_waiters)
		{
			// Already set.
			return;
		}

		// Increment the set-count
		newStateDecomposed=oldStateDecomposed;
		++newStateDecomposed.m_resources;
	} while (!m_state.compare_exchange_weak(
		oldState,
		newStateDecomposed.compose(),
		std::memory_order_acq_rel,
		std::memory_order_relaxed));

	// Did we transition from non-zero waiters and zero set-count
	// to non-zero set-count?
	// If so then we acquired the lock and are responsible for resuming waiters.
	assert(0 <= oldStateDecomposed.m_resources);
	if (0 < oldStateDecomposed.m_waiters && 0 == oldStateDecomposed.m_resources)
	{
		// We acquired the lock.
		resume_waiters(newStateDecomposed.resumable_waiter_count());
	}
}

void cppcoro::async_auto_reset_event::reset() noexcept
{
	std::uint64_t oldState = m_state.load(std::memory_order_relaxed);
	async_semaphore_detail::decomposed_state oldStateDecomposed;
	async_semaphore_detail::decomposed_state newStateDecomposed;
	do
	{
		oldStateDecomposed = async_semaphore_detail::decomposed_state(oldState);
		if (oldStateDecomposed.m_resources <= oldStateDecomposed.m_waiters)
		{
			// Not set. Nothing to do.
			return;
		}

		// Decrement the resource count
		newStateDecomposed=oldStateDecomposed;
		--newStateDecomposed.m_resources;
	} while (!m_state.compare_exchange_weak(
		oldState,
		newStateDecomposed.compose(),
		std::memory_order_relaxed));
}
