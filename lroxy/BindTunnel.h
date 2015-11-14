#ifndef BINDTUNNEL_H
#define BINDTUNNEL_H

#include <string>
#include <boost/scoped_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/net/TcpServer.h>
#include <muduo/base/Timestamp.h>
#include <muduo/base/Types.h>

#include "Tunnel.h"

class ConnectTunnel;
class SocksServer;

class BindTunnel: public Tunnel
{
public:
	BindTunnel(SocksServer* socksServer,
		   	   const muduo::net::TcpConnectionPtr& appClientConn,
		   	   uint8_t VN,
		   	   uint8_t CD,
		   	   muduo::net::InetAddress inetAddr);
	static muduo::string makeName(muduo::net::InetAddress inetAddr, muduo::string USERID);
	virtual void setup();
	virtual void close();

private:
	virtual void onConnection(const muduo::net::TcpConnectionPtr& conn);
	virtual void onMessage(const muduo::net::TcpConnectionPtr& conn,
				   muduo::net::Buffer* buf,
				   muduo::Timestamp time);
	virtual void onWriteComplete(const muduo::net::TcpConnectionPtr& conn);
	virtual void onHighWaterMark(const muduo::net::TcpConnectionPtr& conn);
	virtual void checkTimeout();
	boost::scoped_ptr<muduo::net::TcpServer> server_;
	boost::weak_ptr<ConnectTunnel> primaryTunnel_;
	muduo::string primaryTunnelName_;
	muduo::net::InetAddress listenAddr_;
};

typedef boost::shared_ptr<BindTunnel> BindTunnelPtr;
#endif