# 多优先级的任务调度

程序启动后, 启动调度器.
调度器 通过pendSV进行任务之间的调度,
然后通过链表对优先级进行排序, 通过对优先级bitmap进行位操作来快速找到最高优先级TCB并指向该TCB即可完成任务的切换.


# 多优先级

实现优先级有两种方法:

1. 通用方法

通过将优先级进行`逐个`比较, 从高到低,一个一个排查.

2. 优化方法

铺垫知识
> 在许多实时操作系统（RTOS）中，优先级通常表示为一个位图（bit map），其中每一位对应一个优先级。如果你有32位的整数，那么你就可以有最多32个优先级，每个优先级对应一个位。

``` c
#define portGET_HIGHEST_PRIORITY(uxTopPriority, uxReadyPriorities) uxTopPriority = (31UL - (uint32_t)__clz((uxReadyPriorities)))
```

通过CortexM内核提供的`__clz`指令,可以更快的找到最高优先级的任务, 但是最多支持优先级只有32个优先级。

<font color='red'> 使用__clz()函数结合位操作的方法来查找最高优先级，通常只适用于优先级数量不超过32的情况。这是因为__clz()是对32位整数进行操作的，所以它`最多只能处理32个`不同的优先级。 </font>





## 通用方法

``` c
/*
*************************************************************************
*                               宏定义
*************************************************************************
*/

/* 将任务添加到就绪列表 */                                    
#define prvAddTaskToReadyList( pxTCB )																   \
	taskRECORD_READY_PRIORITY( ( pxTCB )->uxPriority );												   \
	vListInsertEnd( &( pxReadyTasksLists[ ( pxTCB )->uxPriority ] ), &( ( pxTCB )->xStateListItem ) ); \


/* 查找最高优先级的就绪任务：通用方法 */                                    
#if ( configUSE_PORT_OPTIMISED_TASK_SELECTION == 0 )
	/* uxTopReadyPriority 存的是就绪任务的最高优先级 */
	#define taskRECORD_READY_PRIORITY( uxPriority )														\
	{																									\
		if( ( uxPriority ) > uxTopReadyPriority )														\
		{																								\
			uxTopReadyPriority = ( uxPriority );														\
		}																								\
	} /* taskRECORD_READY_PRIORITY */

	/*-----------------------------------------------------------*/

	#define taskSELECT_HIGHEST_PRIORITY_TASK()															\
	{																									\
	UBaseType_t uxTopPriority = uxTopReadyPriority;														\
																										\
		/* 寻找包含就绪任务的最高优先级的队列 */                                                          \
		while( listLIST_IS_EMPTY( &( pxReadyTasksLists[ uxTopPriority ] ) ) )							\
		{																								\
			--uxTopPriority;																			\
		}																								\
																										\
		/* 获取优先级最高的就绪任务的TCB，然后更新到pxCurrentTCB */							            \
		listGET_OWNER_OF_NEXT_ENTRY( pxCurrentTCB, &( pxReadyTasksLists[ uxTopPriority ] ) );			\
		/* 更新uxTopReadyPriority */                                                                    \
		uxTopReadyPriority = uxTopPriority;																\
	} /* taskSELECT_HIGHEST_PRIORITY_TASK */

	/*-----------------------------------------------------------*/

                                 
```


## 宏定义解析

`taskRECORD_READY_PRIORITY `描述: 

1. 检查优先级: 这个宏比较uxPriority（传入的任务优先级）和uxTopReadyPriority（当前记录的最高优先级）。
2. 更新最高优先级: 如果uxPriority大于uxTopReadyPriority，则更新uxTopReadyPriority为uxPriority。

在任务调度中，每当有新任务变为就绪状态时，这个宏会被调用，以确保uxTopReadyPriority总是保持当前系统中最高的优先级。这对于任务调度器的高效运行至关重要，因为它能够帮助调度器快速找到最高优先级的就绪任务。

``` c
// uxPriority: 这是传入的任务优先级。
#define taskRECORD_READY_PRIORITY(uxPriority)  \
    {                                          \
        // uxTopReadyPriority: 这是一个全局变量或静态变量(初始值为空闲任务的优先级)，记录当前系统中最高的就绪任务优先级。
        if ((uxPriority) > uxTopReadyPriority) \
        {                                      \
            uxTopReadyPriority = (uxPriority); \
        }                                      \
    } /* taskRECORD_READY_PRIORITY */
```

`taskSELECT_HIGHEST_PRIORITY_TASK`描述: 

1. 初始化变量: 将当前的最高优先级赋值给uxTopPriority，用来跟踪在就绪任务列表中找到的实际最高优先级。
2. 寻找最高优先级的队列: 遍历就绪任务列表，寻找包含就绪任务的最高优先级队列。如果当前优先级的任务队列为空，则将优先级降级（--uxTopPriority）并继续检查。
3. 更新当前任务控制块: 从找到的最高优先级队列中获取任务控制块（TCB），并将其更新到pxCurrentTCB，以便接下来调度器运行任务。


