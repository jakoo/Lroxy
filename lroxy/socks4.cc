#include <iostream>
#include <boost/bind.hpp>

#include <muduo/base/AsyncLogging.h>
#include <muduo/base/Logging.h>
#include <muduo/base/Thread.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>

#include "SocksServer.h"
#include "Config.h"

using muduo::net::EventLoop;
using muduo::net::InetAddress;
using std::string;

int main(int argc, char* argv[])
{
	string configPath;
	if (argc > 2)
		std::cout << "Usage: " << argv[0] << "[config path]";
	if (argc == 2)
		configPath += argv[1];
	if (configPath.empty())
		configPath += "./config";
	Config config(configPath);

	muduo::Logger::setLogLevel(config.logLevel());
	EventLoop loop;
	SocksServer server(&loop, config);

	//config : listenPort, clientTcpNums, threadNum, logLevel 
	//InetAddress listenAddr(5555);
	server.start();
	loop.loop();
}