#ifndef _TSERVER2_TLS_H_
#define _TSERVER2_TLS_H_
#include <stdio.h>
#include <stdint.h>

namespace xiny120 {
	// 只能在本线程中在一个流程中使用，不能保存到下次使用！类似于 gmtime
	char* _tlsget(); void _tlsfree(); int64_t _tlscap(int64_t size);
}

#endif