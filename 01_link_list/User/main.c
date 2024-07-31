#include "list.h"

/*
* 注意事项：1、该工程使用软件仿真，debug需选择 Ude Simulator
*           2、在Target选项卡里面把晶振Xtal(Mhz)的值改为25，默认是12，
*              改成25是为了跟system_ARMCM3.c中定义的__SYSTEM_CLOCK相同，确保仿真的时候时钟一致
*/

struct xLIST list_demo;

struct xLIST_ITEM list_item1;
struct xLIST_ITEM list_item2;
struct xLIST_ITEM list_item3;


int main(void)
{
    vListInitialise(&list_demo);

    vListInitialiseItem(&list_item1);
    vListInitialiseItem(&list_item2);
    vListInitialiseItem(&list_item3);
    
    vListInsert(&list_demo, &list_item1);
    vListInsert(&list_demo, &list_item2);
    vListInsert(&list_demo, &list_item3);

    while (1)
    {
        /* code */
    }
    

}
