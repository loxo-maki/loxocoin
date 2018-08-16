#ifndef WALLET_H
#define WALLET_H

#include <vector>
#include <string>
#include <sodium.h>

struct keyPair
{
	unsigned char privateKey[crypto_sign_ed25519_SECRETKEYBYTES];
	unsigned char publicKey[crypto_sign_ed25519_PUBLICKEYBYTES];
};

#define ADDRESS_LENGTH ( crypto_sign_ed25519_SECRETKEYBYTES + crypto_sign_ed25519_PUBLICKEYBYTES )

class Wallet
{
	std::vector<keyPair> keyPairs;	
	public:
		Wallet( std::string _filename );
		std::vector<unsigned char> Serialize();
		void Deserialize( unsigned char *w );
		void AddNewKeyPair();
		std::vector<keyPair> *GetAddresses();
		std::string ToString();
		std::string GetFilename();
		
		bool IsPurged( keyPair kp );
		void PurgeSecrets();
	protected:
		std::string filename;
};

#endif