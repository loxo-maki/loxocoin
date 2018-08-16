#ifndef BLOCKDELTA_H
#define BLOCKDELTA_H

#include <sodium.h>
#include <vector>
#include "transaction.h"

#define PACKET_HEADER_SIZE ( sizeof( unsigned char ) + sizeof( unsigned int ) )
#define BLOCKDELTA_HEADER_NUMTRANSACTIONS_OFFSET ( PACKET_HEADER_SIZE + sizeof( unsigned int ) + crypto_sign_ed25519_PUBLICKEYBYTES + crypto_generichash_BYTES + sizeof( unsigned long long int ) )
#define BLOCKDELTA_HEADER_SIZE ( PACKET_HEADER_SIZE + sizeof( unsigned int ) + crypto_sign_ed25519_PUBLICKEYBYTES + crypto_generichash_BYTES + sizeof( unsigned long long int ) + sizeof( unsigned int ) )

// this is arbitrary
#define TRANSACTION_LIMIT 5000

#define PRECISION_SCALE 100000000

#define MAX_DELTA_LENGTH ( BLOCKDELTA_HEADER_SIZE + TRANSACTION_LIMIT * NETTRANS_LENGTH + sizeof( unsigned long long int ) + crypto_generichash_BYTES )

class BlockDelta
{
	unsigned int blockNumber;
	unsigned char facilitator[crypto_sign_ed25519_PUBLICKEYBYTES];	
	unsigned char previousHash[crypto_generichash_BYTES];
	unsigned long long int timestamp; 
	
	unsigned long long int nonce;		
	unsigned char hash[crypto_generichash_BYTES];
		
	std::vector<Transaction> transactions;
	public:
		BlockDelta( unsigned char *_facilitator );
		
		bool Deserialize( unsigned char * netBlock );
		std::vector<unsigned char> Serialize();
		
		void SetBlockNumber( unsigned int _blockNumber );
		void SetPreviousHash( unsigned char *_previousHash );
		void SetNonce( unsigned char *_nonce );
		void SetHash( unsigned char *_hash );
		unsigned long long GetBlockReward();
		
		void AddTransaction( Transaction t );	
		std::vector<Transaction> *GetTransactions();
		bool IsTransactionInBlock( Transaction t );
		
		unsigned char *GetFacilitator();
		unsigned int GetBlockNumber();
		unsigned char *GetPreviousHash();
		unsigned char *GetHash();
		unsigned long long int GetTimestamp();	
		void SetTimestamp();
		unsigned int GetNumTransactions();	
		unsigned long long int GetNonce();
		unsigned long long int *SerializeNonce();
};

#endif