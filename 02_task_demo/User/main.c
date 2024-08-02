#include "FreeRTOS.h"
#include "task.h"

/* 2个任务栈 */
TaskHandle_t Task1_Handle;
#define TASK1_STACK_SIZE 20
StackType_t Task1Stack[TASK1_STACK_SIZE];
TCB_t Task1TCB;

TaskHandle_t Task2_Handle;
#define TASK2_STACK_SIZE 20
StackType_t Task2Stack[TASK2_STACK_SIZE];
TCB_t Task2TCB;


portCHAR flag1;
portCHAR flag2;


void task2_entry(void *p_arg);
void task1_entry(void *p_arg);
void delay(uint32_t count);



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

void task2_entry(void *p_arg)
{
    while (1)
    {
        flag2 = 1;
			delay( 100 );		
			flag2 = 0;
			delay( 100 );

			/* 任务切换，这里是手动切换 */
			taskYIELD();
    }
}

/* 延时函数 */
void delay(uint32_t count)
{
    while (count--)
    {
        ;;
    }
}

int main(void)
{
 /* 硬件初始化 */
	/* 将硬件相关的初始化放在这里，如果是软件仿真则没有相关初始化代码 */
    
    /* 初始化与任务相关的列表，如就绪列表 */
    prvInitialiseTaskLists();
    
    /* 创建任务 */
    Task1_Handle = xTaskCreateStatic( (TaskFunction_t)task1_entry,   /* 任务入口 */
					                  (char *)"Task1",               /* 任务名称，字符串形式 */
					                  (uint32_t)TASK1_STACK_SIZE ,   /* 任务栈大小，单位为字 */
					                  (void *) NULL,                 /* 任务形参 */
					                  (StackType_t *)Task1Stack,     /* 任务栈起始地址 */
					                  (TCB_t *)&Task1TCB );          /* 任务控制块 */
    /* 将任务添加到就绪列表 */                                 
    vListInsertEnd( &( pxReadyTasksLists[1] ), &( ((TCB_t *)(&Task1TCB))->xStateListItem ) );
                                
    Task2_Handle = xTaskCreateStatic( (TaskFunction_t)task2_entry,   /* 任务入口 */
					                  (char *)"Task2",               /* 任务名称，字符串形式 */
					                  (uint32_t)TASK2_STACK_SIZE ,   /* 任务栈大小，单位为字 */
					                  (void *) NULL,                 /* 任务形参 */
					                  (StackType_t *)Task2Stack,     /* 任务栈起始地址 */
					                  (TCB_t *)&Task2TCB );          /* 任务控制块 */
    /* 将任务添加到就绪列表 */                                 
    vListInsertEnd( &( pxReadyTasksLists[2] ), &( ((TCB_t *)(&Task2TCB))->xStateListItem ) );
                                      
    /* 启动调度器，开始多任务调度，启动成功则不返回 */
    vTaskStartScheduler();  

    while (1)
    {
        /* code */
    }
    

}
