#ifndef SOCKSSERVER_H
#define SOCKSSERVER_H

#include <map>

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

#include <muduo/base/Timestamp.h>
#include <muduo/net/TcpServer.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/EventLoop.h>
#include <muduo/base/Types.h>

#include "Tunnel.h"
#include "Config.h"

class SocksServer: private boost::noncopyable
{
public:
	//typedef boost::function<void(muduo::net::EventLoop*)> ThreadInitCallback;

	SocksServer(muduo::net::EventLoop* loop, const Config& config);

	void start();


	TunnelPtr getTunnel(const muduo::string& tunnelName) const ;
	bool addTunnel(const muduo::string& tunnelName, TunnelPtr tunnel);
	bool delTunnel(const muduo::string& tunnelName);

private:
	typedef std::map<muduo::string, TunnelPtr> TunnelMap;

	void onConnection(const muduo::net::TcpConnectionPtr& conn);
	void onMessage(const muduo::net::TcpConnectionPtr& conn,
				   muduo::net::Buffer* buf,
				   muduo::Timestamp time);
	void onWriteComplete(const muduo::net::TcpConnectionPtr& conn);
	void onHighWaterMark(const muduo::net::TcpConnectionPtr& conn);
	void checkTunnel(const muduo::net::TcpConnectionPtr& conn);
	void parseCommand(const muduo::net::TcpConnectionPtr conn, 
					  muduo::net::Buffer* buf, 
					  muduo::Timestamp time);

	muduo::net::EventLoop* loop_;
	muduo::net::TcpServer server_;
	int numClients_;
	int maxNumClients_;
	TunnelMap primaryTunnels_;
};
#endif