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


# SVC中断处理

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

# 任务切换

任务函数,通过taskYIELD()手动切换, 该函数会触发pendSV中断,将pendSV设置为1.随后进入到pendSV中断中断处理函数:`__asm void xPortPendSVHandler( void )`

``` c
/* 任务函数 */
void task1_entry(void *p_arg)
{
    while (1)
    {
			flag1 = 1;
 			delay( 100 );		
			flag1 = 0;
			delay( 100 );

			/* 任务切换，这里是手动切换 */
			taskYIELD();
    }
    
}
```


将r4 到 r11 的值存储到 r0 指向的内存地址,并更新 r0。

str r0, [r2]：将更新后的栈顶指针 r0 存储到当前任务控制块中。

mov r0, #configMAX_SYSCALL_INTERRUPT_PRIORITY：将 configMAX_SYSCALL_INTERRUPT_PRIORITY 的值加载到 r0 中。
msr basepri, r0：将 r0 的值写入 basepri 寄存器，设置中断优先级，进入临界区。设置中断优先级, 过滤中断, 只保留关键中断.

vTaskSwitchContext进行任务切换,切换到另外一个任务.

将另一个任务控制块的的栈顶指针更新到 r0 中。

将关闭的中断重新打开, 恢复r4-r11

更新psp指针


``` c
__asm void xPortPendSVHandler( void )
{
	extern pxCurrentTCB;
	extern vTaskSwitchContext;

	PRESERVE8

    /* 当进入PendSVC Handler时，上一个任务运行的环境即：
       xPSR，PC（任务入口地址），R14，R12，R3，R2，R1，R0（任务的形参）
       这些CPU寄存器的值会自动保存到任务的栈中，剩下的r4~r11需要手动保存 */
    /* 获取任务栈指针到r0 */
	mrs r0, psp
	isb

	ldr	r3, =pxCurrentTCB		/* 加载pxCurrentTCB的地址到r3 */
	ldr	r2, [r3]                /* 加载pxCurrentTCB到r2 */

	stmdb r0!, {r4-r11}			/* 将CPU寄存器r4~r11的值存储到r0指向的地址 */
	str r0, [r2]                /* 将任务栈的新的栈顶指针存储到当前任务TCB的第一个成员，即栈顶指针 */				
                               

	stmdb sp!, {r3, r14}        /* 将R3和R14临时压入堆栈，因为即将调用函数vTaskSwitchContext,
                                  调用函数时,返回地址自动保存到R14中,所以一旦调用发生,R14的值会被覆盖,因此需要入栈保护;
                                  R3保存的当前激活的任务TCB指针(pxCurrentTCB)地址,函数调用后会用到,因此也要入栈保护 */
	mov r0, #configMAX_SYSCALL_INTERRUPT_PRIORITY    /* 进入临界段 */
	msr basepri, r0
	dsb
	isb
	bl vTaskSwitchContext       /* 调用函数vTaskSwitchContext，寻找新的任务运行,通过使变量pxCurrentTCB指向新的任务来实现任务切换 */ 
	mov r0, #0                  /* 退出临界段 */
	msr basepri, r0             /* 恢复中断 */
	ldmia sp!, {r3, r14}        /* 恢复r3和r14 */

	ldr r1, [r3]
	ldr r0, [r1] 				/* 当前激活的任务TCB第一项保存了任务堆栈的栈顶,现在栈顶值存入R0*/
	ldmia r0!, {r4-r11}			/* 出栈 */
	msr psp, r0
	isb
	bx r14                      /* 异常发生时,R14中保存异常返回标志,包括返回后进入线程模式还是处理器模式、
                                   使用PSP堆栈指针还是MSP堆栈指针，当调用 bx r14指令后，硬件会知道要从异常返回，
                                   然后出栈，这个时候堆栈指针PSP已经指向了新任务堆栈的正确位置，
                                   当新任务的运行地址被出栈到PC寄存器后，新的任务也会被执行。*/
	nop
}

```

任务控制块的切换

``` c
void vTaskSwitchContext(void)
{
    /* 两个任务轮流切换 */
    if (pxCurrentTCB == &Task1TCB)
    {
        pxCurrentTCB = &Task2TCB;
    }
    else
    {
        pxCurrentTCB = &Task1TCB;
    }
}
```