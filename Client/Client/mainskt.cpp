#include "stdafx.h"
#include "mainskt.h"
#include <socketclient\clientskt.h>


mainskt::mainskt()
{
	run = true;
	maxfds = 0;
}


mainskt::~mainskt()
{
	vec_logininfo::iterator iter;
	_lock guard1(mt);
	for (iter = li.begin(); iter != li.end(); iter++) {
		delete *iter;
	}
}

mainskt* mainskt::me() {
	static mainskt* p = NULL;
	if (p == NULL) {
		p = new mainskt();
		std::thread(&mainskt::thr_com, p, p).detach();

	}
	return p;
}

bool mainskt::push(logininfo* pli) {
	bool ret = false;
	_lock guard1(mt);
	if ((li.size() + li0.size()) == 0) {
		li0.push_back(pli);
		ret = true;
	}
	return ret;
}

void mainskt::getpeers() {
	vec_logininfo::iterator iter;
	cmdbase cb;
	cb.cmd = c_getpeers;
	_lock guard1(mt);
	for (iter = li.begin(); iter != li.end(); iter++) {
		logininfo* p = *iter;
		p->send(&cb, sizeof(cb));
	}

}

void mainskt::initp2p() {
	xskt* p = new xskt();
	const char* ip = "127.0.0.1";
	strcpy(p->ip, ip);
	p->port = 1156;
	clientskt::me()->push(*p);
	return;
}

void mainskt::connectp2p() {
	uint32_t tt = sizeof(connect2ps) + sizeof(peerinfo) * peers.size() - sizeof(peerinfo);
	_lock guard1(mt);
	char* temp = new char[tt];
	connect2ps* cmd = (connect2ps*)temp;
	cmd->cmd = c_connect2ps;
	int i = 0;
	vec_peerinfo::iterator iter;
	for (iter = peers.begin(); iter != peers.end(); iter++) {
		memcpy(&cmd->pi[i], &(*iter),sizeof(*iter));
		char* ip = inet_ntoa(((sockaddr_in*)&cmd->pi[i].sa)->sin_addr);
		TRACE("%s", ip);
		i++;
	}
	cmd->count = peers.size();
	vec_logininfo::iterator iter1;
	for (iter1 = li.begin(); iter1 != li.end(); iter1++) {
		logininfo* p = *iter1;
		p->send(cmd, tt);
	}

}



void mainskt::thr_com(void*) {
	struct timeval to;
	to.tv_sec = 0;
	to.tv_usec = 20;
	fd_set r, w;
	int fds = 0;
	while (run) {
		vec_logininfo::iterator iter;
		FD_ZERO(&r);
		FD_ZERO(&w);
		fds = 0;
		{
			_lock guard1(mt);
			for (iter = li.begin(); iter != li.end(); iter++) {
				logininfo* p = *iter;
				if (p->skt > fds) fds = p->skt;
				FD_SET(p->skt, &r);
				FD_SET(p->skt, &w);
			}
		}

		if (select(fds + 1, &r, &w, NULL, &to) > 0) {
			_lock guard1(mt);
			for (iter = li.begin(); iter != li.end(); iter++) {
				logininfo* p = *iter;
				if (FD_ISSET(p->skt, &r)) {
					p->_recv();
				}

				if (FD_ISSET(p->skt, &w)) {
					if (p->mo.size() > 0) {
						int sended = send(p->skt, &p->mo[0], p->mo.size(), 0);
						if (sended > 0) {
							p->mo.erase(p->mo.begin(), p->mo.begin() + sended);
						}
						else { // 发送错误，断开连接。
							iter = li.erase(iter);
							li0.push_back(p);
							if (iter == li.end()) break;
						}
					}
				}
			}
		}


		_lock guard1(mt);
		for (iter = li0.begin(); iter != li0.end(); iter++) {
			logininfo* p = *iter;
			struct sockaddr_in servaddr;
			::closesocket(p->skt);
			p->skt = socket(AF_INET, SOCK_STREAM, 0);
			char flag = 1;
			int len = sizeof(flag);
			//if (setsockopt(p->skt, SOL_SOCKET, SO_REUSEADDR, &flag, len) == -1)
			//{
			//	perror("setsockopt");
			//	exit(1);
			//	}
			bool bok = false;
			p->mo.clear();
			p->mi.clear();

			for (int i = 0; i < p->hostcount; i++) {
				memset(&servaddr, 0, sizeof(servaddr));
				servaddr.sin_family = AF_INET;
				servaddr.sin_addr.S_un.S_addr = inet_addr(p->hi[i].ip);
				servaddr.sin_port = htons(p->hi[i].port);

				len = sizeof(sockaddr);
				if (connect(p->skt, (sockaddr*)&servaddr, len) == 0) {
					li.push_back(p);
					if (p->skt > maxfds) maxfds = p->skt;
					li0.erase(iter);
					bok = true;
					break;
				}
			}
			if (bok) break;
		}


	}
}



