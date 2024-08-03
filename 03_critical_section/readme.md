# 临界区保护

临界区资源的保护就是对中断的控制和管理

在FreeRTOS中, 临界区资源保护就是通过basepri寄存器来控制，basepri寄存器是ARM Cortex-M处理器中的一个特殊寄存器，它用于控制中断优先级。

命名规范: 带ISR的宏表示可以在中断中调用,因为有返回值.

``` c
// 先将`basepri`的的值保存在返回值`ulReturn`中, 然后修改`basepri`的值, 然后再修改值, 目的是返回当前的 BASEPRI 值，使得调用者在临界区结束后可以恢复原始的 BASEPRI，确保系统正常运行。
#define portSET_INTERRUPT_MASK_FROM_ISR()		ulPortRaiseBASEPRI()
#define portCLEAR_INTERRUPT_MASK_FROM_ISR(x)	vPortSetBASEPRI(x)		
```

``` c
static portFORCE_INLINE uint32_t ulPortRaiseBASEPRI(void)
{
    uint32_t ulReturn, ulNewBASEPRI = configMAX_SYSCALL_INTERRUPT_PRIORITY;  // 设置中断响应等级是11, 大于11不被响应

    __asm
    {
        /* 保存返回当前的 BASEPRI 值目的是: 使得调用者在临界区结束后可以恢复原始的 BASEPRI，确保系统正常运行。 */
        /* 没有返回值的函数无法恢复原始的 BASEPRI，可能导致系统中断优先级配置错误，影响中断嵌套和响应。 */
		mrs ulReturn, basepri
		msr basepri, ulNewBASEPRI
		dsb
		isb
    }

    return ulReturn;
}
```


为啥有返回值的函数可以实现中断嵌套? 没有的就不行

![](https://raw.githubusercontent.com/fly-t/images/main/blog/readme-2024-08-03-11-37-33.png)