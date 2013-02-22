// Serial.cpp: implementation of the CSerial class.
//
//////////////////////////////////////////////////////////////////////

#include "Serial.h"

CSerial::CSerial()
	{
    m_com_port = 0;
	m_comfile = INVALID_HANDLE_VALUE;
	ZeroMemory(&m_ovlread,sizeof(OVERLAPPED));
	ZeroMemory(&m_ovlwrite,sizeof(OVERLAPPED));
	m_CommThread = 0; m_ThreadStart = m_ThreadStop = 0;
	}

BOOL CSerial::GetState(DCB& dcb)
	{
	if(m_comfile == INVALID_HANDLE_VALUE) return FALSE;
	ZeroMemory(&dcb,sizeof(DCB)); dcb.DCBlength = sizeof(DCB);
	return GetCommState(m_comfile,&dcb);
	}

BOOL CSerial::SetState(DCB& dcb)
	{
	if(m_comfile == INVALID_HANDLE_VALUE) return FALSE;
	return SetCommState(m_comfile,&dcb);
	}

BOOL CSerial::SetParam(int baudrate, int bytesize/*=8*/,
						int parity/*=NOPARITY*/, int stopbits/*=ONESTOPBIT*/,
						int dtrcontrol/*=DTR_CONTROL_DISABLE*/,
						int rtscontrol/*=RTS_CONTROL_DISABLE*/)
	{
	DCB		dcb;

	if(!GetState(dcb)) return FALSE;
	dcb.BaudRate = baudrate;
	dcb.ByteSize = bytesize;
	dcb.Parity = parity;
	dcb.StopBits = stopbits;
	dcb.fDtrControl = dtrcontrol;
	dcb.fRtsControl = rtscontrol;
	return SetState(dcb);
	}

BOOL CSerial::Init(int port_number, int baudrate/*=115200*/)
	{
    wchar_t	txt[32];
	DWORD	tid;

    Cleanup();
	try
		{
        wsprintf(txt,L"\\\\.\\COM%d",port_number);
		if((m_comfile = CreateFile(txt,GENERIC_READ|GENERIC_WRITE,
			0,0,OPEN_EXISTING,FILE_FLAG_OVERLAPPED,0)) == INVALID_HANDLE_VALUE)
				throw 0;
		m_com_port = port_number;
		if(!SetParam(baudrate)) throw 0;
		if(!(m_ovlread.hEvent = CreateEvent(0,1,0,0))) throw 0;
		if(!(m_ovlwrite.hEvent = CreateEvent(0,1,0,0))) throw 0;
		if(!(m_ThreadStart = CreateEvent(0,0,0,0))) throw 0;
		if(!(m_ThreadStop = CreateEvent(0,0,0,0))) throw 0;
		if(!(m_CommThread = CreateThread(0,0,CommProc,
			(LPVOID)this,0,&tid))) throw 0;
		// wait max 5 sec for thread to start
		if(WaitForSingleObject(m_ThreadStart,5000) != WAIT_OBJECT_0) throw 0;
		}
	catch(...) {return FALSE;}
	return TRUE;
	}

DWORD WINAPI CSerial::CommProc(LPVOID lpParameter)
	{
	CSerial	*sp=(CSerial*)lpParameter;
	HANDLE	el[2]={sp->m_ovlread.hEvent,sp->m_ThreadStop};
	DWORD	len;
	char	buf;

	::PurgeComm(sp->m_comfile,PURGE_RXABORT|PURGE_RXCLEAR);
	SetEvent(sp->m_ThreadStart);	// notify class: thread is running
	while(1)
		{
		if(!ReadFile(sp->m_comfile,&buf,1,&len,&sp->m_ovlread))
			{
			if(GetLastError() != ERROR_IO_PENDING)
				{
				COMSTAT		stat;
				DWORD		status;
				ClearCommError(sp->m_comfile,&status,&stat);
				continue;
				}
			if(WaitForMultipleObjects(2,el,FALSE,INFINITE) == WAIT_OBJECT_0 + 1)
				break;
			if(!GetOverlappedResult(sp->m_comfile,&sp->m_ovlread,&len,0))
				continue;
			}
        if(len == 1) emit sp->InChar(buf);
		}
	SetEvent(sp->m_ThreadStart);	// notify app: thread is terminating
	return 0;
	}

BOOL CSerial::Transmitt(const char *txt, int len)
	{
	DWORD	siz;

	if(m_com_port && m_comfile != INVALID_HANDLE_VALUE)
		{
		if(len < 0) len = strlen(txt);
		if(!WriteFile(m_comfile,txt,len,&siz,&m_ovlwrite))
			{
			if(GetLastError() != ERROR_IO_PENDING)
				{m_com_port = 0; return FALSE;}
			}
		if(WaitForSingleObject(m_ovlwrite.hEvent,3000) != WAIT_OBJECT_0)
			{m_com_port = 0; return FALSE;}
		}
	return TRUE;
	}

void CSerial::Cleanup()
	{
	if(m_CommThread)
		{
		SetEvent(m_ThreadStop); Sleep(0);	// signal thread to terminate
		if(WaitForSingleObject(m_ThreadStart,1000) != WAIT_OBJECT_0)
			CloseHandle(m_CommThread);
		m_CommThread = 0;
		}
	if(m_ThreadStop) {CloseHandle(m_ThreadStop); m_ThreadStop = 0;}
	if(m_ThreadStart) {CloseHandle(m_ThreadStart); m_ThreadStart = 0;}
	if(m_comfile != INVALID_HANDLE_VALUE)
		{CloseHandle(m_comfile); m_comfile = INVALID_HANDLE_VALUE;}
	if(m_ovlwrite.hEvent)
		{
		CloseHandle(m_ovlwrite.hEvent);
		ZeroMemory(&m_ovlwrite,sizeof(OVERLAPPED));
		}
	if(m_ovlread.hEvent)
		{
		CloseHandle(m_ovlread.hEvent);
		ZeroMemory(&m_ovlread,sizeof(OVERLAPPED));
		}
    m_com_port = 0;
	}

CSerial::~CSerial()
	{
	Cleanup();
	}
