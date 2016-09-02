#include "holeass.h"
#include <output.h>
#include <cmddef.h>

uint16_t holeass::port;

holeass::holeass()
{
}


holeass::~holeass()
{
}


void holeass::init() {
};

void holeass::NotifyDisconnection(xiny120::_cc* pc) {
	otprint("client gone... %s", pc->ci.ip);
};

bool holeass::NotifyConnection(xiny120::_cc* pc) { 
	otprint("client comming... %s",pc->ci.ip);
	cmdbase cb = { 0 };
	cb.cmd = c_wru;
	send(&cb, sizeof(cb), pc);
	return TRUE; 
};

bool holeass::NotifyReceived(xiny120::_cc* pc, const char* pbuf, const int32_t& len, const uint8_t& status) { 
	otprint("holeass::NotifyReceived..%s",pc->ci.ip);
	cmdbase* op1 = (cmdbase*)pbuf;
	switch (op1->cmd) {
	case c_wru: {
		wru* u = (wru*)pbuf;
		pc->ccid1 = u->ccid;
		cmdbase cb;
		cb.cmd = c_wru_ok;
		send(&cb, sizeof(cb), pc);
		otprint("holeass::NotifyReceived..%s ask for id", pc->ci.ip);
	}break;

	}
	return TRUE; 
};
