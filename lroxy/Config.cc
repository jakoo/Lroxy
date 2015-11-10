#include <iostream>
#include <cstdlib>
#include <map>
#include <exception>
#include <boost/scoped_array.hpp>
#include <boost/regex.hpp>

#include "Config.h"

using std::map;
using std::string;
using std::ifstream;

map<string, muduo::Logger::LogLevel> Config::logLevelMap = makeLogLevelMap();

map<string, muduo::Logger::LogLevel> Config::makeLogLevelMap()
{
	map<string, muduo::Logger::LogLevel> lmap;
	lmap["TRACE"] = muduo::Logger::TRACE;
	lmap["DEBUG"] = muduo::Logger::DEBUG;
	lmap["INFO"] = muduo::Logger::INFO;
	lmap["WARN"] = muduo::Logger::WARN;
	lmap["ERROR"] = muduo::Logger::ERROR;
	lmap["FATAL"] = muduo::Logger::FATAL;
	return lmap;
}

Config::Config(const string& configPath)
  :	maxLine(1024)
{
	ifstream file(configPath);

	if (!file)
	{
		std::cerr << "can't open file " 
				  << '"' << configPath << '"'
				  << std::endl;
		exit(EXIT_FAILURE);
	}
	praseConfig(file);

}

void Config::praseConfig(ifstream& file)
{
	boost::scoped_array<char> str(new char[maxLine]);
	map<string, string> cmap;
	while (file.getline(str.get(), maxLine))
	{
		praseLine(str.get(), cmap);
	}

	if (cmap.find("listenPort") != cmap.end())
		port_ = static_cast<uint16_t>(atoi(cmap["listenPort"].c_str()));
	else
		port_ = 5555;

	if (cmap.find("maxNumClients") != cmap.end())
		maxNumClients_ = atoi(cmap["maxNumClients"].c_str());
	else
		maxNumClients_ = 10;

	if (cmap.find("numThreads") != cmap.end())
		numThreads_ = atoi(cmap["numThreads"].c_str());
	else
		numThreads_  = 1;

	if (cmap.find("logLevel") != cmap.end())
		logLevel_ = logLevelMap[cmap["logLevel"]];
	else
		logLevel_ = muduo::Logger::INFO;
}

void Config::praseLine(char* str, map<string, string>& cmap)
{
	const boost::regex pattern("^\\s*([\\w\\d]+)\\s*=\\s*([\\w\\d]+)\\s*;");
	boost::cmatch m;
	bool found = boost::regex_search(str, m, pattern);
	if (!found)
		return;

	cmap[m.str(1)] = m.str(2);
}