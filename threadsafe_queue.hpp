#pragma once
#include <deque>
#include <mutex>
#include <condition_variable>
#include <algorithm>
#include <vector>
#include "stl_extensions.hpp"

#ifdef TRACY_ENABLE
	#include "Tracy.hpp"
	
	// Need to wrap locks for tracy
	#define MUTEX				TracyLockableN(std::mutex,	m, "ThreadsafeQueue mutex")
	#define CONDITION_VARIABLE	std::condition_variable_any	c

	#define UNIQUE_LOCK			std::unique_lock<LockableBase(std::mutex)> lock(m)
	#define LOCK_GUARD			std::lock_guard<LockableBase(std::mutex)> lock(m)
#else
	#define MUTEX				std::mutex m
	#define CONDITION_VARIABLE	std::condition_variable_any	c

	#define UNIQUE_LOCK			std::unique_lock lock(m)
	#define LOCK_GUARD			std::lock_guard lock(m)
#endif

// based on https://stackoverflow.com/questions/15278343/c11-thread-safe-queue

// multiple producer multiple consumer threadsafe queue with T item
template <typename T>
class ThreadsafeQueue {
	//mutable std::mutex		m;
	MUTEX;
	CONDITION_VARIABLE;

	// used like a queue but using a deque to support iteration (a queue is just wrapper around a deque, so it is not less efficient to use a deque over a queue)
	std::deque<T>			q;

	// use is optional (makes sense to use this to stop threads of thread pools (ie. use this on the job queue), but does not make sense to use this on the results queue)
	bool					shutdown_flag = false;

	// used by pop_all_wait and pop_n_wait to register the min number of elements they are waiting for before the thread goes to sleep
	// this enables avoiding that the main thread gets woken up after single push
	// 0 means signal after every push
	int						wait_counter = 0;

public:
	// push one element onto the queue
	void push (T elem) {
		ZoneScoped;
		//int notify;
		{
			LOCK_GUARD;

			q.emplace_back( std::move(elem) );

			//notify = wait_counter == 0 || q.size() >= wait_counter;
		}
		
		c.notify_one();
	}

	// push multiple elements onto the queue
	void push_n (T* elem, size_t count) {
		ZoneScoped;
		{
			LOCK_GUARD;

			for (size_t i=0; i<count; ++i) {
				ZoneScopedN("push item");
			
				q.emplace_back( std::move(elem[i]) );
				//c.notify_one();
			}
		}

		//for (size_t i=0; i<count; ++i) {
		//	ZoneScopedN("notify_one");
		//	c.notify_one();
		//}

		c.notify_all();
	}

	// wait to dequeue one element from the queue
	// can be called from multiple threads (multiple consumer)
	T pop_wait () {
		UNIQUE_LOCK;

		while(q.empty()) {
			c.wait(lock); // release lock as long as the wait and reaquire it afterwards.
		}

		T val = std::move(q.front());
		q.pop_front();
		return val;
	}

	// deque one element from the queue if there is one
	// can be called from multiple threads (multiple consumer)
	bool try_pop (T* out) {
		LOCK_GUARD;

		if (q.empty())
			return false;

		*out = std::move(q.front());
		q.pop_front();
		return true;
	}
	
	// wait until min elements are available, then dequeue up to max elements
	// writes the elements into their repective indicies in output
	// returns the number of elements dequeued
	size_t pop_n_wait (T output[], size_t min, size_t max) {
		UNIQUE_LOCK;

		// set wait counter, we won't be woken up until the queue contais this many elements
		//wait_counter = min;

		while (q.size() < min) {
			c.wait(lock); // release lock as long as the wait and reaquire it afterwards.
		}

		// reset the counter so future calls will get woken up
		//wait_counter = 0;

		size_t count = std::min(q.size(), max);
		for (size_t i=0; i<count; ++i) {
			output[i] = std::move(q.front());
			q.pop_front();
		}

		return count;
	}

	// wait until min elements are available, then dequeue all elements
	// returns the number of elements dequeued
	size_t pop_all_wait (std_vector<T>* output, size_t min) {
		UNIQUE_LOCK;

		// set wait counter, we won't be woken up until the queue contais this many elements
		//wait_counter = min;

		while (q.size() < min) {
			c.wait(lock); // release lock as long as the wait and reaquire it afterwards.
		}

		// reset the counter so future calls will get woken up
		//wait_counter = 0;

		size_t count = q.size();
		output->reserve(count);

		for (size_t i=0; i<count; ++i) {
			output->emplace_back( std::move(q.front()) );
			q.pop_front();
		}

		return count;
	}

	// dequeue up to max elements (or none); never waits
	// writes the elements into their repective indicies in output
	// returns the number of elements dequeued
	size_t pop_n (T output[], size_t max) {
		LOCK_GUARD;

		size_t count = std::min(q.size(), max);
		for (size_t i=0; i<count; ++i) {
			output[i] = std::move(q.front());
			q.pop_front();
		}

		return count;
	}

	// dequeue all elements (including none); never waits
	// returns the number of elements dequeued
	size_t pop_all (std_vector<T>* output) {
		LOCK_GUARD;

		size_t count = q.size();
		output->reserve(count);

		for (size_t i=0; i<count; ++i) {
			output->emplace_back( std::move(q.front()) );
			q.pop_front();
		}

		return count;
	}

	// wait to dequeue one element from the queue or until shutdown is set
	// returns if element was popped or shutdown was set as enum
	// can be called from multiple threads (multiple consumer)
	enum { POP, SHUTDOWN } pop_or_shutdown_wait (T* out) {
		UNIQUE_LOCK;

		while(!shutdown_flag && q.empty()) {
			c.wait(lock); // release lock as long as the wait and reaquire it afterwards.
		}
		if (shutdown_flag)
			return SHUTDOWN;

		*out = std::move(q.front());
		q.pop_front();
		return POP;
	}

	// set shutdown which all consumers can recieve via pop_or_shutdown
	void shutdown () {
		LOCK_GUARD;

		shutdown_flag = true;
		c.notify_all();
	}
	void reset_shutdown () {
		shutdown_flag = false;
	}

	// iterate queued items with template callback 'void func (T&)' in order from the oldest to the newest pushed
	// elements are allowed to be changed
	template <typename FOREACH>
	void iterate_queue (FOREACH callback) {
		LOCK_GUARD;

		for (auto it=q.begin(); it!=q.end(); ++it) {
			callback(*it);
		}
	}

	// iterate queued items with template callback 'void func (T&)' in order from the newest to the oldest pushed
	// elements are allowed to be changed
	template <typename FOREACH>
	void iterate_queue_newest_first (FOREACH callback) { // front == next to be popped, back == most recently pushed
		LOCK_GUARD;

		for (auto it=q.rbegin(); it!=q.rend(); ++it) {
			callback(*it);
		}
	}

	// remove items if template callback 'bool func (T&)' returns true
	// useful to be able to cancel queued jobs in a threadpool
	template <typename NEED_TO_CANCEL>
	void remove_if (NEED_TO_CANCEL need_to_cancel) {
		LOCK_GUARD;

		for (auto it=q.begin(); it!=q.end();) {
			if (need_to_cancel(*it)) {
				it = q.erase(it);
			} else {
				++it;
			}
		}
	}

	void clear () {
		LOCK_GUARD;

		q.clear();
	}

	template <typename COMPARATOR>
	void sort (COMPARATOR cmp) {
		LOCK_GUARD;

		std::sort(q.begin(), q.end(), cmp);
	}
};

#undef MUTEX				
#undef CONDITION_VARIABLE
#undef UNIQUE_LOCK		
#undef LOCK_GUARD		
