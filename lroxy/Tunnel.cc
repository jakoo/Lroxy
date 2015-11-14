#include <cstring>
#include <string>
#include <arpa/inet.h>
#include <boost/scoped_array.hpp>

#include <muduo/base/Logging.h>
#include <muduo/base/Types.h>

#include "Tunnel.h"

using boost::scoped_array;
using namespace muduo::net;
using namespace muduo;

Tunnel::Tunnel(SocksServer* socksServer, 
	   const TcpConnectionPtr& appClientConn,
	   uint8_t VN, 
	   uint8_t CD,
	   InetAddress inetAddr)
  :	socksServer_(socksServer), 
  	state_(Builded),
  	appClientConn_(appClientConn),
  	appServerConn_(nullptr),
  	VN_(VN),
  	CD_(CD),
  	inetAddr_(inetAddr),
  	name_(string())
{
}

void Tunnel::setState(StatE stat)
{
	state_ = stat;
}

InetAddress Tunnel::createInetAddress(uint16_t port, uint32_t ip)
{
	struct sockaddr_in addr;
	bzero(&addr, sizeof addr);
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(ip);
	return InetAddress(addr);
}

void Tunnel::response(char VN, char CD, InetAddress* inetAddr)
{
	char msg[8];
	msg[0] = VN;
	msg[1] = CD;
	if (inetAddr == nullptr)
		inetAddr = &inetAddr_;
	uint16_t port = inetAddr->portNetEndian();
	uint32_t ip = inetAddr->ipNetEndian();
	memcpy(&msg[2], &port, 2);
	memcpy(&msg[4], &ip, 4);

	TcpConnectionPtr appClientConn = appClientConn_.lock();
	if (appClientConn)
		appClientConn->send(&msg[0], 8);
}
