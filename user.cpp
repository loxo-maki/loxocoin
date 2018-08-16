#include "user.h"
#include <stdio.h>
#include <iostream>
#include "utils.h"

User::User( Wallet *w )
{		
	wallet = w;
	
	state = PS_MAIN;
	waitingIndex = 0;
}	

void User::Prompt()
{
	if ( state == PS_MAIN )
	{
		std::cout << "\n1) New Address\n2) Get Balance/Show Addresses\n3) Transfer\n4) Add To Watch List\n5) Purge Secrets\n9) End\n\nNote: You must get the balance of an account before making any transfer from that account.\nWARNING: Make sure you have a copy of your secret keys before running Purge Secrets or you will not be able to access those accounts!\n";
	}
	else if ( state == PS_ADDRESS_SELECT )
	{		
		std::cout << "Select address index to request balance, or 0 to go back.\n0) Back" << "\n";
		
		DisplayAddresses();
	}
	else if ( state == PS_TRANSFER )
	{
		std::cout << "Select address index to transfer from and to or 0 to cancel.\n0) Cancel" << "\n";
		
		DisplayAddresses();
		std::cout << "\nForeign addresses (cannot transfer from these):\n\n";
	}
	else if ( state == PS_WATCH_ADD )
	{
		std::cout << "Enter address in lowercase or enter x to cancel.\n";
	}
}

bool User::HandleInput()
{		
	std::vector<keyPair> *addresses = wallet->GetAddresses();
	if ( state == PS_MAIN )
	{
		int choice;
		std::cin >> choice;
		if ( choice == 1 ) // add new address
		{
			wallet->AddNewKeyPair();
			WalletToFile();
			
			addresses = wallet->GetAddresses();
			int i = addresses->size() - 1;
			std::cout << "Address: " << UnsignedCharArrayToString( ((*addresses)[i]).publicKey, crypto_sign_ed25519_PUBLICKEYBYTES ) << "\nSecret: " << UnsignedCharArrayToString( ((*addresses)[i]).privateKey, crypto_sign_ed25519_SECRETKEYBYTES ) << "\n\nNOTE: This is the only time your secret will be displayed, so if you intend to purge later record it elsewhere now. It will also be recoverable from your wallet file unless you purge.\n";
			
			state = PS_MAIN;
		}
		else if ( choice == 2 )
		{
			state = PS_ADDRESS_SELECT;			
		}
		else if ( choice == 3 )
		{
			state = PS_TRANSFER;
		}
		else if ( choice == 4 )
		{
			state = PS_WATCH_ADD;
		}
		else if ( choice == 5 ) // purge secrets
		{
			std::cout << "Purge secrets. Are you sure? Press 9 to continue or any other number to cancel.\n";
			int c;
			std::cin >> c;
			if ( c == 9 )
			{
				wallet->PurgeSecrets();
				
				WalletToFile();
			}
			
			state = PS_MAIN;
		}
		else if ( choice == 9 )
		{
			return false;
		}
	}
	else if ( state == PS_ADDRESS_SELECT )
	{
		int choice;
		std::cin >> choice;
		--choice;		
		if ( choice < 0 )
		{
			state = PS_MAIN;
		}
		else if ( (unsigned int)choice < addresses->size() + watchList.size() )
		{
			std::vector<unsigned char> b;
			b.push_back( PT_BALANCE_REQUEST );
			b.resize( sizeof( unsigned char ) + crypto_sign_ed25519_PUBLICKEYBYTES );			
			
			if ( (unsigned int)choice < addresses->size() )
			{
				memcpy( &(b[sizeof( unsigned char )]), ((*addresses)[choice]).publicKey, crypto_sign_ed25519_PUBLICKEYBYTES );
			}
			else
			{
				unsigned char address[crypto_sign_ed25519_PUBLICKEYBYTES];
				StringToUnsignedCharArray( watchList[choice - addresses->size()], address );
				memcpy( &(b[sizeof( unsigned char )]), address, crypto_sign_ed25519_PUBLICKEYBYTES );
			}
			
			state = PS_WAITING;
			NetCom::Send( &(b[0]), b.size(), 12 );			
			std::cout << "Request sent. Awaiting responses.\nEnter 0 to stop.\n";
			std::cin >> choice;
			state = PS_MAIN;
		}	
	}
	else if ( state == PS_TRANSFER )
	{
		std::cout << "From: ";
		int from;
		std::cin >> from;
		--from;
		if ( from < 0 )
		{
			state = PS_MAIN;
		}
		else if ( (unsigned int)from < addresses->size() )
		{
			unsigned char secret[crypto_sign_ed25519_SECRETKEYBYTES];
			if ( wallet->IsPurged( ((*addresses)[from]) ) )
			{
				std::cout << "Enter Secret: ";
				std::string address;
				std::cin >> address;				
				if ( address != "" && address.find_first_not_of("0123456789abcdef") == std::string::npos )
				{
					if ( address.length() == crypto_sign_ed25519_SECRETKEYBYTES )
					{
						StringToUnsignedCharArray( address, secret );
									
					}
					else 
					{
						std::cout << "Incorrect length secret.\n";						
					}
				}
				else 
				{
					std::cout << "Secret contains invalid characters.\n";					
				}
			}
			std::cout << "To: ";
			int to;
			std::cin >> to;
			--to;
			if ( (unsigned int)to >= 0 && (unsigned int)to < addresses->size() )
			{
				std::cout << "Amount: ";
				double amount;
				std::cin >> amount;
				if ( amount > 0 )							
				{
					Transaction *t;
					if ( wallet->IsPurged( ((*addresses)[from]) ) )
					{
						t = new Transaction( ((*addresses)[from]).publicKey, ((*addresses)[to]).publicKey, amount * PRECISION_SCALE, indexes[from], secret );
					}
					else
					{
						t = new Transaction( ((*addresses)[from]).publicKey, ((*addresses)[to]).publicKey, amount * PRECISION_SCALE, indexes[from], ((*addresses)[from]).privateKey );
					}
					waitingIndex = from;
					NetCom::Send( t->GetNetTrans(), NETTRANS_LENGTH, 0 );
					//watchList.push_back( UnsignedCharArrayToString( ((*addresses)[to]).publicKey, crypto_sign_ed25519_PUBLICKEYBYTES ) );
					std::cout << "Transaction sent. Select Get Balance to check progress. To address added to watch list.\n";
					waitingIndex = 0;								
					delete t;
				}
			}			
			state = PS_MAIN;
		}
		else
		{
			std::cout << "Invalid input.\n";
		}
	}
	else if ( state == PS_WATCH_ADD )	
	{
		std::string address;
		std::cin >> address;
		if ( address == "x" )
		{
			state = PS_MAIN;
		}
		if ( address.find_first_not_of("0123456789abcdef") == std::string::npos )
		{
			if ( address.length() == crypto_sign_ed25519_PUBLICKEYBYTES )
			{
				watchList.push_back( address );
				state = PS_MAIN;
			}
			else 
			{
				std::cout << "Incorrect length address.\n";
			}
		}
		else 
		{
			std::cout << "Address contains invalid characters.\n";
		}
	}
	return true;
}

