#include "cbase.h"
#include "filesystem.h"
#include "tier0/threadtools.h"
#include "econ_networking.h"
#include "econ_networking_messages.h"
#include "tier1/smartptr.h"
#include "tier1/utlqueue.h"
#include "inetchannel.h"
#include "steam/steamtypes.h"
#include "steam/steam_api.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


ConVar net_steamcnx_debug( "net_steamcnx_debug", IsDebug() ? "1" : "0", FCVAR_CHEAT|FCVAR_REPLICATED, "Show debug spew for steam based connections, 2 shows all network traffic for steam sockets." );
static ConVar net_steamcnx_allowrelay( "net_steamcnx_allowrelay", "0", FCVAR_CHEAT|FCVAR_REPLICATED, "Allow steam connections to attempt to use relay servers as fallback (best if specified on command line:  +net_steamcnx_allowrelay 1)" );
static ConVar net_steamcnx_usep2p( "net_steamcnx_usep2p", "1", FCVAR_DEVELOPMENTONLY|FCVAR_REPLICATED );

#define STEAM_CNX_COLOR				Color(255,255,100,255)
#define STEAM_CNX_PROTO_VERSION		3

CUtlMap<MsgType_t, IMessageHandler *> &MessageMap( void );


class CEconNetworking : public IEconNetworking
{
public:
	CEconNetworking();
	virtual ~CEconNetworking();

	virtual bool Init( void );
	virtual void Shutdown( void );

#if defined(GAME_DLL)
	void OnClientConnected( CSteamID const &identity ) OVERRIDE;
	void OnClientDisconnected( CSteamID const &steamID ) OVERRIDE;
#endif

	bool SendMessage( CSteamID const &targetID, MsgType_t eMsg, void *pubData, uint32 cubData ) OVERRIDE;
	void RecvMessage( CSteamID const &remoteID, MsgType_t eMsg, void const *pubData, uint32 const cubData ) OVERRIDE;

	virtual void Update( float frametime );

private:
	void HandleNetPacket( CSmartPtr<CNetPacket> const &pPacket )
	{
		IMessageHandler *pHandler = NULL;
		unsigned nIndex = MessageMap().Find(pPacket->Hdr().m_eMsgType);
		if ( nIndex != MessageMap().InvalidIndex() )
			pHandler = MessageMap()[nIndex];

		QueueEconNetworkMessageWork( pHandler, pPacket );
	}

	void SetHeaderSteamIDs( CSmartPtr<CNetPacket> &pPacket, CSteamID const &targetID )
	{
		pPacket->m_Hdr.m_ulTargetID = targetID.ConvertToUint64();

	#if defined GAME_DLL
		CSteamID const *pSteamID = engine->GetGameServerSteamID();
		pPacket->m_Hdr.m_ulSourceID = pSteamID ? pSteamID->ConvertToUint64() : 0LL;
	#else
		CSteamID steamID;
		if ( steamapicontext->SteamUser() )
			steamID = steamapicontext->SteamUser()->GetSteamID();
		pPacket->m_Hdr.m_ulSourceID = steamID.ConvertToUint64();
	#endif
	}

	CUtlQueue< CSmartPtr<CNetPacket> >		m_QueuedMessages;
};

//-----------------------------------------------------------------------------
static CEconNetworking g_Networking;
IEconNetworking *g_pNetworking = &g_Networking;

