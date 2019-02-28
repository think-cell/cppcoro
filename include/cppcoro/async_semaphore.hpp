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

		struct decomposed_state final {
			std::uint32_t m_resources;

			std::uint32_t m_waiters;

			decomposed_state() = default;

			explicit decomposed_state(std::uint64_t state) noexcept
				: m_resources(static_cast<std::uint32_t>(state))
				, m_waiters(static_cast<std::uint32_t>(state >> 32))
			{}

			explicit decomposed_state(std::uint32_t resources, std::uint32_t waiters) noexcept
				: m_resources(resources)
				, m_waiters(waiters)
			{}

			std::uint64_t compose() const noexcept {
				return static_cast<std::uint64_t>(m_resources) | (static_cast<std::uint64_t>(m_waiters) << 32);
			}

			std::uint32_t resumable_waiter_count() const noexcept {
				return std::min(m_resources, m_waiters);
			}
		};
	}

	class async_semaphore_acquire_operation;

	class async_semaphore
	{
	public:

		/// Initialise the semaphore to have initial resources.
		async_semaphore(std::uint32_t initial) noexcept;

		~async_semaphore();

		/// Acquire a resource.
		///
		/// If a resource is already available, the awaiting coroutine continues without suspending.
		/// Otherwise, the coroutine is suspended and later resumed when some
		/// thread calls 'release()'.
		///
		/// Note that the coroutine may be resumed inside a call to 'release()'
		/// or inside another thread's call to 'acquire()'.
		async_semaphore_acquire_operation acquire() const noexcept;

		/// Release count number of resources.
		///
		/// If there are pending coroutines awaiting resources then
		/// pending coroutines are resumed.
		void release(std::uint32_t count = 1) noexcept;

	protected:
		void resume_waiters(std::uint32_t waiterCountToResume) const noexcept;

		// Bits 0-31  - Resource count
		// Bits 32-63 - Waiter count
		mutable std::atomic<std::uint64_t> m_state;

	private:
		template<typename MemoryOrder, typename Func>
		void modify_state(MemoryOrder memoryorder, Func func) const noexcept;

		friend class async_semaphore_acquire_operation;

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
