#include "control.h"
#include "control_tuning.h"
extern volatile unsigned test_count, failure_line;
void test_done(void);
static void Check(int ok,unsigned line)
{
    ++test_count;
    if(!ok) { failure_line=120000U+line; test_done(); }
}
#define CHECK(x) Check((x),__LINE__)
static float Abs(float x) { return x<0?-x:x; }
static void Warm(ControlState *s,ControlParameters *p)
{
    unsigned i;
    Control_Defaults(p); Control_Start(s);
    /* Already rolling; earn 120 ms of real centered guide history. Startup
     * ramp has independent coverage in the fixed-Base/controller suite. */
    s->speed=45;
    for(i=0;i<60;++i) Control_Step(s,p,0,1,0x18,2);
}
void RunArcTests(void)
{
    ControlState s;
    ControlParameters p;
    unsigned i,j,dt;
    int sign;
    /* Independent of new fields: a rolling confirmed corner must request a
     * forward arc on its first lost frame, with the same mean PWM as entry. */
    for(sign=-1;sign<=1;sign+=2) {
        Warm(&s,&p);
        for(i=0;i<3;++i) Control_Step(&s,&p,sign*1500,1,sign>0?0xF8:0x1F,2);
        {
            float before=(s.left+s.right)/20.0f;
            Control_Step(&s,&p,0,0,0,2);
            CHECK(s.mode==CONTROL_TURN && s.left>0 && s.right>0);
            CHECK(sign*(s.left-s.right)>0 && Abs(s.speed-before)<0.001f);
            CHECK(!s.brake_count && !s.brake_ms);
        }
        CHECK(s.recovery_arc);
        for(i=0;i<13;++i) {
            Control_Step(&s,&p,0,0,0,2);
            CHECK(s.recovery_arc && s.left>0 && s.right>0);
        }
        Control_Step(&s,&p,0,0,0,2);
        CHECK(!s.recovery_arc && s.recovery_ms==28);
        CHECK(s.left==sign*500 && s.right==-sign*500);
        /* No endless forward loss when the arc does not find the line. */
        for(i=0;i<610;++i) Control_Step(&s,&p,0,0,0,2);
        CHECK(s.mode==CONTROL_FAULT && !s.left && !s.right);
    }
    /* Every visible reacquisition cancels the maneuver, including its first
     * frame. Successful arcs keep entry duty rather than a 40% rejoin cap. */
    for(j=0;j<14;++j) for(sign=-1;sign<=1;sign+=2) {
        float entering;
        Warm(&s,&p);
        for(i=0;i<3;++i) Control_Step(&s,&p,sign*1500,1,sign>0?0xF8:0x1F,2);
        entering=(s.left+s.right)/20.0f;
        Control_Step(&s,&p,0,0,0,2);
        for(i=0;i<j;++i) Control_Step(&s,&p,0,0,0,2);
        Control_Step(&s,&p,0,1,0x18,2);
        CHECK(s.mode==CONTROL_ALIGN && s.left>0 && s.right>0);
        CHECK(s.target==45 && s.speed>=entering && s.speed<=45);
        for(i=0;i<5;++i) Control_Step(&s,&p,0,1,0x18,2);
        CHECK(s.mode==CONTROL_TRACK && s.post_turn_ms);
        Control_Step(&s,&p,0,1,0x18,2);
        CHECK(s.target==45 && s.left>0 && s.right>0);
        /* A post-turn retry does not buy another forward-arc allowance. */
        for(i=0;i<3;++i) Control_Step(&s,&p,0,0,0,2);
        CHECK(s.mode==CONTROL_TURN && !s.recovery_arc && s.turn_dir==sign);
    }
    /* Max changes and supported frame intervals never exceed output bounds. */
    for(dt=1;dt<=4;++dt) {
        unsigned elapsed=0;
        Warm(&s,&p); s.history=1; s.last_error=2.0f;
        s.guide_valid=s.curve_valid=0;
        s.left=s.right=850; s.speed=85; p.base_percent=p.max_percent=85;
        do { Control_Step(&s,&p,0,0,0,dt); elapsed+=dt; } while(elapsed<6);
        CHECK(s.mode==CONTROL_TURN && s.left>0 && s.right>0);
        CHECK(s.left<=850 && s.right<=850);
        p.max_percent=20;
        Control_Step(&s,&p,0,0,0,dt);
        CHECK(s.left>0 && s.left<=200 && s.right>0 && s.right<=200);
        for(i=0;i<35/dt+1;++i) Control_Step(&s,&p,0,0,0,dt);
        CHECK(s.left==200 && s.right==-200 && !s.recovery_arc);
    }
    /* A single central-eye steering kick is not frozen across a white dash. */
    for(sign=-1;sign<=1;sign+=2) {
        Warm(&s,&p);
        Control_Step(&s,&p,sign*500,1,sign>0?0x10:0x08,2);
        {
            int live=s.left-s.right, l,r;
            Control_Step(&s,&p,0,0,0,2);
            l=s.left; r=s.right;
            CHECK(Abs(l-r)<Abs(live)*0.2f && l>0 && r>0);
            for(i=0;i<99;++i) {
                Control_Step(&s,&p,0,0,0,2);
                CHECK(s.mode==CONTROL_GAP && s.left==l && s.right==r);
            }
            Control_Step(&s,&p,sign*500,1,sign>0?0x10:0x08,2);
            CHECK(s.mode==CONTROL_TRACK && s.derivative==0 && s.left>0 && s.right>0);
        }
        Control_Stop(&s,FAULT_ADC);
        Control_Step(&s,&p,0,1,0x18,2);
        CHECK(s.mode==CONTROL_FAULT && !s.left && !s.right && !s.recovery_arc);
    }
}
