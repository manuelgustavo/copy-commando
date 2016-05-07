#pragma once

#pragma once

#include <thread>
#include <cassert>
#include <condition_variable>
#include <atomic>
#include "trace.h"

namespace thread_tools {
	// will notify an event once when notify() is called and when the class is destroyed.
	class event_notifier {
	public:
		event_notifier(std::mutex* pMutex, std::condition_variable* cv) : m_p_mutex{ pMutex }, m_p_cv{ cv } {
			assert(pMutex && cv);
		}

		~event_notifier() {
			notify();
		}

		void notify() {
			if (!m_notified && m_p_mutex && m_p_cv) {
				{
					std::lock_guard<std::mutex> lk(*m_p_mutex); // guarantees the requestor will be waiting for a response
					m_notified = true;
				}
				m_p_cv->notify_all(); // notify request
			}
		}

		void reset() {
			std::lock_guard<std::mutex> lk(*m_p_mutex);
			m_notified = false;
		}

	private:
		std::mutex* m_p_mutex;
		std::condition_variable* m_p_cv;
		bool m_notified{ false };
	};

	class thread_wrapper {
	public:
		virtual ~thread_wrapper(void) {
			die();
		}

		virtual std::shared_ptr<std::thread> run() {
			TRACE("starting thread\n");
			std::unique_lock<std::mutex> lk(m_mutex_started);
			m_thread.reset(new std::thread(std::ref(*this)));
			m_cv_started.wait(lk); // wait for response event
			TRACE("thread 0x%x started!\n", m_thread->get_id());
			return m_thread;
		}

		virtual void die() {
#ifdef _DEBUG
			std::thread::id thread_id;// = m_thread->get_id();
			if (m_thread != nullptr) {
				thread_id = m_thread->get_id();
				TRACE("stopping thread 0x%x\n", thread_id);
			} else {
				TRACE("m_thread is null. Means the thread never started.\n");
			}
#endif _DEBUG
			m_stop_now.store(true, std::memory_order_release);

			if (m_thread != nullptr && m_thread->joinable()) {
				m_thread->join();
			}
			m_is_running.store(false);

#ifdef _DEBUG
			if (m_thread != nullptr)
				TRACE("thread 0x%x stopped!\n", thread_id);
#endif _DEBUG
		}
		virtual void operator ()() = 0;

		virtual bool isRunning() {
			return m_is_running.load(std::memory_order_acquire);
		}

	protected:
		inline void notify_started() {
			event_notifier startup_notify(&m_mutex_started, &m_cv_started);

			m_is_running.store(true, std::memory_order_release);
			startup_notify.notify();
		}

		std::mutex m_mutex_started;
		std::condition_variable m_cv_started;

		std::shared_ptr<std::thread> m_thread;
		std::atomic<bool> m_stop_now{ false };
		std::mutex m_mutex_stop_now;
		std::atomic<bool> m_is_running{ false };
	};
} //End of namespace safestore

