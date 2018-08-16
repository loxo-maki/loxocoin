#include <sodium.h>
#include <iostream>
#include <cstdio>
#include "wallet.h"
#include "block.h"
#include "utils.h"
#include <errno.h>
#include <stdio.h>
#include <sstream>
#include <thread>

#include <string.h>

#include "transaction.h"

#include "netcom.h"

#include "miner.h"
#include "node.h"
#include "user.h"

#include "packethandlerinterface.h"

void printAddress( unsigned char *a )
{
	for ( unsigned int i = 0; i < crypto_sign_ed25519_PUBLICKEYBYTES; ++i )
	{
		printf( "%02x", a[i] );
	}
}

int main( int argc, char* argv[] )
{
	if ( sodium_init() < 0 ) 
	{
        	std::cout << "Sodium failed\n";
        	return 1;
    	}
	
	// launch with ./loxocoin [u/n/n] [wallet] [walletIndex] [0/1]
	
	// defaults
	netMode mode = NETMODE_USER;
	std::string wallet = "wallet";
	unsigned long walletIndex = 0;
	bool addressRotate = false;
	
	std::vector<std::string> options;
	for ( int i = 1; i < argc; ++i ) 
	{
		options.push_back( argv[i] );
	}

	if ( argc >= 2 )
	{
		if ( options[0] == "u" )
		{
		}
		else if ( options[0] == "n" )
		{
			mode = NETMODE_NODE;
		}
		else if ( options[0] == "m" )
		{
			mode = NETMODE_MINER;
		}
		else 
		{
			printf( "Invalid mode specified\n" );
		}
		if ( argc >= 3 )
		{
			wallet = options[1];
			if ( mode == NETMODE_MINER )
			{
				if ( argc >= 4 )
				{
					walletIndex = strtoul( &((options[2])[0]), NULL, 0 );					
					if ( argc >= 5 )
					{
						if ( options[3] == "1" )
						{
							addressRotate = true;
						}
					}
				}
			}
		}				
	}
	
	std::cout << "Mode: " << ( ( mode == NETMODE_USER ) ? "User" : ( ( mode == NETMODE_NODE ) ? "Node" : "Miner" ) ) << " Wallet: " << wallet << " Index: " << walletIndex << " Rotate: " << ( addressRotate ? "True" : "False" ) << "\n";
	
	std::vector<unsigned char> fileBuffer;
	
	// open wallet or create if does not exist
	Wallet *w = NULL;	
	FILE *walletFile = fopen( &(wallet[0]), "rb");
	if ( walletFile == NULL ) 
	{
		w = new Wallet( wallet );
		w->AddNewKeyPair();
		walletFile = fopen( &(wallet[0]), "wb");
		fileBuffer = w->Serialize();
		fwrite( &(fileBuffer[0]) , sizeof( unsigned char ), fileBuffer.size(), walletFile );
		fclose( walletFile );
	}
	fseek( walletFile, 0, SEEK_END );
	long int fileSize = ftell( walletFile );
	rewind( walletFile );			
	fileBuffer.clear();
	fileBuffer.resize( fileSize );
	fread( &(fileBuffer[0]), 1, fileSize, walletFile );
	fclose( walletFile );	
	w = new Wallet( wallet );
	w->Deserialize( &(fileBuffer[0]) );
	
	
	PacketHandlerInterface *phi = NULL;
	
	if ( mode == NETMODE_USER )
	{
		phi = new User( w );		
	}
	else if ( mode == NETMODE_NODE )
	{
		phi = new Node();		
	}	
	else if ( mode == NETMODE_MINER )
	{
		phi = new Miner( w, walletIndex, addressRotate );				
	}
	
	NetCom::Init( phi );
	
	// Get Block up to date
	if ( mode != NETMODE_USER )
	{
		packetType pt = PT_LEDGER_REQUEST;
		NetCom::Send( (unsigned char *)&pt, sizeof( unsigned char ), 1 );
	}
	
	std::thread netListen (NetCom::Listen);
	
	if ( mode == NETMODE_MINER )
	{
		((Miner *)phi)->Mine();
	}
	else if ( mode == NETMODE_USER )
	{
		((User *)phi)->Prompt();
		while ( ((User *)phi)->HandleInput() )
		{
			((User *)phi)->Prompt();	
		}
		netListen.detach();
	}	
	else
	{	
		netListen.join();
	}
	
	return 0;
}