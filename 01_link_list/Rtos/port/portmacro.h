/**
 * @file portmacro.h
 * @brief 移植相关的宏定义, 不同的机器(8bit 16bit 32bit)直接配置不同的数据类型.方便上层的代码复用.
 * @author Tengfei Zhang (izhangtengfei@163.com)
 * @version 1.0
 * @date 2024-07-31 12:07
 * 
 * @copyright Copyright (c) 2024  Tengfei Zhang All rights reserved
 * 
 * @par 修改日志:
 * <table>
 * <tr><th>Date       <th>Version <th>Author  <th>Description
 * <tr><td>2024-07-31 12:07 <td>1.0     <td>wangh     <td>内容
 * </table>
 */
#ifndef PORTMACRO_H
#define PORTMACRO_H

#include "stdint.h"
#include "stddef.h"


/* 数据类型重定义 */
#define portCHAR		char
#define portFLOAT		float
#define portDOUBLE	double
#define portLONG		long
#define portSHORT		short
#define portSTACK_TYPE	uint32_t
#define portBASE_TYPE	long

typedef portSTACK_TYPE StackType_t;
typedef long BaseType_t;
typedef unsigned long UBaseType_t;

#if( configUSE_16_BIT_TICKS == 1 )
	typedef uint16_t TickType_t;
	#define portMAX_DELAY ( TickType_t ) 0xffff
#else
	typedef uint32_t TickType_t;
	#define portMAX_DELAY ( TickType_t ) 0xffffffffUL
#endif

#endif /* PORTMACRO_H */
