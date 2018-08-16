#ifndef UTILS_H
#define UTILS_H

#include <sodium.h>
#include <string>
#include <string.h>

std::string addressToString( unsigned char *a );

std::string hashToString( unsigned char *h );

std::string UnsignedCharArrayToString( unsigned char *a, unsigned long long int length );

void StringToUnsignedCharArray( std::string hex, unsigned char *a );

#endif