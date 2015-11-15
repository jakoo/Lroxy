#include <string>
#include <boost/any.hpp>
#include <boost/bind.hpp>

#include <muduo/net/Endian.h>
#include <muduo/base/Types.h>

#include "SocksServer.h"
#include "ConnectTunnel.h"
#include "BindTunnel.h"

using std::map;
using boost::any;
using boost::any_cast;
using boost::bind;
using namespace muduo::net;
using namespace muduo;

SocksServer::SocksServer(EventLoop* loop, const Config& config)
:	loop_(loop),
server_(loop, config.listenAddr(), "SocksServer"),
numClients_(0),
maxNumClients_(config.maxNumClients()),
primaryTunnels_(TunnelMap())
{
	server_.setThreadNum(config.numThreads());
	server_.setConnectionCallback(bind(&SocksServer::onConnection, this, _1));
	server_.setMessageCallback(bind(&SocksServer::onMessage, this, _1, _2, _3));
}

void SocksServer::start()
{
	server_.start();
}

TunnelPtr SocksServer::getTunnel(const string& tunnelName) const
{
	TunnelMap::const_iterator it = primaryTunnels_.find(tunnelName);

	if (it == primaryTunnels_.cend())
		return nullptr;

	return it->second;
}

bool SocksServer::addTunnel(const string& tunnelName, TunnelPtr tunnel)
{
	TunnelMap::iterator it = primaryTunnels_.find(tunnelName);

	if (it != primaryTunnels_.end()) //exist name
		return false;

	primaryTunnels_[tunnelName] = tunnel;
	return true;
}

bool SocksServer::delTunnel(const string& tunnelName)
{
	TunnelMap::iterator it = primaryTunnels_.find(tunnelName);

	if (it == primaryTunnels_.end()) // tunnle doesn't exist
		return false;

	primaryTunnels_.erase(it);
	return true;
}

