#ifndef NODE_H
#define NODE_H

#include "netcom.h"
#include "block.h"

class Node : public PacketHandlerInterface
{
	public:

		Node();
		~Node();
				
		bool TransactionHandler( unsigned char *_t, unsigned int len = 0, bufferevent *bev = NULL );
		bool DeltaHandler( unsigned char *delta, unsigned int len = 0, bufferevent *bev = NULL );
		std::vector<unsigned char> LedgerRequestHandler( unsigned char *p, unsigned int len = 0, bufferevent *bev = NULL );
		bool LedgerHandler( unsigned char *block, unsigned int len = 0, bufferevent *bev = NULL );
		std::vector<unsigned char> BalanceRequestHandler( unsigned char *b, unsigned int len = 0, bufferevent *bev = NULL );
			
	protected:	
		Block *currentBlock;

};

#endif