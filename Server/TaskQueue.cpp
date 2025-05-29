#include "pch.h"
#include "TaskQueue.h"
#include "ServerObjectManager.h"
#include "Monster.h"	
#include "IOContext.h"

void TaskQueue::Init(HANDLE iocpHandle) noexcept
{
	m_iocpHandle = iocpHandle;
	m_flag = true;

}

void TaskQueue::AddTask(const Task& task)noexcept
{
	m_taskQueue.push(task);
}

void TaskQueue::ProcessTask() noexcept
{
	do {
		do {
			Task task;
			if(false == m_taskQueue.try_pop(task)) {
				break;
			}
			else {
				if(task.timeAfter > std::chrono::high_resolution_clock::now()) {
					m_taskQueue.push(task);
					break;
				}

				switch(const auto type = task.taskType) {
					case EVENT_TYPE::HELLO:
					{
						#ifdef AI_LUA
						EventContext* context = new EventContext;
						context->type = task.taskType;
						context->ai_target_obj = task.targetObjID;
						PostQueuedCompletionStatus(m_iocpHandle, 1, task.objID/*몬스터 아이디*/, context);
						#endif
						break;
					}
					case EVENT_TYPE::MOVE:
					{
						EventContext* context = new EventContext;
						context->type = task.taskType;
						PostQueuedCompletionStatus(m_iocpHandle, 1, task.objID, context);
						break;
					}
					case EVENT_TYPE::ATTACK:
					{
						EventContext* context = new EventContext;
						context->type = task.taskType;
						PostQueuedCompletionStatus(m_iocpHandle, 1, task.targetObjID, context);
						break;
					}
					case EVENT_TYPE::HEAL:
					{
						EventContext* context = new EventContext;
						context->type = task.taskType;
						PostQueuedCompletionStatus(m_iocpHandle, 1, task.objID, context);
						break;
					}
					case EVENT_TYPE::REVIVE:
					{
						EventContext* context = new EventContext;
						context->type = task.taskType;
						PostQueuedCompletionStatus(m_iocpHandle, 1, task.objID, context);
						break;
					}
					default:
						break;
				}
			}
		} while(m_flag);
		this_thread::sleep_for(chrono::milliseconds(10));
	} while(m_flag);
}
