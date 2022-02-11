#include "CTaskQueue.h"
#include "CTaskQueueImpl.h"

CTaskQueue::CTaskQueue()
{
	m_queueLock = CreateMutex(NULL, FALSE, NULL);
}

CTaskQueue::~CTaskQueue()
{
	if (NULL != m_queueLock)
	{
		ReleaseMutex(m_queueLock);
		CloseHandle(m_queueLock);
		m_queueLock = NULL;
	}
}

int CTaskQueue::AddTask(CTask* pTask)
{
	Lock();
	int result = _AddTask(pTask);
	UnLock();

	return result;
}

CTaskQueue* CTaskQueue::Instance()
{
	static CTaskQueueImpl taskQueue;
	return &taskQueue;
}

int CTaskQueue::Lock()
{
	if (NULL != m_queueLock)
	{
		WaitForSingleObject(m_queueLock, INFINITE);
	}

	return 0;
}

int CTaskQueue::UnLock()
{
	if (NULL != m_queueLock)
	{
		ReleaseMutex(m_queueLock);
	}

	return 0;
}

int CTaskQueue::Run()
{
	CTask* pTask = NULL;

	while (IsRunning())
	{
		Lock();
		if (m_queue.empty())
		{
			UnLock();
			Sleep(1);
			continue;
		}

		pTask = _GetTask();
		UnLock();

		OnTask(pTask);

		delete pTask;
		pTask = NULL;
	}

	Clear();

	return 0;
}

int CTaskQueue::_AddTask(CTask* pTask)
{
	if (NULL != pTask)
		m_queue.push(pTask);

	return 0;
}

CTask* CTaskQueue::_GetTask()
{
	CTask* pTask = m_queue.front();
	m_queue.pop();

	return pTask;
}

void CTaskQueue::Clear()
{
	Lock();

	while (!m_queue.empty())
	{
		CTask* pTask = m_queue.front();
		m_queue.pop();

		delete pTask;
		pTask = NULL;
	}

	UnLock();
}
