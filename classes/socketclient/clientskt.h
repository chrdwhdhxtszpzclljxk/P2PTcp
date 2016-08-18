#pragma once
class clientskt
{
private:
	clientskt();
	virtual ~clientskt();
public:
	static clientskt* me() {
		static clientskt* p = 0;
		if (p == 0) {
			p = new clientskt();
		}
		return p;
	}
};

