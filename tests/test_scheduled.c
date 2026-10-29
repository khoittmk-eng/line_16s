#include "control.h"
#include "control_tuning.h"
extern volatile unsigned test_count,failure_line;
void test_done(void);
static void Check(int ok,unsigned line)
{
    ++test_count;
    if(!ok) { failure_line=130000U+line; test_done(); }
}
#define CHECK(x) Check((x),__LINE__)
static float Abs(float x) { return x<0?-x:x; }
static void Warm(ControlState *s,ControlParameters *p)
{
    unsigned i;
    Control_Defaults(p); Control_Start(s); s->speed=45;
    for(i=0;i<60;++i) Control_Step(s,p,0,1,0x18,2);
    p->kd=0; /* Isolate the scheduled P from existing derivative filtering. */
}
void RunScheduledTests(void)
{
    ControlParameters p;
    ControlState s;
    unsigned i,j,dt;
    int sign;
    for(sign=-1;sign<=1;sign+=2) {
        Warm(&s,&p);
        s.last_error=sign*1.5f; s.derivative=s.planner_rate=0;
        for(i=0;i<2;++i) {
            Control_Step(&s,&p,sign*1500,1,sign>0?0xF8:0x1F,2);
            CHECK(Abs(s.last_correction-sign*18.0f)<0.001f);
        }
        Control_Step(&s,&p,sign*1500,1,sign>0?0xF8:0x1F,2);
        /* A visible confirmed corner must get extra steering before any loss.
         * This fails on B27 even though both versions already have loss arcs. */
        CHECK(sign*s.last_correction>19.79f && sign*s.last_correction<19.81f);
        CHECK(s.mode==CONTROL_TRACK && s.target==45 && s.left>0 && s.right>0 && !s.brake_count);
        for(i=0;i<10;++i) {
            float before=Abs(s.last_correction);
            Control_Step(&s,&p,sign*1500,1,sign>0?0xF8:0x1F,2);
            CHECK(Abs(s.last_correction)<=before+1.801f);
            CHECK(s.corner_gain<=0.301f && s.mode==CONTROL_TRACK);
        }
        CHECK(Abs(s.last_correction-sign*23.4f)<0.001f);
        /* Improving error immediately releases assistance, leaving live PD. */
        Control_Step(&s,&p,sign*1400,1,sign>0?0xF8:0x1F,2);
        CHECK(s.corner_gain==0 && Abs(s.last_correction-sign*16.8f)<0.001f);
        /* Opposite corner must re-confirm, not inherit the old gain. */
        Control_Step(&s,&p,-sign*1500,1,sign>0?0x1F:0xF8,2);
        CHECK(!s.corner_gain && sign*s.last_correction<0);
        for(i=0;i<8;++i) Control_Step(&s,&p,-sign*1500,1,sign>0?0x1F:0xF8,2);
        CHECK(-sign*s.last_correction>18 && s.corner_pending_dir==-sign);
        Control_Step(&s,&p,0,1,0x18,2);
        CHECK(s.corner_gain==0 && s.mode==CONTROL_TRACK);
    }
    /* Ordinary soft curves, centered broad groups and an isolated outer eye
     * never select the scheduled corner law. No false geometry-based pivot. */
    for(j=0;j<3;++j) {
        Warm(&s,&p);
        for(i=0;i<20;++i) Control_Step(&s,&p,j==2?3500:1500,1,j==0?0x30:j==1?0x3C:0x80,2);
        CHECK(!s.corner_gain && s.mode==CONTROL_TRACK && s.target==45);
    }
    for(dt=1;dt<=4;++dt) for(sign=-1;sign<=1;sign+=2) {
        Warm(&s,&p); p.ki=2; p.kp=100;
        for(i=0;i<100;++i) {
            Control_Step(&s,&p,sign*2000,1,sign>0?0xF8:0x1F,dt);
            CHECK(s.left>=0 && s.right>=0 && s.left<=850 && s.right<=850);
            CHECK(Abs(s.integral)<0.001f); /* Saturated assist cannot wind up I. */
        }
        p.max_percent=10;
        Control_Step(&s,&p,sign*2000,1,sign>0?0xF8:0x1F,dt);
        CHECK(s.left>=0 && s.right>=0 && s.left<=100 && s.right<=100);
        Control_Stop(&s,FAULT_UI);
        Control_Step(&s,&p,0,1,0x18,dt);
        CHECK(s.mode==CONTROL_FAULT && !s.left && !s.right && !s.corner_gain);
    }
    /* Faster white search, cautious when any black signal appears, immediate
     * forward PD on valid line. Neither branch renews the search deadline. */
    for(sign=-1;sign<=1;sign+=2) {
        Warm(&s,&p);
        Control_Step(&s,&p,sign*2500,1,sign>0?0x40:0x02,2);
        for(i=0;i<20;++i) Control_Step(&s,&p,0,0,0,2);
        CHECK(s.mode==CONTROL_TURN && !s.recovery_arc && !s.corner_gain);
        CHECK(s.left==sign*500 && s.right==-sign*500 && s.target==50);
        {
            unsigned before=s.recovery_ms;
            Control_Step(&s,&p,0,0,0xFF,2);
            CHECK(s.left==sign*360 && s.right==-sign*360 && s.target==36);
            CHECK(s.recovery_ms==before+2);
        }
        Control_Step(&s,&p,sign*500,1,sign>0?0x10:0x08,2);
        CHECK(s.mode==CONTROL_ALIGN && s.left>0 && s.right>0 && !s.corner_gain);
        CHECK(sign*(s.left-s.right)>0);
        for(i=0;i<650;++i) Control_Step(&s,&p,0,0,0,2);
        CHECK(s.mode==CONTROL_FAULT && s.line_stop_reason==LINE_STOP_TURN_TIMEOUT);
    }
}
