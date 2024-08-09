# 空闲任务

cpu是一直在运行的, 没有任务执行的时候cpu也是一直在空跑.

这样就比较浪费资源, 所以开辟了一个空闲任务处理一些无关紧要的事情.

![](https://raw.githubusercontent.com/fly-t/images/main/blog/readme-2024-08-03-12-06-20.png)

1. 空闲任务的优先级设置为0, 为最低优先级.
2. 空闲任务运行时, 尝试执行其他任务, 检查其他任务延时是否结束,如果延时没有结束就继续执行空闲任务
3. 如果当前执行任务1, 尝试执行任务2, 检测任务2是否处于延时状态,如果不在就切换新任务.
4. 如果所有任务都处于延时状态就进入到空闲任务

## 在TCB_t结构体中添加`xTicksToDelay`字段,用于延时

``` c

typedef struct tskTaskControlBlock
{
    volatile StackType_t *pxTopOfStack; /* 栈顶 */

    ListItem_t xStateListItem; /* 任务节点 */

    StackType_t *pxStack; /* 任务栈起始地址 */
                          
    char pcTaskName[configMAX_TASK_NAME_LEN];/* 任务名称，字符串形式 */
	TickType_t xTicksToDelay; /* 用于延时 */    
} tskTCB;
typedef tskTCB TCB_t;
```

## 重写任务切换代码

``` c
void vTaskSwitchContext( void )
{
	/* 如果当前线程是空闲线程，那么就去尝试执行线程1或者线程2，
       看看他们的延时时间是否结束，如果线程的延时时间均没有到期，
       那就返回继续执行空闲线程 */
	if( pxCurrentTCB == &IdleTaskTCB )
	{
		if(Task1TCB.xTicksToDelay == 0)
		{            
            pxCurrentTCB =&Task1TCB;
		}
		else if(Task2TCB.xTicksToDelay == 0)
		{
            pxCurrentTCB =&Task2TCB;
		}
		else
		{
			return;		/* 线程延时均没有到期则返回，继续执行空闲线程 */
		} 
	}
	else
	{
		/*如果当前线程是线程1或者线程2的话，检查下另外一个线程,如果另外的线程不在延时中，就切换到该线程
        否则，判断下当前线程是否应该进入延时状态，如果是的话，就切换到空闲线程。否则就不进行任何切换 */
		if(pxCurrentTCB == &Task1TCB)
		{
			if(Task2TCB.xTicksToDelay == 0)
			{
                pxCurrentTCB =&Task2TCB;
			}
			else if(pxCurrentTCB->xTicksToDelay != 0)
			{
                pxCurrentTCB = &IdleTaskTCB;
			}
			else 
			{
				return;		/* 返回，不进行切换，因为两个线程都处于延时中 */
			}
		}
		else if(pxCurrentTCB == &Task2TCB)
		{
			if(Task1TCB.xTicksToDelay == 0)
			{
                pxCurrentTCB =&Task1TCB;
			}
			else if(pxCurrentTCB->xTicksToDelay != 0)
			{
                pxCurrentTCB = &IdleTaskTCB;
			}
			else 
			{
				return;		/* 返回，不进行切换，因为两个线程都处于延时中 */
			}
		}
	}
}
```

## 延时函数


5. 创建延时函数

``` c
void vTaskDelay( const TickType_t xTicksToDelay )
{
    TCB_t *pxTCB = NULL;
    
    /* 获取当前任务的TCB */
    pxTCB = pxCurrentTCB;
    
    /* 设置延时时间 */
    pxTCB->xTicksToDelay = xTicksToDelay;
    
    /* 任务切换 */
    taskYIELD();
}
```

6. 配置systick





初始化systick

``` c
/* SysTick 配置寄存器 */
#define portNVIC_SYSTICK_CTRL_REG			( * ( ( volatile uint32_t * ) 0xe000e010 ) )
#define portNVIC_SYSTICK_LOAD_REG			( * ( ( volatile uint32_t * ) 0xe000e014 ) )

// 配置时钟相关的位
#ifndef configSYSTICK_CLOCK_HZ
	#define configSYSTICK_CLOCK_HZ configCPU_CLOCK_HZ
	/* 确保SysTick的时钟与内核时钟一致 */
	#define portNVIC_SYSTICK_CLK_BIT	( 1UL << 2UL )
#else
	#define portNVIC_SYSTICK_CLK_BIT	( 0 )
#endif
#define portNVIC_SYSTICK_INT_BIT			( 1UL << 1UL )
#define portNVIC_SYSTICK_ENABLE_BIT			( 1UL << 0UL )

void vPortSetupTimerInterrupt( void )
{
     /* 设置重装载寄存器的值 */
    portNVIC_SYSTICK_LOAD_REG = ( configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ ) - 1UL;
    
    /* 设置系统定时器的时钟等于内核时钟
       使能SysTick 定时器中断
       使能SysTick 定时器 */
    portNVIC_SYSTICK_CTRL_REG = ( portNVIC_SYSTICK_CLK_BIT | 
                                  portNVIC_SYSTICK_INT_BIT |
                                  portNVIC_SYSTICK_ENABLE_BIT ); 
}
```


更新systick 和 更新就绪列表中所有线程的xTicksToDelay

``` c
void xTaskIncrementTick( void )
{
    TCB_t *pxTCB = NULL;
    BaseType_t i = 0;
    
    /* 更新系统时基计数器xTickCount，xTickCount是一个在port.c中定义的全局变量 */
    const TickType_t xConstTickCount = xTickCount + 1;
    xTickCount = xConstTickCount;

    
    /* 扫描就绪列表中所有线程的xTicksToDelay，如果不为0，则减1 */
	for(i=0; i<configMAX_PRIORITIES; i++)
	{
        pxTCB = ( TCB_t * ) listGET_OWNER_OF_HEAD_ENTRY( ( &pxReadyTasksLists[i] ) );
		if(pxTCB->xTicksToDelay > 0)
		{
			pxTCB->xTicksToDelay --;
		}
	}
    
    /* 任务切换 */
    portYIELD();
}
```

中断服务函数

``` c
void xPortSysTickHandler( void )
{
	/* 关中断 */
    vPortRaiseBASEPRI();
    
    /* 更新系统时基 */
    xTaskIncrementTick();

	/* 开中断 */
    vPortClearBASEPRIFromISR();
}
```

# 实现效果

![](https://raw.githubusercontent.com/fly-t/images/main/blog/readme-2024-08-09-11-37-12.png)

# 总结

阻塞延时就是不让cpu执行无意义的:++ --操作. 利用systick进行挂起任务, 让cpu执行不在延时列表中的任务, 都处于延时中时, 就执行默认的
空闲任务.

这个操作和软件定时器很像.