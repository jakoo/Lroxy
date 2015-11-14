#include <netinet/in.h>
#include <arpa/inet.h>
#include <boost/bind.hpp>
#include <boost/scoped_array.hpp>
#include <boost/shared_ptr.hpp>

#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/Buffer.h>
#include <muduo/net/TcpServer.h>
#include <muduo/base/Types.h>

#include "SocksServer.h"
#include "ConnectTunnel.h"
#include "BindTunnel.h"

using boost::bind;
using boost::scoped_array;
using boost::shared_ptr;
using boost::dynamic_pointer_cast;
using namespace muduo;
using namespace muduo::net;

BindTunnel::BindTunnel(SocksServer* socksServer,
		   		  	   const muduo::net::TcpConnectionPtr& appClientConn,
		   		  	   uint8_t VN,
		   		  	   uint8_t CD,
		   		  	   InetAddress inetAddr)
  : Tunnel(socksServer, appClientConn, VN, CD, inetAddr),
  	server_(nullptr),
  	primaryTunnel_(),
  	primaryTunnelName_(),
  	listenAddr_()
{}

string BindTunnel::makeName(InetAddress inetAddr, string USERID)
{
	uint32_t ip = inetAddr.ipNetEndian();
	uint16_t port = inetAddr.portNetEndian();
	scoped_array<char> ipStr(new char[INET_ADDRSTRLEN]);
	scoped_array<char> portStr(new char[10]);
	inet_ntop(AF_INET, &ip, &ipStr[0], INET_ADDRSTRLEN);
	sprintf(&portStr[0], "%u", port);
	return string("BindTunnel ") + string(&ipStr[0]) + " " +
		   string(&portStr[0]) + " " + USERID;
}

void BindTunnel::setup()
{
	name_ = string("BindTunnel ") + inetAddr_.toIpPort();
	primaryTunnelName_ = string("ConnectTunnel ") + inetAddr_.toIpPort();

	primaryTunnel_ = dynamic_pointer_cast<ConnectTunnel>(socksServer_->getTunnel(primaryTunnelName_));
	if (primaryTunnel_.lock()) // primary connection doesn't exist
	{
		LOG_TRACE << "Primary connection doesn't exist, error BIND request : "
				  << name_;
		close();
		return;
	}

	// random port
	server_.reset(new TcpServer(EventLoop::getEventLoopOfCurrentThread(), listenAddr_,
				  string(name_ + "server_").c_str()));
	server_->setConnectionCallback(bind(&BindTunnel::onConnection, 
		dynamic_pointer_cast<BindTunnel>(shared_from_this()), _1));
	server_->setMessageCallback(bind(&BindTunnel::onMessage, 
		dynamic_pointer_cast<BindTunnel>(shared_from_this()), _1, _2, _3));
	server_->setWriteCompleteCallback(bind(&BindTunnel::onWriteComplete, 
		dynamic_pointer_cast<BindTunnel>(shared_from_this()), _1));
	server_->start();
	EventLoop::getEventLoopOfCurrentThread()->runAfter(120, bind(&BindTunnel::checkTimeout, 
		dynamic_pointer_cast<BindTunnel>(shared_from_this())));

	response(0, 90, &listenAddr_);
}

void BindTunnel::close()
{
	setState(Closed);

	TcpConnectionPtr conn = appClientConn_.lock();
	if (conn && conn->connected())
	{
		if (conn->outputBuffer()->readableBytes() == 0)
			conn->forceClose();
		else
			conn->shutdown();
	}

	ConnectTunnelPtr primaryTunnel = primaryTunnel_.lock();
	if (primaryTunnel)
	{
		primaryTunnel->delBindTunnel(name_);
	}
}

void BindTunnel::onConnection(const TcpConnectionPtr& conn)
{
	ConnectTunnelPtr primaryTunnel = primaryTunnel_.lock();

	//Error!
	if (!primaryTunnel)
	{
		close();
		return;
	}

	// different IP between DSTIP and peerIP
	// request reject, close ALL
	if (conn->peerAddress().toIp() != primaryTunnel->appServerConn()->peerAddress().toIp())
	{
		response(0, 91);
		primaryTunnel->close();
		return;
	}

	// Success
	appServerConn_ = conn;
    conn->setTcpNoDelay(true);
    conn->setHighWaterMarkCallback(bind(&BindTunnel::onHighWaterMark, 
    	dynamic_pointer_cast<BindTunnel>(shared_from_this()), _1), 64*1024*1024);
	primaryTunnel->addBindTunnel(name_, dynamic_pointer_cast<BindTunnel>(shared_from_this()));
	response(0, 90);
}

void BindTunnel::onMessage(const TcpConnectionPtr& conn,
						   Buffer* buff,
						   Timestamp time)
{
	TcpConnectionPtr appClientConn = appClientConn_.lock();

	// Error
	if (!appClientConn)
	{
		close();
		return;
	}

	if (!appClientConn->connected())
	{
		close();
		return;
	}

	appClientConn->send(buff);
}

void BindTunnel::onWriteComplete(const TcpConnectionPtr& conn)
{
	if (closed())
	{
		conn->forceClose();
		return;
	}

	TcpConnectionPtr appClientConn = appClientConn_.lock();
	if (appClientConn)
	{
		if (!appClientConn->isReading())
		{
			appClientConn->startRead();
		}
	}
}

void BindTunnel::onHighWaterMark(const muduo::net::TcpConnectionPtr& conn)
{
	// Manage memory
	TcpConnectionPtr appClientConn = appClientConn_.lock();
	if (appClientConn)
		appClientConn->stopRead();
}

void BindTunnel::checkTimeout()
{
	if (appServerConn_ == nullptr)
		close();
}
