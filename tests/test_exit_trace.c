/* Synthetic command trace with gradual inward movement and a lagging rate. */
#include "control.h"
volatile unsigned test_count,failure_line,trace_count;
volatile int trace[64][8];
__attribute__((noinline)) void test_done(void) { while(1) {} }
void test_entry(void)
{
    static ControlState s;
    ControlParameters p;
    unsigned i;
    Control_Defaults(&p); p.ki=p.kd=0;
    Control_Start(&s); s.speed=45;
    for(i=0;i<64;++i) {
        int e=i<8?0:i<16?1200:i<32?1500:i<40?1500-((int)i-31)*20:i<56?-1500:0;
        unsigned mask=i<8?0x18:i<16?0x30:i<40?0xF8:i<56?0x1F:0x18;
        Control_Step(&s,&p,(int16_t)e,1,(uint8_t)mask,2);
        trace[i][0]=(i+1)*2; trace[i][1]=e;
        trace[i][2]=(int)(s.last_correction*1000.0f);
        trace[i][3]=s.left; trace[i][4]=s.right;
        trace[i][5]=(int)(s.target*1000.0f);
        trace[i][6]=s.mode; trace[i][7]=(int)(s.corner_gain*1000.0f);
    }
    trace_count=64; test_done();
}
