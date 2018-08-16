#ifndef BLOCK_H
#define BLOCK_H

#include <sodium.h>
#include "blockdelta.h"
#include "ledger.h"
#include <string>

class Block
{
	BlockDelta *blockDelta;	
	Ledger *ledger;	
	bool updated;
	
	public:
		Block( Block *previousBlock, unsigned char *facilitator ); // use previousBlock == NULL for first block, facilitator = NULL for receive block (i.e. non-mined block, first block)
		~Block();
		
		unsigned int GetBlockNumber();
		unsigned char *GetHash();	
		unsigned int GetNumTransactions();
		BlockDelta *GetBlockDelta();
		
		Ledger *GetLedger();
		
		unsigned char *GetFacilitator();
		
		bool AddTransaction( Transaction t );
	
		std::vector<unsigned char> Serialize();
		void Deserialize( unsigned char *block );
		std::vector<unsigned char> Mine(); // returns serialized mined blockDelta ready for sending
				
		bool UpdateWithBlockDelta( unsigned char * bd ); // updates block with mined blockDelta received from network, returns true if valid and successful, false otherwise
	
		std::string ToString( BlockDelta *bd = NULL, Ledger *l = NULL );
		
		bool ValidateTransaction( Transaction t );	
		
		unsigned long long GetAccountBalance( unsigned char *address );	
		unsigned long GetAccountIndex( unsigned char *address );
		
	protected:		
		bool IsHashDifficultEnough( unsigned char *hash );			
		bool IsSkeleton();								
};

#endif