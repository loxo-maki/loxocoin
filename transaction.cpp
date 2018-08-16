#include "transaction.h"
#include <cmath>
#include <string.h>
#include <chrono>
#include "netmode.h"

Transaction::Transaction( unsigned char *_from, unsigned char *_to, unsigned long long int _amount, unsigned long int _index, unsigned char *privateKey )
{
	memcpy( from, _from, crypto_sign_ed25519_PUBLICKEYBYTES );
	memcpy( to, _to, crypto_sign_ed25519_PUBLICKEYBYTES );
	
	toFacilitator = ceil( TRANSACTION_FEE * _amount );
	amount = _amount - toFacilitator;
	
	unsigned char trans[TRANSACTION_LENGTH];			
	memcpy( &(trans[0]), from, crypto_sign_ed25519_PUBLICKEYBYTES );
	memcpy( &(trans[crypto_sign_ed25519_PUBLICKEYBYTES]), to, crypto_sign_ed25519_PUBLICKEYBYTES );
	memcpy( &(trans[crypto_sign_ed25519_PUBLICKEYBYTES + crypto_sign_ed25519_PUBLICKEYBYTES]), &amount, sizeof( unsigned long long int ) );	
	memcpy( &(trans[crypto_sign_ed25519_PUBLICKEYBYTES + crypto_sign_ed25519_PUBLICKEYBYTES + sizeof( unsigned long long int )]), &toFacilitator, sizeof( unsigned long long int ) );	
	
	timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	memcpy( &(trans[crypto_sign_ed25519_PUBLICKEYBYTES + crypto_sign_ed25519_PUBLICKEYBYTES + sizeof( unsigned long long int ) + sizeof( unsigned long long int )]), &timestamp, sizeof( unsigned long long int ) );	
	
	index = _index;
	memcpy( &(trans[crypto_sign_ed25519_PUBLICKEYBYTES + crypto_sign_ed25519_PUBLICKEYBYTES + sizeof( unsigned long long int ) + sizeof( unsigned long long int ) + sizeof( unsigned long long int )]), &index, sizeof( unsigned long int ) );	
	
	packetType pt = PT_TRANSACTION;
	memcpy( netTrans, &pt, sizeof( unsigned char ) );
	// sign	(puts signature at the start of the trnsaction message)	
	crypto_sign_ed25519( &(netTrans[sizeof( unsigned char )]), &netTransLength, trans, TRANSACTION_LENGTH, privateKey );
	memcpy( signature, &(netTrans[sizeof( unsigned char )]), crypto_sign_ed25519_BYTES );
}

Transaction::Transaction( unsigned char *t )
{
	memcpy( &signature, &(t[sizeof( unsigned char )]), crypto_sign_ed25519_BYTES );
	memcpy( &from, &(t[sizeof( unsigned char ) + crypto_sign_ed25519_BYTES]), crypto_sign_ed25519_PUBLICKEYBYTES );
	memcpy( &to, &(t[sizeof( unsigned char ) + crypto_sign_ed25519_BYTES + crypto_sign_ed25519_PUBLICKEYBYTES]), crypto_sign_ed25519_PUBLICKEYBYTES );
	memcpy( &amount, &(t[sizeof( unsigned char ) + crypto_sign_ed25519_BYTES + crypto_sign_ed25519_PUBLICKEYBYTES + crypto_sign_ed25519_PUBLICKEYBYTES]), sizeof( unsigned long long int ) );
	memcpy( &toFacilitator, &(t[sizeof( unsigned char ) + crypto_sign_ed25519_BYTES + crypto_sign_ed25519_PUBLICKEYBYTES + crypto_sign_ed25519_PUBLICKEYBYTES + sizeof( unsigned long long int )]), sizeof( unsigned long long int ) );
	memcpy( &timestamp, &(t[sizeof( unsigned char ) + crypto_sign_ed25519_BYTES + crypto_sign_ed25519_PUBLICKEYBYTES + crypto_sign_ed25519_PUBLICKEYBYTES + sizeof( unsigned long long int ) + sizeof( unsigned long long int )]), sizeof( unsigned long long int ) );
	memcpy( &index, &(t[sizeof( unsigned char ) + crypto_sign_ed25519_BYTES + crypto_sign_ed25519_PUBLICKEYBYTES + crypto_sign_ed25519_PUBLICKEYBYTES + sizeof( unsigned long long int ) + sizeof( unsigned long long int ) + sizeof( unsigned long long int )]), sizeof( unsigned long int ) );
	
	netTransLength = 0; // until Verify is called	
	memcpy( netTrans, &(t[0]), NETTRANS_LENGTH );
}

unsigned char *Transaction::GetFrom()
{
	return from;
}

unsigned char *Transaction::GetTo()
{
	return to;
}

unsigned long long int Transaction::GetAmount()
{
	return amount;
}

unsigned long long int Transaction::GetTimestamp()
{
	return timestamp;
}

unsigned char *Transaction::GetSignature()
{
	return signature;
}		
		
unsigned long long int Transaction::GetToFacilitator() const
{
	return toFacilitator;
}

unsigned long int Transaction::GetIndex()
{
	return index;
}

unsigned char *Transaction::GetNetTrans()
{
	return netTrans;	
}

unsigned long long int Transaction::GetNetTransLength()
{
	return netTransLength;	
}

bool Transaction::Verify()
{
	if ( amount == 0 )
	{
		return false;
	}
	
	// is signature correct
	unsigned char tempTrans[TRANSACTION_LENGTH];	
	if ( crypto_sign_ed25519_open( tempTrans, &netTransLength, &(netTrans[sizeof( unsigned char )]), TRANSACTION_LENGTH + crypto_sign_ed25519_BYTES, from ) != 0 )
	{
		return false;
	}	
	netTransLength = NETTRANS_LENGTH;
	
	// is fee correct
	if ( toFacilitator != ceil( TRANSACTION_FEE * ( amount + toFacilitator ) ) )
	{
		return false;
	}
	
	return true;
}

bool Transaction::operator==( const Transaction& rhs )
{ 
	return ( memcmp( from, rhs.from, crypto_sign_ed25519_PUBLICKEYBYTES ) == 0 && memcmp( to, rhs.to, crypto_sign_ed25519_PUBLICKEYBYTES ) == 0 && amount == rhs.amount && index == rhs.index );
}

bool Transaction::operator<( const Transaction& rhs ) const
{
	return ( toFacilitator > rhs.GetToFacilitator() );
}