#include "block.h"
#include <cmath>
#include <string.h>
#include <vector>
#include <algorithm>
#include "utils.h"
#include "netmode.h"

Block::Block( Block *previousBlock, unsigned char *facilitator )
{
	blockDelta = new BlockDelta( facilitator );	
	ledger = new Ledger();
	if ( previousBlock != NULL )
	{
		blockDelta->SetBlockNumber( previousBlock->GetBlockNumber() + 1 );
		blockDelta->SetPreviousHash( previousBlock->GetHash() );
		
		delete ledger;
		ledger = new Ledger( *(previousBlock->GetLedger()) );
	}
	updated = false;
}

void Block::Deserialize( unsigned char *block )
{
	delete blockDelta;
	blockDelta = new BlockDelta( NULL );	
	
	unsigned int blockNumber;
	memcpy( &blockNumber, &(block[sizeof( unsigned char )]), sizeof( unsigned int ) );
	blockDelta->SetBlockNumber( blockNumber );
	blockDelta->SetHash( &(block[sizeof( unsigned char ) + sizeof( unsigned int )]) );
	
	ledger->Deserialize( &(block[sizeof( unsigned char ) + sizeof( unsigned int ) + crypto_generichash_BYTES]) );
}

bool Block::IsSkeleton()
{
	char zeroBlock[crypto_sign_ed25519_PUBLICKEYBYTES];
	memset( zeroBlock, 0, crypto_sign_ed25519_PUBLICKEYBYTES );
	return ( memcmp( blockDelta->GetFacilitator(), zeroBlock, crypto_sign_ed25519_PUBLICKEYBYTES ) == 0 );
}

std::vector<unsigned char> Block::Mine()
{
	std::vector<unsigned char> bd;

	blockDelta->SetTimestamp();
	ledger->BlockReward( blockDelta->GetFacilitator(), blockDelta->GetBlockReward() ); // this "seals" the block
	
	
	bd = blockDelta->Serialize();
	unsigned long long int deltaSize = bd.size();
	
	std::vector<unsigned char> data(bd);
	std::vector<unsigned char> led = ledger->Serialize();
	data.insert( data.end(), led.begin(), led.end() );
		
	// calculate nonce and hash	
	unsigned int nonceOffset = data.size();	
	data.resize( data.size() + sizeof( unsigned long long int ) );
	
	unsigned char blockHash[crypto_generichash_BYTES];	
	
	unsigned long long int nonce = 0;
	for ( nonce = 0; nonce <= std::numeric_limits<unsigned long long>::max(); ++nonce )
	{
		if ( updated )
		{
			bd.clear();
			updated = false;
			return bd;
		}
		memcpy( &(data[nonceOffset]), &nonce, sizeof( unsigned long long int ) );
		crypto_generichash( blockHash, crypto_generichash_BYTES, &(data[PACKET_HEADER_SIZE]), data.size() - PACKET_HEADER_SIZE, NULL, 0 );
		if ( IsHashDifficultEnough( blockHash ) )
		{
			break;
		}
	}
	
	// once found
	
	blockDelta->SetNonce( (unsigned char *)&nonce ); // for debugging
	
	// now strip the ledger information since this is not needed for network transfer
	deltaSize += sizeof( unsigned long long int ) + crypto_generichash_BYTES;	
	bd.resize( deltaSize );
	memcpy( &(bd[deltaSize - sizeof( unsigned long long int ) - crypto_generichash_BYTES]), &nonce, sizeof( unsigned long long int ) );
	
	memcpy( &(bd[deltaSize - crypto_generichash_BYTES]), blockHash, crypto_generichash_BYTES );
	
	blockDelta->SetHash( blockHash ); // for debugging
	
	return bd;
}

std::vector<unsigned char> Block::Serialize()
{
	std::vector<unsigned char> block;
	block.resize( sizeof( unsigned char ) + sizeof( unsigned int ) + crypto_generichash_BYTES );
	packetType pt = PT_LEDGER;
	memcpy( &(block[0]), &pt, sizeof( unsigned char ) );
	unsigned int bn = GetBlockNumber();		
	memcpy( &(block[sizeof( unsigned char )]), &bn, sizeof( unsigned int ) );
	memcpy( &(block[sizeof( unsigned char ) + sizeof( unsigned int )]), blockDelta->GetHash(), crypto_generichash_BYTES );
	std::vector<unsigned char> ls = ledger->Serialize();
	block.insert( block.end(), ls.begin(), ls.end() );		
	return block;	
}

