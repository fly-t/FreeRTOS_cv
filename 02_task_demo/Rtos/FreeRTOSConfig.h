#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* 是否使用16bit的systick 根据sysytick的大小决定. 0关闭, 1打开 */
#define configUSE_16_BIT_TICKS		0
#define configMAX_TASK_NAME_LEN     16
#define configSUPPORT_STATIC_ALLOCATION           1

#define configKERNEL_INTERRUPT_PRIORITY 		255   /* 高四位有效，即等于0xff，或者是15 */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 	191   /* 高四位有效，即等于0xb0，或者是11 */
#endif /* FREERTOS_CONFIG_H */
