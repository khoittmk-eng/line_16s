#include "control.h"
#include "control_tuning.h"
extern volatile unsigned test_count, failure_line;
void test_done(void);
static void Check(int ok,unsigned line)
{
    ++test_count;
    if(!ok) { failure_line=100000U+line; test_done(); }
}
#define CHECK(x) Check((x),__LINE__)
static float Abs(float x) { return x<0?-x:x; }
static void Warm(ControlState *s,ControlParameters *p)
{
    unsigned i;
    Control_Start(s);
    for(i=0;i<800;++i) Control_Step(s,p,0,1,0x18,2);
}
void RunCornerFlowTests(void)
{
    ControlState s;
    ControlParameters p;
    unsigned i,j;
    int sign;
    /* Independent of defaults: P must act on a visible soft bend immediately.
     * B25's extra slew only supplies 1.6 instead of 6 in the first 2 ms frame. */
    Control_Defaults(&p); p.base_percent=p.straight_percent=45; p.max_percent=85;
    p.kp=12; p.ki=p.kd=0;
    for(sign=-1;sign<=1;sign+=2) {
        Warm(&s,&p);
        Control_Step(&s,&p,sign*500,1,sign>0?0x10:0x08,2);
        CHECK(Abs(s.last_correction-sign*6.0f)<0.001f);
        CHECK(s.left==(sign>0?510:390) && s.right==(sign>0?390:510));
        CHECK(s.mode==CONTROL_TRACK && s.target==45 && !s.brake_count);
        Control_Step(&s,&p,-sign*500,1,sign>0?0x08:0x10,2);
        CHECK(Abs(s.last_correction+sign*6.0f)<0.001f);
    }
    Control_Defaults(&p);
    CHECK(p.base_percent==45 && p.straight_percent==45 && p.max_percent==85);
    /* Continuous soft bends remain forward PD, no pulse/pivot/straight boost. */
    for(sign=-1;sign<=1;sign+=2) {
        Warm(&s,&p);
        for(j=0;j<6;++j) for(i=0;i<120;++i) {
            int e=(int)(i<=60?i:120-i)*20*sign;
            Control_Step(&s,&p,(int16_t)e,1,sign>0?0x30:0x0C,2);
            CHECK(s.mode==CONTROL_TRACK && s.target==45 && !s.brake_count);
            CHECK(s.left>0 && s.right>0 && s.left<=850 && s.right<=850);
            CHECK(s.speed_bonus==0);
        }
        /* Known corner, loss, immediate valid reacquisition: forward correction
         * in the first frame, no new brake dwell and no reset down to 30%. */
        for(i=0;i<3;++i) Control_Step(&s,&p,sign*1500,1,sign>0?0xF8:0x1F,2);
        Control_Step(&s,&p,0,0,0,2);
        CHECK(s.mode==CONTROL_TURN && s.recovery_arc && s.left>0 && s.right>0 && sign*(s.left-s.right)>0);
        Control_Step(&s,&p,sign*1000,1,sign>0?0x30:0x0C,2);
        CHECK(s.mode==CONTROL_ALIGN && s.target==45 && s.speed>44 && s.speed<=45);
        CHECK(s.left>0 && s.right>0 && sign*(s.left-s.right)>0 && !s.brake_count);
        /* An opposite corner before alignment completes still changes direction. */
        Control_Step(&s,&p,-sign*2500,1,sign>0?0x02:0x40,2);
        Control_Step(&s,&p,0,0,0,2);
        CHECK(s.mode==CONTROL_TURN && s.turn_dir==-sign);
    }
    /* A short missing sample on a stable curve holds the trusted wheel pair. */
    Warm(&s,&p);
    for(i=0;i<150;++i) Control_Step(&s,&p,1200,1,0x30,2);
    {
        int16_t l=s.left,r=s.right;
        Control_Step(&s,&p,0,0,0x24,2);
        CHECK(s.left==l && s.right==r && s.mode==CONTROL_PROBE);
        Control_Step(&s,&p,1200,1,0x30,2);
        CHECK(s.mode==CONTROL_TRACK && s.left==l && s.right==r);
    }
    /* A low-speed search must not jump to 40 on reacquisition. */
    p.base_percent=p.straight_percent=p.max_percent=10;
    Warm(&s,&p);
    for(i=0;i<3;++i) Control_Step(&s,&p,1500,1,0xF8,2);
    Control_Step(&s,&p,0,0,0,2);
    Control_Step(&s,&p,0,1,0x18,2);
    CHECK(s.mode==CONTROL_ALIGN && s.target==10 && s.left==100 && s.right==100);
    Control_Stop(&s,FAULT_UI);
    Control_Step(&s,&p,0,1,0x18,2);
    CHECK(s.mode==CONTROL_FAULT && !s.left && !s.right);
}
