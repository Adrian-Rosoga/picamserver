#pragma once

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>
#include <iostream>

/*
 * function_wrapper from "C++ Concurrency in Action", Anthony Williams
 * ©2012 by Manning Publications Co. All rights reserved.
 */
class function_wrapper
{
	struct impl_base {
		virtual void call() = 0;
		virtual ~impl_base() {}
	};
	std::unique_ptr<impl_base> impl;
	template<typename F>
	struct impl_type : impl_base
	{
		F f;
		impl_type(F&& f_) : f(std::move(f_)) {}
		void call() { f(); }
	};
public:
	template<typename F>
	function_wrapper(F&& f) :
		impl(new impl_type<F>(std::move(f)))
	{}

	function_wrapper() {}

	void operator()() { impl->call(); }

	function_wrapper(function_wrapper&& other) :
		impl(std::move(other.impl))
	{}

	function_wrapper& operator=(function_wrapper&& other)
	{
		impl = std::move(other.impl);
		return *this;
	}

	function_wrapper(const function_wrapper&) = delete;
	function_wrapper(function_wrapper&) = delete;
	function_wrapper& operator=(const function_wrapper&) = delete;
};


class ThreadPool {
public:
	explicit ThreadPool(size_t);
	template<class F, class... Args>
	void enqueue(F&& f, Args&&... args);
	~ThreadPool();
private:
	// need to keep track of threads so we can join them
	std::vector< std::thread > workers;
	// the task queue
	std::queue< function_wrapper > tasks;

	// synchronization
	std::mutex queue_mutex;
	std::condition_variable condition;
	bool stop;
};

// the constructor just launches some amount of workers
inline ThreadPool::ThreadPool(size_t threads)
	: stop(false)
{
	for (size_t i = 0; i<threads; ++i)
		workers.emplace_back( [this]
	{
		for (;;)
		{
			function_wrapper task;
			{
				std::unique_lock<std::mutex> lock(this->queue_mutex);
				this->condition.wait(lock,
					[this] { return this->stop || !this->tasks.empty(); });
				if (this->stop && this->tasks.empty())
					return;
				task = std::move(this->tasks.front());
				this->tasks.pop();
			}
			task();
		}
	}
	);
}

// add new work item to the pool
template<class F, class... Args>
void ThreadPool::enqueue(F&& f, Args&&... args)
{
	auto task(std::packaged_task<void()>(
		std::bind(std::forward<F>(f), std::forward<Args>(args)...)));

	{
		std::unique_lock<std::mutex> lock(queue_mutex);

		// don't allow enqueueing after stopping the pool
		if (stop)
			throw std::runtime_error("enqueue on stopped ThreadPool");

		tasks.emplace(std::move(task));
	}
	condition.notify_one();
    std::cout << "ThreadPool queue_size=" << tasks.size() << std::endl;
}

// the destructor joins all threads
inline ThreadPool::~ThreadPool()
{
	{
		std::unique_lock<std::mutex> lock(queue_mutex);
		stop = true;
	}
	condition.notify_all();
	for (std::thread &worker : workers)
		worker.join();
}
