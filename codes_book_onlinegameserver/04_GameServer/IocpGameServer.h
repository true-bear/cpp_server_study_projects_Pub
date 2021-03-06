#pragma once
#include "../NetLib/IocpServer.h"
#include "TickThread.h"
#include "Player.h"

enum eGameServerSystemMsg{
	SYSTEM_TEMPMSG,
	SYSTEM_UPDATE_TEMPPLAYERPOS,
};

class IocpGameServer : public IOCPServer, Singleton
{
	DECLEAR_SINGLETON( IocpGameServer );

public:
	IocpGameServer(void);
	~IocpGameServer(void);

	union FuncProcess
	{
		void (*funcProcessPacket)( Player* pPlayer,  DWORD dwSize , char* pRecvedMsg );

		FuncProcess()
		{
			funcProcessPacket = NULL;
		}
	};

	//client가 접속 수락이 되었을 때 호출되는 함수
	virtual	bool	OnAccept( Connection *lpConnection );
	//client에서 packet이 도착했을 때 순서 성 있게 처리되어지는 패킷처리
	virtual	bool	OnRecv(Connection* lpConnection,  DWORD dwSize , char* pRecvedMsg);
	//client에서 packet이 도착했을 때 순서 성 없이 곧바로 처리 되는 패킷처리
	virtual	bool	OnRecvImmediately(Connection* lpConnection,  DWORD dwSize , char* pRecvedMsg);
	//client와 연결이 종료되었을 때 호출되는 함수
	virtual	void	OnClose(Connection* lpConnection);
	
	virtual bool	OnSystemMsg( Connection* lpConnection , DWORD dwMsgType , LPARAM lParam );

	//서버 시작 함수
	virtual bool	ServerStart();

	bool			ConnectToNpcServer();

	//.ini화일로 부터 문자열을 읽어온다.
	int				GetINIString( char* szOutStr , char* szAppName , char* szKey , int nSize , char* szFileName );
	//.ini화일로 부터 숫자를 읽어온다.
	int				GetINIInt( char* szAppName , char* szKey , char* szFileName );

	//틱쓰레드 포인터 반환
	inline TickThread*     GetTickThread() { return m_pTickThread; }
	inline DWORD			GetServerTick() { return m_pTickThread->GetTickCount(); }
	//패킷처리 함수 설정
	void			InitProcessFunc();

	void			ProcessSystemMsg( Player* pPlayer , DWORD dwMsgType , LPARAM lParam );

	//고유한 키를 얻어낸다.
	DWORD			GeneratePrivateKey() { return ++m_dwPrivateKey; }

	inline Connection* GetNpcServerConn() { return m_pNpcServerConn; }

private:
	char m_szLogFileName[ MAX_LOGFILENAME_LENGTH ];
	//틱 쓰레드
	TickThread*		m_pTickThread;
	//패킷 처리함수 포인터
	FuncProcess			m_FuncProcess[ MAX_PROCESSFUNC ];
	DWORD				m_dwPrivateKey;
	Connection*		m_pNpcServerConn;
};

CREATE_FUNCTION( IocpGameServer , g_GetIocpGameServer );
