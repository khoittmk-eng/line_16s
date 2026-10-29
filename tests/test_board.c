/* Tests actual board register requests; no claim about physical SWD access. */
#include "board.h"
#include "board_init.h"
extern volatile unsigned test_count, failure_line;
void test_done(void);
static void Check(int ok,unsigned line)
{
    ++test_count;
    if(!ok) { failure_line=110000U+line; test_done(); }
}
#define CHECK(x) Check((x),__LINE__)
void RunBoardTests(void)
{
    unsigned i,j;
    uint32_t other=AFIO_MAPR_USART1_REMAP|AFIO_MAPR_TIM2_REMAP_PARTIALREMAP1;
    for(i=0;i<8;++i) for(j=0;j<2;++j) {
        AFIO->MAPR=other|(i<<24)|(j<<27);
        Board_RemapOLEDAndKeepSWD();
        CHECK(AFIO->MAPR==(other|AFIO_MAPR_SWJ_CFG_JTAGDISABLE|AFIO_MAPR_I2C1_REMAP));
        Board_RemapOLEDAndKeepSWD();
        CHECK(AFIO->MAPR==(other|AFIO_MAPR_SWJ_CFG_JTAGDISABLE|AFIO_MAPR_I2C1_REMAP));
    }
    AFIO->MAPR=0;
    Board_Init();
    CHECK(AFIO->MAPR==(AFIO_MAPR_SWJ_CFG_JTAGDISABLE|AFIO_MAPR_I2C1_REMAP));
    CHECK(!(GPIOA->ODR&(GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_10|GPIO_Pin_11)));
    CHECK(!(GPIOB->ODR&(GPIO_Pin_6|GPIO_Pin_7)));
}
