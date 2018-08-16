#ifndef MINER_H
#define MINER_H

#include <vector>
#include "transaction.h"
#include "block.h"
#include "wallet.h"
#include "netcom.h"

class Miner : public PacketHandlerInterface
{
	public:

		Miner( Wallet *w, unsigned long walletIndex = 0, bool addressRotate = false );
		~Miner();
				
		bool TransactionHandler( unsigned char *_t, unsigned int len = 0, bufferevent *bev = NULL );
		bool LedgerHandler( unsigned char *block, unsigned int len = 0, bufferevent *bev = NULL );
		bool DeltaHandler( unsigned char *delta, unsigned int len = 0, bufferevent *bev = NULL );
		void Mine();
	
	protected:	
		std::vector<Transaction> transactions;
		Block *currentBlock;
		
		Wallet *wallet;
		unsigned long currentWalletIndex;
		bool rotateAddress;	
		bool blockUpdated;

};

#endif