// Make sure we allocate a network node for every player, "server full" denials, the Master server heartbeat, and potential RCON connections.
#define MAXNETNODES MAXPLAYERS+2

extern UINT8 net_nodecount, net_playercount;
extern UINT16 net_ringid;
extern UINT8 playernode[MAXPLAYERS];
extern SINT8 nodetoplayer[MAXNETNODES];
extern SINT8 nodetoplayer2[MAXNETNODES]; // say the numplayer for this node if any (splitscreen)
extern UINT8 playerpernode[MAXNETNODES]; // used specialy for scplitscreen
extern boolean nodeingame[MAXNETNODES]; // set false as nodes leave game

void D_NetOpen(void);
boolean D_NetConnect(const char *hostname, const char *port);

void Net_GetNetStat(UINT8 node, UINT32 *ping, UINT32 *packetLoss);
void Net_AckTicker(void);
void D_CheckNetGame(void);
void D_CloseConnection(void);
void Net_CloseConnection(INT32 node);

void Net_SendJoin(void);
void Net_SendCharacter(void);
void Net_SendClientMove(boolean force);
void Net_SendClientJump(void);

void Net_SpawnPlayer(UINT8 pnum, UINT8 node);
void Net_SendChat(char *line);
void Net_SendPlayerDamage(UINT8 pnum, UINT8 damagetype);
void Net_SendMobjMove(mobj_t *mobj);
void Net_SendRemove(UINT16 id);
void Net_SendKill(UINT16 id, UINT16 kid);
void Net_SendPlayerRings(UINT8 pnum);
void Net_ResetLevel(void);
void Net_AwardPowerup(player_t *player, powertype_t power, UINT16 data);
