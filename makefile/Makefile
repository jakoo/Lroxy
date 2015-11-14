#MUDUO_DIRECTORY ?= $(HOME)/build/debug-install
#MUDUO_DIRECTORY ?= $(HOME)/build/install
MUDUO_DIRECTORY ?= ../deps/muduo
MUDUO_INCLUDE = $(MUDUO_DIRECTORY)/include
MUDUO_LIBRARY = $(MUDUO_DIRECTORY)/lib
BOOST_LIBRARY ?= ../deps/boost
SRC = ../lroxy

CXXFLAGS = -g -O0 -Wall -Wextra -Werror \
	   -Wconversion -Wno-unused-parameter \
	   -Wold-style-cast -Woverloaded-virtual \
	   -Wpointer-arith -Wshadow -Wwrite-strings \
	   -march=native -rdynamic \
	   -I$(MUDUO_INCLUDE) \
	   -std=c++0x

LDFLAGS = -L$(MUDUO_LIBRARY) -lmuduo_net_cpp11 -lmuduo_base_cpp11 -lpthread -lrt \
		  $(BOOST_LIBRARY)/libboost_regex.so

all: socks4 clean
	
socks4: socks4.o SocksServer.o Config.o Tunnel.o ConnectTunnel.o BindTunnel.o
	g++ $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

socks4.o: $(SRC)/socks4.cc $(SRC)/SocksServer.h $(SRC)/Config.h
	g++ -c $(CXXFLAGS) -o $@ $(SRC)/socks4.cc $(LDFLAGS)

SocksServer.o: $(SRC)/SocksServer.cc $(SRC)/SocksServer.h $(SRC)/ConnectTunnel.h \
			   $(SRC)/BindTunnel.h $(SRC)/Config.h
	g++ -c $(CXXFLAGS) -o $@ $(SRC)/SocksServer.cc $(LDFLAGS)

Config.o: $(SRC)/Config.cc $(SRC)/Config.h
	g++ -c $(CXXFLAGS) -o $@ $(SRC)/Config.cc $(LDFLAGS)

Tunnel.o: $(SRC)/Tunnel.cc $(SRC)/Tunnel.h
	g++ -c $(CXXFLAGS) -o $@ $(SRC)/Tunnel.cc $(LDFLAGS)

ConnectTunnel.o: $(SRC)/ConnectTunnel.cc $(SRC)/ConnectTunnel.h
	g++ -c $(CXXFLAGS) -o $@ $(SRC)/ConnectTunnel.cc $(LDFLAGS)

BindTunnel.o: $(SRC)/BindTunnel.cc $(SRC)/BindTunnel.h
	g++ -c $(CXXFLAGS) -o $@ $(SRC)/BindTunnel.cc $(LDFLAGS)

clean:
	rm -f socks4.o SocksServer.o Config.o \
		  Tunnel.o ConnectTunnel.o BindTunnel.o \
		  core

.PHONY: all clean
