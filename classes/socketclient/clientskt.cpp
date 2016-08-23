#include "clientskt.h"
#include "../Client/Client/mainskt.h"

clientskt::clientskt()
{
	run = true;
	skts0.port = 0;
	skts.skt = 0;
}


clientskt::~clientskt()
{
}


void clientskt::push(const xskt& p) {
	_lock guard1(mt);
	skts0 = p;
}

void clientskt::thr_accept(SOCKET pv) {
	SOCKET s = (SOCKET)pv;
	SOCKET ns = 0;
	struct sockaddr_in addr;
	int len;
	while (run) {
		memset(&addr, 0, sizeof(addr));
		len = sizeof(addr);
		xskt* x = new xskt();
		x->skt = accept(s, (sockaddr*)&addr,&len);
		_lock guard1(mt);
		peers.push_back(x);
		x->send("hello", 4);
	}

}

void clientskt::thr_com(void*) {
	struct timeval to;
	to.tv_sec = 0;
	to.tv_usec = 20;
	DWORD err = 0;
	fd_set r,w;
	int fds = 0;
	struct sockaddr_in addr;
	xskt* p = NULL;
	listxskt::iterator iter;
	int len = 0;
	char flag = 1;
	while (run) {
		FD_ZERO(&r);
		FD_ZERO(&w);
		if (skts.skt != 0) {
			fds = skts.skt;
			FD_SET(skts.skt, &r);
			FD_SET(skts.skt, &w);
			//FD_SET(skts.sktp2p, &w);
		}
		{
			_lock guard1(mt);
			for (iter = peers.begin(); iter != peers.end(); iter++) {
				FD_SET((*iter)->skt, &r);
				FD_SET((*iter)->skt, &w);
			}
		}

		if (select(0, &r, &w, NULL, &to) > 0) {
			if (FD_ISSET(skts.skt, &r)) {
				skts._recv();
			}
			if (FD_ISSET(skts.skt, &w)) {
				
				if (skts.mo.size() > 0) {
					int sended = ::send(skts.skt, &skts.mo[0], skts.mo.size(), 0);
					if (sended > 0) {
						skts.mo.erase(skts.mo.begin(), skts.mo.begin() + sended);
					}
					else { // 发送错误，断开连接。
					}
				}

			}


			_lock guard1(mt);
			for (iter = peers.begin(); iter != peers.end(); iter++) {
				if (FD_ISSET((*iter)->skt, &r)) {
					(*iter)->_recv();
				}

				if (FD_ISSET((*iter)->skt, &w)) {
					p = (*iter);
					if (p->mo.size() > 0) {
						int sended = ::send(p->skt, &p->mo[0], p->mo.size(), 0);
						if (sended > 0) {
							p->mo.erase(p->mo.begin(), p->mo.begin() + sended);
						}
						else { // 发送错误，断开连接。
							iter = peers.erase(iter);
							if (iter == peers.end()) break;
						}
					}
				}
			}
			
		}

		_lock guard1(mt);
		if (skts0.port != 0) {
			::closesocket(skts0.skt);
			skts0.skt = socket(AF_INET, SOCK_STREAM, 0);
			memset(&addr, 0, sizeof(addr));
			addr.sin_family = AF_INET;
			addr.sin_addr.S_un.S_addr = inet_addr(skts0.ip);
			addr.sin_port = htons(skts0.port);

			if (setsockopt(skts0.skt, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) == -1) {
				MessageBox(NULL, L"错误：setsockopt SO_REUSEADDR 0", L"", MB_OK);
				
			}else if (connect(skts0.skt, (sockaddr*)&addr, sizeof(sockaddr)) == 0) {
				memset(&addr, 0, sizeof(addr));
				len = sizeof(addr);
				char ip[128] = {0};
				DWORD iplen = sizeof(ip);
				if (getsockname(skts0.skt, (sockaddr*)&addr, &len) == 0) {
					WSAAddressToStringA((LPSOCKADDR)&addr, sizeof(addr), 0,ip,&iplen );
					::closesocket(skts0.sktp2p);
					//::closesocket(p->skt);
					skts0.sktp2p = socket(AF_INET, SOCK_STREAM, 0);
					if (setsockopt(skts0.sktp2p, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) == -1) {
						MessageBox(NULL, L"错误：setsockopt SO_REUSEADDR 1", L"", MB_OK);
						
					}else if (bind(skts0.sktp2p, (SOCKADDR*)&addr, sizeof(addr)) != 0)	{
						err = GetLastError();
						MessageBox(NULL, L"错误：bind sktp2p", L"", MB_OK);
						
					}else if (listen(skts0.sktp2p, 5) == -1) {
						err = GetLastError();
						MessageBox(NULL, L"错误：listen sktp2p", L"", MB_OK);
					}else {
						skts = skts0;
						skts0.port = 0;
						std::thread(&clientskt::thr_accept, clientskt::me(), skts0.sktp2p).detach();

					}

				}

			}
		}


	}
}

bool xskt::notifyrecv(char* pbuf, int32_t len, const uint8_t& status) { 
	bool ret = true;
	cmdbase* pcb = (cmdbase*)pbuf;
	switch (pcb->cmd)
	{
	case c_wru: {
		wru w ;
		w.cmd = c_wru;
		w.ccid = mainskt::me()->getccid();
		send(&w, sizeof(w));
	}break;
	case c_wru_ok: {
		mainskt::me()->connectp2p();
	}break;
	default:
		break;
	}
	return ret; 
};

bool xskt::_recv() {
	char buf[2048] = { 0 }; int32_t recved = 0, total = 0, count = 0; const int32_t size = sizeof(buf); uint8_t status; bool newrecv = false, ret = true;
	std::lock_guard<std::recursive_mutex&> guard1(clientskt::me()->mt);
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

bool xskt::send(void* p1, uint32_t len, const uint8_t& status) {
	bool ret = false;
	char* p = (char*)p1;
	//mo.insert(mo.end(), p, p + len);

	uint32_t u1 = status; u1 = (u1 << 24) | (len + sizeof(int32_t)); std::lock_guard<std::recursive_mutex&> guard1(clientskt::me()->mt);;
	mo.insert(mo.end(), (char*)&u1, ((char*)(&u1)) + sizeof(u1)); mo.insert(mo.end(), p, p + len);

	return ret;
}