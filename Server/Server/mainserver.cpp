#include "mainserver.h"



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

bool mainserver::NotifyReceived(xiny120::_cc*, const char* pbuf, const int32_t&, const uint8_t&) {
	op* op1 = (op*)pbuf;
	switch (op1->cmd) {
	case getcs: {

	}break;
		
	case a2b0: {

	}
		break;

	}
	return TRUE;
};
