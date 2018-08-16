#include "ledger.h"
#include <stdio.h>
#include <string.h>
#include "utils.h"
#include <algorithm>

Ledger::Ledger()
{
}

std::vector<unsigned char> Ledger::Serialize()
{
	unsigned long numAccounts = static_cast<unsigned long>(accounts.size());
	std::vector<unsigned char> netLedger;
	netLedger.resize( sizeof( unsigned long ) + numAccounts * ACCOUNT_LENGTH );
	
	memcpy( &(netLedger[0]), &numAccounts, sizeof( unsigned long ) );
	
	int i = 0;
	for( std::map<std::string, account>::iterator it = accounts.begin(); it != accounts.end(); ++it )
	{
		account a = (*it).second;
		memcpy( &(netLedger[sizeof( unsigned long ) + i * ACCOUNT_LENGTH]), a.address, crypto_sign_ed25519_PUBLICKEYBYTES );
		memcpy( &(netLedger[sizeof( unsigned long ) + i * ACCOUNT_LENGTH + crypto_sign_ed25519_PUBLICKEYBYTES] ), &(a.balance), sizeof( unsigned long long ) );
		memcpy( &(netLedger[sizeof( unsigned long ) + i * ACCOUNT_LENGTH + crypto_sign_ed25519_PUBLICKEYBYTES + sizeof( unsigned long long )] ), &(a.index), sizeof( unsigned long ) );
		++i;
	}
	
	return netLedger;	
}

void Ledger::Deserialize( unsigned char *led )
{
	unsigned long numAccounts;
	memcpy( &numAccounts, &(led[0]), sizeof( unsigned long ) );
	
	for ( unsigned long i = 0; i < numAccounts; ++i )
	{
		account a;
		memcpy( a.address, &(led[sizeof( unsigned long ) + i * ACCOUNT_LENGTH]), crypto_sign_ed25519_PUBLICKEYBYTES );
		memcpy( &(a.balance), &(led[sizeof( unsigned long ) + i * ACCOUNT_LENGTH + crypto_sign_ed25519_PUBLICKEYBYTES] ), sizeof( unsigned long long ) );
		memcpy( &(a.index), &(led[sizeof( unsigned long ) + i * ACCOUNT_LENGTH + crypto_sign_ed25519_PUBLICKEYBYTES + sizeof( unsigned long long )] ), sizeof( unsigned long ) );
		accounts.insert( {addressToString( a.address ), a} );
	}
}

bool Ledger::ApplyBlockDelta( BlockDelta *bd )
{
	if ( bd == NULL )
	{
		return false;
	}
	
	// update ledger
	std::vector<Transaction> *transactions = bd->GetTransactions();
	for( std::vector<Transaction>::iterator it = transactions->begin(); it != transactions->end(); ++it )
	{
		Transaction t = *it;
		if ( VerifyTransaction( t ) )
		{
			DoTransaction( t, bd->GetFacilitator() );
		}
		else
		{
			return false;
		}
	}
	
	// award to facilitator	
	BlockReward( bd->GetFacilitator(), bd->GetBlockReward() );
	
	return true;	
}

void Ledger::CheckCull()
{
	if ( accounts.size() > MAX_ACCOUNTS )
	{		
		unsigned int cull = CULL_FRACTION * MAX_ACCOUNTS;		
		
		std::vector<unsigned long long> balances;		
		balances.reserve( accounts.size() );
		for( std::map<std::string, account>::iterator it = accounts.begin(); it != accounts.end(); ++it )
		{			 
			balances.push_back( ((*it).second).balance );
		}
		std::sort ( balances.begin(), balances.end() );
		unsigned long long cullValue = balances[cull];

		std::vector<std::string> cullKeys;
		for( std::map<std::string, account>::iterator it = accounts.begin(); it != accounts.end(); ++it )
		{	
			std::string key = (*it).first;		 
			account a = (*it).second;	
			if ( a.balance <= cullValue )
			{
				cullKeys.push_back( key );
			}
		}
		for( std::vector<std::string>::iterator it = cullKeys.begin(); it != cullKeys.end(); ++it ) 
		{
			accounts.erase( (*it) );	
		}		
	}
}

bool Ledger::DoesAccountExist( unsigned char *address )
{
	if ( accounts.find( addressToString( address ) ) == accounts.end() )
	{
		return false;
	}
	return true;
}

unsigned long Ledger::GetAccountIndex( unsigned char *address )
{
	if ( DoesAccountExist( address ) )
	{
		return (accounts.at( addressToString( address ) )).index;
	}
	return 0;
}

unsigned long long Ledger::GetAccountBalance( unsigned char *address )
{
	if ( DoesAccountExist( address ) )
	{
		return (accounts.at( addressToString( address ) )).balance;
	}
	return 0;
}

void Ledger::CreateIfNotExists( unsigned char *address )
{
	if ( !DoesAccountExist( address ) )
	{
		account a;
		memcpy( a.address, address, crypto_sign_ed25519_PUBLICKEYBYTES );
		a.balance = 0;
		a.index = 0;
		accounts.insert( {addressToString( a.address ), a} );
	}	
}

bool Ledger::VerifyTransaction( Transaction t )
{
	// make sure account exists (can only transfer from already existing account) and is not duplicate transaction (check index)					
	unsigned char *from = t.GetFrom();
	if ( DoesAccountExist( from ) && t.GetIndex() == GetAccountIndex( from ) + 1 )
	{ 
		// make sure has enough funds to pay
		if ( GetAccountBalance( from ) >= t.GetAmount() ) // ledger is updated on the fly, so handles case where >1 from same account in Block
		{
			return true;			
		}
	}	
	return false;
}

void Ledger::DoTransaction( Transaction t, unsigned char *facilitator )
{
	account& a = accounts.at( addressToString( t.GetFrom() ) );
	a.balance -= ( t.GetAmount() + t.GetToFacilitator() );
	++(a.index);
	
	if ( a.balance == 0 ) 
	{
		accounts.erase( addressToString( t.GetFrom() ) );
	}

	CreateIfNotExists( t.GetTo() );
	account& b = accounts.at( addressToString( t.GetTo() ) );
	b.balance += t.GetAmount();	
	
	CreateIfNotExists( facilitator );
	account& c = accounts.at( addressToString( facilitator ) );
	c.balance += t.GetToFacilitator();	
}

void Ledger::BlockReward( unsigned char *facilitator, unsigned long long reward )
{
	CreateIfNotExists( facilitator );
	account& c = accounts.at( addressToString( facilitator ) );
	c.balance += reward;	
}

std::string Ledger::ToString()
{
	std::string s;
	for( std::map<std::string, account>::iterator it = accounts.begin(); it != accounts.end(); ++it )
	{
		account a = (*it).second;
		s += "Address: " + addressToString( a.address ) + " - ";
		s += "Balance: " + std::to_string( a.balance ) + " - ";
		s += "Index: " + std::to_string( a.index ) + "\n";
	}	
	return s;
}

// *** add checks for overflow errors