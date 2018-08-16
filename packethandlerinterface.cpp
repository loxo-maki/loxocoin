#include "packethandlerinterface.h"

PacketHandlerInterface::PacketHandlerInterface()
{
	
}

bool PacketHandlerInterface::TransactionHandler( unsigned char *, unsigned int, bufferevent * ) { return false; };
bool PacketHandlerInterface::DeltaHandler( unsigned char *, unsigned int, bufferevent * ) { return false; };
std::vector<unsigned char> PacketHandlerInterface::LedgerRequestHandler( unsigned char *, unsigned int, bufferevent * ) { std::vector<unsigned char>b; return b; };
bool PacketHandlerInterface::LedgerHandler( unsigned char *, unsigned int, bufferevent * ) { return false; };	
std::vector<unsigned char> PacketHandlerInterface::BalanceRequestHandler( unsigned char *, unsigned int, bufferevent * ) { std::vector<unsigned char>b; return b; };	
bool PacketHandlerInterface::BalanceHandler( unsigned char *, unsigned int, bufferevent * ) { return false; };		

