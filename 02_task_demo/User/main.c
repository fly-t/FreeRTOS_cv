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

void task2_entry(void *p_arg);
void task1_entry(void *p_arg);
void delay(uint32_t count);



/* 任务函数 */
void task1_entry(void *p_arg)
{
    while (1)
    {
        /* code */
    }
    
}

void task2_entry(void *p_arg)
{
    while (1)
    {
        /* code */
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


    while (1)
    {
        /* code */
    }
    

}
