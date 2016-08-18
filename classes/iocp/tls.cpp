
#include "tls.h"

namespace xiny120 {
	const int64_t _tls_len = 1024 * 1024 * 5;
	__declspec(thread)	char* __tls = NULL;

	/* ֻ���ڱ��߳�����һ��������ʹ�ã����ܱ��浽�´�ʹ�ã������� gmtime*/
	char* _tlsget(){ if (__tls == NULL) __tls = new char[_tls_len]; return __tls; }
	void _tlsfree(){ if (__tls != NULL) delete[] __tls; }
	int64_t _tlscap(int64_t size){ return _tls_len / size; } // �������ݵĸ���
}