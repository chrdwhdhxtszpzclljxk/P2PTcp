// Server.cpp : 定义控制台应用程序的入口点。
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
	otprint("打洞主服务端口号(%d)启动成功！",mainserver::port);
	if (holeass::me() == NULL) return 0;
	otprint("打洞协助服务端口号(%d)启动成功！",holeass::port);
	getchar();
    return 0;

}

