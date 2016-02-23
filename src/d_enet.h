// Make sure we allocate a network node for every player, "server full" denials, the Master server heartbeat, and potential RCON connections.
#define MAXNETNODES MAXPLAYERS+2

extern UINT8 net_nodecount, net_playercount;
extern UINT8 playernode[MAXPLAYERS];
extern SINT8 nodetoplayer[MAXNETNODES];
extern SINT8 nodetoplayer2[MAXNETNODES]; // say the numplayer for this node if any (splitscreen)
extern UINT8 playerpernode[MAXNETNODES]; // used specialy for scplitscreen
extern boolean nodeingame[MAXNETNODES]; // set false as nodes leave game

boolean Net_GetNetStat(void);
void Net_AckTicker(void);
boolean D_CheckNetGame(void);
void D_CloseConnection(void);
void Net_CloseConnection(INT32 node);

void Net_SendJoin(void);
