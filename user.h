#ifndef USER_H
#define USER_H

#include "netcom.h"
#include "wallet.h"
#include <vector>
#include <string>
#include <map>

class User : public PacketHandlerInterface
{
	enum prompt_state
	{
		PS_MAIN = 0,
		PS_ADDRESS_SELECT,
		PS_TRANSFER,	
		PS_WATCH_ADD,	
		PS_WAITING	
	};
	
	public:

		User( Wallet *w );
		
		void Prompt();
		
		bool HandleInput();
				
		bool BalanceHandler( unsigned char *b, unsigned int len = 0, bufferevent *bev = NULL );
			
	protected:	
		void DisplayAddresses();
		void WalletToFile();
	
		Wallet *wallet;
		std::map<int,int> indexes;
		std::vector<std::string> watchList;		
		prompt_state state;
		int waitingIndex;
};

#endif