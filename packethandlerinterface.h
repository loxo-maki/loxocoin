#ifndef PHI_H
#define PHI_H

#include <event2/bufferevent.h>
#include <vector>

class PacketHandlerInterface
{
	public:
		PacketHandlerInterface();
	
		virtual bool TransactionHandler( unsigned char *, unsigned int, bufferevent * );
		virtual bool DeltaHandler( unsigned char *, unsigned int, bufferevent * );
		virtual std::vector<unsigned char> LedgerRequestHandler( unsigned char *, unsigned int, bufferevent * );
		virtual bool LedgerHandler( unsigned char *, unsigned int, bufferevent * );	
		virtual std::vector<unsigned char> BalanceRequestHandler( unsigned char *, unsigned int, bufferevent * );	
		virtual bool BalanceHandler( unsigned char *, unsigned int, bufferevent * );		
};

#endif