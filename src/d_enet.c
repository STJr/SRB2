#include "doomdef.h"
#include "doomstat.h"
#include "d_enet.h"

boolean Net_GetNetStat(void)
{
	// set getbps, sendbps, gamelostpercent, lostpercent, etc.
	return false;
}

void Net_AckTicker(void)
{
}

boolean Net_AllAckReceived(void)
{
	return true;
}

void D_SetDoomcom(void)
{
	//net_nodecount = net_playercount = 0;
}

boolean D_CheckNetGame(void)
{
	multiplayer = false;
	server = true;
	D_ClientServerInit();
	return false;
}

void D_CloseConnection(void)
{
	netgame = false;
	addedtogame = false;
}

void Net_UnAcknowledgPacket(INT32 node)
{
}

void Net_CloseConnection(INT32 node)
{
}

void Net_AbortPacketType(UINT8 packettype)
{
}

void Net_SendAcks(INT32 node)
{
}

void Net_WaitAllAckReceived(UINT32 timeout)
{
}
