#include <stdio.h>
#include <direct.h>
#include <io.h>
#include <string>
#include <strstream>
#include "iocpbase.h"
#include "output.h"

namespace xiny120 {

	// fopen, fclose, fread, fwrite, fgetc, fgets, fputc, fputs, freopen, fseek, ftell, rewind
	filetransport::filetransport() :fp(NULL), err(0), basepath("e:\\mzgp\\web\\"){};
	filetransport::~filetransport(){ close(); };
	bool filetransport::doing(){ return fp != NULL; };
	void filetransport::close(){ if (fp != NULL){ fclose(fp); fp = NULL; } };
	bool filetransport::open(std::string file){ 
		filename = file; std::string path; path = basepath + "\\" +  filename;
		if ((fp = fopen(path.c_str(), "rb")) == NULL){ err = errno; return false; }
		else { return true; }
	};
	int32_t filetransport::read(char* buf, const int32_t& len){
		int32_t ret = 0;
		if (fp != NULL){ ret = fread(buf, sizeof(char), len, fp);}
		return ret;
	}
	int filetransport::size(){ if (fp == NULL) return 0; if (fseek(fp, 0, SEEK_END) == 0) return ftell(fp); else return 0; };
	int filetransport::size(std::string file){ 
		std::string path; path = basepath + "\\" + file; 
		FILE* fptmp; int ret = 0; 
		if ((fptmp = fopen(path.c_str(), "rb")) != NULL){ 
			if (fseek(fptmp, 0, SEEK_END) == 0) ret = ftell(fptmp); fclose(fptmp);
		}
		return ret;
	};
	bool filetransport::seek(int pos){ if (fp == NULL) return false; if (fseek(fp, pos, SEEK_SET) == 0) return true; return false; };

	readbuf::readbuf(){oper = Read;}
	bool readbuf::setupbuf(_cc*& pc){
		bool ret = false;
		int32_t left = pc->left();
		if (left <= 0){
			pc->move(); left = pc->left();
			if (left <= 0){ pc->realloc(); left = pc->left();ret = true;}else{ ret = true;}
		}else{ ret = true;}
		if (ret){buf.buf = pc->cur; buf.len = left;}
		return ret;
	}

	initbuf::initbuf(){ oper = Init; };
	DisConnectExBuffer::DisConnectExBuffer(){oper = DisConnectEx;}
	holdbuf::holdbuf(const void* pVoid, const uint32_t& _len, const uint8_t& status){
		uint32_t len = _len + sizeof(uint32_t), u1 = status; u1 = u1 << 24;
		buf.len = len; buf.buf = new char[len]; len = len | u1;
		if (buf.buf != NULL){ memmove(buf.buf, &len, sizeof(len)); memmove(buf.buf + sizeof(len), pVoid, _len);}
		pending = 0;
	}
	holdbuf::~holdbuf(){delete[] buf.buf;}
	writeallbuf::writeallbuf(holdbuf * _buf){oper = WriteAll; pending = 0; pbuf = _buf; pbuf->inc();}
	writeallbuf::~writeallbuf(){if (0 == pbuf->dec()) delete pbuf;}
	writebuf::writebuf(const void* pVoid, const uint32_t& _len, const uint8_t& status){
		oper = Write; uint32_t len = _len + sizeof(len), u1 = status; u1 = u1 << 24;
		buf.len = len; buf.buf = new char[len]; len = len | u1;
		if (buf.buf != NULL){ memmove(buf.buf, &len, sizeof(len));	memmove(buf.buf + sizeof(len), pVoid, _len);}
	}
	writebuf::~writebuf(){delete[] buf.buf;}

	int32_t clientinfo::pushpath(std::string _path){
		if (_access(_path.c_str(), 06) == 0) searchpaths.push_back(_path);
		return searchpaths.size();
	}

	_cc::_cc(SOCKET _s) :s(_s), packlen(0),ios(1){ len = maxbuf; pre = cur = buf = new char[len]; }

	int32_t _cc::realloc(){ 
		uint32_t len1 = len + incbuf; char* tmp = new char[len1];
		memmove(tmp, buf, len); delete[] buf; buf = tmp; len = len1; 
		return len;
	}

