
#include "tls.h"

namespace xiny120 {
	const int64_t _tls_len = 1024 * 1024 * 5;
	__declspec(thread)	char* __tls = NULL;

	/* 只能在本线程中在一个流程中使用，不能保存到下次使用！类似于 gmtime*/
	char* _tlsget(){ if (__tls == NULL) __tls = new char[_tls_len]; return __tls; }
	void _tlsfree(){ if (__tls != NULL) delete[] __tls; }
	int64_t _tlscap(int64_t size){ return _tls_len / size; } // 容纳数据的个数
}