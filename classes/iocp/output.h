#ifndef __TSERVER2_OUTPUT_H_
#define __TSERVER2_OUTPUT_H_
#include <stdio.h>
#include <stdint.h>


int32_t otprint(const char* fmt,...);
int32_t otprint(const wchar_t* fmt, ...);

#endif