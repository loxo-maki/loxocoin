#include "node.h"
#include <stdio.h>
#include <string.h>
#include <vector>

Node::Node()
{		
	FILE *blockFile;
	blockFile = fopen( "block", "rb");
	
	if ( blockFile == NULL )
	{
		// start the blockchain (SLBC)
		currentBlock = new Block( NULL, NULL ); // this is the most recent verified block, in this case not an actual block, just a skeleton waiting for first mine	
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
}	

bool Node::TransactionHandler( unsigned char *_t, unsigned int len, bufferevent *bev )
{
	Transaction *t = new Transaction( _t );
	if ( currentBlock->ValidateTransaction( *t ) )
	{
		NetCom::Send( _t, len, 0 );		
	}
	delete t;
	return false;
}

bool Node::DeltaHandler( unsigned char *delta, unsigned int len, bufferevent *bev )
{
	if ( currentBlock->UpdateWithBlockDelta( &(delta[0]) ) )
	{
		std::vector<unsigned char> block = currentBlock->Serialize();
		FILE *blockFile;
		blockFile = fopen( "block", "wb");
		fwrite( &(block[0]) , sizeof( unsigned char ), block.size(), blockFile );
		fclose( blockFile );
		
		NetCom::Send( delta, len, 0 );
		
		return true;
	}
	
	return false;	
}

std::vector<unsigned char> Node::LedgerRequestHandler( unsigned char *p, unsigned int len, bufferevent *bev )
{
	std::vector<unsigned char> cb = currentBlock->Serialize();

	return cb;	
}

bool Node::LedgerHandler( unsigned char *block, unsigned int len, bufferevent *bev )
{
	currentBlock->Deserialize( &(block[0]) );	
	
	return false;	
}

std::vector<unsigned char> Node::BalanceRequestHandler( unsigned char *br, unsigned int len, bufferevent *bev )
{	
	std::vector<unsigned char> b;
	b.resize( sizeof( unsigned char ) + sizeof( unsigned long long ) + sizeof( unsigned long ) );
	
	packetType pt = PT_BALANCE;
	memcpy( &(b[0]), &pt, sizeof( unsigned char ) );
	unsigned long long ab = currentBlock->GetAccountBalance( &(br[sizeof( unsigned char )])	);
	memcpy( &(b[sizeof( unsigned char )]), &ab, sizeof( unsigned long long ) );
	unsigned long ai = currentBlock->GetAccountIndex( &(br[sizeof( unsigned char )]) );
	memcpy( &(b[sizeof( unsigned char ) + sizeof( unsigned long long )]), &ai, sizeof( unsigned long ) );
	return b;
}

Node::~Node()
{
	delete currentBlock;
}