void User::DisplayAddresses()
{
	std::vector<keyPair> *addresses = wallet->GetAddresses();
				
	int i = 1;
	for( std::vector<keyPair>::iterator it = addresses->begin(); it != addresses->end(); ++it )
	{
		keyPair a = (*it);
		
		std::cout << i << ") " << UnsignedCharArrayToString( a.publicKey, crypto_sign_ed25519_PUBLICKEYBYTES ) << "\n";
		++i;
	}
	
	std::cout << "\nWatch list:\n";
	for( std::vector<std::string>::iterator it = watchList.begin(); it != watchList.end(); ++it )
	{
		std::string s = (*it);
		
		std::cout << i << ") " << s << "\n";
		
		++i;
	}	
}

bool User::BalanceHandler( unsigned char *b, unsigned int len, bufferevent *bev )
{
	if ( state == PS_WAITING )
	{		
		unsigned long long balance;
		memcpy( &balance, &(b[sizeof( unsigned char )]), sizeof( unsigned long long ) );
		unsigned long index;
		memcpy( &index, &(b[sizeof( unsigned char ) + sizeof( unsigned long long )]), sizeof( unsigned long ) );
		indexes[waitingIndex] = index;
		
		double bal = balance / (double)PRECISION_SCALE;
		
		std::cout << "Balance received: " << std::to_string( bal ) << "\n";
	}
	return false;	
}

void User::WalletToFile()
{
	FILE *walletFile = fopen( &((wallet->GetFilename())[0]), "wb");
	std::vector<unsigned char> fileBuffer;
	fileBuffer = wallet->Serialize();
	fwrite( &(fileBuffer[0]) , sizeof( unsigned char ), fileBuffer.size(), walletFile );
	fclose( walletFile );
}