bool Block::UpdateWithBlockDelta( unsigned char * bd )
{	
	BlockDelta *newBD = new BlockDelta( NULL );
	if ( newBD->Deserialize( &(bd[0]) ) == false )
	{
		delete newBD;
		return false;	
	}
	
	if ( ( newBD->GetBlockNumber() != blockDelta->GetBlockNumber() + 1 ) && !( newBD->GetBlockNumber() == 0 && this->IsSkeleton() ) )
	{
		delete newBD;
		return false;
	}
		
	if ( memcmp( newBD->GetPreviousHash(), blockDelta->GetHash(), crypto_generichash_BYTES ) != 0 )
	{
		delete newBD;
		return false;
	}
			
	Ledger *newLedger = new Ledger( *ledger );
	if ( newLedger->ApplyBlockDelta( newBD ) == false )
	{
		delete newBD;
		delete newLedger;
		return false;
	}
	
	std::vector<unsigned char> nbd = newBD->Serialize();
	
	std::vector<unsigned char> block(nbd);
	std::vector<unsigned char> led = newLedger->Serialize();
	block.insert( block.end(), led.begin(), led.end() );
	
	unsigned int nonceOffset = block.size();	
	block.resize( block.size() + sizeof( unsigned long long int ) );
		
	memcpy( &(block[nonceOffset]), newBD->SerializeNonce(), sizeof( unsigned long long int ) );	
	
	unsigned char blockHash[crypto_generichash_BYTES];		
	crypto_generichash( blockHash, crypto_generichash_BYTES, &(block[PACKET_HEADER_SIZE]), block.size() - PACKET_HEADER_SIZE, NULL, 0 );
	
	// check hashes are equal	
	if ( memcmp( blockHash, newBD->GetHash(), crypto_generichash_BYTES ) != 0 )
	{
		delete newBD;
		delete newLedger;
		return false;
	}
	
	if ( !IsHashDifficultEnough( blockHash ) )
	{
		delete newBD;
		delete newLedger;
		return false;
	}	
	
	// if correct, replace old blockDelta and ledger (delete old ones)
	delete blockDelta;
	blockDelta = newBD;
	delete ledger;
	ledger = newLedger;
	
	ledger->CheckCull();
	
	updated = true;

	return true;
}

// I know this is bullshit
bool Block::IsHashDifficultEnough( unsigned char *hash )
{
	unsigned int minLeadingZeroes = floor( 1 + 2 * std::log2( ( 1000 + blockDelta->GetBlockNumber() ) / 1000 ) );
	unsigned int numZeroes = 0;
	for ( unsigned int i = 0; i < crypto_generichash_BYTES; ++i )
	{	
		for ( int j = 7; j >= 0; --j )
		{
			if ( ( hash[i] & ( 1 << j ) ) == 0 ) 
			{ 
				++numZeroes;
				if ( numZeroes >= minLeadingZeroes )
				{
					return true;
				}
			}
			else
			{
				if ( numZeroes < minLeadingZeroes )
				{
					return false;
				}		
				return true;		
			}			
		}
	}	
	return false;	
}

std::string Block::ToString( BlockDelta *bd, Ledger *l )
{
	if ( bd == NULL )
	{
		bd = blockDelta;
	}
	if ( l == NULL )
	{
		l = ledger;
	}
	
	std::string s = "Block:\n\nHeader:\n\n";
	s += "Block Number: " + std::to_string( bd->GetBlockNumber() ) + "\n";
	s += "Facilitator: " + addressToString( bd->GetFacilitator() ) + "\n";
	s += "Previous Hash: " + hashToString( bd->GetPreviousHash() ) + "\n";
	s += "Timestamp: " + std::to_string( bd->GetTimestamp() ) + "\n\n";
	s += "Ledger:\n\n" + l->ToString() + "\n";
	s += "Nonce: " + std::to_string( bd->GetNonce() ) + "\n";
	s += "Hash: " + hashToString( bd->GetHash() );	
	return s;
}

unsigned int Block::GetNumTransactions()
{
	return (blockDelta->GetTransactions())->size();
}

unsigned int Block::GetBlockNumber()
{
	return blockDelta->GetBlockNumber();
}

BlockDelta *Block::GetBlockDelta()
{
	return blockDelta;
}

unsigned char *Block::GetHash()
{
	return blockDelta->GetHash();
}

Ledger *Block::GetLedger()
{
	return ledger;
}

bool Block::AddTransaction( Transaction t )
{
	if ( ValidateTransaction( t ) )
	{				
		blockDelta->AddTransaction( t );
		ledger->DoTransaction( t, blockDelta->GetFacilitator() ); // note that facilitator transaction rewards are added in order, but mining rewards are added at the end
		return true;
	}
	return false;
}

bool Block::ValidateTransaction( Transaction t )
{	
	if ( t.Verify() ) // transaction level verification
	{
		// blockDelta level check
		// make sure is not duplicate transaction
		if ( !blockDelta->IsTransactionInBlock( t ) )			
		{		
			// Ledger level verification
			if ( ledger->VerifyTransaction( t ) )
			{
					return true;			
			}			
		}
	}	
	return false;
}

unsigned char *Block::GetFacilitator()
{
	return blockDelta->GetFacilitator();
}

unsigned long long Block::GetAccountBalance( unsigned char *address )
{
	return ledger->GetAccountBalance( address );
}

unsigned long  Block::GetAccountIndex( unsigned char *address )
{
	return ledger->GetAccountIndex( address );
}

Block::~Block()
{
	delete blockDelta;
	delete ledger;
}