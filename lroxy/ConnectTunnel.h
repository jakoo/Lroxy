#ifndef CONNECTTUNNEL_H
#define CONNECTTUNNEL_H

#include <map>
#include <string>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <muduo/net/TcpClient.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/net/InetAddress.h>
#include <muduo/base/Timestamp.h>
#include <muduo/base/Types.h>

#include "Tunnel.h"

class BindTunnel;
typedef boost::shared_ptr<BindTunnel> BindTunnelPtr;

class ConnectTunnel: public Tunnel
{
public:
	ConnectTunnel(SocksServer* socksServer,
		   		  const muduo::net::TcpConnectionPtr& appClientConn,
		   		  uint8_t VN,
		   		  uint8_t CD,
		   		  muduo::net::InetAddress inetaddr);
	virtual void setup();
	virtual void close();
	static muduo::string makeName(muduo::net::InetAddress inetaddr, muduo::string);
	void addBindTunnel(muduo::string bindTunnelName, BindTunnelPtr bindTunnel);
	void delBindTunnel(muduo::string bindTunnelName);
	//muduo::net::InetAddress appServerAddress();

private:
	virtual void onConnection(const muduo::net::TcpConnectionPtr& conn);
	virtual void onMessage(const muduo::net::TcpConnectionPtr& conn,
				   muduo::net::Buffer* buf,
				   muduo::Timestamp time);
	virtual void onWriteComplete(const muduo::net::TcpConnectionPtr& conn);
	virtual void onHighWaterMark(const muduo::net::TcpConnectionPtr& conn);
	virtual void checkTimeout();
	boost::scoped_ptr<muduo::net::TcpClient> client_;
	std::map<muduo::string, BindTunnelPtr> bindTunnels_;
};

typedef boost::shared_ptr<ConnectTunnel> ConnectTunnelPtr;
#endif