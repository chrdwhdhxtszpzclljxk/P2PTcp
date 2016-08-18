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
#pragma warning (disable : 4996)


class xskt {
public:
	SOCKET skt;
	char ip[24];
	uint16_t port;

	virtual BOOL notifyrecv(char* pbuf, int32_t len) { return FALSE; };

};

typedef std::list<xskt*> listxskt;
typedef std::lock_guard<std::recursive_mutex> _lock;

class clientskt
{
private:
	clientskt() ;
	virtual ~clientskt();
public:
	static clientskt* me() {
		static clientskt* p = NULL;
		if (p == NULL) {
			p = new clientskt();
			std::thread(&clientskt::thr_com, p, p).detach();
		}
		return p;
	}

	void push(xskt* skt);
private:
	std::atomic<bool> run;
	listxskt skts,skts0;
	
	std::thread thr_con_o,thr_com_o;
	std::recursive_mutex mt;
	void thr_con(void*);
	void thr_com(void*);
};

