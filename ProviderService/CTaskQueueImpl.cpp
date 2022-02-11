#include "CTaskQueueImpl.h"
#include "ThreadImpl.h"

CTaskQueueImpl::CTaskQueueImpl()
{
	m_bRunning = FALSE;
	m_thread = new CThreadImpl(ThreadProc, this);
}

CTaskQueueImpl::~CTaskQueueImpl()
{
	if (NULL != m_thread)
	{
		delete m_thread;
		m_thread = NULL;
	}
}

void CTaskQueueImpl::Start()
{
	m_bRunning = TRUE;
	m_thread->Start();
}

void CTaskQueueImpl::Stop()
{
	m_bRunning = FALSE;
	m_thread->Stop();
}

int CTaskQueueImpl::OnTask(CTask* pTask)
{
	pTask->Execute();

	return 0;
}

BOOL CTaskQueueImpl::IsRunning()
{
	return m_bRunning;
}

void CTaskQueueImpl::ThreadProc(void* pParam)
{
	CTaskQueueImpl* pThis = (CTaskQueueImpl*)pParam;
	pThis->Run();
}
