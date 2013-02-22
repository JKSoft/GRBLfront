// Serial.h: interface for the CSerial class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SERIAL_H__0F61E6F9_22A5_445A_AB8A_7073245AEC68__INCLUDED_)
#define AFX_SERIAL_H__0F61E6F9_22A5_445A_AB8A_7073245AEC68__INCLUDED_

#pragma once

#include <windows.h>
#include <QObject.h>

class CSerial : public QObject
	{
    Q_OBJECT

    OVERLAPPED		m_ovlread,m_ovlwrite;
	HANDLE			m_ThreadStart,m_ThreadStop;
	HANDLE			m_CommThread;

	static DWORD WINAPI CommProc(LPVOID lpParameter);

	public:
	int				m_com_port;
	HANDLE			m_comfile;

					CSerial();
	BOOL			Init(int port_number, int baudrate=115200);
	BOOL			SetParam(int baudrate, int bytesize=8,
							int parity=NOPARITY, int stopbits=ONESTOPBIT,
							int dtrcontrol=DTR_CONTROL_DISABLE,
							int rtscontrol=RTS_CONTROL_DISABLE);
	BOOL			GetState(DCB& dcb);	
	BOOL			SetState(DCB& dcb);	
	BOOL			Transmitt(const char *txt, int len=-1);
	BOOL			PurgeComm(DWORD flags) {return ::PurgeComm(m_comfile,flags);}
	void			Cleanup();
	virtual			~CSerial();

    signals:
    void            InChar(int chr);
    };

#endif // !defined(AFX_SERIAL_H__0F61E6F9_22A5_445A_AB8A_7073245AEC68__INCLUDED_)
