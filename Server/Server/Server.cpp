// Server.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include "holeass.h"
#include "mainserver.h"

int main()
{
	//mainserver ms;
	//ms.start(1157, 20000, 2, 20);
	if (mainserver::me() == NULL) return 0;
	if (holeass::me() == NULL) return 0;
	getchar();
    return 0;

}

