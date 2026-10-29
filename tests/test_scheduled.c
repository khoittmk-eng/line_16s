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
static void SignedCornerTests(void)
{
    ControlState s;
    ControlParameters p;
    unsigned i,dt,j;
    int sign;
    for(dt=1;dt<=4;++dt) for(sign=-1;sign<=1;sign+=2) {
        unsigned elapsed=0;
        Warm(&s,&p); s.last_error=sign*2.0f; s.derivative=s.planner_rate=0;
        for(i=0;i<60/dt+1;++i) {
            Control_Step(&s,&p,sign*2000,1,sign>0?0xF8:0x1F,dt);
            elapsed+=dt;
            if(elapsed<6) CHECK(s.left>=0 && s.right>=0 && !s.corner_gain);
            CHECK(s.mode==CONTROL_TRACK && s.target==45 && !s.brake_count);
            CHECK(s.left>=-200 && s.right>=-200 && s.left<=850 && s.right<=850);
        }
        CHECK(sign*(s.left-s.right)==960 && s.left*s.right<0);
        CHECK(s.corner_pending_dir==sign && !s.recovery_ms);
        /* New Max applies in the same frame, including the reverse ceiling. */
        p.max_percent=10;
        Control_Step(&s,&p,sign*2000,1,sign>0?0xF8:0x1F,dt);
        CHECK(s.left>=-100 && s.right>=-100 && s.left<=100 && s.right<=100);
        Control_Step(&s,&p,0,1,0x18,dt);
        CHECK(s.left>=0 && s.right>=0 && !s.corner_gain && s.mode==CONTROL_TRACK);
        Control_Stop(&s,FAULT_UI);
        CHECK(!s.left && !s.right && s.mode==CONTROL_FAULT);
    }
    for(sign=-1;sign<=1;sign+=2) {
        /* Confirmation and error alone cannot bypass cue direction or shape. */
        for(j=0;j<4;++j) {
            unsigned mask=j==0?0x3C:j==1?0x80:j==2?0x30:sign>0?0x1F:0xF8;
            Warm(&s,&p);
            for(i=0;i<20;++i) Control_Step(&s,&p,sign*2500,1,(uint8_t)mask,2);
            CHECK(s.left>=0 && s.right>=0 && !s.corner_gain);
        }
        Warm(&s,&p);
        for(i=0;i<20;++i) Control_Step(&s,&p,sign*2500,1,sign>0?0xF8:0x1F,2);
        CHECK(s.left*s.right<0);
        s.planner_rate=sign*30.0f;
        for(i=1;i<=3;++i) Control_Step(&s,&p,sign*(2500-i*20),1,sign>0?0xF8:0x1F,2);
        CHECK(s.corner_releasing && s.left>=0 && s.right>=0);
        for(i=0;i<8;++i) Control_Step(&s,&p,sign*2440,1,sign>0?0xF8:0x1F,2);
        CHECK(s.corner_releasing && !s.corner_gain && s.left>=0 && s.right>=0);
        for(i=1;i<=3;++i) Control_Step(&s,&p,sign*(2440+i*20),1,sign>0?0xF8:0x1F,2);
        CHECK(!s.corner_releasing);
        for(i=0;i<20;++i) Control_Step(&s,&p,sign*2500,1,sign>0?0xF8:0x1F,2);
        CHECK(s.left*s.right<0);
        Warm(&s,&p);
        for(i=0;i<20;++i) Control_Step(&s,&p,sign*2500,1,sign>0?0xF8:0x1F,2);
        p.kd=0.12f; s.derivative=-sign*1000.0f; s.planner_rate=sign*30.0f;
        Control_Step(&s,&p,sign*2500,1,sign>0?0xF8:0x1F,2);
        CHECK(s.left>=0 && s.right>=0); /* Opposing D cannot select signed drive. */
        p.kd=0;
        Control_Step(&s,&p,-sign*2500,1,sign>0?0x1F:0xF8,2);
        CHECK(!s.corner_gain && s.left>=0 && s.right>=0);
        for(i=0;i<20;++i) Control_Step(&s,&p,-sign*2500,1,sign>0?0x1F:0xF8,2);
        CHECK(s.left*s.right<0 && -sign*(s.left-s.right)>0);
        /* Line loss after signed drive enters existing pivot, not a fresh
         * forward arc captured from a reversing wheel. Rejoin remains PD. */
        Warm(&s,&p);
        for(i=0;i<20;++i) Control_Step(&s,&p,sign*2000,1,sign>0?0xF8:0x1F,2);
        Control_Step(&s,&p,0,0,0,2);
        CHECK(s.mode==CONTROL_TURN && !s.recovery_arc && s.turn_dir==sign);
        CHECK(s.left==sign*500 && s.right==-sign*500 && !s.corner_gain);
        Control_Step(&s,&p,sign*500,1,sign>0?0x10:0x08,2);
        CHECK(s.mode==CONTROL_ALIGN && s.left>=0 && s.right>=0);
        for(i=0;i<650;++i) Control_Step(&s,&p,0,0,0,2);
        CHECK(s.mode==CONTROL_FAULT && s.line_stop_reason==LINE_STOP_TURN_TIMEOUT);
    }
}
void RunScheduledTests(void)
{
    ControlParameters p;
    ControlState s;
    unsigned i,j,dt;
    int sign;
    SignedCornerTests();
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
        CHECK(sign*s.last_correction>20.87f && sign*s.last_correction<20.89f);
        CHECK(s.mode==CONTROL_TRACK && s.target==45 && s.left>0 && s.right>0 && !s.brake_count);
        for(i=0;i<10;++i) {
            float before=Abs(s.last_correction);
            Control_Step(&s,&p,sign*1500,1,sign>0?0xF8:0x1F,2);
            CHECK(Abs(s.last_correction)<=before+2.881f);
            CHECK(s.corner_gain<=0.501f && s.mode==CONTROL_TRACK);
        }
        CHECK(Abs(s.last_correction-sign*27.0f)<0.001f);
        CHECK(sign*(s.left-s.right)==540); /* B30: 72/18 versus B29's 68.4/21.6. */
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
        /* B34 allows the inner wheel to reverse on a larger confirmed cue. */
        for(i=0;i<12;++i) Control_Step(&s,&p,sign*2000,1,sign>0?0xF8:0x1F,2);
        CHECK(sign*(s.left-s.right)==960 && s.left*s.right<0);
        CHECK(s.mode==CONTROL_TRACK && s.target==45 && !s.brake_count);
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
            CHECK(s.left>=-200 && s.right>=-200 && s.left<=850 && s.right<=850);
            CHECK(Abs(s.integral)<0.001f); /* Saturated assist cannot wind up I. */
        }
        p.max_percent=10;
        Control_Step(&s,&p,sign*2000,1,sign>0?0xF8:0x1F,dt);
        CHECK(s.left>=-100 && s.right>=-100 && s.left<=100 && s.right<=100);
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
