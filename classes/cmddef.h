#pragma once
#include <stdint.h>
#include <vector>

typedef std::vector<char> vec_buf;
typedef std::lock_guard<std::recursive_mutex> _lock;

enum {
	c_li = 50,
	c_getpeers,
	c_connect2ps,
	c_wru,
	c_wru_ok,
};

struct peerinfo {
	uint64_t ccid;
	SOCKADDR sa;
};

struct cmdbase {
	uint16_t cmd;
};

struct cmdgetpeers : public cmdbase {
	uint32_t count;
	peerinfo pi[1];
};

struct connect2ps : public cmdbase {
	uint32_t count;
	peerinfo pi[1];
};

struct wru : public cmdbase {
	uint64_t ccid;
};