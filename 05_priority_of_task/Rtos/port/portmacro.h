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
#include "FreeRTOSConfig.h"
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


/* 中断控制状态寄存器：0xe000ed04
 * Bit 28 PENDSVSET: PendSV 悬起位
 */
#define portNVIC_INT_CTRL_REG		( * ( ( volatile uint32_t * ) 0xe000ed04 ) )
#define portNVIC_PENDSVSET_BIT		( 1UL << 28UL )

#define portSY_FULL_READ_WRITE		( 15 )

/* __isb: 指令用于确保之前所有的指令执行完成并刷新指令缓存后，才允许继续执行后续的指令 */
/* __dsb: 用于确保在它之前的所有数据访问操作完成后，才允许后续的操作继续进行 */
#define portYIELD()                                           \
        {                                                     \
        /* 触发PendSV，产生上下文切换 */                        \
            portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT;   \
            __dsb(portSY_FULL_READ_WRITE);                    \
            __isb(portSY_FULL_READ_WRITE);                    \
        }


/* 临界区管理 */
extern void vPortEnterCritical( void );
extern void vPortExitCritical( void );

#define portENTER_CRITICAL()					vPortEnterCritical()
#define portEXIT_CRITICAL()						vPortExitCritical()				
				
/* 不带返回值的开关中断:不可以嵌套,不能在中断中使用 */
#define portDISABLE_INTERRUPTS() vPortRaiseBASEPRI()
#define portENABLE_INTERRUPTS() vPortSetBASEPRI(0)

#define portSET_INTERRUPT_MASK_FROM_ISR()		ulPortRaiseBASEPRI()
#define portCLEAR_INTERRUPT_MASK_FROM_ISR(x)	vPortSetBASEPRI(x)				
				
				
				
#define portINLINE __inline

#ifndef portFORCE_INLINE
#define portFORCE_INLINE __forceinline
#endif

static portFORCE_INLINE void vPortSetBASEPRI(uint32_t ulBASEPRI)
{
    __asm
    {
        /* Barrier instructions are not used as this function is only used to
        lower the BASEPRI value. */
		msr basepri, ulBASEPRI
    }
}

static portFORCE_INLINE void vPortRaiseBASEPRI(void)
{
    uint32_t ulNewBASEPRI = configMAX_SYSCALL_INTERRUPT_PRIORITY;

    __asm
    {
        /* Set BASEPRI to the max syscall priority to effect a critical
        section. */
		msr basepri, ulNewBASEPRI
		dsb
		isb
    }
}

static portFORCE_INLINE uint32_t ulPortRaiseBASEPRI(void)
{
    uint32_t ulReturn, ulNewBASEPRI = configMAX_SYSCALL_INTERRUPT_PRIORITY;  // 设置中断响应等级是11, 大于11不被响应

    __asm
    {
        /* Set BASEPRI to the max syscall priority to effect a critical
        section. */
		mrs ulReturn, basepri
		msr basepri, ulNewBASEPRI
		dsb
		isb
    }

    return ulReturn;
}

#define portTASK_FUNCTION(vFunction, pvParameters) void vFunction(void *pvParameters)

// 开中断
static portFORCE_INLINE void vPortClearBASEPRIFromISR( void )
{
	__asm
	{
		/* Set BASEPRI to 0 so no interrupts are masked.  This function is only
		used to lower the mask in an interrupt, so memory barriers are not 
		used. */
		msr basepri, #0
	}
}

#endif /* PORTMACRO_H */