``` c
#define taskSELECT_HIGHEST_PRIORITY_TASK()                                              \
    {                                                                                   \
        UBaseType_t uxTopPriority = uxTopReadyPriority;                                 \
                                                                                        \
        /* 寻找包含就绪任务的最高优先级的队列,如果为空就降低优先级 */                       \
        while (listLIST_IS_EMPTY(&(pxReadyTasksLists[uxTopPriority])))                  \
        {                                                                               \
            --uxTopPriority;                                                            \
        }                                                                               \
                                                                                        \
        /* 获取优先级最高的就绪任务的TCB，然后更新到pxCurrentTCB */  \
        listGET_OWNER_OF_NEXT_ENTRY(pxCurrentTCB, &(pxReadyTasksLists[uxTopPriority])); \
        /* 更新uxTopReadyPriority, 这里为啥要将值写回到uxTopReadyPriority是因为,如果当前列表为空,--uxTopPriority;之后需要修改uxTopReadyPriority的值*/             
                                             \
        uxTopReadyPriority = uxTopPriority;                                             \
    } /* taskSELECT_HIGHEST_PRIORITY_TASK */
```


# 优化方法

计算前导0指令:  计算一个变量从高位开始到第一次出现1前面的0的个数, 利用该方法寻找到最高优先级的就绪任务tcb


``` c
// 全局变量, 用于跟踪系统中最高优先级的就绪任务。
static volatile UBaseType_t uxTopReadyPriority = tskIDLE_PRIORITY;

#define portGET_HIGHEST_PRIORITY(uxTopPriority, uxReadyPriorities) uxTopPriority = (31UL - (uint32_t)__clz((uxReadyPriorities)))


/* 这两个宏定义只有在选择优化方法时才用，这里定义为空 */
#define taskRESET_READY_PRIORITY( uxPriority )
#define portRESET_READY_PRIORITY( uxPriority, uxTopReadyPriority ) 

#define taskRECORD_READY_PRIORITY(uxPriority) portRECORD_READY_PRIORITY(uxPriority, uxTopReadyPriority)


#define taskSELECT_HIGHEST_PRIORITY_TASK()                                              \
    {                                                                                   \
        UBaseType_t uxTopPriority;                                                      \
                                                                                        \
        /* 寻找最高优先级 */                                                     \
        portGET_HIGHEST_PRIORITY(uxTopPriority, uxTopReadyPriority);                    \
        /* 获取优先级最高的就绪任务的TCB，然后更新到pxCurrentTCB */  \
        listGET_OWNER_OF_NEXT_ENTRY(pxCurrentTCB, &(pxReadyTasksLists[uxTopPriority])); \
    } /* taskSELECT_HIGHEST_PRIORITY_TASK() */


#define taskRESET_READY_PRIORITY(uxPriority)                          \
    {                                                                 \
        portRESET_READY_PRIORITY((uxPriority), (uxTopReadyPriority)); \
    }
```


# 修改变量

`TCB_t `

``` c
typedef struct tskTaskControlBlock
{
    volatile StackType_t *pxTopOfStack; /* 栈顶 */
    ListItem_t xStateListItem; 
    StackType_t *pxStack; 
    char pcTaskName[configMAX_TASK_NAME_LEN];
	TickType_t xTicksToDelay; 
    UBaseType_t uxPriority; /* +++++++++++++++ 任务优先级 */
} tskTCB;
```

`prvInitialiseNewTask`

``` c
static void prvInitialiseNewTask(TaskFunction_t pxTaskCode,         /* 任务入口 */
                                 const char *const pcName,          /* 任务名称，字符串形式 */
                                 const uint32_t ulStackDepth,       /* 任务栈大小，单位为字 */
                                 void *const pvParameters,          /* 任务形参 */
                                 UBaseType_t uxPriority,            /* ++++++++++++++任务优先级，数值越大，优先级越高 */
                                 TaskHandle_t *const pxCreatedTask, /* 任务句柄 */
                                 TCB_t *pxNewTCB)                   /* 任务控制块 */
{
    // ...

    /* 初始化优先级 */
    if (uxPriority >= (UBaseType_t)configMAX_PRIORITIES)
    {
        uxPriority = (UBaseType_t)configMAX_PRIORITIES - (UBaseType_t)1U;
    }
    pxNewTCB->uxPriority = uxPriority;
    
    // ...
}
```

`prvAddNewTaskToReadyList`

