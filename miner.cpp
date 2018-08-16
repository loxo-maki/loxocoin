#include "miner.h"
#include <algorithm>
#include <stdio.h>

Miner::Miner( Wallet *w, unsigned long walletIndex, bool addressRotate )
{		
	FILE *blockFile;
	blockFile = fopen( "block", "rb");
	
	if ( blockFile == NULL )
	{
		currentBlock = new Block( NULL, NULL );
		std::vector<unsigned char> block = currentBlock->Serialize();
		blockFile = fopen( "block", "wb");
		fwrite( &(block[0]) , sizeof( unsigned char ), block.size(), blockFile );
		fclose( blockFile );
		delete currentBlock;
		blockFile = fopen( "block", "rb");
	}	
	
	std::vector<unsigned char> fileBuffer;
	fseek( blockFile, 0, SEEK_END );
	long int fileSize = ftell( blockFile );
	rewind( blockFile );	
	fileBuffer.clear();
	fileBuffer.resize( fileSize );
	fread( &(fileBuffer[0]), sizeof( unsigned char ), fileSize, blockFile );
	fclose( blockFile );	
	
	currentBlock = new Block( NULL, NULL );
	currentBlock->Deserialize( &(fileBuffer[0]) );
	
	wallet = w;
	currentWalletIndex = walletIndex;
	rotateAddress = addressRotate;	
	
	blockUpdated = false;
}	

void Miner::Mine()
{
	while ( true )
	{
		if ( blockUpdated )
		{
			blockUpdated = false;
			
			Block *miningBlock = new Block( currentBlock, ((*(wallet->GetAddresses()))[currentWalletIndex]).publicKey );
			
			// add queued transactions to block and remove any that are invalid
			std::sort( transactions.begin(), transactions.end() );
			std::vector<std::vector<Transaction>::iterator> cull;	
			for( std::vector<Transaction>::iterator it = transactions.begin(); it != transactions.end(); ++it )
			{
				Transaction t = *it;
				if ( miningBlock->AddTransaction( t ) == false )
				{
					cull.push_back( it );
				}
			}
			for( std::vector<std::vector<Transaction>::iterator>::iterator it = cull.begin(); it != cull.end(); ++it ) 
			{
				transactions.erase( *it );	
			}	
			
			std::vector<unsigned char> minedBlock = miningBlock->Mine();	
			if ( minedBlock.size() != 0 )
			{		
				if ( rotateAddress )
				{
					currentWalletIndex = ( currentWalletIndex + 1 ) % (wallet->GetAddresses())->size();
				}
				NetCom::Send( &(minedBlock[0]), minedBlock.size(), 0 ); // our block will only be updated when it is received back as a delta
			}
			delete miningBlock;
		}
	}
}			
		
bool Miner::TransactionHandler( unsigned char *_t, unsigned int len, bufferevent *bev )
{
	Transaction *t = new Transaction( _t );
	if ( t->Verify() )
	{
		if ( std::find( transactions.begin(), transactions.end(), *t ) == transactions.end() )
		{
			transactions.push_back( *t );
			return true;
		}
	}
	delete t;
	return false;
}

bool Miner::LedgerHandler( unsigned char *block, unsigned int len, bufferevent *bev )
{
	currentBlock->Deserialize( &(block[0]) );
	
	blockUpdated = true;	
	
	return false;
}

bool Miner::DeltaHandler( unsigned char *delta, unsigned int len, bufferevent *bev )
{
	if ( currentBlock->UpdateWithBlockDelta( &(delta[0]) ) )
	{
		std::vector<unsigned char> block = currentBlock->Serialize();
		FILE *blockFile;
		blockFile = fopen( "block", "wb");
		fwrite( &(block[0]) , sizeof( unsigned char ), block.size(), blockFile );
		fclose( blockFile );
		
		blockUpdated = true;
		
		return true;
	}
	return false;
}

Miner::~Miner()
{
	delete currentBlock;
}