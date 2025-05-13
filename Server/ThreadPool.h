#pragma once
#include "Singleton.hpp"

class ThreadPool : public Singleton<ThreadPool> {
	SINGLETON(ThreadPool)
public:

private:
	int mMaxWorkerThreadCount;
	std::mutex		m_mutex;
	vector<jthread> m_threads;
	bitset<1>		m_stopAll;
	queue<function<void()>> m_tasks;
	condition_variable m_cv;

public:
	bool Init() noexcept;
	void Join() noexcept;
	int GetMaxWorkerThreadCount() const noexcept { return mMaxWorkerThreadCount; }

public:
	template<typename Func, typename... Args>
	auto EnqueueJob(Func&& func, Args&&... args) -> future<invoke_result_t<Func, Args...>>
	{
		using ReturnType = invoke_result_t<Func, Args...>;

		if(m_stopAll.test(0))
			throw runtime_error("ThreadManager Stoped");

		auto task = make_shared<packaged_task<ReturnType()>>(bind(forward<Func>(func), forward<Args>(args)...));

		future<ReturnType> ret = task->get_future();
		{
			lock_guard<mutex> lock(m_mutex);
			m_tasks.push([task]() { (*task)(); });
		}

		m_cv.notify_one();
		return ret;
	}

public:
	// TLS 초기화 할 것들
	static void InitTLS() noexcept;
	// TLS 삭제 될 때 해야 할 일들.
	static void DestroyTLS() noexcept;

private:
	void Work() noexcept;

};

