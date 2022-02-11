#pragma once
#include "CTaskQueue.h"
#include "IThread.h"

class CTaskQueueImpl : public CTaskQueue
{
public:
	CTaskQueueImpl();
	virtual ~CTaskQueueImpl();

	void Start();
	void Stop();

	virtual int OnTask(CTask* pTask) override;
	virtual BOOL IsRunning() override;
private:
	static void ThreadProc(void* pParam);
private:
	IThread* m_thread;
	volatile BOOL m_bRunning;
};

