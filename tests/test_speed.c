#include "control.h"
#include "control_tuning.h"
extern volatile unsigned test_count, failure_line;
void test_done(void);
static void Check(int ok, unsigned line)
{
    ++test_count;
    if(!ok) { failure_line=40000U+line; test_done(); }
}
#define CHECK(x) Check((x),__LINE__)
static float Abs(float x) { return x<0?-x:x; }
static void CruiseAt(ControlState *s, ControlParameters *p, unsigned duty)
{
    unsigned i;
    Control_Defaults(p); p->base_percent=p->max_percent=duty; Control_Start(s);
    for(i=0;i<1500;++i) Control_Step(s,p,0,1,0x18,2);
    CHECK(s->left==duty*10 && s->right==duty*10);
}
static void FastLoss(ControlState *s, ControlParameters *p, int sign)
{
    unsigned i;
    CruiseAt(s,p,90);
    Control_Step(s,p,sign*1500,1,sign>0?0x60:0x06,2);
    for(i=0;i<3;++i) Control_Step(s,p,0,0,0,2);
    CHECK(s->mode==CONTROL_BRAKE && !s->left && !s->right && s->brake_count==1);
}
static void CheckEntryAndSteering(void)
{
    ControlParameters p;
    ControlState seed,s;
    unsigned i,dt;
    int sign;
    Control_Defaults(&p); Control_Start(&seed);
    for(i=0;i<600;++i) Control_Step(&seed,&p,0,1,0x18,2);
    CHECK(seed.speed==60 && seed.fast_history_ms);
    for(sign=-1;sign<=1;sign+=2) {
        s=seed;
        for(i=0;i<2;++i) {
            Control_Step(&s,&p,sign*1500,1,sign>0?0xF8:0x1F,2);
            CHECK(s.mode==CONTROL_TRACK && !s.brake_count);
        }
        Control_Step(&s,&p,sign*1500,1,sign>0?0xF8:0x1F,2);
        CHECK(s.mode==CONTROL_ENTRY_BRAKE && !s.left && !s.right && s.brake_count==1);
        for(i=2;i<8;i+=2) {
            Control_Step(&s,&p,sign*1500,1,sign>0?0xF8:0x1F,2);
            CHECK(s.mode==CONTROL_ENTRY_BRAKE && !s.left && !s.right);
        }
        Control_Step(&s,&p,sign*1500,1,sign>0?0xF8:0x1F,2);
        CHECK(s.mode==CONTROL_TRACK && s.target==32 && s.left>=0 && s.right>=0);
        for(i=0;i<100;++i) Control_Step(&s,&p,sign*1500,1,sign>0?0xF8:0x1F,2);
        CHECK(s.mode==CONTROL_TRACK && s.brake_count==1 && s.entry_latched);
        for(i=0;i<300;++i) Control_Step(&s,&p,0,1,0x18,2);
        CHECK(!s.entry_latched && s.fast_history_ms);
        for(i=0;i<3;++i) Control_Step(&s,&p,sign*1500,1,sign>0?0xF8:0x1F,2);
        CHECK(s.mode==CONTROL_ENTRY_BRAKE && s.brake_count==2);
        Control_Step(&s,&p,0,1,0x18,2);
        CHECK(s.mode==CONTROL_TRACK && s.left>0 && s.right>0 && !s.entry_brake_ms);

        s=seed;
        for(i=0;i<3;++i) Control_Step(&s,&p,sign*1500,1,sign>0?0xF8:0x1F,2);
        Control_Step(&s,&p,0,0,0,2);
        CHECK(s.mode==CONTROL_BRAKE && s.brake_ms==6 && s.recovery_ms==2 && s.turn_dir==sign);
        for(i=0;i<3;++i) Control_Step(&s,&p,0,0,0,2);
        CHECK(s.mode==CONTROL_TURN && s.brake_count==1 && s.recovery_ms==8);
        for(i=0;i<596;++i) Control_Step(&s,&p,0,0,0,2);
        CHECK(s.mode==CONTROL_FAULT && s.line_stop_reason==LINE_STOP_TURN_TIMEOUT);

        s=seed;
        /* Ordinary off-center curve never triggers early brake just from error. */
        for(i=0;i<100;++i) Control_Step(&s,&p,sign*1200,1,sign>0?0x30:0x0C,2);
        CHECK(!s.fast_history_ms && s.mode==CONTROL_TRACK && !s.brake_count);
        Control_Step(&s,&p,0,0,0x24,2);
        Control_Step(&s,&p,sign*500,1,sign>0?0x10:0x08,2);
        CHECK(Abs(s.last_correction-sign*6.0f)<0.001f); /* No slow release after reacquisition. */
        for(i=0;i<3;++i) Control_Step(&s,&p,sign*1500,1,sign>0?0xF8:0x1F,2);
        CHECK(s.mode==CONTROL_TRACK && !s.brake_count);

        s=seed;
        Control_Step(&s,&p,sign*500,1,sign>0?0x10:0x08,2);
        CHECK(Abs(s.last_correction)<=1.601f && s.left>0 && s.right>0 && !s.brake_count);
        Control_Step(&s,&p,sign*3500,1,sign>0?0x80:0x01,2);
        CHECK(Abs(s.last_correction)>10 && s.mode==CONTROL_TRACK); /* Edge bypasses slew. */
    }
    /* The entry pulse is bounded at all accepted caller intervals. */
    for(dt=1;dt<=4;++dt) {
        s=seed;
        for(i=0;i<(6+dt-1)/dt;++i) Control_Step(&s,&p,1500,1,0xF8,dt);
        CHECK(s.mode==CONTROL_ENTRY_BRAKE);
        for(i=0;i<(8+dt-1)/dt;++i) Control_Step(&s,&p,1500,1,0xF8,dt);
        CHECK(s.mode==CONTROL_TRACK && s.brake_count==1);
    }
    s=seed;
    for(i=0;i<3;++i) Control_Step(&s,&p,1500,1,0xF8,2);
    Control_Step(&s,&p,0,0,0,2);
    Control_Step(&s,&p,0,1,0x18,2);
    CHECK(s.mode==CONTROL_ALIGN && s.left==300 && s.right==300 && !s.brake_ms);
    for(i=0;i<5;++i) Control_Step(&s,&p,0,1,0x18,2);
    CHECK(s.mode==CONTROL_TRACK && s.post_turn_ms);
    for(i=0;i<29;++i) {
        Control_Step(&s,&p,0,1,0x18,2);
        CHECK(s.target==35 && s.speed<=35 && s.mode==CONTROL_TRACK);
    }
    Control_Step(&s,&p,0,1,0x18,2);
    CHECK(s.target==55 && s.mode==CONTROL_TRACK && s.speed_bonus==0);
    s=seed;
    for(i=0;i<3;++i) Control_Step(&s,&p,1500,1,0xF8,2);
    Control_Stop(&s,FAULT_UI); Control_Step(&s,&p,0,1,0x18,2);
    CHECK(s.mode==CONTROL_FAULT && !s.left && !s.right && !s.entry_brake_ms && !s.fast_history_ms);
}
void RunSpeedTests(void)
{
    ControlParameters p;
    ControlState s, mirror;
    unsigned i,j,dt;
    int sign;
    CheckEntryAndSteering();
    /* B17: stable moderate bends keep Base, outward movement slows earlier. */
    Control_Defaults(&p); Control_Start(&s);
    CHECK(p.base_percent==55 && p.max_percent==85);
    for(i=0;i<1000;++i) Control_Step(&s,&p,0,1,0x18,2);
    CHECK(s.target==60 && s.left==600 && s.right==600);
    CHECK(p.straight_percent==60);
    p.straight_percent=62;
    for(i=0;i<60;++i) Control_Step(&s,&p,0,1,0x18,2);
    CHECK(s.target==62 && s.left==620 && s.right==620 && p.base_percent==55);
    p.max_percent=61;
    Control_Step(&s,&p,0,1,0x18,2);
    CHECK(s.target==61 && s.left<=610 && s.right<=610);
    p.max_percent=85; p.straight_percent=20;
    Control_Step(&s,&p,0,1,0x18,2);
    CHECK(s.target==55 && s.speed_bonus==0); /* Defensive clamp below Base. */
    p.straight_percent=60;
    Control_Step(&s,&p,1900,1,0x60,2);
    CHECK(s.target<55 && s.target>=32 && s.speed_reason==SPEED_CURVE);
    CHECK(s.mode==CONTROL_TRACK && s.left>0 && s.right>0 && !s.brake_count);
    CHECK(!s.curve_valid); /* Outward drift must not earn the curve bridge. */
    for(i=0;i<300;++i) Control_Step(&s,&p,1900,1,0x60,2);
    CHECK(s.target==55 && s.speed==55 && s.mode==CONTROL_TRACK && !s.brake_count);
    /* Moderate stable curve -> fragmented center: retain exact steering briefly,
     * but never turn an 8 ms bridge into an unbounded straight-gap allowance. */
    {
        int16_t left=s.left, right=s.right;
        CHECK(s.curve_valid);
        for(i=0;i<4;++i) {
            Control_Step(&s,&p,0,0,0x24,2);
            CHECK(s.mode==CONTROL_PROBE && s.left==left && s.right==right && !s.brake_count);
        }
        Control_Step(&s,&p,1900,1,0x60,2);
        CHECK(s.mode==CONTROL_TRACK && s.left==left && s.right==right && s.derivative==0);
        for(i=0;i<5;++i) Control_Step(&s,&p,0,0,0x24,2);
        CHECK(s.mode==CONTROL_PROBE && s.target==15 && s.left<left && s.lost_ms==10);
        Control_Step(&s,&p,1900,1,0x60,2);
        left=s.left; right=s.right;
        for(i=0;i<2;++i) {
            Control_Step(&s,&p,0,0,0,2);
            CHECK(s.mode==CONTROL_PROBE && s.left==left && s.right==right);
        }
        Control_Step(&s,&p,0,0,0,2);
        CHECK((s.mode==CONTROL_BRAKE || s.mode==CONTROL_TURN) && s.turn_dir==1 && !s.may_cross_gap);
        Control_Start(&s);
        for(i=0;i<500;++i) Control_Step(&s,&p,0,1,0x18,2);
    }
    for(i=0;i<300;++i) Control_Step(&s,&p,1200,1,0x30,2);
    CHECK(s.target==55 && !s.brake_count);
    for(i=0;i<500;++i) Control_Step(&s,&p,0,1,0x18,2);
    for(i=0;i<120;++i) Control_Step(&s,&p,0,0,0,2);
    CHECK(s.mode==CONTROL_GAP && s.left==550 && s.right==550);
    Control_Step(&s,&p,0,1,0x18,2);
    CHECK(s.mode==CONTROL_TRACK && s.target==55);
    for(i=0;i<4;++i) Control_Step(&s,&p,1500,1,0xF8,2);
    CHECK(s.mode==CONTROL_TRACK && s.target==32 && s.left>=0 && s.right>=0);
    /* Both directions of a repeated square pattern retain bounded recovery
     * despite the faster pivot/recenter profile and brief central handoffs. */
    for(j=0;j<4;++j) {
        sign=(j&1U)?-1:1;
        for(i=0;i<4;++i) Control_Step(&s,&p,sign*1500,1,sign>0?0xF8:0x1F,2);
        for(i=0;i<6;++i) Control_Step(&s,&p,0,1,0x18,2);
        for(i=0;i<3;++i) Control_Step(&s,&p,0,0,0,2);
        CHECK((s.mode==CONTROL_TURN || s.mode==CONTROL_BRAKE) && s.turn_dir==sign && !s.may_cross_gap);
        /* A wheel may still be moving forward on entry: allow the specified
         * 600pp/s reversal ramp, rather than assuming every turn starts at zero. */
        for(i=0;i<85;++i) Control_Step(&s,&p,0,0,0,2);
        CHECK(s.left==sign*360 && s.right==-sign*360);
        Control_Step(&s,&p,0,1,0x18,2);
        CHECK(s.mode==CONTROL_ALIGN && s.left==300 && s.right==300 && !s.brake_ms);
        for(i=0;i<5;++i) Control_Step(&s,&p,0,1,0x18,2);
        CHECK(s.mode==CONTROL_TRACK && s.post_turn_ms && s.speed_guard_ms && s.recovery_ms<1200);
        {
            float before=s.speed;
            Control_Step(&s,&p,0,1,0x18,2);
            CHECK(s.speed>before+0.319f && s.speed<=before+0.321f && !s.curve_valid);
        }
    }
    Control_Defaults(&p); p.base_percent=45; p.straight_percent=55; p.max_percent=75; Control_Start(&s);
    /* B15 earns a bounded bonus; Max is still not a cruise target. */
    for(i=0;i<1500;++i) {
        float before=s.speed;
        Control_Step(&s,&p,0,1,0x18,2);
        CHECK(s.speed<=before+0.161f && s.target>=45 && s.target<=55);
        CHECK(s.left<=550 && s.right<=550 && !s.brake_count);
    }
    CHECK(s.left==550 && s.right==550 && p.max_percent==75);
    for(sign=-1;sign<=1;sign+=2) {
        for(i=0;i<800;++i) Control_Step(&s,&p,sign*500,1,sign>0?0x10:0x08,2);
        CHECK(s.speed==55 && s.target==55 && !s.brake_count);
    }
    /* Eye jitter and a broad central group do not throttle gentle curves. */
    for(i=0;i<1000;++i) {
        Control_Step(&s,&p,(i&1U)?100:-100,1,0x18,2);
        CHECK(s.mode==CONTROL_TRACK && s.target==55 && s.speed==55 && !s.brake_count);
    }
    for(i=0;i<100;++i) Control_Step(&s,&p,0,1,0x3C,2);
    CHECK(s.target==55 && s.speed==55 && s.mode==CONTROL_TRACK);
    /* A stable soft bend is faster than B14, without full brake or stop. */
    for(i=0;i<500;++i) Control_Step(&s,&p,1200,1,0x30,2);
    CHECK(s.target==45 && s.left>0 && s.right>0 && !s.brake_count);
    /* Accelerated straight -> long dash: exact initial guide, then bounded
     * proportional removal of the bonus, no repeated Start required. */
    for(i=0;i<500;++i) Control_Step(&s,&p,0,1,0x18,2);
    for(j=0;j<10;++j) {
        for(i=0;i<20;++i) Control_Step(&s,&p,0,0,0,2);
        Control_Step(&s,&p,0,1,0x18,2);
        CHECK(s.mode==CONTROL_TRACK && s.speed==55 && s.target==55 && !s.brake_count);
    }
    for(i=0;i<150;++i) {
        float before=s.speed;
        Control_Step(&s,&p,0,0,0,2);
        CHECK(s.mode==CONTROL_GAP && !s.brake_count && s.left==s.right);
        CHECK(s.speed<=before && s.speed>=44.9f);
        if(i<30) CHECK(s.left==550);
    }
    CHECK(s.left==450 && s.right==450);
    Control_Step(&s,&p,0,1,0x18,2);
    CHECK(s.mode==CONTROL_TRACK && s.target==45 && s.derivative==0);
    for(j=0;j<10;++j) {
        for(i=0;i<20;++i) Control_Step(&s,&p,0,1,0x18,2);
        for(i=0;i<60;++i) Control_Step(&s,&p,0,0,0,2);
        CHECK(s.mode==CONTROL_GAP && s.left==450 && s.right==450);
    }
    for(i=0;i<500;++i) Control_Step(&s,&p,0,1,0x18,2);
    CHECK(s.speed==55);
    for(i=0;i<100;++i) Control_Step(&s,&p,1500,1,0xF8,2);
    CHECK(s.target==32 && s.speed==32 && s.mode==CONTROL_TRACK && s.speed_reason==SPEED_CORNER);
    /* Sharp-curve target changes continuously rather than stepping down at error 1.0. */
    Control_Step(&s,&p,1501,1,0x30,2);
    CHECK(s.target==45);
    /* Brief center between sharp corners must not earn another burst. */
    for(i=0;i<100;++i) {
        Control_Step(&s,&p,0,1,0x18,2);
        CHECK(s.target==45 && s.speed_bonus==0 && s.mode==CONTROL_TRACK);
    }
    for(i=0;i<100;++i) Control_Step(&s,&p,2000,1,0x60,2);
    CHECK(s.target==45 && s.speed_reason==SPEED_BASE);
    for(i=0;i<100;++i) Control_Step(&s,&p,3000,1,0x80,2);
    CHECK(s.target==32 && s.speed==32 && s.mode==CONTROL_TRACK && !s.brake_count);
    /* High-duty dashes preserve requested wheel commands without a hidden Base reduction. */
    CruiseAt(&s,&p,90);
    for(j=0;j<10;++j) {
        int16_t left=s.left,right=s.right;
        for(i=0;i<100;++i) {
            Control_Step(&s,&p,0,0,0,2);
            CHECK(s.mode==CONTROL_GAP && s.left==left && s.right==right && !s.brake_count);
        }
        for(i=0;i<20;++i) Control_Step(&s,&p,0,1,0x18,2);
    }
    for(sign=-1;sign<=1;sign+=2) {
        FastLoss(&s,&p,sign);
        for(i=2;i<CONTROL_BRAKE_PULSE_MS;i+=2) {
            Control_Step(&s,&p,0,0,0,2);
            CHECK(s.mode==CONTROL_BRAKE && !s.left && !s.right);
        }
        Control_Step(&s,&p,0,0,0,2);
        CHECK(s.mode==CONTROL_TURN && s.left==sign*12 && s.right==-sign*12);
        for(i=0;i<35;++i) Control_Step(&s,&p,0,0,0,2);
        CHECK(s.left==sign*360 && s.right==-sign*360 && s.speed==36);
        Control_Step(&s,&p,0,1,0x18,2);
        CHECK(s.mode==CONTROL_ALIGN && s.left==300 && s.right==300);
        FastLoss(&s,&p,sign);
        Control_Step(&s,&p,sign*1500,1,sign>0?0x40:0x02,2);
        CHECK(s.mode==CONTROL_ALIGN && !s.brake_ms && s.left>=0 && s.right>=0);
        CHECK(s.left<=600 && s.right<=600);
        for(i=0;i<650;++i) Control_Step(&s,&p,sign*1500,i%4==0,i%4==0?0x40:0,2);
        CHECK(s.mode==CONTROL_FAULT && s.line_stop_reason==LINE_STOP_TURN_TIMEOUT);
    }
    /* Accepted frame intervals still obey bounded braking and forward tracking. */
    for(dt=1;dt<=4;++dt) {
        CruiseAt(&s,&p,90);
        for(i=0;i<120/dt;++i) Control_Step(&s,&p,3500,1,0x80,dt);
        CHECK(s.speed<=32 && s.mode==CONTROL_TRACK);
    }
    Control_Defaults(&p); p.base_percent=45; p.straight_percent=55; p.max_percent=75; Control_Start(&s); Control_Start(&mirror);
    for(i=0;i<300;++i) {
        int16_t e=(int16_t)(i<150?i*20:(300-i)*20);
        Control_Step(&s,&p,e,1,0x40,2);
        Control_Step(&mirror,&p,-e,1,0x02,2);
        CHECK(s.left==mirror.right && s.right==mirror.left);
        CHECK(Abs(s.speed-mirror.speed)<0.001f);
        CHECK(s.left>=0 && s.right>=0 && s.left<=750 && s.right<=750);
    }
    FastLoss(&s,&p,1); Control_Stop(&s,FAULT_UI);
    Control_Step(&s,&p,0,1,0x18,2);
    CHECK(s.mode==CONTROL_FAULT && !s.left && !s.right && !s.brake_ms && !s.guide_valid);
    Control_Defaults(&p); p.base_percent=p.max_percent=15;
    for(sign=-1;sign<=1;sign+=2) {
        Control_Start(&s);
        for(i=0;i<100;++i) Control_Step(&s,&p,sign*3000,1,sign>0?0x80:0x01,2);
        for(i=0;i<100;++i) Control_Step(&s,&p,0,0,0,2);
        CHECK(s.mode==CONTROL_TURN && s.left==sign*150 && s.right==-sign*150 && !s.brake_count);
        Control_Step(&s,&p,0,1,0x18,2);
        CHECK(s.mode==CONTROL_ALIGN && s.left==150 && s.right==150 && s.target==15);
    }
}