``` c
#define taskENTER_CRITICAL()		       portENTER_CRITICAL()
#define taskENTER_CRITICAL_FROM_ISR()      portSET_INTERRUPT_MASK_FROM_ISR()

#define taskEXIT_CRITICAL()			       portEXIT_CRITICAL()
#define taskEXIT_CRITICAL_FROM_ISR( x )    portCLEAR_INTERRUPT_MASK_FROM_ISR( x )


static void prvAddNewTaskToReadyList( TCB_t *pxNewTCB )
{
	/* 进入临界段 */
	taskENTER_CRITICAL();
	{
		/* 全局任务计时器加一操作 */
        uxCurrentNumberOfTasks++;
        
        /* 如果pxCurrentTCB为空，则将pxCurrentTCB指向新创建的任务 */
		if( pxCurrentTCB == NULL )
		{
			pxCurrentTCB = pxNewTCB;

			/* 如果是第一次创建任务，则需要初始化任务相关的列表 */
            if( uxCurrentNumberOfTasks == ( UBaseType_t ) 1 )
			{
				/* 初始化任务相关的列表 */
                prvInitialiseTaskLists();
			}
		}
		else /* 如果pxCurrentTCB不为空，则根据任务的优先级将pxCurrentTCB指向最高优先级任务的TCB */
		{
				if( pxCurrentTCB->uxPriority <= pxNewTCB->uxPriority )
				{
					pxCurrentTCB = pxNewTCB;
				}
		}
		uxTaskNumber++;
        
		/* 将任务添加到就绪列表 */
        prvAddTaskToReadyList( pxNewTCB );

	}
	/* 退出临界段 */
	taskEXIT_CRITICAL();
}
```

`xTaskCreateStatic`

``` c
TaskHandle_t xTaskCreateStatic(	TaskFunction_t pxTaskCode,           /* 任务入口 */
					            const char * const pcName,           /* 任务名称，字符串形式 */
					            const uint32_t ulStackDepth,         /* 任务栈大小，单位为字 */
					            void * const pvParameters,           /* 任务形参 */
                                UBaseType_t uxPriority,              /* +++++++++++++任务优先级，数值越大，优先级越高 */
					            StackType_t * const puxStackBuffer,  /* 任务栈起始地址 */
					            TCB_t * const pxTaskBuffer )         /* 任务控制块 */
{
	TCB_t *pxNewTCB;
	TaskHandle_t xReturn;

	if( ( pxTaskBuffer != NULL ) && ( puxStackBuffer != NULL ) )
	{		
		pxNewTCB = ( TCB_t * ) pxTaskBuffer; 
		pxNewTCB->pxStack = ( StackType_t * ) puxStackBuffer;

		/* 创建新的任务 */
		prvInitialiseNewTask( pxTaskCode, pcName, 
                            ulStackDepth, pvParameters,
                            uxPriority, // ++++++++++++ 优先级
                            &xReturn, pxNewTCB);
        
		/* 将任务添加到就绪列表 */
		prvAddNewTaskToReadyList( pxNewTCB );

	}
	else
	{
		xReturn = NULL;
	}

	return xReturn;
}
```

全局变量: 当前任务个数

``` c
static volatile UBaseType_t uxCurrentNumberOfTasks 	= ( UBaseType_t ) 0U;
static UBaseType_t uxTaskNumber 					= ( UBaseType_t ) 0U;
```


`vTaskStartScheduler`

``` c
void vTaskStartScheduler(void)
{
// ...

    xIdleTaskHandle = xTaskCreateStatic((TaskFunction_t)prvIdleTask,   /* 任务入口 */
                                        (char *)"IDLE",                /* 任务名称，字符串形式 */
                                        (uint32_t)ulIdleTaskStackSize, /* 任务栈大小，单位为字 */
                                        (void *)NULL,                  /* 任务形参 */
                                        (UBaseType_t)tskIDLE_PRIORITY, /* ++++++++++任务优先级，数值越大，优先级越高 */
                                        (StackType_t *)pxIdleTaskStackBuffer, /* 任务栈起始地址 */
                                        (TCB_t *)pxIdleTaskTCBBuffer);        /* 任务控制块 */
    {
    // vListInsertEnd(&(pxReadyTasksLists[0]), &(((TCB_t *)pxIdleTaskTCBBuffer)->xStateListItem));
    // ...
    }
    // ...
}
```

`vTaskDelay`

``` c
void vTaskDelay(const TickType_t xTicksToDelay)
{
    TCB_t *pxTCB = NULL;

    /* 获取当前任务的TCB */
    pxTCB = pxCurrentTCB;

    /* 设置延时时间 */
    pxTCB->xTicksToDelay = xTicksToDelay;
    
    /* 将任务从就绪列表移除 */
    // uxListRemove( &( pxTCB->xStateListItem ) );
    taskRESET_READY_PRIORITY(pxTCB->uxPriority);
    
    /* 任务切换 */
    taskYIELD();
}
```


`vTaskSwitchContext`

``` c
/* 任务切换，即寻找优先级最高的就绪任务 */
void vTaskSwitchContext( void )
{
	/* 获取优先级最高的就绪任务的TCB，然后更新到pxCurrentTCB */
    taskSELECT_HIGHEST_PRIORITY_TASK();
}
```

# 结果

![](https://raw.githubusercontent.com/fly-t/images/main/blog/readme-2024-08-09-16-56-23.png)


