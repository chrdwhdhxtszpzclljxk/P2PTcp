#include "mainserver.h"
#include "cmddef.h"


mainserver::mainserver()
{
}


mainserver::~mainserver()
{
}


void mainserver::init() {
};

void mainserver::NotifyDisconnection(xiny120::_cc*) {
};

bool mainserver::NotifyConnection(xiny120::_cc*) {
	return TRUE;
};

bool mainserver::NotifyReceived(xiny120::_cc* pc, const char* pbuf, const int32_t&, const uint8_t&) {
	cmdbase* op1 = (cmdbase*)pbuf;
	switch (op1->cmd) {
	case c_getpeers: {
		xiny120::_ccs::iterator iter;
		const uint32_t maxcount = 50;
		char* cmdbuf = new char[sizeof(cmdgetpeers) + sizeof(peerinfo) * maxcount];
		cmdgetpeers* c1 = (cmdgetpeers*)cmdbuf;
		c1->cmd = c_getpeers;
		c1->count = 0;
		for (iter = ccs.begin(); iter != ccs.end(); iter++) {
			if (c1->count >= maxcount) break;
			if (pc->ccid != iter->first) {
				c1->pi[c1->count].ccid = iter->first;
				memcpy(&c1->pi[c1->count].sa, &iter->second->ci.sadr, sizeof(c1->pi[c1->count]));
				c1->count++;
			}
		}
		int len = sizeof(cmdgetpeers) + sizeof(peerinfo) * (c1->count) - sizeof(peerinfo);
		send(cmdbuf, len , pc);
	}break;

	}
	return TRUE;
};
