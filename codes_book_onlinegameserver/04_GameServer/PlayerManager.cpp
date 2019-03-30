#include "stdafx.h"
#include "PlayerManager.h"

IMPLEMENT_SINGLETON( PlayerManager );

PlayerManager::PlayerManager(void)
{
	m_pPlayer = NULL;
	m_pTempPlayer = NULL;
}

PlayerManager::~PlayerManager(void)
{
	if( NULL != m_pPlayer )
		delete [] m_pPlayer;
	if( NULL != m_pTempPlayer )
		delete [] m_pTempPlayer;
}

bool PlayerManager::CreatePlayer( INITCONFIG &initConfig , DWORD dwMaxPlayer )
{
	m_pPlayer = new Player[ dwMaxPlayer ];
	for( int i=0 ; i< (int)dwMaxPlayer ; i++ )
	{
		initConfig.nIndex = i ;
		if( m_pPlayer[i].CreateConnection( initConfig ) == false )
			return false;
	}
	return true;
}

bool PlayerManager::AddPlayer( Player* pPlayer )
{
	Monitor::Owner lock( m_csPlayer );
	PLAYER_IT player_it;
	player_it = m_mapPlayer.find( pPlayer->GetPKey() );
	//�̹� ���ӵǾ� �ִ� �÷��̾���
	if( player_it != m_mapPlayer.end() )
	{
		LOG( LOG_ERROR_NORMAL , 
			"SYSTEM | PlayerManager::AddPlayer() | PKey(%d)�� �̹� m_mapPlayer�� �ֽ��ϴ�.",
			pPlayer->GetPKey() );
		return false;
	}
	//�÷��̾� ����Ű�Ҵ�
	pPlayer->SetPKey(g_GetIocpGameServer()->GeneratePrivateKey() );
	
	m_mapPlayer.insert( PLAYER_PAIR( pPlayer->GetPKey() , pPlayer ) );
	return true;
}

bool PlayerManager::RemovePlayer( Player* pPlayer )
{
	Monitor::Owner lock( m_csPlayer );
	PLAYER_IT player_it;
	player_it = m_mapPlayer.find( pPlayer->GetPKey() );
	//�ش� �÷��̾ ���� ���
	if( player_it == m_mapPlayer.end() )
	{
		LOG( LOG_ERROR_NORMAL , 
			"SYSTEM | PlayerManager::RemovePlayer() | PKey(%d)�� m_mapPlayer�� �����ϴ�.",
			pPlayer->GetPKey() );
		return false;
	}
	m_mapPlayer.erase( pPlayer->GetPKey() );
	
	g_GetAreaManager()->RemovePlayerFromArea( pPlayer, pPlayer->GetArea() );
	return true;
}
Player* PlayerManager::FindPlayer( DWORD dwPkey )
{
	Monitor::Owner lock( m_csPlayer );
	PLAYER_IT player_it;
	player_it = m_mapPlayer.find( dwPkey );
	//�ش� �÷��̾ ���� ���
	if( player_it == m_mapPlayer.end() )
	{
		LOG( LOG_ERROR_NORMAL , 
			"SYSTEM | PlayerManager::FindPlayer() | PKey(%d)�� m_mapPlayer�� �����ϴ�.",
			dwPkey );
		return NULL;
	}
	return (Player*)player_it->second;
}

void PlayerManager::CreateTempPlayer( int nTempPlayerCnt )
{
	m_pTempPlayer = new Player[ nTempPlayerCnt ];
	for( int i = 0 ; i < nTempPlayerCnt ; i++ )
	{
		m_pTempPlayer[ i ].SetPKey(g_GetIocpGameServer()->GeneratePrivateKey() );
		m_pTempPlayer[ i ].SetTempPlayInfo();
		m_mapTempPlayer.insert( 
			PLAYER_PAIR( m_pTempPlayer[ i ].GetPKey() , &m_pTempPlayer[ i ] ) );
	}
	LOG( LOG_INFO_LOW , 
		"SYSTEM | PlayerManager::DestroyTempPlayer() | ���� �÷��̾� ���� ����!!");
}

void PlayerManager::DestroyTempPlayer()
{
	Monitor::Owner lock( m_csPlayer );
	m_mapTempPlayer.clear();
	delete [] m_pTempPlayer;
	LOG( LOG_INFO_LOW , 
		"SYSTEM | PlayerManager::DestroyTempPlayer() | ���� �÷��̾� �ı� ����!!");
}

