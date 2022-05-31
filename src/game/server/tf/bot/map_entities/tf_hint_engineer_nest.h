//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef TF_HINT_ENGINEER_NEST_H
#define TF_HINT_ENGINEER_NEST_H

#include "tf_hint_sentrygun.h"
#include "tf_hint_teleexit.h"

class CTFBotHintEngineerNest : public CBaseTFBotHintEntity
{
	DECLARE_CLASS( CTFBotHintEngineerNest, CBaseTFBotHintEntity );
public:
	CTFBotHintEngineerNest();
	virtual ~CTFBotHintEngineerNest();
	
	virtual int UpdateTransmitState() OVERRIDE;
	
	virtual void Spawn() OVERRIDE;
	
	virtual HintType GetHintType() const OVERRIDE;
	
	void DetonateStaleNest();
	
	CTFBotHintSentrygun *GetSentryHint() const;
	CTFBotHintTeleporterExit *GetTeleporterHint() const;
	
	bool IsStaleNest() const;
	
private:
	void HintThink();
	void HintTeleporterThink();
	
	void DetonateObjectsFromHints(const CUtlVector<CHandle<CBaseTFBotHintEntity>>& hints);
	CBaseTFBotHintEntity *GetHint(const CUtlVector<CHandle<CBaseTFBotHintEntity>>& hints) const;
	
	CUtlVector< CHandle<CBaseTFBotHintEntity> > m_SentryHints;
	CUtlVector< CHandle<CBaseTFBotHintEntity> > m_TeleHints;
	CNetworkVar( bool, m_bHasActiveTeleporter );
};

#endif
