#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <map>
#include <fstream>
#include <netinet/in.h>
#include <boost/noncopyable.hpp>

#include <muduo/base/Logging.h>

class Config: private boost::noncopyable
{
public:
	static std::map<std::string, muduo::Logger::LogLevel> logLevelMap;
	static std::map<std::string, muduo::Logger::LogLevel> makeLogLevelMap();

	Config(const std::string& configPath);
	uint16_t port() {return port_;};
	int numThreads() {return numThreads_;};
	int maxNumClients() {return maxNumClients_;};
	muduo::Logger::LogLevel logLevel() {return logLevel_;};
private:
	void praseConfig(std::ifstream& file);
	void praseLine(char* str, std::map<std::string, std::string>& cmap);
	int maxLine;
	uint16_t port_;
	int numThreads_;
	int maxNumClients_;
	//char configKey[] = {"listenPort", "maxNumClients", "numThreads", "logLevel"};
	muduo::Logger::LogLevel logLevel_;
};
#endif