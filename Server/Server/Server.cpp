// Server.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include "holeass.h"
#include "mainserver.h"
#include <output.h>

int main()
{
	//mainserver ms;
	//ms.start(1157, 20000, 2, 20);
	mainserver::port = 1157;
	holeass::port = 1156;
	if (mainserver::me() == NULL) return 0;
	otprint("��������˿ں�(%d)�����ɹ���",mainserver::port);
	if (holeass::me() == NULL) return 0;
	otprint("��Э������˿ں�(%d)�����ɹ���",holeass::port);
	getchar();
    return 0;

}

