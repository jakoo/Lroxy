#ifndef TUNNEL_H
#define TUNNEL_H

#include <string>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <muduo/net/TcpClient.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/net/TcpServer.h>
#include <muduo/net/InetAddress.h>
#include <muduo/base/Types.h>


class SocksServer;

class Tunnel: public boost::enable_shared_from_this<Tunnel>, 
			  private boost::noncopyable
{
public:
	enum StatE {Builded, Closed};

	Tunnel(SocksServer* socksServer,
		   const muduo::net::TcpConnectionPtr& appClientConn,
		   uint8_t VN,
		   uint8_t CD,
		   muduo::net::InetAddress inetAddr);
	virtual ~Tunnel() {};
	virtual void setup() = 0;
	virtual void close() = 0;
	StatE state() const { return state_; };
	bool closed() const { return state_ == Closed; };
	void setState(StatE);
	const muduo::net::TcpConnectionPtr& appServerConn() const { return appServerConn_; };
	//TcpConnectionPtr& appClientConn();

protected:
	muduo::net::InetAddress createInetAddress(uint16_t port, uint32_t ip);
	void response(char VN, char CD, muduo::net::InetAddress* inetAddr = nullptr);
	virtual void onConnection(const muduo::net::TcpConnectionPtr& conn) = 0;
	virtual void onMessage(const muduo::net::TcpConnectionPtr& conn,
				   		   muduo::net::Buffer* buf,
				   		   muduo::Timestamp time) = 0;
	virtual void onWriteComplete(const muduo::net::TcpConnectionPtr& conn) = 0;
	virtual void onHighWaterMark(const muduo::net::TcpConnectionPtr& conn) = 0;
	virtual void checkTimeout() = 0;
	SocksServer* socksServer_;
	StatE state_;
	boost::weak_ptr<muduo::net::TcpConnection> appClientConn_;
	muduo::net::TcpConnectionPtr appServerConn_;
	uint8_t VN_;
	uint8_t CD_;
	muduo::net::InetAddress inetAddr_;//DSTaddr
	muduo::string name_;
};

typedef boost::shared_ptr<Tunnel> TunnelPtr;

#endif