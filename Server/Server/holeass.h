#pragma once
#include "iocp\iocpbase.h"


class holeass :
	public xiny120::iocpserver
{
private:
	holeass();
	~holeass();
public:
	static holeass *  me() {
		static holeass* p = NULL;
		if (p == NULL) {
			p = new holeass();
			if (p != NULL) {
				if (!p->start(1156, 20000, 2, 40)) {
					delete p;
					p = NULL;
				}
			}
		}
		return p;
	}

	xiny120::_ccs* getccs() { return &ccs; };
	virtual void init();
	virtual void NotifyDisconnection(xiny120::_cc*);
	virtual bool NotifyConnection(xiny120::_cc*);
	virtual bool NotifyReceived(xiny120::_cc*, const char*, const int32_t&, const uint8_t&);


};

