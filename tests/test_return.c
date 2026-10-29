#include "control.h"
#include "control_tuning.h"
extern volatile unsigned test_count,failure_line;
void test_done(void);
static void Check(int ok,unsigned line)
{
    ++test_count;
    if(!ok) { failure_line=140000U+line; test_done(); }
}
#define CHECK(x) Check((x),__LINE__)
static void Seed(ControlState *s,ControlParameters *p,int sign)
{
    Control_Defaults(p); p->kd=0;
    Control_Start(s); s->history=1; s->speed=45; s->left=s->right=450;
    s->last_error=sign*2.0f; s->derivative=s->planner_rate=sign*30.0f;
    s->shape_ms=6; s->shape_dir=(int8_t)sign;
    s->corner_pending_ms=200; s->corner_pending_dir=(int8_t)sign;
    s->corner_gain=0.6f;
}
void RunCornerReturnTests(void)
{
    ControlState s;
    ControlParameters p;
    unsigned i,dt;
    int sign;
    for(sign=-1;sign<=1;sign+=2) {
        Seed(&s,&p,sign);
        for(i=1;i<=2;++i) {
            Control_Step(&s,&p,sign*(2000-i*20),1,sign>0?0xF8:0x1F,2);
            CHECK(s.corner_gain>0.5f && s.mode==CONTROL_TRACK);
        }
        Control_Step(&s,&p,sign*1940,1,sign>0?0xF8:0x1F,2);
        /* B28 still trusts its lagging outward planner here and retains .564.
         * Confirmed fresh inward movement must begin fading extra P sooner. */
        CHECK(s.corner_gain<0.5f && s.corner_gain>0 && sign*s.planner_rate>0);
        CHECK(s.corner_releasing && s.corner_inward_ms>=6);
        CHECK(s.mode==CONTROL_TRACK && s.target==45 && s.left>0 && s.right>0);
        for(i=4;i<=8;++i) Control_Step(&s,&p,sign*(2000-i*20),1,sign>0?0xF8:0x1F,2);
        CHECK(s.corner_gain==0 && s.corner_releasing);
        /* A plateau after returning must not rebuild assist against old rate. */
        for(i=0;i<5;++i) Control_Step(&s,&p,sign*1840,1,sign>0?0xF8:0x1F,2);
        CHECK(s.corner_gain==0 && sign*s.planner_rate>0);
        Control_Step(&s,&p,sign*1870,1,sign>0?0xF8:0x1F,2);
        CHECK(s.corner_releasing && s.corner_gain==0);
        Control_Step(&s,&p,sign*1870,1,sign>0?0xF8:0x1F,2);
        CHECK(s.corner_releasing && s.corner_outward_ms==0);
        for(i=1;i<=3;++i) Control_Step(&s,&p,sign*(1870+i*20),1,sign>0?0xF8:0x1F,2);
        CHECK(!s.corner_releasing && s.corner_gain>0);
        CHECK(s.corner_pending_dir==sign && s.target==45 && !s.brake_count);
        /* Sign/cue changes clear the trend and must re-confirm the next bend. */
        Control_Step(&s,&p,-sign*1500,1,sign>0?0x1F:0xF8,2);
        CHECK(!s.corner_gain && !s.corner_releasing && !s.corner_inward_ms);
        for(i=0;i<4;++i) Control_Step(&s,&p,-sign*1500,1,sign>0?0x1F:0xF8,2);
        CHECK(s.corner_gain>0 && s.corner_pending_dir==-sign);
    }
    /* Alternating noise cannot confirm six milliseconds of returning. */
    for(sign=-1;sign<=1;sign+=2) {
        Seed(&s,&p,sign);
        for(i=0;i<14;++i) {
            Control_Step(&s,&p,sign*((i&1U)?2000:1980),1,sign>0?0xF8:0x1F,2);
            CHECK(!s.corner_releasing && s.corner_inward_ms<=2);
        }
        Control_Step(&s,&p,0,0,0xFF,2);
        CHECK(!s.corner_gain && !s.corner_releasing && !s.corner_inward_ms && !s.corner_outward_ms);
        CHECK(s.mode==CONTROL_TURN && s.turn_dir==sign);
    }
    /* The confirmation is elapsed time, not a hardcoded count of 3 frames. */
    for(dt=1;dt<=4;++dt) for(sign=-1;sign<=1;sign+=2) {
        unsigned elapsed=0,step=0;
        Seed(&s,&p,sign);
        do {
            ++step;
            Control_Step(&s,&p,sign*(2000-step*20),1,sign>0?0xF8:0x1F,dt);
            elapsed+=dt;
            if(elapsed<6) CHECK(!s.corner_releasing);
            CHECK(s.left>=0 && s.right>=0 && s.left<=850 && s.right<=850);
        } while(elapsed<6);
        CHECK(s.corner_releasing && s.corner_inward_ms>=6);
        Control_Stop(&s,FAULT_UI);
        Control_Step(&s,&p,0,1,0x18,dt);
        CHECK(s.mode==CONTROL_FAULT && !s.left && !s.right);
        CHECK(!s.corner_gain && !s.corner_releasing && !s.corner_inward_ms);
    }
}
