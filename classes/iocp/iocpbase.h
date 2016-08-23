#ifndef __IOCPBASE_H
#define __IOCPBASE_H
#include <stdio.h>
#include <WinSock2.h>
#include <mswsock.h>
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"Mswsock.lib")
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <map>
#pragma warning (disable : 4996)

namespace xiny120 {
	class _cc;

	// ֻ���ڱ��߳�����һ��������ʹ�ã����ܱ��浽�´�ʹ�ã������� gmtime
	//char* _tlsget(); void _tlsfree(); int64_t _tlscap(int64_t size);

	const int32_t maxbuf = 10240, incbuf = 1024, limitbuf = 1024*1024*2;
	enum iotype{ Init = 0, Read, Write, WriteAll, DisConnectEx};

	class iobuf :public OVERLAPPED{ // ����IO ���Ļ���
	public:
		iobuf(){ ZeroMemory(this, sizeof(iobuf)); }
		inline int32_t getoper(){return oper;}
		WSABUF* getbuf(){ return (&buf); }
	protected:
		WSABUF buf; int32_t oper;
	};
	class initbuf :public iobuf{ public: initbuf();	};
	class DisConnectExBuffer :public iobuf{ public: DisConnectExBuffer(); };
	class readbuf :public iobuf{
	public:
		readbuf();
		bool setupbuf(_cc*& pContext);
	};

	class holdbuf{
	public:
		holdbuf(const void* pVoid, const uint32_t& len, const uint8_t& status = 0);
		~holdbuf();
		WSABUF* getbuf() { return &buf; };
		inline int32_t getpending(){ return pending; };
		inline int32_t inc(){ return ::InterlockedIncrement(&pending); }
		inline int32_t dec(){ return ::InterlockedDecrement(&pending); }
	private:
		WSABUF	buf; LONG	pending;
	};

	class writeallbuf : public iobuf{
	public:
		writeallbuf(holdbuf* pwsaBuf);
		~writeallbuf();
		inline int32_t inc(){ return ::InterlockedIncrement(&pending); }
		inline int32_t dec(){ return ::InterlockedDecrement(&pending); }
		WSABUF* getbuf(){ return pbuf->getbuf();}
	private:
		holdbuf* pbuf; LONG pending;
	};

	class writebuf :public iobuf{
	public:
		writebuf(const void* pVoid, const uint32_t&, const uint8_t& status = 0);
		~writebuf();
	};

	class filetransport{
	public:
		filetransport();
		~filetransport();
		bool doing();
		void close();
		bool open(std::string file);
		int32_t read(char*, const int32_t&);
		int size();
		bool seek(int32_t pos); // ���Ǵ�FILE_BEGIN
		int error(){ return err; };
		int size(std::string);
		uint32_t pubid, year; int64_t filetime;
	private:
		std::string basepath; std::string filename; FILE * fp; int32_t err; int64_t createtime, lastio;
	};

	class clientinfo{ // clientinfo
	public:
		// 174406	Ϊ b55 (���̿�)
		// 10		Ϊ b52
		// 4163		Ϊ tech
		// 174384	Ϊ vcd40 40Сʱ
		// 7653		Ϊ xuexi 117�ڿγ�
		// 177247	Ϊ ����������ʽ(��ʽ��)
		// 175011   Ϊ ����ɼ�����֮һ
		const uint64_t p174406 = 0x1;
		const uint64_t p10 = 0x2;
		const uint64_t p4163 = 0x4;
		const uint64_t p174384 = 0x8;
		const uint64_t p7653 = 0x10;
		const uint64_t p177247 = 0x20;
		const uint64_t p175011 = 0x40;
		const uint64_t pother = 0x80;

	public:
		clientinfo() : login(false){};
		~clientinfo(){};

		int32_t pushpath(std::string _path);
		uint64_t id,usergroupid, permission; char un[64], pwd[32], ip[32]; int64_t logintime, lastactive; bool login;
		SOCKADDR sadr;

		std::vector<std::string> searchpaths; filetransport ft;
	};

	class _cc{ // clientcontext
	public:
		_cc(SOCKET _s);
		~_cc(){ delete[] buf; };
		int32_t realloc();
		int32_t left(){ return (len - (cur - buf)); };
		int32_t move();
		SOCKET s; uint64_t ccid,ccid1; LONG ios,maxio; char* buf, *pre, *cur; uint32_t len, packlen; uint8_t initcode[16],status; clientinfo ci;

	};

	typedef std::map<uint64_t, _cc*>	_ccs;
	typedef std::lock_guard<std::recursive_mutex> _lock;

	class iocpbase{
	public:
		iocpbase();
		~iocpbase();

		virtual void init(){};
		virtual void NotifyDisconnection(_cc*){};
		virtual bool NotifyConnection(_cc*){ return TRUE; };
		virtual bool NotifyReceived(_cc*, const char*, const int32_t&, const uint8_t&){ return TRUE; };

		bool send(const void*, const int32_t&, _cc*,const uint8_t& status = 0);
		int32_t getonlines(){ return ccs.size(); };
		void setecho(bool b){ echo = b; };
		void printonlines();
		void lock() { ccsmt.lock(); }
		void unlock() { ccsmt.unlock(); };
	protected:
		bool iocpbase::setupiothread();
		bool createport();
		_cc* allocclient(SOCKET s);
		bool postinit(_cc* pContext);
		_cc* socket2client(SOCKET cs);
		bool decio(_cc* pContext);
		bool incio(_cc* p);
	protected:
		LPFN_DISCONNECTEX m_pDisConnectEx;
		HANDLE cpport; bool shutdown; bool echo;
		uint64_t ccidnow; volatile int32_t conns, lastmaxconns; int32_t iothreads, maxconns,port,maxpending;
		_ccs ccs;
	private:
		void iothread(void*);
		void heartbeatthread(void*);
		bool ioprogress(LPOVERLAPPED pBuff, _cc *pContext, const uint32_t& dwSize);
		bool aread(_cc *pc);
		bool onread(_cc* pc, const uint32_t& iosize, LPOVERLAPPED pover);
		void freeclient(_cc* p);
		bool bind(SOCKET socket,const HANDLE& hCompletionPort, const uint32_t& dwCompletionKey);
		bool freebuf(LPOVERLAPPED pBuff);
		int32_t closesocket(_cc *mp, bool bG = false);
		
		std::recursive_mutex ccsmt;
	public:
		static tm localtime(const int64_t& _now);

	};

	class iocpserver : public iocpbase{
	public:
		bool start(const int32_t& port = 999, const int32_t& maxconns = 20000, const int32_t& maxiothreads = 10, const int32_t& maxpending = 800);
		void stop();

		iocpserver();
		virtual ~iocpserver();

	public:
	private:
		SOCKET	socket;	// ����SOCKET
		HANDLE	event;
		int32_t iolisteners;

	protected:
		virtual bool startup();
		bool setuplistener();
		void listenerthread(void*);
		

	};
}

#endif