void SocksServer::onConnection(const TcpConnectionPtr& conn)
{
	LOG_TRACE << conn->peerAddress().toIpPort() << " -> "
	<< conn->localAddress().toIpPort() << " is "
	<< (conn->connected() ? "UP" : "DOWN");	

	++numClients_;
	if (conn->connected())
	{
		if (numClients_ > maxNumClients_)
		{  
			conn->forceClose();
			LOG_TRACE << "Reach maxNumClients. Refuse request from "
			<< conn->peerAddress().toIpPort();

			--numClients_;
			return;
		}

		LOG_DEBUG << "conn->set (Callback) : " << conn->name();
		conn->setWriteCompleteCallback(bind(&SocksServer::onWriteComplete, this, _1));
		conn->setHighWaterMarkCallback(bind(&SocksServer::onHighWaterMark, this, _1), 64*1024*1024);

		conn->setTcpNoDelay(true);
		// if no tunnel created after 2 minutes, close the connection
		loop_->runAfter(120, bind(&SocksServer::checkTunnel, this, conn)); 
	}
  	else //disconnected
  	{
  		--numClients_;

  		if (!conn->getContext().empty())
  		{
  			TunnelPtr tunnel = any_cast<TunnelPtr>(conn->getContext());
  			tunnel->close(); // appServerConn will be closed by TcpClient/TcpServer destructor
  		}
  	}
  }

  void SocksServer::onMessage(const TcpConnectionPtr& conn,
  	muduo::net::Buffer* buf,
  	muduo::Timestamp time)
  {							   
  	if (conn->getContext().empty())
  	{
  		parseCommand(conn, buf, time);
  		return;
  	}

	// Tunnel established:
  	TunnelPtr tunnel = any_cast<TunnelPtr>(conn->getContext());
  	TcpConnectionPtr appServerConn = tunnel->appServerConn();

	// Shouldn't get data from appClientConn while appServerConn isn't established
	// Error! Stop connect, close tunnel and close this conn
  	if (!appServerConn)
  	{
  		tunnel->close();
  		conn->shutdown();
  		return;
  	}

	// Relay data:
  	size_t size = buf->readableBytes();
  	appServerConn->send(buf);
  	LOG_TRACE << conn->name() << " recv " << size 
  	<< " bytes at " << time.toString();
  }

  void SocksServer::onWriteComplete(const TcpConnectionPtr& conn)
  {
  	TunnelPtr tunnel = any_cast<TunnelPtr>(conn->getContext());
  	if (tunnel->closed())
  		conn->forceClose();

  	TcpConnectionPtr appServerConn = tunnel->appServerConn();
  	if (appServerConn && !appServerConn->isReading())
  		appServerConn->startRead();
  }

  void SocksServer::onHighWaterMark(const muduo::net::TcpConnectionPtr& conn)
  {
  	TunnelPtr tunnel = any_cast<TunnelPtr>(conn->getContext());
  	TcpConnectionPtr appServerConn = tunnel->appServerConn();
  	if (appServerConn && appServerConn->isReading())
  		appServerConn->stopRead();
  }

  void SocksServer::checkTunnel(const TcpConnectionPtr& conn)
  {
  	if (!conn->getContext().empty())
  		return;

  	LOG_TRACE << conn->name() << " wait CONNECT command timeout";
  	conn->forceClose();
  }

  void SocksServer::parseCommand(const muduo::net::TcpConnectionPtr conn, 
  	muduo::net::Buffer* buf, 
  	muduo::Timestamp time)
  {	
  	LOG_DEBUG << "ParseCommand, connName : " << conn->name();
  	char VN;
  	char CD;
  	const void* DSTPORT;
  	const void* DSTIP;

  	/*if (buf->readableBytes() > 128)
  	{
  		conn->forceClose();
  	}
  	else */
  	if (buf->readableBytes() > 8)
  	{
  		const char* begin = buf->peek() + 8;
  		const char* end = buf->peek() + buf->readableBytes();
  		const char* where = std::find(begin, end, '\0');
  		if (where != end)
  		{
  			VN = buf->peek()[0];
  			CD = buf->peek()[1];
  			DSTPORT = buf->peek() + 2;
  			DSTIP = buf->peek() + 4;

  			sockaddr_in addr;
  			bzero(&addr, sizeof addr);
  			addr.sin_family = AF_INET;
  			addr.sin_port = *static_cast<const in_port_t*>(DSTPORT);
  			addr.sin_addr.s_addr = *static_cast<const uint32_t*>(DSTIP);

  			//bool socks4a = sockets::networkToHost32(addr.sin_addr.s_addr) < 256;
  			bool okay = true;
  			/*
  			if (socks4a)
  			{
  				const char* endOfHostName = std::find(where+1, end, '\0');
  				if (endOfHostName != end)
  				{
  					string hostname = where+1;
  					where = endOfHostName;
  					LOG_INFO << "Socks4a host name " << hostname;
  					InetAddress tmp;
  					if (InetAddress::resolve(hostname, &tmp))
  					{
  						addr.sin_addr.s_addr = tmp.ipNetEndian();
  						okay = true;
  					}
  				}
  				else
  				{
  					return;
  				}
  			}
  			else
  			{
  				okay = true;
  			}
			*/
  			InetAddress inetAddr(addr);
  			if (VN == 4 && CD == 1 && okay)
  			{
  				TunnelPtr tunnel(new ConnectTunnel(this, conn, VN, CD, inetAddr));
  				tunnel->setup();
          	//tunnel->connect();
          	//g_tunnels[conn->name()] = tunnel;
  				buf->retrieveUntil(where+1);
          	//char response[] = "\000\x5aUVWXYZ";
          	//memcpy(response+2, &addr.sin_port, 2);
          	//memcpy(response+4, &addr.sin_addr.s_addr, 4);
          	//conn->send(response, 8);
				conn->setContext(tunnel);
  			}
  			else if (VN == 4 && CD == 2 && okay)
  			{
  				TunnelPtr tunnel(new ConnectTunnel(this, conn, VN, CD, inetAddr));
  				tunnel->setup();
  				buf->retrieveUntil(where+1);
				conn->setContext(tunnel);
  			}
  			else
  			{
  				LOG_TRACE << "Unknown Command, close connection : "
						  << conn->name();
  				char response[] = "\000\x5bUVWXYZ";
  				conn->send(response, 8);
  				conn->shutdown();
  			}
  		}
  	}
}
