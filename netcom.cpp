#include "netcom.h"

#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/event.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

PacketHandlerInterface *NetCom::ph = NULL;
std::map<int, sendDataInfo> NetCom::sdi;
int NetCom::dataIndex = 0;
std::vector<std::string> NetCom::hosts;	

void NetCom::Init( PacketHandlerInterface *_ph )
{
	ph = _ph;
	dataIndex = 0;
	
	FILE *hostsFile;
	hostsFile = fopen( "hosts", "r");	
	if ( hostsFile != NULL )
	{
		char *line = NULL;
    		size_t len = 0;
		while ( getline( &line, &len, hostsFile ) != -1 ) 
		{
			std::string s( line );
			s.erase( s.find_last_not_of( " \n\r\t" ) + 1 );
			hosts.push_back( s );
		}
		if ( line )
		{
			free( line );
		}
	}
	fclose( hostsFile );
}

void NetCom::Listen()
{
	event_base *tcpbase;
        evconnlistener *tcplistener;
        sockaddr_in tcpsin;
        
        memset( &tcpsin, 0, sizeof( tcpsin ) );
        tcpsin.sin_family = AF_INET;
        tcpsin.sin_addr.s_addr = htonl( INADDR_ANY );
        tcpsin.sin_port = htons( LOXO_PORT );
		
	tcpbase = event_base_new();
		
	tcplistener = evconnlistener_new_bind( tcpbase, Accept, NULL, LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE, -1, (struct sockaddr*)&tcpsin, sizeof(tcpsin) );
        if ( !tcplistener ) 
        {
                printf( "Couldn't create tcp listener\n" );
                return;
        }
	
	event_base_dispatch( tcpbase );	
	event_base_free( tcpbase );
}

void NetCom::Accept( evconnlistener *listener, evutil_socket_t sock, sockaddr *address, int socklen, void *arg )
{
        event_base *base = evconnlistener_get_base( listener );
        bufferevent *bev = bufferevent_socket_new( base, sock, BEV_OPT_CLOSE_ON_FREE );
        bufferevent_setcb( bev, ReadRequest, NULL, HandleReadEvent, NULL);
        bufferevent_enable( bev, EV_READ|EV_WRITE );
}

void NetCom::HandleReadEvent( bufferevent *bev, short events, void *arg )
{
        if ( events & ( BEV_EVENT_EOF | BEV_EVENT_ERROR ) ) 
        {
                bufferevent_free( bev );
        }
}

void NetCom::ReadRequest( bufferevent *bev, void *arg )
{
        unsigned char buf[MAX_PACKET_LENGTH];
	unsigned long n;
	evbuffer *input = bufferevent_get_input( bev );	
	while ( ( n = evbuffer_get_length( input ) ) > 0 ) 
	{
		n = evbuffer_remove( input, buf, sizeof( buf ) );
	}

	packetType pt;
	memcpy( &pt, buf, sizeof( unsigned char ) );
	
	if ( pt == PT_TRANSACTION )
	{	
		ph->TransactionHandler( buf, TRANSACTION_LENGTH, bev );			
		bufferevent_free( bev );	
	}
	else if ( pt == PT_DELTA )
	{
		unsigned int packetLength;
		memcpy( &packetLength, &(buf[sizeof( unsigned char )]), sizeof( unsigned int ) );
		ph->DeltaHandler( buf, packetLength, bev );
		bufferevent_free( bev );
	}
	else if ( pt == PT_LEDGER_REQUEST )
	{
		std::vector<unsigned char> b = ph->LedgerRequestHandler( buf, 0, bev );
		bufferevent_write( bev, &(b[0]), b.size() );
	}
	else if ( pt == PT_BALANCE_REQUEST )
	{
		std::vector<unsigned char> b = ph->BalanceRequestHandler( buf, 0, bev );
		bufferevent_write( bev, &(b[0]), b.size() );
	}
}

