#include "blockdelta.h"
#include <cmath>
#include <string.h>
#include <chrono>
#include <algorithm>
#include "netmode.h"

BlockDelta::BlockDelta( unsigned char *_facilitator )
{
	blockNumber = 0;
	
	if ( _facilitator != NULL )
	{
		memcpy( facilitator, _facilitator, crypto_sign_ed25519_PUBLICKEYBYTES );		
	}
	else
	{
		memset( facilitator, 0, crypto_sign_ed25519_PUBLICKEYBYTES );		
	}
		
	memset( previousHash, 0, crypto_generichash_BYTES );
	timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	nonce = 0;	
	memset( hash, 0, crypto_generichash_BYTES );
}

bool BlockDelta::Deserialize( unsigned char * netBlock )
{
	memcpy( &blockNumber, &(netBlock[PACKET_HEADER_SIZE]), sizeof( unsigned int ) );
	memcpy( facilitator, &(netBlock[PACKET_HEADER_SIZE + sizeof( unsigned int )]), crypto_sign_ed25519_PUBLICKEYBYTES );
	memcpy( previousHash, &(netBlock[PACKET_HEADER_SIZE + sizeof( unsigned int ) + crypto_sign_ed25519_PUBLICKEYBYTES]), crypto_generichash_BYTES );
	memcpy( &timestamp, &(netBlock[PACKET_HEADER_SIZE + sizeof( unsigned int ) + crypto_sign_ed25519_PUBLICKEYBYTES + crypto_generichash_BYTES]), sizeof( unsigned long long int ) );
	
	unsigned int numTransactions;
	memcpy( &numTransactions, &(netBlock[BLOCKDELTA_HEADER_NUMTRANSACTIONS_OFFSET]), sizeof( unsigned int ) );
	if ( numTransactions <= TRANSACTION_LIMIT )
	{
		for ( unsigned int i = 0; i < numTransactions; ++i )
		{
			Transaction *t = new Transaction( &(netBlock[BLOCKDELTA_HEADER_SIZE + i * NETTRANS_LENGTH]) );
			if ( t->Verify() )
			{	
				AddTransaction( *t );
				delete t;
			}
			else
			{
				delete t;
				return false;
			}
		}
	}
	
	memcpy( &nonce, &(netBlock[BLOCKDELTA_HEADER_SIZE + numTransactions * NETTRANS_LENGTH]), sizeof( unsigned long long int ) );
	memcpy( hash, &(netBlock[BLOCKDELTA_HEADER_SIZE + numTransactions * NETTRANS_LENGTH + sizeof( unsigned long long int )]), crypto_generichash_BYTES );
	
	return true;
}

std::vector<unsigned char> BlockDelta::Serialize()
{
	unsigned int numTransactions = GetNumTransactions();
	std::vector<unsigned char> netBlock;
	netBlock.resize( BLOCKDELTA_HEADER_SIZE + numTransactions * NETTRANS_LENGTH ); // there is some bloat here as the Transaction packets still contain their packet type identifiers
	
	packetType pt = PT_DELTA;
	memcpy( &(netBlock[0]), &pt, sizeof( unsigned char ) );		
	
	memcpy( &(netBlock[sizeof( unsigned char ) + sizeof( unsigned int )]), &blockNumber, sizeof( unsigned int ) );
	memcpy( &(netBlock[sizeof( unsigned char ) + sizeof( unsigned int ) + sizeof( unsigned int )]), facilitator, crypto_sign_ed25519_PUBLICKEYBYTES );
	memcpy( &(netBlock[sizeof( unsigned char ) + sizeof( unsigned int ) + sizeof( unsigned int ) + crypto_sign_ed25519_PUBLICKEYBYTES]), previousHash, crypto_generichash_BYTES );
	memcpy( &(netBlock[sizeof( unsigned char ) + sizeof( unsigned int ) + sizeof( unsigned int ) + crypto_sign_ed25519_PUBLICKEYBYTES + crypto_generichash_BYTES]), &timestamp, sizeof( unsigned long long int ) );
	
	// transactions
	memcpy( &(netBlock[BLOCKDELTA_HEADER_NUMTRANSACTIONS_OFFSET]), &numTransactions, sizeof( unsigned int ) );
	
	int i = 0;
	for( std::vector<Transaction>::iterator it = transactions.begin(); it != transactions.end(); ++it )
	{
		Transaction t = *it;
		memcpy( &(netBlock[BLOCKDELTA_HEADER_SIZE + i * NETTRANS_LENGTH]), t.GetNetTrans(), NETTRANS_LENGTH );
		++i;
	}
	
	unsigned int packetLength = BLOCKDELTA_HEADER_SIZE + numTransactions * NETTRANS_LENGTH + sizeof( unsigned long long int ) + crypto_generichash_BYTES;
	
	memcpy( &(netBlock[sizeof( unsigned char )]), &packetLength, sizeof( unsigned int ) );	
	
	return netBlock;
}

void BlockDelta::SetBlockNumber( unsigned int _blockNumber )
{
	blockNumber = _blockNumber;		
}

void BlockDelta::SetPreviousHash( unsigned char *_previousHash )
{
	memcpy( previousHash, _previousHash, crypto_generichash_BYTES );
}

void BlockDelta::SetHash( unsigned char *_hash )
{
	memcpy( hash, _hash, crypto_generichash_BYTES );
}

void BlockDelta::AddTransaction( Transaction t )
{
	if ( !IsTransactionInBlock( t ) )
	{
		transactions.push_back( t );
	}
}

std::vector<Transaction> *BlockDelta::GetTransactions()
{
	return &transactions;
}

bool BlockDelta::IsTransactionInBlock( Transaction t )
{
	return ( std::find( transactions.begin(), transactions.end(), t ) != transactions.end() );
}

unsigned char *BlockDelta::GetFacilitator()
{
	return facilitator;
}

unsigned int BlockDelta::GetBlockNumber()
{
	return blockNumber;
}

unsigned char *BlockDelta::GetPreviousHash()
{
	return previousHash;
}

unsigned char *BlockDelta::GetHash()
{
	return hash;
}

unsigned long long int BlockDelta::GetTimestamp()
{
	return timestamp;
}

void BlockDelta::SetTimestamp()
{
	timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();	
}

unsigned int BlockDelta::GetNumTransactions()
{
	return static_cast<unsigned int>(transactions.size());
}

unsigned long long BlockDelta::GetBlockReward()
{
	return std::max( (unsigned long long int)( ceil( 40000000 / ( 20 + GetBlockNumber() ) ) * PRECISION_SCALE ), (unsigned long long int)8 );
}

unsigned long long int BlockDelta::GetNonce()
{
	return nonce;
}

unsigned long long int *BlockDelta::SerializeNonce()
{
	return &nonce;
}

void BlockDelta::SetNonce( unsigned char *_nonce )
{
	memcpy( &nonce, _nonce, sizeof( unsigned long long int ) );	
}