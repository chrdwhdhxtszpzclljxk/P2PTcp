#include "clientskt.h"


clientskt::clientskt()
{
	run = true;
}


clientskt::~clientskt()
{
}


void clientskt::push(xskt* p) {
	_lock guard1(mt);
	skts0.push_back(p);
}



void clientskt::thr_com(void*) {
	struct timeval to;
	to.tv_sec = 0;
	to.tv_usec = 20;
	fd_set r;
	while (run) {
		listxskt::iterator iter;
		if (select(0, &r, NULL, NULL, &to) > 0) {
			_lock guard1(mt);
			for (iter = skts.begin(); iter != skts.end(); iter++) {
				xskt* p = *iter;

			}
		}


		_lock guard1(mt);
		for (iter = skts0.begin(); iter != skts0.end(); iter++) {
			xskt* p = *iter;
			struct sockaddr_in servaddr;
			::closesocket(p->skt);
			p->skt = socket(AF_INET, SOCK_STREAM, 0);
			memset(&servaddr, 0, sizeof(servaddr));
			servaddr.sin_family = AF_INET;
			servaddr.sin_addr.S_un.S_addr = inet_addr(p->ip);
			servaddr.sin_port = htons(p->port);
			char flag = 1;
			int len = sizeof(flag);
			if (setsockopt(p->skt, SOL_SOCKET, SO_REUSEADDR, &flag, len) == -1)
			{
				perror("setsockopt");
				exit(1);
			}
			len = sizeof(sockaddr);
			if (connect(p->skt, (sockaddr*)&servaddr, len) == 0) {
				
				sockaddr_in my;
				memset(&my, 0, sizeof(my));
				int len = sizeof(my);

				int ret = getsockname(p->skt, (sockaddr*)&my, &len);
				if (ret == 0){

				}


				skts.push_back(p);
				skts0.remove(p);
				break;
			}
		}


	}
}