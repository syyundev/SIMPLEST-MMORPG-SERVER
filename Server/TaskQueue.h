#pragma once

#include "Singleton.hpp"

struct Task {
	int												objID;
	std::chrono::high_resolution_clock::time_point	timeAfter;
	EVENT_TYPE										taskType;
	int												targetObjID;

	constexpr bool operator < (const Task& _Left) const { return (timeAfter > _Left.timeAfter); }
};

class TaskQueue : public Singleton<TaskQueue> {
	SINGLETON(TaskQueue)
public:
	HANDLE						m_iocpHandle;
	concurrency::concurrent_priority_queue<Task> m_taskQueue;
	std::atomic_bool			m_flag;

public:
	void Init(HANDLE iocpHandle) noexcept;
	void AddTask(const Task& task)noexcept;
	void SetFlag(bool flag) { m_flag = flag; }

public:
	void ProcessTask() noexcept;
};

