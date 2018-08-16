#ifndef LEDGER_H
#define LEDGER_H

#include <sodium.h>
#include <map>
#include <string>
#include <vector>
#include "transaction.h"
#include "blockdelta.h"

#define ACCOUNT_LENGTH ( crypto_sign_ed25519_PUBLICKEYBYTES + sizeof( unsigned long long ) + sizeof( unsigned long ) )
// this keeps the ledger size below 1GB (in future should not be hard limited)
#define MAX_ACCOUNTS 21000000
#define CULL_FRACTION 0.05

class Ledger
{
	struct account
	{
		unsigned char address[crypto_sign_ed25519_PUBLICKEYBYTES];
		unsigned long long balance;
		unsigned long index;
	};
	
	
	std::map<std::string, account> accounts;
	
	public:
		Ledger();
		std::vector<unsigned char> Serialize();
		void Deserialize( unsigned char *led );
		bool DoesAccountExist( unsigned char *address );
		unsigned long GetAccountIndex( unsigned char *address );
		unsigned long long GetAccountBalance( unsigned char *address );
		bool VerifyTransaction( Transaction t );
		void DoTransaction( Transaction t, unsigned char *facilitator );
		bool ApplyBlockDelta( BlockDelta *bd );
		void BlockReward( unsigned char *facilitator, unsigned long long reward );
		void CheckCull();
		
		std::string ToString();
	
	protected:
		void CreateIfNotExists( unsigned char *address );
		
};

#endif