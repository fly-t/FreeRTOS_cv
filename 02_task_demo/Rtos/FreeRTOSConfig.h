#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* 是否使用16bit的systick 根据sysytick的大小决定. 0关闭, 1打开 */
#define configUSE_16_BIT_TICKS		                0
/* 任务名字最大长度 */
#define configMAX_TASK_NAME_LEN                     16
/* 任务栈静态初始化开关 */
#define configSUPPORT_STATIC_ALLOCATION             1
/* 任务最大优先级 */
#define configMAX_PRIORITIES                        5

/* 为啥高四位有效: Cortex-M处理器中，NVIC（嵌套向量中断控制器）通常只使用优先级寄存器的高4位来设置中断优先级。为啥是uint32因为是32bits处理器 */
#define configKERNEL_INTERRUPT_PRIORITY 255           /* 高四位有效，即等于0xff，或者是15,  */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 	191   /* 高四位有效，即等于0xb0，或者是11 */


#define xPortPendSVHandler   PendSV_Handler
#define xPortSysTickHandler  SysTick_Handler
#define vPortSVCHandler      SVC_Handler

#endif /* FREERTOS_CONFIG_H */
