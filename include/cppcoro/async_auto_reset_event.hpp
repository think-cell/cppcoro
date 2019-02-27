///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Lewis Baker
// Licenced under MIT license. See LICENSE.txt for details.
///////////////////////////////////////////////////////////////////////////////
#ifndef CPPCORO_ASYNC_AUTO_RESET_EVENT_HPP_INCLUDED
#define CPPCORO_ASYNC_AUTO_RESET_EVENT_HPP_INCLUDED

#include "async_semaphore.hpp"
#include <experimental/coroutine>
#include <atomic>
#include <cstdint>

namespace cppcoro
{
	/// An async auto-reset event is a coroutine synchronisation abstraction
	/// that allows one or more coroutines to wait until some thread calls
	/// set() on the event.
	///
	/// When a coroutine awaits a 'set' event the event is automatically
	/// reset back to the 'not set' state, thus the name 'auto reset' event.
	class async_auto_reset_event : private async_semaphore
	{
	public:

		/// Initialise the event to either 'set' or 'not set' state.
		async_auto_reset_event(bool initiallySet) noexcept;

		/// Wait for the event to enter the 'set' state.
		///
		/// If the event is already 'set' then the event is set to the 'not set'
		/// state and the awaiting coroutine continues without suspending.
		/// Otherwise, the coroutine is suspended and later resumed when some
		/// thread calls 'set()'.
		///
		/// Note that the coroutine may be resumed inside a call to 'set()'
		/// or inside another thread's call to 'operator co_await()'.
		using async_semaphore::operator co_await;

		/// Set the state of the event to 'set'.
		///
		/// If there are pending coroutines awaiting the event then one
		/// pending coroutine is resumed and the state is immediately
		/// set back to the 'not set' state.
		///
		/// This operation is a no-op if the event was already 'set'.
		void set() noexcept;

		/// Set the state of the event to 'not-set'.
		///
		/// This is a no-op if the state was already 'not set'.
		void reset() noexcept;
	};
}

#endif
