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

class hostinfo {
public:
	char ip[32];
	uint16_t port;
};

typedef std::vector<char> vec_buf;

class logininfo {
public:
	bool send(void*, uint32_t len, const uint8_t& s = 0);
	bool _recv();
	virtual bool notifyrecv(const char*, const int32_t& iLen, const uint8_t& status);
	SOCKET skt;
	char un[32];
	char pwd[32];
	int8_t hostcount;
	hostinfo hi[100];
	vec_buf mo, mi;
};
typedef std::vector<peerinfo> vec_peerinfo;
typedef std::vector<logininfo*> vec_logininfo;
typedef std::lock_guard<std::recursive_mutex> _lock;
class mainskt
{
private:
	mainskt();
	~mainskt();
public:
	static mainskt* me();
	void close() {
		run = false;
		Sleep(1000 * 5);
		delete this;
	}
	bool push(logininfo*);
	void getpeers();
	std::recursive_mutex mt;
	int maxfds;
	vec_peerinfo peers;
private:
	std::atomic<bool> run;
	vec_logininfo li,li0;
	
	void thr_com(void*);
	void thr_com1(void*);
};

