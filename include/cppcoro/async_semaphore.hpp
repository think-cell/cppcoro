///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Lewis Baker
// Licenced under MIT license. See LICENSE.txt for details.
///////////////////////////////////////////////////////////////////////////////
#ifndef CPPCORO_ASYNC_SEMAPHORE_HPP_INCLUDED
#define CPPCORO_ASYNC_SEMAPHORE_HPP_INCLUDED

#include <experimental/coroutine>
#include <atomic>
#include <cstdint>
#include <algorithm>

namespace cppcoro
{
	namespace async_semaphore_detail
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

	class async_semaphore_acquire_operation;

	class async_semaphore
	{
	public:

		/// Initialise the semaphore to have initial resources.
		async_semaphore(std::uint32_t initial) noexcept;

		~async_semaphore();

		/// Wait for the event to enter the 'set' state.
		///
		/// If the event is already 'set' then the event is set to the 'not set'
		/// state and the awaiting coroutine continues without suspending.
		/// Otherwise, the coroutine is suspended and later resumed when some
		/// thread calls 'set()'.
		///
		/// Note that the coroutine may be resumed inside a call to 'set()'
		/// or inside another thread's call to 'operator co_await()'.
		async_semaphore_acquire_operation operator co_await() const noexcept;

		/// Set the state of the event to 'set'.
		///
		/// If there are pending coroutines awaiting the event then one
		/// pending coroutine is resumed and the state is immediately
		/// set back to the 'not set' state.
		///
		/// This operation is a no-op if the event was already 'set'.
		void set() noexcept;

	protected:
		void resume_waiters_if_locked(const std::uint64_t oldState) const noexcept;

		// Bits 0-31  - Set count
		// Bits 32-63 - Waiter count
		mutable std::atomic<std::uint64_t> m_state;

	private:

		friend class async_semaphore_acquire_operation;

		void resume_waiters(std::uint64_t initialState) const noexcept;

		mutable std::atomic<async_semaphore_acquire_operation*> m_newWaiters;

		mutable async_semaphore_acquire_operation* m_waiters;

	};

	class async_semaphore_acquire_operation
	{
	public:

		async_semaphore_acquire_operation() noexcept;

		explicit async_semaphore_acquire_operation(const async_semaphore& event) noexcept;

		async_semaphore_acquire_operation(const async_semaphore_acquire_operation& other) noexcept;

		bool await_ready() const noexcept { return m_event == nullptr; }
		bool await_suspend(std::experimental::coroutine_handle<> awaiter) noexcept;
		void await_resume() const noexcept {}

	private:

		friend class async_semaphore;

		const async_semaphore* m_event;
		async_semaphore_acquire_operation* m_next;
		std::experimental::coroutine_handle<> m_awaiter;
		std::atomic<std::uint32_t> m_refCount;
	};
}

#endif