void PlayerManager::UpdateTempPlayerPos()
{
	Monitor::Owner lock( m_csPlayer );
	PLAYER_IT player_it;
	for( player_it = m_mapTempPlayer.begin(); player_it != m_mapTempPlayer.end() ; player_it++ )
	{
		Player* pTempPlayer = player_it->second;
		int nPosX = pTempPlayer->GetPos() % COL_LINE;
		int nPosY = pTempPlayer->GetPos() / COL_LINE;
		int nTemp = 0;
		if( rand()%2 )
			nTemp = ( rand() %2 + 1) * -1;
		else
			nTemp = ( rand() %2 + 1) * 1;

		//�������� ������ ������
		int nDir = rand() % 3;
		switch( nDir )
		{
			//����
		case 0:
			{
				nPosX += nTemp;
				//�ʵ带 ������
				if( nPosX <= 0 )
					nPosX += (nTemp * nTemp);
				else if( nPosX >= ROW_LINE )
					nPosX -= (nTemp * nTemp);
			}
			break;
			//����
		case 1:
			{
				nPosY += nTemp;
				if( nPosY <= 0 )
					nPosY += (nTemp * nTemp);
				else if( nPosY >= ROW_LINE )
					nPosY -= (nTemp * nTemp);
			}
			break;
			//�밢��
		case 2:
			{
				nPosX += nTemp;
				nPosY += nTemp;
				if( nPosX <= 0  )
					nPosX += (nTemp * nTemp);
				else if( nPosX >= ROW_LINE )
					nPosX -= (nTemp * nTemp);
				if( nPosY <= 0 )
					nPosY += (nTemp * nTemp);
				else if( nPosY >= ROW_LINE )
					nPosY -= (nTemp * nTemp);
			}
			break;
		}

		pTempPlayer->SetBPos( pTempPlayer->GetPos() );
		pTempPlayer->SetPos( (nPosY * COL_LINE) + nPosX );
		bool bRet = g_GetAreaManager()->TransAreaPlayer( pTempPlayer );
		g_GetAreaManager()->Send_MovePlayerToActiveAreas( pTempPlayer );
		//���� �÷��̾ �ٸ� �������� �̵��Ͽ��ٸ�
		if( true == bRet )
			g_GetAreaManager()->Send_MovePlayerToInActiveAreas( pTempPlayer );
		
	}
}

void PlayerManager::CheckKeepAliveTick( DWORD dwServerTick )
{
	Monitor::Owner lock( m_csPlayer );
	PLAYER_IT player_it;
	for( player_it = m_mapPlayer.begin(); player_it != m_mapPlayer.end() ; player_it++ )
	{
		Player* pPlayer = (Player*)player_it->second;
		if( pPlayer->GetIsConfirm() == false )
			continue;
		//���� ���� ƽ�� �÷��̾ KeepAlive_Cn��Ŷ�� ���� ���� ƽ���� ���̰� KEEPALIVE_TICK����
		//ũ�ٸ� �߸��� ������ �÷��̾��� �Ǵ��ϰ� ������ �����Ѵ�.
		if( (g_GetIocpGameServer()->GetServerTick() - pPlayer->GetKeepAliveTick() ) > KEEPALIVE_TICK )
		{
			g_GetIocpGameServer()->CloseConnection( pPlayer );
			LOG( LOG_INFO_NORMAL ,
				"SYSTEM | PlayerManager::CheckKeepAliveTick() |  PKey(%d) �÷��̾�� ���� �ð����� ��Ŷ�� ���� ����",
				pPlayer->GetPKey() );
		}
	}
}

void PlayerManager::Send_WorldPlayerInfosToConnectPlayer( Player* pPlayer )
{
	Monitor::Owner lock( m_csPlayer );
	PLAYER_IT player_it;
	g_GetVBuffer()->Init();
	g_GetVBuffer()->SetShort( WorldPlayerInfo_VAq );
	short sPlayerCnt = (short)m_mapPlayer.size() + (short)m_mapTempPlayer.size() - 1;
	g_GetVBuffer()->SetShort( sPlayerCnt );
	for( player_it = m_mapPlayer.begin(); player_it != m_mapPlayer.end() ; player_it++ )
	{
		Player* pWorldPlayer = (Player*)player_it->second;
		if( pWorldPlayer == pPlayer )
			continue;
		g_GetVBuffer()->SetInteger( pWorldPlayer->GetPKey() );
		g_GetVBuffer()->SetString( pWorldPlayer->GetId() );
		g_GetVBuffer()->SetString( pWorldPlayer->GetNickName() );
		g_GetVBuffer()->SetString( pWorldPlayer->GetName() );
		g_GetVBuffer()->SetInteger( pWorldPlayer->GetPos() );
		g_GetVBuffer()->SetChar( pWorldPlayer->GetLevel() );
		g_GetVBuffer()->SetChar( pWorldPlayer->GetStr() );
		g_GetVBuffer()->SetChar( pWorldPlayer->GetDur() );
		g_GetVBuffer()->SetInteger( pWorldPlayer->GetHp() );
		g_GetVBuffer()->SetInteger( pWorldPlayer->GetExp() );
	}
	for( player_it = m_mapTempPlayer.begin(); player_it != m_mapTempPlayer.end() ; player_it++ )
	{
		Player* pTempPlayer = (Player*)player_it->second;
		g_GetVBuffer()->SetInteger( pTempPlayer->GetPKey() );
		g_GetVBuffer()->SetString( pTempPlayer->GetId() );
		g_GetVBuffer()->SetString( pTempPlayer->GetNickName() );
		g_GetVBuffer()->SetString( pTempPlayer->GetName() );
		g_GetVBuffer()->SetInteger( pTempPlayer->GetPos() );
		g_GetVBuffer()->SetChar( pTempPlayer->GetLevel() );
		g_GetVBuffer()->SetChar( pTempPlayer->GetStr() );
		g_GetVBuffer()->SetChar( pTempPlayer->GetDur() );
		g_GetVBuffer()->SetInteger( pTempPlayer->GetHp() );
		g_GetVBuffer()->SetInteger( pTempPlayer->GetExp() );
	}
	char* pWorldPlayerInfos = pPlayer->PrepareSendPacket(g_GetVBuffer()->GetCurBufSize() );
	if( NULL == pWorldPlayerInfos )
		return;
	g_GetVBuffer()->CopyBuffer( pWorldPlayerInfos );
	pPlayer->SendPost(g_GetVBuffer()->GetCurBufSize() );
}

