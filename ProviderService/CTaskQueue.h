#pragma once
#include <windows.h>
#include <queue>

class CTask
{
public:
	virtual ~CTask() = 0 {};

	virtual int Execute() = 0;
	virtual int Cancel() = 0;
};

class CTaskQueue
{
public:
	CTaskQueue();
	virtual ~CTaskQueue();

	int AddTask(CTask* pTask);

	virtual int OnTask(CTask* pTask) = 0;
	virtual BOOL IsRunning() = 0;

	static CTaskQueue* Instance();
protected:
	int Lock();
	int UnLock();

	int Run();

	int _AddTask(CTask* pTask);
	CTask* _GetTask();

	void Clear();
private:
	std::queue<CTask*> m_queue;
	HANDLE m_queueLock;
};

