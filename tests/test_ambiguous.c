#include "control.h"
#include "line_estimator.h"
#include "control_tuning.h"
#include "fixtures/b22-corner-seq2.h"
extern volatile unsigned test_count, failure_line;
volatile unsigned b23_replay_min_x10, b23_replay_rejoin_x10, b23_replay_end_x10;
void test_done(void);
static void Check(int ok,unsigned line)
{
    ++test_count;
    if(!ok) { failure_line=70000U+line; test_done(); }
}
#define CHECK(x) Check((x),__LINE__)
static uint8_t Mirror(uint8_t mask)
{
    unsigned i; uint8_t result=0;
    for(i=0;i<8;++i) if(mask&(1U<<i)) result|=(uint8_t)(1U<<(7U-i));
    return result;
}
static float Abs(float x) { return x<0?-x:x; }
static void Warm(ControlState *s,ControlParameters *p,unsigned duty,int sign)
{
    unsigned i;
    Control_Defaults(p); p->base_percent=p->straight_percent=duty; p->max_percent=90;
    Control_Start(s);
    /* Seed an already moving command, then earn 48 ms of real guide history.
     * Startup acceleration is covered separately by the captured raw replay. */
    s->speed=(float)duty;
    for(i=0;i<24;++i) Control_Step(s,p,sign*500,1,sign>0?0x10:0x08,2);
}
static void Bounds(void)
{
    ControlParameters p;
    ControlState s;
    unsigned dt,i,j;
    int sign;
    for(sign=-1;sign<=1;sign+=2) for(dt=1;dt<=4;++dt) {
        int16_t left,right;
        float previous,ratio;
        Warm(&s,&p,35,sign); ratio=(float)s.left/s.right; previous=s.speed;
        for(i=dt;i<=32;i+=dt) {
            Control_Step(&s,&p,sign*30000,0,i%2?0x98:0xFF,dt); /* Invalid centroid must be ignored. */
            CHECK(s.mode==CONTROL_PROBE && s.probe_continue && s.target>=31.9f);
            CHECK(s.speed<=previous && s.left>0 && s.right>0);
            CHECK(Abs((float)s.left/s.right-ratio)<0.01f);
            previous=s.speed;
        }
        Control_Step(&s,&p,0,0,0xFF,dt);
        CHECK(s.mode==CONTROL_PROBE && !s.probe_continue && s.target==15);
        while(s.mode!=CONTROL_FAULT) Control_Step(&s,&p,0,0,0xFF,dt);
        CHECK(s.line_stop_reason==LINE_STOP_AMBIGUOUS && !s.left && !s.right);
        /* Real white bypasses this allowance and preserves the old loss rules. */
        Warm(&s,&p,35,sign);
        for(i=0;i<3;++i) Control_Step(&s,&p,0,0,0xFF,2);
        Control_Step(&s,&p,0,0,0,2);
        CHECK(!s.probe_continue && (s.mode==CONTROL_TURN || s.mode==CONTROL_BRAKE));
        /* Confirmed asymmetric split preempts the continuation at 6 ms. */
        Warm(&s,&p,35,sign);
        Control_Step(&s,&p,0,0,0xFF,2);
        for(i=0;i<3;++i) Control_Step(&s,&p,0,0,sign>0?0xE8:0x17,2);
        CHECK(!s.probe_continue && (s.mode==CONTROL_TURN || s.mode==CONTROL_BRAKE) && s.turn_dir==sign);
        /* Pending/post-turn chain protection always has priority. */
        Warm(&s,&p,35,sign);
        for(i=0;i<3;++i) Control_Step(&s,&p,sign*1500,1,sign>0?0xF8:0x1F,2);
        for(i=0;i<3;++i) Control_Step(&s,&p,0,0,0xFF,2);
        CHECK(!s.probe_continue && (s.mode==CONTROL_TURN || s.mode==CONTROL_BRAKE) && s.turn_dir==sign);
        /* A short ambiguous blip that returns to line never restarts speed. */
        Warm(&s,&p,35,sign);
        for(i=0;i<15;++i) Control_Step(&s,&p,0,0,0xFF,2);
        left=s.left; right=s.right;
        Control_Step(&s,&p,0,1,0x18,2);
        CHECK(s.mode==CONTROL_TRACK && s.speed>=31.9f && !s.probe_continue);
        CHECK(s.left>0 && s.right>0 && left>0 && right>0);
    }
    /* High straight duty decelerates immediately; no blind full-speed bridge. */
    Warm(&s,&p,80,1);
    for(i=0;i<16;++i) {
        float previous=s.speed;
        Control_Step(&s,&p,0,0,0xFF,2);
        CHECK(s.speed<previous && s.target==32 && s.left<=900 && s.right<=900);
    }
    CHECK(s.speed<61 && s.speed>60);
    /* Low-speed startup does not accelerate through invalid data. */
    Warm(&s,&p,10,1);
    for(i=0;i<16;++i) {
        float previous=s.speed;
        int16_t left=s.left,right=s.right;
        Control_Step(&s,&p,0,0,0xFF,2);
        CHECK(s.speed<=previous && s.left<=left && s.right<=right);
    }
    /* No trusted central history, stale guide, high rate or a corner guard -> old PROBE. */
    for(j=0;j<6;++j) {
        Warm(&s,&p,35,1);
        if(j==0) s.stable_ms=38;
        if(j==1) s.guide_age_ms=11;
        if(j==2) s.planner_rate=5;
        if(j==3) s.speed_guard_ms=100;
        if(j==4) s.last_error=1.0f;
        if(j==5) { s.post_turn_ms=100; s.post_turn_dir=1; }
        Control_Step(&s,&p,0,0,0xFF,2);
        CHECK(!s.probe_continue && s.target==15);
    }
    /* Once the central anchor disappears, it cannot restart within the same loss. */
    Warm(&s,&p,35,1);
    Control_Step(&s,&p,0,0,0xFF,2);
    Control_Step(&s,&p,0,0,0x24,2);
    CHECK(!s.probe_continue);
    Control_Step(&s,&p,0,0,0xFF,2);
    CHECK(!s.probe_continue && s.target==15);
    /* Visible flicker must not rearm another 32 ms allowance. */
    Warm(&s,&p,35,1);
    for(j=0;j<5;++j) {
        Control_Step(&s,&p,0,0,0xFF,2);
        for(i=0;i<3;++i) Control_Step(&s,&p,0,1,0x18,2);
    }
    Control_Step(&s,&p,0,0,0xFF,2);
    CHECK(!s.probe_continue && s.target==15);
    Control_Stop(&s,FAULT_ADC); CHECK(!s.probe_continue && !s.left && !s.right);
}
static void RawReplay(void)
{
    LineEstimator line;
    ControlParameters p;
    ControlState s;
    unsigned i,j,sign;
    b23_replay_min_x10=1000;
    for(sign=0;sign<2;++sign) {
        Line_Init(&line); Control_Defaults(&p); Control_Start(&s);
        for(j=0;j<8;++j) { line.low[j]=captured_low[j]; line.high[j]=captured_high[j]; line.white[j]=captured_white[j]; }
        line.calibrated=line.white_reference_valid=1; line.raw_high_mask=0;
        /* The capture starts 109 ms after Start. Earlier samples were not
         * recorded: use the first known raw frame for this approximate warmup. */
        for(i=0;i<54;++i) {
            Line_Update(&line,captured_frames[0].raw);
            Control_Step(&s,&p,sign?-line.error:line.error,line.valid,sign?Mirror(line.mask):line.mask,2);
        }
        for(i=0;i<sizeof(captured_frames)/sizeof(captured_frames[0]);++i) {
            const CapturedFrame *r=&captured_frames[i];
            /* Missing intermediate 2 ms raw frames use last-known data. This
             * checks software response to a recorded trace, not vehicle motion. */
            if(r->dt==4 && i) {
                Line_Update(&line,captured_frames[i-1].raw);
                Control_Step(&s,&p,sign?-line.error:line.error,line.valid,sign?Mirror(line.mask):line.mask,2);
            }
            Line_Update(&line,r->raw);
            CHECK(line.mask==r->mask && line.valid==r->valid);
            if(line.valid) CHECK(line.error==r->error);
            Control_Step(&s,&p,sign?-line.error:line.error,line.valid,sign?Mirror(line.mask):line.mask,2);
            CHECK(s.mode!=CONTROL_FAULT && s.mode!=CONTROL_BRAKE && s.mode!=CONTROL_TURN);
            CHECK(s.left>0 && s.right>0 && s.left<=850 && s.right<=850);
            /* Raw rows 80..87 are the hardware ambiguity. Old B22 collapsed
             * target to 15%; preserve a bounded forward request instead. */
            if(i>=80 && i<=87) CHECK(s.target>=31.9f && s.speed>=31.9f);
            if(i==88) CHECK(s.mode==CONTROL_TRACK && s.speed>=31.9f);
            if(!sign) {
                unsigned common=(unsigned)(s.speed*10.0f);
                if(i>=80 && i<=87 && common<b23_replay_min_x10) b23_replay_min_x10=common;
                if(i==88) b23_replay_rejoin_x10=common;
                if(i==159) b23_replay_end_x10=common;
            }
        }
    }
}
void RunAmbiguousTests(void)
{
    RawReplay();
    Bounds();
}
