#ifndef NETMODE_H
#define NETMODE_H

#include "transaction.h"
#include "blockdelta.h"

#define NET_VERSION 0.1

enum netMode
{
	NETMODE_USER = 0,
	NETMODE_NODE,	
	NETMODE_MINER	
};

enum packetType
{
	PT_UNKNOWN = 0,
	PT_TRANSACTION,
	PT_DELTA,		
	PT_LEDGER_REQUEST,
	PT_LEDGER,
	PT_BALANCE_REQUEST,
	PT_BALANCE,
	PT_NODE_LIST_REQUEST,
	PT_NODE_LIST
};

#define LOXO_PORT 60003

#define MAX_PACKET_LENGTH MAX_DELTA_LENGTH

#endif
