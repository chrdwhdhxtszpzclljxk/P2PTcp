#ifndef _TSERVER2_TLS_H_
#define _TSERVER2_TLS_H_
#include <stdio.h>
#include <stdint.h>

namespace xiny120 {
	// ֻ���ڱ��߳�����һ��������ʹ�ã����ܱ��浽�´�ʹ�ã������� gmtime
	char* _tlsget(); void _tlsfree(); int64_t _tlscap(int64_t size);
}

#endif