bool logininfo::notifyrecv(const char* pBuf, const int32_t& iLen, const uint8_t& status) {
	bool ret = false;
	char ip[24] = { 0 };
	DWORD iplen = sizeof(ip);
	cmdbase* cb = (cmdbase*)pBuf;
	switch (cb->cmd) {
	case c_getpeers: {
		cmdgetpeers* peers = (cmdgetpeers*)pBuf;
		sockaddr_in* p1;
		mainskt::me()->peers.clear();
		for (int i = 0; i < peers->count; i++) {
			p1 = (sockaddr_in*)&peers->pi[i].sa;
			mainskt::me()->peers.push_back(peers->pi[i]);
			WSAAddressToStringA((LPSOCKADDR)p1, sizeof(SOCKADDR), NULL, ip, &iplen);
			TRACE("%s\r\n", ip);
		}

	}
		break;
	}

	return ret;
}

bool logininfo::send(void* p1, uint32_t len, const uint8_t& status) {
	bool ret = false;
	char* p = (char*)p1;
	//mo.insert(mo.end(), p, p + len);

	uint32_t u1 = status; u1 = (u1 << 24) | (len + sizeof(int32_t)); std::lock_guard<std::recursive_mutex&> guard1(mainskt::me()->mt);;
	mo.insert(mo.end(), (char*)&u1, ((char*)(&u1)) + sizeof(u1)); mo.insert(mo.end(), p, p + len);

	return ret;
}

bool logininfo::_recv() {
	char buf[2048] = { 0 }; int32_t recved = 0, total = 0, count = 0; const int32_t size = sizeof(buf); uint8_t status; bool newrecv = false, ret = true;
	std::lock_guard<std::recursive_mutex&> guard1(mainskt::me()->mt);
	do {
		recved = recv(skt, buf, size, 0);
		if (recved < 0 && (h_errno != WSAEWOULDBLOCK || errno != WSAEINPROGRESS)) { ret = false; break; } // 遇到错误，如果不是（io正在处理错误）就失败
		else if (recved == 0) { ret = false;  break; } // 对方断线
		total += recved;
		newrecv = true; mi.insert(mi.end(), buf, buf + recved); if (recved < size) break;
	} while (false);
	if (newrecv) {
		while (mi.size() >= sizeof(int32_t)) { // 当收到的字节大于等于_INT时，就可以开始分析逻辑包了。
			count = *(int32_t*)(&mi[0]); status = count >> 24; count = count & 0x00FFFFFF;
			if (count > mi.size()) break; // 如果剩余的缓冲不够组成一个包，退出循环去接收新包。
			if (notifyrecv(&mi[4], count - sizeof(int32_t), status) != true) return false; // 因为逻辑包处理错误，所以需要骗一下socket说他关闭了。
			mi.erase(mi.begin(), mi.begin() + count);
		}
	}
	//log("recved! %s %d\r\n", m_addr.c_str(),total);
	return ret; // 出错时select应该会处理。
}