void NetCom::Send( unsigned char *data, unsigned int length, unsigned int sendLimit )
{
	event_base *tcpbase;
	bufferevent *bev;
	sockaddr_in tcpsin;
	
	tcpbase = event_base_new();
	
	memset( &tcpsin, 0, sizeof(tcpsin) );
	tcpsin.sin_family = AF_INET;	
	tcpsin.sin_port = htons( LOXO_PORT );
	bev = bufferevent_socket_new( tcpbase, -1, BEV_OPT_CLOSE_ON_FREE );
	
	unsigned int sends = 0;
	for( std::vector<std::string>::iterator it = hosts.begin(); it != hosts.end(); ++it )
	{
		std::string s = *it;	
	
		inet_pton( AF_INET, &(s[0]), &(tcpsin.sin_addr) );	
		
		sendDataInfo sdil;
		sdil.data = data;
		sdil.length = length;
		sdil.base = tcpbase;
		sdil.dataIndex = dataIndex;
		sdi[dataIndex++] = sdil;
		
		bufferevent_setcb( bev, ReadResponse, NULL, HandleSendEvent, &(sdi[sdil.dataIndex]) );
		
	    	bufferevent_enable( bev, EV_READ|EV_WRITE );
		
		if ( bufferevent_socket_connect( bev, (struct sockaddr *)&tcpsin, sizeof(tcpsin) ) < 0 ) // this does not actually connect, just sets up socket for connecting
		{
			printf( "Error: %s\n", evutil_socket_error_to_string( EVUTIL_SOCKET_ERROR() ) );         
			bufferevent_free( bev );
		}
		else
		{
			++sends;
		}
		/*if ( sendLimit > 0 && sends >= sendLimit )
		{
			break;
		}*/
	}
	
	event_base_dispatch( tcpbase );
	event_base_free( tcpbase );
}

void NetCom::ReadResponse( bufferevent *bev, void *arg )
{
        evbuffer *input = bufferevent_get_input( bev );	
        
        unsigned char buf[MAX_PACKET_LENGTH];
	unsigned long n;	
	while ( ( n = evbuffer_get_length( input ) ) > 0 ) 
	{
		n = evbuffer_remove( input, buf, sizeof( buf ) );
	}

	packetType pt;
	memcpy( &pt, buf, sizeof( unsigned char ) );
	
	if ( pt == PT_LEDGER )
	{
		ph->LedgerHandler( buf, 0, bev );		
	}
	if ( pt == PT_BALANCE )
	{
		ph->BalanceHandler( buf, 0, bev );
	}
	SendComplete( bev, arg );
}

void NetCom::HandleSendEvent( bufferevent *bev, short events, void *arg )
{
    if ( events & BEV_EVENT_CONNECTED ) 
    {
    	 sendDataInfo sdil = (*((sendDataInfo *)arg));    	 

	 unsigned char t;         
	 memcpy( &t, sdil.data, sizeof( unsigned char ) );
	 packetType pt;
	 pt = (packetType)t;
         if ( pt == PT_LEDGER_REQUEST || pt == PT_BALANCE_REQUEST )
         {
         	bufferevent_setcb( bev, ReadResponse, NULL, HandleSendEvent, arg );
         }
         else  // close after send if not waiting for response
         {
         	bufferevent_setcb( bev, NULL, SendComplete, HandleSendEvent, arg );
         }
         
         bufferevent_write( bev, sdil.data, sdil.length );
    } 
    else if ( events & ( BEV_EVENT_EOF | BEV_EVENT_ERROR ) ) 
    {
         SendComplete( bev, arg );
    }
}

void NetCom::SendComplete( bufferevent *bev, void *arg ) // may not need this
{
	bufferevent_free( bev );
	ClearSendMem( (sendDataInfo *)arg );
}

void NetCom::ClearSendMem( sendDataInfo *sdil )
{
	std::map<int, sendDataInfo>::iterator it = sdi.find( (*sdil).dataIndex );
  	if ( it != sdi.end() )
  	{
    		sdi.erase ( it );
    	}	
}