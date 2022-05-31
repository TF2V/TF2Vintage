//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef TF_BOT_MISSION_REPROGRAMMED_H
#define TF_BOT_MISSION_REPROGRAMMED_H

class CTFBotMissionReprogrammed : public Action< CTFBot >
{
public:

	virtual const char *GetName( void ) const OVERRIDE;
};

#endif