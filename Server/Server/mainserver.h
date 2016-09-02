#pragma once
#include "iocp\iocpbase.h"



class mainserver :
	public xiny120::iocpserver
{
private:
	mainserver();
	~mainserver();
public:
	static mainserver *  me() {
		static mainserver* p = NULL;
		if (p == NULL) {
			p = new mainserver();
			if (p != NULL) {
				if (!p->start(port, 20000, 2, 40)) {
					delete p;
					p = NULL;
				}
			}
		}
		return p;
	}
	virtual void init();
	virtual void NotifyDisconnection(xiny120::_cc*);
	virtual bool NotifyConnection(xiny120::_cc*);
	virtual bool NotifyReceived(xiny120::_cc*, const char*, const int32_t&, const uint8_t&);
	static uint16_t port;
};

