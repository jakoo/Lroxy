#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <map>
#include <fstream>
#include <netinet/in.h>
#include <boost/noncopyable.hpp>

#include <muduo/base/Logging.h>
#include <muduo/net/InetAddress.h>

class Config: private boost::noncopyable
{
public:
	static std::map<std::string, muduo::Logger::LogLevel> logLevelMap;
	static std::map<std::string, muduo::Logger::LogLevel> makeLogLevelMap();

	Config(const std::string& configPath);
	std::string listenIp() const { return ip_; };
	uint16_t listenPort() const { return port_; };
	muduo::net::InetAddress listenAddr() const { return inetAddress_; };
	int numThreads() const { return numThreads_; };
	int maxNumClients() const { return maxNumClients_; };
	muduo::Logger::LogLevel logLevel() const { return logLevel_; };
	
private:
	void praseConfig(std::ifstream& file);
	void praseLine(char* str, std::map<std::string, std::string>& cmap);

	int maxLine;
	std::string ip_;
	uint16_t port_;
	muduo::net::InetAddress inetAddress_;
	int numThreads_;
	int maxNumClients_;
	muduo::Logger::LogLevel logLevel_;
};
#endif