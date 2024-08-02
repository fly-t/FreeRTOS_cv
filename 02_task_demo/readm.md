# 任务的定义

1. 将任务数据保存在栈中
2. 创建任务栈(数组)
3. 创建任务函数

# 任务调度

> 通过pendSV寄存器触发任务调度

1. 启动调度器, 先将`portNVIC_SYSPRI2_REG`寄存器的`PENDSV`和`SYSTICK`优先级设置为最低.

``` c
BaseType_t xPortStartScheduler( void )
{
    /* 配置PendSV 和 SysTick 的中断优先级为最低 */
	portNVIC_SYSPRI2_REG |= portNVIC_PENDSV_PRI;
	portNVIC_SYSPRI2_REG |= portNVIC_SYSTICK_PRI;

	/* 启动第一个任务，不再返回 */
	prvStartFirstTask();

	/* 不应该运行到这里 */
	return 0;
}
```

2. 启动第一个任务

> 配置CM3内核的msp指针到stm32的向量表开始地址. 并开启中断, 并软件触发中断启动任务.


![](https://raw.githubusercontent.com/fly-t/images/main/blog/readm-2024-08-02-19-46-18.png)

CM3内核中默认项量表的地址是0x00000000, 但是可以动态设置.

动态设置的方法就是通过向量表偏移寄存器设置偏移量,向量表偏移寄存器是0xE000ED08.

大多数stm32芯片的向量表开始地址定义到了 `0X08000000`。

![](https://raw.githubusercontent.com/fly-t/images/main/blog/readm-2024-08-02-19-57-05.png)

``` c
__asm void prvStartFirstTask( void )
{
    // PRESERVE8 指示编译器保证堆栈指针的8字节对齐，这在ARM Cortex-M架构中是必须的，以确保栈操作的正确性。
	PRESERVE8

    /* 偏移量寄存器地址:0xE000ED08 */ 
	ldr r0, =0xE000ED08  // 将立即数'0xE000ED08'放到r0
	ldr r0, [r0]         // 将0xE000ED08的起始地址放到r0,
	ldr r0, [r0]         // 将0xE000ED08起始地址中的msp的值放到r0

	msr msp, r0          // 将寄存器 r0 的值（主堆栈指针的初始值）写入主堆栈指针（MSP）寄存器 
    
	/* 使能全局中断 */
	cpsie i
	cpsie f
	dsb
	isb     // 这是一种内存屏障，用于确保在执行后续指令之前所有内存操作都已完成。
	
    /* 调用SVC去启动第一个任务,中断号可以是0-255  */
	svc 0   // 触发一个软件中断（SuperVisor Call），以启动第一个任务。在FreeRTOS中，0常用于切换到第一个任务的上下文。
	nop     // 是一个空操作指令，不执行任何操作。它们在这里用于占位或对齐，通常没有实际的效果，但可以用来使代码对齐或作为调试时的占位符。
	nop
}
```


# 中断处理

ldr指令:

正确: 
> ldr r1, =0x20000000
> ldr r0,[r1]         

错误:
> ldr r1, =0x20000000  ; 假设 r1 现在是内存地址 0x20000000
> ldr r0,r1        ; 从内存地址 0x20000000 加载数据到 r0  

为啥不可以: <font color='red'> 在ARM汇编语言中，ldr r0, r1 这样的指令在标准的ARM指令集下是无效的。原因在于，ldr 指令的操作数不允许直接将一个寄存器作为地址源来加载另一个寄存器的值 </font>


``` c
/* 处理svc中断 */
__asm void vPortSVCHandler( void )
{
    extern pxCurrentTCB;
    
    PRESERVE8

	ldr	r3, =pxCurrentTCB	// pxCurrentTCB地址赋值给r3.  是一个指向 TCB_t 的指针，此处即为指向第一个任务TCB_t 的指针
	ldr r1, [r3]			// 加载pxCurrentTCB->pxTopOfStack指针到r1 
	ldr r0, [r1]			// pxCurrentTCB->pxTopOfStack指向的数据 到r0，目前r0的值等于第一个任务堆栈的栈顶 
	ldmia r0!, {r4-r11}		// 以r0为基地址，pxCurrentTCB->pxTopOfStack所指向的(栈)里面的内容恢复到r4~r11寄存器，同时r0会递增,
                            // ! 表示在加载后自动更新 r0，即 r0（栈指针）向上移动以指向下一个数据

	msr psp, r0				/* 将r0的值，即任务的栈指针更新到psp */
	isb                     // 保证指令的顺序和一致性。

	mov r0, #0              /* 设置r0的值为0 */
	msr	basepri, r0         // 将基准优先级寄存器（BASEPRI）设置为 0，取消任何中断优先级屏蔽，恢复全局中断使能状态
	orr r14, #0xd           /* 当从SVC中断服务退出前,通过向r14寄存器最后4位按位或上0x0D，
                               使得硬件在退出时使用进程堆栈指针PSP完成出栈操作并返回后进入线程模式、返回Thumb状态 */
    
	bx r14                  /* 异常返回，这个时候栈中的剩下内容将会自动加载到CPU寄存器：
                               xPSR，PC（任务入口地址），R14，R12，R3，R2，R1，R0（任务的形参）
                               同时PSP的值也将更新，即指向任务栈的栈顶 */
}
```