#include "pch.h"
#include "ThreadPool.h"

bool ThreadPool::Init() noexcept
{
	mMaxWorkerThreadCount = std::thread::hardware_concurrency();

	m_stopAll.reset();

	// For Main-Thread
	InitTLS();

	for(unsigned char i = 0; i < mMaxWorkerThreadCount; ++i)
		m_threads.emplace_back([this]() { InitTLS();  Work(); DestroyTLS(); });

#ifdef _DEBUG
	printf_s(std::format("Thread Pool Init").c_str());
#endif // _DEBUG

	return true;
}

void ThreadPool::Join() noexcept
{
	m_stopAll.set();
	m_cv.notify_all();
	// For Main-Thread
	DestroyTLS();
	m_threads.clear();
#ifdef _DEBUG
	printf_s(std::format("Thread Pool Join, ThreadCount = {}", m_threads.size()).c_str());
#endif
}

void ThreadPool::InitTLS() noexcept
{
	static atomic<unsigned int> id;
	TLS_ThreadID = id.fetch_add(1);
#ifdef  _DEBUG
	printf_s(std::format("Init Thread:{}\n", TLS_ThreadID).c_str());
#endif //  _DEBUG
}

void ThreadPool::DestroyTLS() noexcept
{
}

void ThreadPool::Work() noexcept
{
	// 일감이 들어오지 않으면, 쿨쿨 잔다.
	while(true) {
		unique_lock<mutex> lock(m_mutex);

		// 일감이 생겼거나, stopAll켜지면 빠져나온다
		m_cv.wait(lock, [this]() { return !m_tasks.empty() || m_stopAll[0]; });

		if(m_stopAll[0] && m_tasks.empty())
			return;
		auto task = move(m_tasks.front());
		m_tasks.pop();
		lock.unlock();
		task();
	}
}
