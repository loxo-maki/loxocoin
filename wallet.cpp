#include "wallet.h"
#include "utils.h"

Wallet::Wallet( std::string _filename )
{
	filename = _filename;
}

void Wallet::AddNewKeyPair()
{
	keyPair k;
	
	unsigned char  seed[crypto_sign_SEEDBYTES];                       
	randombytes_buf(k.privateKey, crypto_sign_ed25519_SECRETKEYBYTES );
	crypto_sign_ed25519_sk_to_seed( seed, k.privateKey );                
	crypto_sign_seed_keypair( k.privateKey, k.privateKey, seed );      
	crypto_sign_ed25519_keypair( k.publicKey, k.privateKey );      
	     	
     	keyPairs.push_back( k );
}

std::vector<unsigned char> Wallet::Serialize()
{
	unsigned long numAddresses = keyPairs.size();
	std::vector<unsigned char> w;
	w.resize( sizeof( unsigned long ) + numAddresses * ADDRESS_LENGTH );
	memcpy( &(w[0]), &numAddresses, sizeof( unsigned long ) );	
	int i = 0;
	for( std::vector<keyPair>::iterator it = keyPairs.begin(); it != keyPairs.end(); ++it )
	{
		keyPair a = (*it);
		memcpy( &(w[sizeof( unsigned long ) + i * ADDRESS_LENGTH]), a.privateKey, crypto_sign_ed25519_SECRETKEYBYTES );
		memcpy( &(w[sizeof( unsigned long ) + i * ADDRESS_LENGTH + crypto_sign_ed25519_SECRETKEYBYTES] ), a.publicKey, crypto_sign_ed25519_PUBLICKEYBYTES );
		++i;
	}
	return w;
}

void Wallet::Deserialize( unsigned char *w )
{
	unsigned long numAddresses;
	memcpy( &numAddresses, &(w[0]), sizeof( unsigned long ) );
	
	for ( unsigned long i = 0; i < numAddresses; ++i )
	{
		keyPair a;
		memcpy( a.privateKey, &(w[sizeof( unsigned long ) + i * ADDRESS_LENGTH]), crypto_sign_ed25519_SECRETKEYBYTES );
		memcpy( a.publicKey, &(w[sizeof( unsigned long ) + i * ADDRESS_LENGTH + crypto_sign_ed25519_SECRETKEYBYTES] ), crypto_sign_ed25519_PUBLICKEYBYTES );
		keyPairs.push_back( a );
	}
}

std::vector<keyPair> *Wallet::GetAddresses()
{
	return &keyPairs;	
}

std::string Wallet::GetFilename()
{
	return filename;
}

std::string Wallet::ToString()
{
	std::string s = "";
	for( std::vector<keyPair>::iterator it = keyPairs.begin(); it != keyPairs.end(); ++it )
	{
		keyPair a = (*it);
		s += UnsignedCharArrayToString( a.privateKey, crypto_sign_ed25519_SECRETKEYBYTES ) + " ";
		s += UnsignedCharArrayToString( a.publicKey, crypto_sign_ed25519_PUBLICKEYBYTES );
	}
	return s;
}

bool Wallet::IsPurged( keyPair kp )
{
	char zeroBlock[crypto_sign_ed25519_SECRETKEYBYTES];
	memset( zeroBlock, 0, crypto_sign_ed25519_SECRETKEYBYTES );
	return ( memcmp( kp.privateKey, zeroBlock, crypto_sign_ed25519_SECRETKEYBYTES ) == 0 );
}

void Wallet::PurgeSecrets()
{
	for( std::vector<keyPair>::iterator it = keyPairs.begin(); it != keyPairs.end(); ++it )
	{
		keyPair a = (*it);
		memset( a.privateKey, 0, crypto_sign_ed25519_SECRETKEYBYTES );
	}	
}