void PlayerManager::Send_LoginPlayer( Player* pPlayer )
{
	Monitor::Owner lock( m_csPlayer );
	PLAYER_IT player_it;
	for( player_it = m_mapPlayer.begin(); player_it != m_mapPlayer.end() ; player_it++ )
	{
		Player* pWorldPlayer = (Player*)player_it->second;
		if( pWorldPlayer == pPlayer )
			continue;
		LoginPlayerSn* pLogin = 
			(LoginPlayerSn*)pWorldPlayer->PrepareSendPacket( sizeof(LoginPlayerSn) );
		if( NULL == pLogin )
			continue;
		pLogin->s_sType = LoginPlayer_Sn;
		pLogin->s_byDur = pPlayer->GetDur();
		pLogin->s_byLevel = pPlayer->GetLevel();
		pLogin->s_byStr = pPlayer->GetStr();
		pLogin->s_dwExp = pPlayer->GetExp();
		pLogin->s_dwHp = pPlayer->GetHp();
		pLogin->s_dwPKey = pPlayer->GetPKey();
		pLogin->s_dwPos = pPlayer->GetPos();
		strncpy( pLogin->s_szId , pPlayer->GetId() , MAX_ID_LENGTH );
		strncpy( pLogin->s_szName , pPlayer->GetName() , MAX_NAME_LENGTH );
		strncpy( pLogin->s_szNickName , pPlayer->GetNickName() , MAX_NICKNAME_LENGTH );
		pWorldPlayer->SendPost( sizeof(LoginPlayerSn) );
	}
}

void PlayerManager::Send_LogoutPlayer( Player* pPlayer )
{
	Monitor::Owner lock( m_csPlayer );
	PLAYER_IT player_it;
	for( player_it = m_mapPlayer.begin(); player_it != m_mapPlayer.end() ; player_it++ )
	{
		Player* pWorldPlayer = (Player*)player_it->second;
		if( pWorldPlayer == pPlayer )
			continue;
		LogoutPlayerSn* pLogout = 
			(LogoutPlayerSn*)pWorldPlayer->PrepareSendPacket( sizeof(LogoutPlayerSn) );
		if( NULL == pLogout )
			continue;
		pLogout->s_dwPKey = pPlayer->GetPKey();
		pLogout->s_sType = LogoutPlayer_Sn;
		pWorldPlayer->SendPost( sizeof(LogoutPlayerSn) );
	}
}

void PlayerManager::Send_MovePlayer( Player* pPlayer )
{
	Monitor::Owner lock( m_csPlayer );
	PLAYER_IT player_it;
	for( player_it = m_mapPlayer.begin(); player_it != m_mapPlayer.end() ; player_it++ )
	{
		Player* pWorldPlayer = (Player*)player_it->second;
		if( pWorldPlayer == pPlayer )
			continue;
		MovePlayerSn* pMove = 
			(MovePlayerSn*)pWorldPlayer->PrepareSendPacket( sizeof(MovePlayerSn) );
		if( NULL == pMove )
			continue;
		pMove->s_dwPKey = pPlayer->GetPKey();
		pMove->s_sType = MovePlayer_Sn;
		pMove->s_dwCPos = pPlayer->GetBPos();
		pMove->s_dwTPos = pPlayer->GetPos();
		pWorldPlayer->SendPost( sizeof(MovePlayerSn) );
	}
}
void PlayerManager::Send_RecvBufferFromNpcServer( char* pRecvBuffer , DWORD dwSize )
{
	Monitor::Owner lock( m_csPlayer );
	PLAYER_IT player_it;
	for( player_it = m_mapPlayer.begin(); player_it != m_mapPlayer.end() ; player_it++ )
	{
		Player* pWorldPlayer = (Player*)player_it->second;
		char* pSendBuffer = pWorldPlayer->PrepareSendPacket( dwSize );
		if( NULL == pSendBuffer )
			continue;
		CopyMemory( pSendBuffer , pRecvBuffer , dwSize );
		pWorldPlayer->SendPost( dwSize );
	}
}

