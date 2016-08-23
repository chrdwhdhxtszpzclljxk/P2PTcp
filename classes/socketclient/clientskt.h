#pragma once

#include <WinSock2.h>
#include <stdint.h>
#include <list>

#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"Mswsock.lib")
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <map>
#include <atomic>
#include <cmddef.h>
#pragma warning (disable : 4996)

//class peer {
//public:
//	SOCKET skt;
//	char ip[24];
//	uint16_t port;
//	virtual bool notify_recv(char* buf, int32_t len) { return false; };
//};

class xskt {
public:
	SOCKET skt,sktp2p;
	char ip[24];
	uint16_t port;
	virtual bool notifyrecv(char* pbuf, int32_t len, const uint8_t& status);// { return false; };
	bool send(void*, uint32_t len, const uint8_t& s = 0);
	bool _recv();
	vec_buf mo, mi;
};

typedef std::list<xskt*> listxskt;
typedef std::lock_guard<std::recursive_mutex> _lock;

class clientskt
{
private:
	clientskt() ;
	virtual ~clientskt();
public:
	std::recursive_mutex mt;
	static clientskt* me() {
		static clientskt* p = NULL;
		if (p == NULL) {
			p = new clientskt();
			std::thread(&clientskt::thr_com, p, p).detach();
		}
		return p;
	}

	void push(const xskt& skt);
	void send() {
		skts.send("hello", 4, 0);
	}
private:
	std::atomic<bool> run;
	xskt skts,skts0;
	
	listxskt peers;
	
	std::thread thr_con_o,thr_com_o;
	void thr_com(void*);
	//void thr_peer_com(void*);
	void thr_accept(SOCKET s);
};