	int32_t _cc::move(){
		int32_t used = cur - pre; memmove(buf, pre, used); pre = buf; cur = buf + used;
		return len;
	}


	iocpbase::iocpbase() : echo (false){
		WSADATA wsaData = { 0 };
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) otprint("\r\n******************\r\nWSAStartup error\r\n");
		ccidnow = 0; m_pDisConnectEx = NULL; maxconns = 100000; conns = 0; shutdown = false; iothreads = 4;
	}


	iocpbase::~iocpbase(){}

	bool iocpbase::createport(){
		// First open a temporary socket that we will use to create the
		// completion port.  In NT 3.51 it will not be necessary to specify
		// the FileHandle parameter of CreateIoCompletionPort()--it will
		// be legal to specify FileHandle as NULL.  However, for NT 3.5
		// we need an overlapped file handle.
		//
		// Create the completion port that will be used by all the workers
		// threads.
		cpport = CreateIoCompletionPort((HANDLE)INVALID_HANDLE_VALUE, NULL, 0, 8);
		if (cpport == NULL) return FALSE;
		return TRUE;
	}

	/*
	* AssociateIncomingClientWithContext
	*
	* This function do the Following:
	* 1) Does some simpleSecutity Stuff (e.g one connection per client, etc..)
	* 2) Allocates an Context for the Socket.
	* 3) Configures the Socket.
	* 4) Associate the Socket and the context with the completion port.
	* 5) Fires an IOInitialize So the IOWORKERS Start to work on the connection.
	*/

	_cc* iocpbase::socket2client(SOCKET cs){
		if (shutdown || cs == INVALID_SOCKET) return NULL;
		if (conns >= maxconns){
			LINGER li; li.l_onoff = 1; li.l_linger = 0;
			setsockopt(cs, SOL_SOCKET, SO_LINGER, (char *)&li, sizeof(li));
			::closesocket(cs);cs = INVALID_SOCKET;
		}
		if (cs == INVALID_SOCKET) return NULL;
		_cc* pContext = allocclient(cs);
		if (pContext != NULL){
			if (bind(cs, cpport, (ULONG_PTR)pContext)){ return pContext;}
			else{ decio(pContext);}
		}
		return NULL;
	}

	bool iocpbase::postinit(_cc* pContext){
		bool bRet = false;
		if (pContext != NULL){
			// Trigger first IO Completion Request
			// Otherwise the Worker thread will remain blocked waiting for GetQueuedCompletionStatus...
			// The first message that gets queued up is ClientIoInitializing - see ThreadPoolFunc and 
			// IO_MESSAGE_HANDLER
			// Important!! EnterIOLoop must notify that the socket and the structure  
			// pContext have an Pendling IO operation ant should not be deleted.
			// This is nessesary to avoid Access violation. 
			initbuf* pbuf = new initbuf();
			if (pbuf != NULL){
				BOOL bSuccess = PostQueuedCompletionStatus(cpport, 0, (ULONG_PTR)pContext, pbuf);
				if (bSuccess || (GetLastError() == ERROR_IO_PENDING)){ bRet = true; return bRet;}
				freebuf(pbuf);
			}
			decio(pContext);	// IOͶ��û�ɹ������ܾ��� �߳� IOWorkerThreadProc �ͷ�IO����Ҫ�ֹ��ͷš�
		}
		return bRet;
	}

	bool iocpbase::bind(SOCKET socket, const HANDLE& hCompletionPort, const uint32_t& dwCompletionKey){
		HANDLE h = CreateIoCompletionPort((HANDLE)socket, hCompletionPort, dwCompletionKey, 0);
		return h == hCompletionPort;
	}


	_cc* iocpbase::allocclient(SOCKET s){
		_cc* pContext = new _cc(s);
		if (pContext != NULL){
			_lock guard1(ccsmt);
			ccidnow++; pContext->ccid = ccidnow;
			if (ccs.find(ccidnow) != ccs.end()){
			}else{
				ccs[pContext->ccid] = pContext;	conns++;
				if (conns > lastmaxconns){ lastmaxconns = conns; }
			}
		}
		return pContext;
	}

	void iocpbase::freeclient(_cc* p){
			{
				_lock guard1(ccsmt);
				size_t er = ccs.erase(p->ccid);
				//printf(("Context erase ID��%d SOCKET��%d CCID��%I64u ERASE��%d\r\n"),p->m_ci.m_iUserId,p->m_Socket,p->m_u64CCID,er);
			}
		delete p;
		p = NULL;
	}

	/*
	* Notifyes that this Client Context Strukture is currently in the
	* IOCompetetion lopp and are used by a another thread.
	* This funktion and ExitIOLoop is used to avoid possible Access Violation
	*/

	bool iocpbase::incio(_cc* p){
		bool ret = false; LONG left = 0;
		if (p != NULL){
			if (p->ios <= 0){	// pendingio ����С�ڵ���0
			}else{ left = InterlockedIncrement(&p->ios); ret = true;}
		}
		return ret;
	}

	/*
	* Notifies that the ClientContext is no longer in used by thread x, and
	* have been removed from the competition port. This function decreses the
	* m_nNumberOfPendlingIO and returns it.
	*
	* if it return zero (0) then it is safe to delete the structure from the heap.
	* ���� FALSE����ʾ�û����������ߣ�����TRUE ��ʾ�û�IO����Ϊ0�������ˡ�
	*/

	bool iocpbase::decio(_cc* pContext){
		bool ret = false; LONG left = 0, PENDING = 0; uint64_t CCID = 0; 
		if (pContext != NULL){
			left = InterlockedDecrement(&pContext->ios);
			if (left <= 0){
				conns--;
				closesocket(pContext);	// ������ NotifyDisc...ǰ��ر�socket����ȻNotifyDisc����ѭ����
				NotifyDisconnection(pContext);
				ret = TRUE;
			}
			CCID = pContext->ccid; PENDING = pContext->ios;
			if (ret) freeclient(pContext);
		}
		//wchar_t szOut[1024] = {0};
		//swprintf_s(szOut,_countof(szOut),L"ExitIOLoop CCID��%I64u \t PENDING��%d\r\n",CCID,PENDING);
		//OutputDebugStringW(szOut);		
		return ret;
	}

	bool iocpbase::freebuf(LPOVERLAPPED pBuff){
		if (pBuff == NULL) return false;
		iobuf* pbase = (iobuf*)pBuff;
		switch (pbase->getoper()){
		case Init: delete (initbuf*)pBuff; break;
		case Read: delete (readbuf*)pBuff; break;
		case Write: delete (writebuf*)pBuff; break;
		case DisConnectEx: delete (DisConnectExBuffer*)pBuff; break;
		case WriteAll:{
			writeallbuf* pWta = (writeallbuf*)pBuff;
			if (pWta->dec() == 0){ delete pWta; pWta = NULL;}
		}break;
		}
		pBuff = NULL;
		return true;
	}

	int32_t iocpbase::closesocket(_cc *mp, bool bG){
		int32_t ret = 0, dwError = 0;
		if (bG && m_pDisConnectEx != NULL){
			if (incio(mp)){
				DisConnectExBuffer*  pOverlappedBuffer = new DisConnectExBuffer();
				BOOL bRet = m_pDisConnectEx(mp->s, pOverlappedBuffer, 0, 0);
				if (bRet || ((dwError = WSAGetLastError()) == ERROR_IO_PENDING)){
					bRet = TRUE;
					//_tprintf(_T("m_pDisConnectEx��%d %d\r\n"),dwError,bRet);
					//_tprintf(_T("m_pDisConnectEx��%d PENDING��%d OK!\r\n"), mp->m_Socket, mp->m_nNumberOfPendlingIO);
					return true;
				}else{
					//_tprintf(_T("m_pDisConnectEx��%d PENDING��%d FAIL!\r\n"), mp->m_Socket, mp->m_nNumberOfPendlingIO);
				}
				//printf("\nCloseSocket\n");
				decio(mp);
				freebuf(pOverlappedBuffer);
			}
		}else{
			if (mp->s != INVALID_SOCKET){ ret = ::closesocket(mp->s); mp->s = INVALID_SOCKET;}
			else{
				//_tprintf(_T("\r\n\r\nsocket�ر��ˡ�����û���ų����б� %s \t %d\r\n"),mp->m_ci.m_szUserName,mp->m_Socket);
			}
			//_tprintf(_T("\n��̫���ܵĴ���ɣ�socket�ر��ˡ�����û���ų����б� %s CCID��%I64u \t PENDING��%d\r\n"),mp->m_ci.m_tcUserName,mp->m_u64CCID,mp->m_nNumberOfPendlingIO);
		}
		return ret;
	}

	bool iocpbase::setupiothread(){
		for (int32_t i = 0; i < iothreads; i++){ std::thread(&iocpbase::iothread, this,this).detach();}
		std::thread(&iocpbase::heartbeatthread, this, this).detach();
		return true;
	}

	void iocpbase::iothread(void* p){
		CoInitialize(NULL);
		iocpbase* pThis = reinterpret_cast<iocpbase*>(p);
		if (pThis){
			bool err = false, bIOSuccesed = false; BOOL io = false; DWORD iosize = 0, _errno = 0; _cc* pc = NULL;
			iobuf* pbase = NULL; LPOVERLAPPED pover = NULL; ULONG_PTR ulOut = 0;
			while (!err){
				bIOSuccesed = false; iosize = 0;
				io = GetQueuedCompletionStatus(pThis->cpport, &iosize, &ulOut, &pover, INFINITE);
				pc = (_cc*)ulOut; pbase = (iobuf*)pover;
				if (io){
					if (pover && pc){
						bIOSuccesed = pThis->ioprogress(pover, pc, iosize);
					}else if ((pc == NULL) && (pover == NULL) && pThis->shutdown){
						err = true;				// �����ʾ�����������̡߳�
						continue;
					}else{}
				}else{	// If Something whent wrong..
					_errno = GetLastError();
					if (_errno != WAIT_TIMEOUT){ // It was not an Time out event we wait for ever (INFINITE) 
						if (pc != NULL){
							/*
							* ERROR_NETNAME_DELETED Happens when the communication socket
							* is cancelled and you have pendling WSASend/WSARead that are not finished.
							* Then the Pendling I/O (WSASend/WSARead etc..) is cancelled and we return with
							* ERROR_NETNAME_DELETED..
							*/
							if (_errno == ERROR_NETNAME_DELETED){
							}else{ // Should not get here if we do: disconnect the client and cleanup & report. 
								//wchar_t szError[1024] = {0};
								//swprintf_s(szError,_countof(szError),L"�����ܵĴ���%d CCID��%I64u\r\n",dwIOError,lpClientContext->m_u64CCID);
								//OutputDebugStringW(szError);
								//assert(0);
							}
						}else{
							// We shall never come here  
							// anyway this was an error and we should exit the worker thread
							//bError = TRUE;
						}
					}else{
						// ���������е��������
						//assert(0);
					}
				} 

				//printf("\nIOWorkerThreadProc\n");
				pThis->decio(pc);	// IO�����ˣ������Ƿ�ɹ�������Ҫ��δ��IO��ȥ1
				pThis->freebuf(pover);
				pover = NULL;
			}
		}

		CoUninitialize();
		return;
	}

	bool iocpbase::ioprogress(LPOVERLAPPED pbuf, _cc* pc, const uint32_t& len){
		if (pbuf == NULL || pc == NULL) return false;
		bool ret = true; int32_t err = 0; iobuf* base = (iobuf *)pbuf;
		switch (base->getoper()){
		case Init: ret = NotifyConnection(pc); if (ret) aread(pc); err = 1; break;
		case Read: ret = onread(pc, len, pbuf);	err = 2; break;
		case WriteAll: pc->ci.lastactive = time(NULL); err = 3; break;
		case Write:pc->ci.lastactive = time(NULL); err = 4;	break;
		case DisConnectEx: closesocket(pc); break;
		default:ret = false;break;
		}
		return ret;
	}

	bool iocpbase::aread(_cc *pc){ // ��������ÿͻ����Ǹ���ϻ���Ŷ����������copy�ˡ��д��Ľ���
		bool ret = false; uint32_t recved = 0, iosize = 0, wsaerr = 0, flags = MSG_PARTIAL;
		LPWSABUF buf = NULL; readbuf* read = NULL;
		if (pc != NULL){
			read = new readbuf();
			if (read != NULL){
				read->setupbuf(pc);
				buf = read->getbuf();
				if (incio(pc)){ // ����һ��IO����֮ǰ��������IO��
					recved = WSARecv(pc->s, buf, 1, (LPDWORD)&iosize, (LPDWORD)&flags, read, NULL);
					if (recved != SOCKET_ERROR || (wsaerr = WSAGetLastError()) == WSA_IO_PENDING){
						ret = true;
						return ret;
					}
					decio(pc); // IOͶ��û�ɹ������ܾ��� �߳� IOWorkerThreadProc �ͷ�IO����Ҫ�ֹ��ͷš�
				}
				freebuf(read);
			}
		}
		return ret;
	}

	bool iocpbase::onread(_cc* pc, const uint32_t& iosize, LPOVERLAPPED pover){
		uint32_t used = 0; bool ret = false; readbuf* read = (readbuf*)pover;
		if (iosize == 0 || pover == NULL || pc == NULL) return ret;
		// ����ÿ�� ClientContext ֻͶ��һ�� ��ȡ ���룬�����Ļ��� ClientContext ����������ľͲ���ͬ���ˡ�
		// ʵ���� һ��Ͷ�ݶ�� ��ȡ��Ҳû�б�Ҫ��������㷱�أ�������Ҳ���أ����ԼӴ��ȡ���ĳ��ȣ���û�б�Ҫ
		// Ͷ�ݶ����ȡ ����Ͷ�ݶ����ȡ �� ������ɰ���˳����ң�����Ҫ�԰���������������û�б�ҪͶ�ݶ����ȡ��
		// ���� Ͷ��ʱ�� LPWSABUF Ҳ֧��һ�������Ͷ�ݶ��������һ���� WSARecvͶ�ݶ��LPWSABUF��ȡ���壬���ҷ���ʱҲ������ģ�
		// ������ ʹ�� WSARecv ÿ��Ͷ��һ�� LPWSABUF �����ж��Ͷ�ݶ����󣩡�
		pc->cur += iosize;			// ������dwIoSize�����ݣ�Ų��һ�µ�ǰ����ָ�룬׼�����������ݡ�
		used = pc->cur - pc->pre;	// �ж��Ѿ��յ��ĵ�û����������߼�����ʣ�����ݵĳ��ȡ�
		if (pc->packlen == 0 && used >= sizeof(uint32_t)){// ��ǰ���ݹ�ȡ���߼��������ˡ�[����4][����1][����n]
			pc->packlen = *(uint32_t*)pc->pre; // ȡ���¸��߼����ĳ���
			pc->status = pc->packlen >> 24; 
			pc->packlen = pc->packlen & 0x00FFFFFF;
		}
		if (pc->packlen > limitbuf){// �߼������ܴ������Ľ��ܻ�������
			// ��ǰ�߼����ĳ��ȣ����ڰ�����������󳤶��ˣ����صĴ���
			//_tprintf(_T("�û���%s(%s) ID��%d CCID��%I64u ���������߼������ڻ�������(%d) > (%d)\r\n"),
			//	pc->m_ci.m_tcUserName, pc->m_ci.m_szMyIP, pc->m_ci.m_iUserId, pc->m_u64CCID, pc->m_uPackLen, pc->m_uiRecvLen);
			//pc->m_dwErrorCode = ERROR_READCOMPLETED_BUFUFFER_OF02;
			goto ErrorOut;
		}
		if (pc->packlen > 0){		// ���߼������ݵ��
			while (used >= pc->packlen && pc->packlen > sizeof(uint32_t)){	// ���������Ƿ����������߼�����
				// �����һ���������߼��������д���
				if (NotifyReceived(pc,pc->pre + sizeof(uint32_t), pc->packlen - sizeof(uint32_t),pc->status) == false) goto ErrorOut;
				// �Ѿ�������һ���߼����������ж�ʣ�໺�����Ƿ����������߼�����
				pc->pre += pc->packlen; pc->packlen = 0; // �Ѿ���������ݶ�����
				if (pc->cur < pc->pre) goto ErrorOut;
				used = pc->cur - pc->pre;	// ʣ���δ�������������ݵĳ��ȡ�
				if (used == 0) break;
				if (used >= sizeof(uint32_t)){	// ���ȹ�ȡ���¸��߼��ĳ����ˣ�
					pc->packlen = *(uint32_t*)pc->pre; // ȡ���¸��߼����ĳ��ȡ�
					pc->status = pc->packlen >> 24; pc->packlen = pc->packlen & 0x00FFFFFF;
					if (used < pc->packlen) break;
				}else break;
				if (pc->packlen > limitbuf)	goto ErrorOut;
			}
		}

		ret = aread(pc);	// Ͷ��һ�ζ�ȡIO��
		return ret;
	ErrorOut:
		return false;

	}
	// �Ż��ռ䣬���������ȷ���ռ䣬Ȼ���÷�������д�ش���Ϣ��Ȼ���ύ���Ϳ���ʡһ���ڴ�copy!
	bool iocpbase::send(const void* p, const int32_t& len, _cc* pc, const uint8_t& status){
		bool ret = true; LPWSABUF pbuf = NULL; uint32_t val = 0, iosize = 0, flag = MSG_PARTIAL, err;
		if (len == 0) return true;
		if (pc != NULL && pc->s != INVALID_SOCKET){
			if (incio(pc)){	// ����һ��IO����֮ǰ��������IO��
				writebuf* write = new writebuf(p, len,status); pbuf = write->getbuf();
				val = WSASend(pc->s, pbuf, 1, (LPDWORD)&iosize, flag, write, NULL);
				if (val != SOCKET_ERROR || (err = WSAGetLastError()) == WSA_IO_PENDING){ ret = true; return ret;	}
				decio(pc); delete write;// IOͶ��û�ɹ������ܾ��� �߳� IOWorkerThreadProc �ͷ�IO����Ҫ�ֹ��ͷš�
			}
		}
		return ret;
	}

	tm iocpbase::localtime(const int64_t& _now){
		int64_t now = _now;
		if (now < 0 || now >= 0x793406fffi64) now = 0;
		return *::_localtime64(&now);
	}


	void iocpbase::printonlines(){
		std::strstream ss; std::string str,str1;
		{
			_lock guard1(ccsmt);
			for (_ccs::iterator iter = ccs.begin(); iter != ccs.end(); iter++){
				ss << "[" << iter->first << "]" << iter->second->ci.id << "_" << iter->second->ci.un << "_" << iter->second->ci.ip
					<< "[" << time(NULL) - iter->second->ci.logintime << "," << time(NULL) - iter->second->ci.lastactive << "]";
				ss >> str1; ss.clear();
				str += str1;
				str += "\r\n";
			}
			ss << "�����ߣ�" << ccs.size();
			ss >> str1;
			str += str1;
			str += "\r\n";
		}
		printf(str.c_str());
	}



	void iocpbase::heartbeatthread(void*) {
		while (true) {
			{
				Sleep(1000 * 60 * 4);
			}
			otprint("��������...");
			/*
			_lock guard1(ccsmt);
			KEEPALIVE kl = { 0 };
			kl.oh.c = C_KEEP;
			int tt = ccs.size();
			int ts = 0;
			for (_ccs::iterator iter = ccs.begin(); iter != ccs.end(); iter++) {
				if ((time(NULL) - iter->second->ci.lastactive) >  60 * 10) {
					ts++;
					if (send(&kl, sizeof(kl), iter->second))
						iter->second->ci.lastactive = time(NULL);
				}
			}
			
			otprint("��������[%d,%d]",tt,ts);*/
		}
	}

	iocpserver::iocpserver() :socket(INVALID_SOCKET), iolisteners(2){}
	iocpserver::~iocpserver(){}

	void iocpserver::stop(){
		shutdown = true;
		for (int i = 0; i < iothreads; i++)
			PostQueuedCompletionStatus(cpport, 0, 0, 0);
		Sleep(2 * 1000);
	}

	bool iocpserver::start(const int32_t& _port, const int32_t& _maxconns, const int32_t& _maxiothreads, const int32_t& _maxpending){
		maxconns = _maxconns; iothreads = _maxiothreads; port = _port; shutdown = false; maxpending = _maxpending;
		return startup();
	}

	bool iocpserver::startup(){
		conns = 0; lastmaxconns = 0; shutdown = false;
		if (createport()){
			if (port > 0){
				init();
				if (setuplistener()){
					if (setupiothread()){
						return true;
					}
				}
			}
		}
		return false;
	}

	bool iocpserver::setuplistener(){
		socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (socket != INVALID_SOCKET){
			GUID	GuidAcceptEx = WSAID_DISCONNECTEX; DWORD dwBytes = 0;
			// Get AccpetEx Function
			if (WSAIoctl(socket,SIO_GET_EXTENSION_FUNCTION_POINTER,	&GuidAcceptEx,sizeof(GuidAcceptEx),
				&m_pDisConnectEx,sizeof(m_pDisConnectEx),&dwBytes,NULL,	NULL) == 0){
				event = WSACreateEvent();				// Event for handling Network IO
				if (event != WSA_INVALID_EVENT){
					// The listener is ONLY interested in FD_ACCEPT
					// That is when a client connects to or IP/Port
					// Request async notification
					if (WSAEventSelect(socket, event, FD_ACCEPT) == 0){
						SOCKADDR_IN	saServer;
						saServer.sin_port = htons(port);	// Listen on our designated Port#
						saServer.sin_family = AF_INET;		// Fill in the rest of the address structure
						saServer.sin_addr.s_addr = INADDR_ANY;
						if (::bind(socket, (LPSOCKADDR)&saServer, sizeof(struct sockaddr)) != SOCKET_ERROR){
							if (listen(socket, 5) != SOCKET_ERROR){	// Set the socket to listen
								 for (int32_t i = 0; i < this->iolisteners; i++){
									std::thread(&iocpserver::listenerthread, this,this).detach();
								}
								return true;
							}
						}
					}
					WSACloseEvent(event);
				}
			}
			::closesocket(socket);
		}

		return false;
	}

	void iocpserver::listenerthread(void* pParam){
		bool ok = false; char ip[20] = { 0 }; int32_t err = 0; WSANETWORKEVENTS events; DWORD ret,len; _cc* pc;
		iocpserver* pThis = reinterpret_cast<iocpserver*>(pParam); WSA_MAXIMUM_WAIT_EVENTS; WSA_WAIT_FAILED;
		if (pThis){
			while (!pThis->shutdown){
				err = 0; ok = false; ip[0] = 0;
				ret = WSAWaitForMultipleEvents(1, &pThis->event, FALSE, 1000, FALSE);
				if (ret == WSA_WAIT_TIMEOUT) continue; if (ret == WSA_WAIT_FAILED) break;
				//printf("WSAWaitForMultipleEvents ok\r\n");
				if(WSAEnumNetworkEvents(pThis->socket, pThis->event, &events) == SOCKET_ERROR) break;
				if(events.lNetworkEvents & FD_ACCEPT){
					if (events.iErrorCode[FD_ACCEPT_BIT] == 0 && !pThis->shutdown){
						SOCKET	client = INVALID_SOCKET; 
						//do {
							SOCKADDR sa;  int32_t ret = -1, sasize = sizeof(sa);
							client = WSAAccept(pThis->socket, (SOCKADDR*)&sa, &sasize, 0, 0);	// ͬ�����ӣ�����ȡһ�¿ͻ�IP��ַ������¼
							if (client != INVALID_SOCKET) {
								pc = pThis->socket2client(client);	// ������ͬ������ӡ�
								if (pc != NULL && pThis->postinit(pc)) {	// ����һ��io
									len = sizeof(pc->ci.ip);
									WSAAddressToStringA((LPSOCKADDR)&sa, sizeof(sa), NULL, pc->ci.ip, &len); ok = true;
								}
							}
							else {
								err = WSAGetLastError();
							}
						//} while (client != INVALID_SOCKET);
					}else break;
				}
				//_tprintf(_T("�û���%s Accept Status��%d\r\n"),wcIp,bSuccessed);
			}
		}
		err = GetLastError();
		return;
	}



}