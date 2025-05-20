#include "pch.h"
#include "TaskQueue.h"

#include "ServerObjectManager.h"
#include "Monster.h"	
#include "IOContext.h"

void TaskQueue::AddTask(const Task& task)noexcept
{
	lock_guard<mutex> lk{ m_mutex };
	m_taskQueue.push(task);
}

void TaskQueue::DoTask()
{
	using namespace chrono;
	do {
		do {

			m_mutex.lock();
			if(m_taskQueue.empty()) {
				m_mutex.unlock();
				break;
			}

			auto& task = m_taskQueue.top();
			if(task.timeAfter > std::chrono::high_resolution_clock::now()) {
				m_mutex.unlock();
				break;
			}
			m_mutex.unlock();

			switch(task.taskType) {
				case TASK_TYPE::MOVE:
				{
					EventContext* context = new EventContext;
					context->type = task.taskType;
					PostQueuedCompletionStatus(m_iocpHandle, 1, task.objID, context);
					break;
				}
				default:
					break;
			}

			{
				lock_guard<mutex> lk{ m_mutex };
				m_taskQueue.pop();
			}
		} while(m_flag);
		this_thread::sleep_for(chrono::milliseconds(10));
	} while(m_flag);
}
