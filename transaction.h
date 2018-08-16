#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <sodium.h>

#define TRANSACTION_FEE 0.001
#define TRANSACTION_LENGTH ( crypto_sign_ed25519_PUBLICKEYBYTES + crypto_sign_ed25519_PUBLICKEYBYTES + sizeof( unsigned long long int ) + sizeof( unsigned long long int ) + sizeof( unsigned long long int ) + sizeof( unsigned long int ) )
#define NETTRANS_LENGTH ( sizeof( unsigned char ) + TRANSACTION_LENGTH + crypto_sign_ed25519_BYTES )

class Transaction
{
	unsigned char from[crypto_sign_ed25519_PUBLICKEYBYTES];
	unsigned char to[crypto_sign_ed25519_PUBLICKEYBYTES];
	unsigned long long int amount;
	unsigned char signature[crypto_sign_ed25519_BYTES];
	unsigned long long int toFacilitator;
	unsigned long long int timestamp; // not used
	unsigned long int index;
	
	unsigned char netTrans[NETTRANS_LENGTH];
	unsigned long long int netTransLength; // remove this
	
	public:
		Transaction( unsigned char *_from, unsigned char *_to, unsigned long long int _amount, unsigned long int _index, unsigned char *privateKey );	
		Transaction( unsigned char * );
		unsigned char *GetFrom();
		unsigned char *GetTo();
		unsigned long long int GetAmount();
		unsigned long long int GetTimestamp();
		unsigned char *GetSignature();
		unsigned long long int GetToFacilitator() const;
		unsigned long int GetIndex();
		
		unsigned char *GetNetTrans();
		unsigned long long int GetNetTransLength();		
		
		bool Verify();
		
		bool operator==( const Transaction& rhs );
		
		bool operator<( const Transaction& rhs ) const;		
};

#endif