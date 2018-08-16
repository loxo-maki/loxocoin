#ifndef NETCOM_H
#define NETCOM_H

#include <string>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include "netmode.h"
#include <map>
#include "packethandlerinterface.h"

struct sendDataInfo
{
	unsigned char *data; 
	unsigned int length;
	event_base *base;
	int dataIndex;
};

class NetCom;

class NetCom
{
	public:

		static void Init( PacketHandlerInterface *_ph );
		static void Listen();
		static void Send( unsigned char *data, unsigned int length, unsigned int sendLimit ); // sendLimit = 0 will send to all hosts
				
	protected:
	
		NetCom() {};
	
		static void Accept( evconnlistener *listener, evutil_socket_t sock, sockaddr *address, int socklen, void *arg );
		static void HandleReadEvent( bufferevent *bev, short events, void *arg );
		static void ReadRequest( bufferevent *bev, void *arg );		
		static void ReadResponse( bufferevent *bev, void *arg );
		static void HandleSendEvent( bufferevent *bev, short events, void *arg );
		static void SendComplete( bufferevent *bev, void *arg );
		static void ClearSendMem( sendDataInfo *sdi );
		
		static PacketHandlerInterface *ph;
		static std::map<int, sendDataInfo> sdi;
		static int dataIndex;
		static std::vector<std::string> hosts;	
		static event_base *listener;	
};

#endif