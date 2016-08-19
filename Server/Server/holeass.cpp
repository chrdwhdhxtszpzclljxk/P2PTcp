#include "holeass.h"
#include <output.h>



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

	return TRUE; 
};

bool holeass::NotifyReceived(xiny120::_cc* pc, const char* pbuf, const int32_t& len, const uint8_t& status) { 
	otprint("holeass::NotifyReceived");
	//op* op1 = (op*)pbuf;
	//switch (op1->cmd) {
	//a2b0: {

	//	}
	//	break;

//	}
	return TRUE; 
};
