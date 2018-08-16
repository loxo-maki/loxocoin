#include "utils.h"
#include <sstream>
#include <iomanip>

std::string addressToString( unsigned char *a )
{
	char s[2 * crypto_sign_ed25519_PUBLICKEYBYTES];
	for ( unsigned int i = 0; i < crypto_sign_ed25519_PUBLICKEYBYTES; ++i )
	{
		sprintf( &(s[2 * i]), "%02x", ((unsigned char *)a)[i] );
	}
	return std::string( s );
}

std::string hashToString( unsigned char *h )
{
	char s[2 * crypto_generichash_BYTES];
	for ( unsigned int i = 0; i < crypto_generichash_BYTES; ++i )
	{
		sprintf( &(s[2 * i]), "%02x", ((unsigned char *)h)[i] );
	}
	return std::string( s );
}

std::string UnsignedCharArrayToString( unsigned char *a, unsigned long long int length )
{
	std::stringstream ss;
	for( unsigned int i = 0; i < length; ++i )
	{
        	ss << std::hex << std::setw(2) << std::setfill('0') << (int)a[i];
        }
    	return ss.str();
}

void StringToUnsignedCharArray( std::string hex, unsigned char *a )
{
	int pos = 0;
	for ( unsigned int count = 0; count < hex.size()/sizeof( unsigned char ); ++count ) 
	{
		sscanf( &(hex[pos]), "%2hhx", &(a[count]) );
		pos += 2;
	}
}