//-----------------------------------------------------------------------------
DEFINE_FIXEDSIZE_ALLOCATOR_MT( CNetPacket, 128, UTLMEMORYPOOL_GROW_FAST );
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNetPacket::Init( uint32 size, MsgType_t eMsg )
{
	m_Hdr = {eMsg, size};

	m_pMsg = malloc( size + sizeof( MsgHdr_t ) );
	Q_memcpy( m_pMsg, &m_Hdr, sizeof( MsgHdr_t ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNetPacket::InitFromMemory( void const *pMemory, uint32 size )
{
	Assert( pMemory );

	m_pMsg = malloc( size );
	Q_memcpy( m_pMsg, pMemory, size );

	Q_memcpy( &m_Hdr, m_pMsg, sizeof( MsgHdr_t ) );

	Assert( ( m_Hdr.m_unMsgSize + sizeof( MsgHdr_t ) ) == size );
}

//-----------------------------------------------------------------------------
// Purpose: Ref count
//-----------------------------------------------------------------------------
int CNetPacket::AddRef( void )
{
	return ThreadInterlockedIncrement( &m_cRefCount );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CNetPacket::Release( void )
{
	Assert( m_cRefCount > 0 );
	int nRefCounts = ThreadInterlockedDecrement( &m_cRefCount );
	if ( nRefCounts == 0 )
	{
		if ( m_pMsg )
			free( m_pMsg );

		delete this;
	}

	return nRefCounts;
}



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEconNetworking::CEconNetworking()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEconNetworking::~CEconNetworking()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CEconNetworking::Init( void )
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconNetworking::Shutdown( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconNetworking::Update( float frametime )
{
	if ( !m_QueuedMessages.IsEmpty() )
	{
		CSmartPtr<CNetPacket> pPacket;
		m_QueuedMessages.RemoveAtHead( pPacket );

		HandleNetPacket( pPacket );
	}
}
#if defined( GAME_DLL )
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconNetworking::OnClientConnected( CSteamID const &steamID )
{
	CProtobufMsg<CServerHelloMsg> msg;
	CSteamID const *remoteID = engine->GetGameServerSteamID();

	uint unVersion = 0;
	FileHandle_t fh = filesystem->Open( "version.txt", "r", "MOD" );
	if ( fh && filesystem->Size( fh ) > 0 )
	{
		char version[48];
		filesystem->ReadLine( version, sizeof( version ), fh );
		unVersion = CRC32_ProcessSingleBuffer( version, Q_strlen( version ) + 1 );
	}
	filesystem->Close( fh );

	msg->set_version( unVersion );
	msg->set_remote_steamid( remoteID->ConvertToUint64() );

	const int nLength = msg->ByteSize();
	CArrayAutoPtr<byte> array( new byte[ nLength ]() );
	msg->SerializeWithCachedSizesToArray( array.Get() );

	SendMessage( steamID, k_EServerHelloMsg, array.Get(), nLength );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconNetworking::OnClientDisconnected( CSteamID const &steamID )
{	
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CEconNetworking::SendMessage( CSteamID const &targetID, MsgType_t eMsg, void *pubData, uint32 cubData )
{
	CSmartPtr<CNetPacket> pPacket( new CNetPacket );
	if ( !pPacket )
		return false;

	pPacket->Init( cubData, eMsg );
	// Very simple malicious intent awareness
	if ( pPacket->Hdr().m_eMsgType != eMsg )
		return false;

	Q_memcpy( pPacket->MutableData(), pubData, cubData );

	SetHeaderSteamIDs( pPacket, targetID );

#ifdef GAME_DLL
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CSteamID const *pPlayerID = engine->GetClientSteamID( INDEXENT( i ) );
		if ( pPlayerID && *pPlayerID == targetID )
		{
			INetChannel *pNetChan = dynamic_cast<INetChannel *>( engine->GetPlayerNetInfo( i ) );
			if ( pNetChan )
			{
				CEconNetMsg msg( pPacket );
				return pNetChan->SendNetMsg( msg );
			}
		}
	}
#else
	INetChannel *pNetChan = dynamic_cast<INetChannel *>( engine->GetNetChannelInfo() );
	if ( pNetChan )
	{
		CEconNetMsg msg( pPacket );
		return pNetChan->SendNetMsg( msg );
	}
#endif
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconNetworking::RecvMessage( CSteamID const &remoteID, MsgType_t eMsg, void const *pubData, uint32 const cubData )
{
	CNetPacket *pPacket = new CNetPacket();
	pPacket->InitFromMemory( pubData, cubData );

	// Dumb hack, Steam IDs are 0 when received here, remove when we know why
	if( pPacket->m_Hdr.m_ulSourceID == 0 )
	{
		pPacket->m_Hdr.m_ulSourceID = remoteID.ConvertToUint64();

	#if defined GAME_DLL
		CSteamID const *pSteamID = engine->GetGameServerSteamID();
		pPacket->m_Hdr.m_ulTargetID = pSteamID ? pSteamID->ConvertToUint64() : 0LL;
	#else
		CSteamID steamID;
		if ( steamapicontext && steamapicontext->SteamUser() )
			steamID = steamapicontext->SteamUser()->GetSteamID();
		pPacket->m_Hdr.m_ulTargetID = steamID.ConvertToUint64();
	#endif
	}

	m_QueuedMessages.Insert( pPacket );
}

//-----------------------------------------------------------------------------

CUtlMap<MsgType_t, IMessageHandler *> &MessageMap( void )
{
	static CUtlMap< MsgType_t, IMessageHandler * > s_MessageTypes( DefLessFunc( MsgType_t ) );
	return s_MessageTypes;
}

void RegisterEconNetworkMessageHandler( MsgType_t eMsg, IMessageHandler *pHandler )
{
	Assert( MessageMap().Find( eMsg ) == MessageMap().InvalidIndex() );
	MessageMap().Insert( eMsg, pHandler );
}

//-----------------------------------------------------------------------------
