// Make sure we allocate a network node for every player, "server full" denials, the Master server heartbeat, and potential RCON connections.
#define MAXNETNODES MAXPLAYERS+2

boolean Net_GetNetStat(void);
void Net_AckTicker(void);
boolean Net_AllAckReceived(void);
void D_SetDoomcom(void);
void D_SaveBan(void);
boolean D_CheckNetGame(void);
void D_CloseConnection(void);
void Net_UnAcknowledgPacket(INT32 node);
void Net_CloseConnection(INT32 node);
void Net_AbortPacketType(UINT8 packettype);
void Net_SendAcks(INT32 node);
void Net_WaitAllAckReceived(UINT32 timeout);
