#include <arpa/inet.h>
#include <netinet/in.h>
#include <boost/any.hpp>
#include <boost/scoped_array.hpp>
#include <boost/bind.hpp>

#include <muduo/net/InetAddress.h>
#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>
#include <muduo/base/StringPiece.h>

#include "ConnectTunnel.h"
#include "BindTunnel.h"
#include "SocksServer.h"

using std::map;
using boost::any_cast;
using boost::dynamic_pointer_cast;
using boost::scoped_array;
using boost::bind;
using namespace muduo::net;
using namespace muduo;

ConnectTunnel::ConnectTunnel(SocksServer* socksServer,
		   		  			 const TcpConnectionPtr& appClientConn,
					   		 uint8_t VN,
					   		 uint8_t CD,
					   		 InetAddress inetAddr)
  : Tunnel(socksServer, appClientConn, VN, CD, inetAddr),
  	client_(nullptr),
  	bindTunnels_(map<string, BindTunnelPtr>())
{
}
/*
string ConnectTunnel::makeName(InetAddress inetAddr, string USERID)
{
	uint32_t ip = inetAddr.ipNetEndian();
	uint16_t port = inetAddr.portNetEndian();
	scoped_array<char> ipStr(new char[INET_ADDRSTRLEN]);
	scoped_array<char> portStr(new char[10]);
	inet_ntop(AF_INET, &ip, &ipStr[0], INET_ADDRSTRLEN);
	sprintf(&portStr[0], "%u", ntohs(port));
	return string("ConnectTunnel ") + string(&ipStr[0]) + " " +
		   string(&portStr[0]) + " " + USERID;
}*/

void ConnectTunnel::addBindTunnel(string bindTunnelName, BindTunnelPtr bindTunnel)
{
	bindTunnels_[bindTunnelName] = bindTunnel;
}

void ConnectTunnel::delBindTunnel(string bindTunnelName)
{
	map<string, BindTunnelPtr>::iterator it;
	it = bindTunnels_.find(bindTunnelName);
	bindTunnels_.erase(it);
}

/*
InetAddress ConnectTunnel::appServerAddress()
{
	return createInetAddress(DSTPORT, DSTIP)
}
*/

void ConnectTunnel::setup()
{
	name_ = string("ConnectTunnel ") + inetAddr_.toIpPort();

	if (state_ != Builded)
	{
		LOG_DEBUG << "Error setup while Tunnel isn't Builded"
				  << "close tunnel : " << name_;
		close();
	}

	LOG_DEBUG << "ConnectTunnel Setup : " << name_;
	socksServer_->addTunnel(name_, shared_from_this());
	string clientName = name_ + " TcpClient";
	client_.reset(new TcpClient(EventLoop::getEventLoopOfCurrentThread(), 
								inetAddr_, clientName.c_str()));

	client_->setMessageCallback(bind(&ConnectTunnel::onMessage, 
		dynamic_pointer_cast<ConnectTunnel>(shared_from_this()), _1, _2, _3));
	client_->setConnectionCallback(bind(&ConnectTunnel::onConnection, 
		dynamic_pointer_cast<ConnectTunnel>(shared_from_this()), _1));
	client_->setWriteCompleteCallback(bind(&ConnectTunnel::onWriteComplete, 
		dynamic_pointer_cast<ConnectTunnel>(shared_from_this()), _1));

	LOG_DEBUG << "ConnectTunnel client_->connect() appServerConn InetAddress : "
			  << inetAddr_.toIpPort();
	client_->connect();

	EventLoop::getEventLoopOfCurrentThread()->runAfter(120, 
		bind(&ConnectTunnel::checkTimeout, 
			dynamic_pointer_cast<ConnectTunnel>(shared_from_this())));
}

void ConnectTunnel::close()
{
	setState(Closed);
	// Close all bind tunnel
	for (map<string, BindTunnelPtr>::iterator it = bindTunnels_.begin();
		 it != bindTunnels_.end(); ++it)
	{
		it->second->close();
	}

	TcpConnectionPtr conn = appClientConn_.lock();
	if (conn && conn->connected())
	{
		if (conn->outputBuffer()->readableBytes() == 0)
			conn->forceClose();
		else
			conn->shutdown();
	}

	socksServer_->delTunnel(name_);
}	

void ConnectTunnel::onConnection(const TcpConnectionPtr& conn)
{
	LOG_TRACE << conn->peerAddress().toIpPort() << " -> "
			  << conn->localAddress().toIpPort() << " is "
			  << (conn->connected() ? "UP" : "DOWN");

	if (conn->connected())
	{
		TcpConnectionPtr appClientConn = appClientConn_.lock();
		if (!appClientConn)
		{
			LOG_TRACE << "appClientConn disconnected while appServerConn established : "
					  << name_;
			close();
			return;
		}

		LOG_TRACE << "appServerConn established : "
				  << name_;
		appServerConn_ = conn;
	    conn->setTcpNoDelay(true);
	    conn->setHighWaterMarkCallback(bind(&ConnectTunnel::onHighWaterMark, 
	    									dynamic_pointer_cast<ConnectTunnel>(shared_from_this()), 
	    									_1), 
	    							   64*1024*1024);
		response(0, 90);
	}
	else
	{
		close();
	}
}

void ConnectTunnel::onMessage(const TcpConnectionPtr& conn,
			   muduo::net::Buffer* buf,
			   muduo::Timestamp time)
{
	TcpConnectionPtr appClientConn = appClientConn_.lock();
	if (!appClientConn)
	{
		LOG_TRACE << "appClientConn disconnected while appServerConn onMsg : "
				  << name_;
		close();
		return;
	}

	LOG_TRACE << conn->name() << " recv " << buf->readableBytes()
			  << " bytes at " << time.toString()
			  << " in tunnel : " << name_;
	appClientConn->send(buf);
}

void ConnectTunnel::onWriteComplete(const TcpConnectionPtr& conn)
{
	if (closed())
		conn->forceClose();
	
	TcpConnectionPtr appClientConn = appClientConn_.lock();
	if (appClientConn)
	{
		if (!appClientConn->isReading())
		{
			appClientConn->startRead();
		}
	}
}

void ConnectTunnel::onHighWaterMark(const TcpConnectionPtr& conn)
{
	TcpConnectionPtr appClientConn = appClientConn_.lock();
	if (appClientConn)
		appClientConn->stopRead();
}

void ConnectTunnel::checkTimeout()
{
	if (appServerConn_ != nullptr)
		return;

	client_->stop();
	response(0, 91);
	close();
}