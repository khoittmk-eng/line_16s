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
/* Keep the released 55/85 profile covered independently of new defaults. */
static void Profile55(ControlParameters *p)
{
    Control_Defaults(p); p->base_percent=p->straight_percent=55;
}

static float Abs(float x) { return x<0?-x:x; }
static void CruiseAt(ControlState *s, ControlParameters *p, unsigned duty)
{
    unsigned i;
    Profile55(p); p->base_percent=p->max_percent=duty; Control_Start(s);
    for(i=0;i<1500;++i) Control_Step(s,p,0,1,0x18,2);
    CHECK(s->left==duty*10 && s->right==duty*10);
}
static void FastLoss(ControlState *s, ControlParameters *p, int sign)
{
    unsigned i;
    CruiseAt(s,p,90);
    Control_Step(s,p,sign*1500,1,sign>0?0x60:0x06,2);
    for(i=0;i<3;++i) Control_Step(s,p,0,0,0,2);
    CHECK(s->mode==CONTROL_TURN && !s->brake_count && !s->brake_ms);
    CHECK(sign*(s->left-s->right)>0);
}
static void CheckEntryAndSteering(void)
{
    ControlParameters p;
    ControlState seed,s;
    unsigned i,dt;
    int sign;
    Profile55(&p); Control_Start(&seed);
    for(i=0;i<600;++i) Control_Step(&seed,&p,0,1,0x18,2);
    CHECK(seed.speed==55);
    for(sign=-1;sign<=1;sign+=2) {
        for(dt=1;dt<=4;++dt) {
            s=seed;
            /* Confirmed broad corners must keep moving and steer immediately,
             * without the former zero-output pulse at any accepted interval. */
            for(i=0;i<100;++i) {
                Control_Step(&s,&p,sign*1500,1,sign>0?0xF8:0x1F,dt);
                CHECK(s.mode==CONTROL_TRACK && !s.brake_count);
                CHECK(s.left>0 && s.right>0 && sign*(s.left-s.right)>0);
                CHECK(s.left<=850 && s.right<=850);
            }
            CHECK(s.target==p.base_percent && s.corner_pending_dir==sign);
            /* Immediate opposite square corner changes steering, not old heading. */
            for(i=0;i<3;++i) Control_Step(&s,&p,-sign*1500,1,sign>0?0x1F:0xF8,2);
            CHECK(s.corner_pending_dir==-sign && sign*(s.left-s.right)<0);
            for(i=0;i<3;++i) Control_Step(&s,&p,0,0,0,2);
            CHECK(s.mode==CONTROL_TURN && s.turn_dir==-sign && !s.may_cross_gap);
        }
        s=seed;
        for(i=0;i<100;++i) Control_Step(&s,&p,sign*1200,1,sign>0?0x30:0x0C,2);
        CHECK(s.mode==CONTROL_TRACK && !s.brake_count && s.left>0 && s.right>0);
        Control_Step(&s,&p,0,0,0x24,2);
        Control_Step(&s,&p,sign*500,1,sign>0?0x10:0x08,2);
        CHECK(Abs(s.last_correction-sign*6.0f)<0.001f);
        s=seed;
        Control_Step(&s,&p,sign*500,1,sign>0?0x10:0x08,2);
        CHECK(Abs(s.last_correction)>6.0f && Abs(s.last_correction)<10.0f && s.left>0 && s.right>0);
        Control_Step(&s,&p,sign*3500,1,sign>0?0x80:0x01,2);
        CHECK(Abs(s.last_correction)>10 && s.mode==CONTROL_TRACK);
    }
    /* Legacy Straight settings cannot create a boost or a slowdown on a bend. */
    p.straight_percent=80; s=seed;
    for(i=0;i<1000;++i) Control_Step(&s,&p,0,1,0x18,2);
    s.last_error=1.0f; s.derivative=s.planner_rate=0;
    s.speed_bonus=0;
    Control_Step(&s,&p,1000,1,0x30,2);
    CHECK(s.speed==55 && s.target==55 && s.speed_bonus==0 && s.left>0 && s.right>0);
    Control_Step(&s,&p,3500,1,0x80,2);
    CHECK(s.speed==55 && s.target==55 && s.mode==CONTROL_TRACK); /* Steering, not a planner cap. */
    /* Direct high-duty recovery still yields immediately to valid line. */
    FastLoss(&s,&p,1);
    Control_Step(&s,&p,0,1,0x18,2);
    CHECK(s.mode==CONTROL_ALIGN && s.recovery_arc && s.left>0 && s.right>0 && s.target==90 && !s.brake_ms);
    for(i=0;i<5;++i) Control_Step(&s,&p,0,1,0x18,2);
    CHECK(s.mode==CONTROL_TRACK && s.post_turn_ms);
    for(i=0;i<29;++i) {
        Control_Step(&s,&p,0,1,0x18,2);
        CHECK(s.target==90 && s.speed<=90 && s.mode==CONTROL_TRACK);
    }
    Control_Step(&s,&p,0,1,0x18,2);
    CHECK(s.target==90 && s.mode==CONTROL_TRACK && s.speed_bonus==0);
    Control_Stop(&s,FAULT_UI); Control_Step(&s,&p,0,1,0x18,2);
    CHECK(s.mode==CONTROL_FAULT && !s.left && !s.right);
}
void RunSpeedTests(void)
{
    ControlParameters p;
    ControlState s, mirror;
    unsigned i,j,dt;
    int sign;
    CheckEntryAndSteering();
    /* B25: both straight and visible curves retain Base as the cruise target. */
    Profile55(&p); Control_Start(&s);
    CHECK(p.base_percent==55 && p.max_percent==85);
    for(i=0;i<1000;++i) Control_Step(&s,&p,0,1,0x18,2);
    CHECK(s.target==55 && s.left==550 && s.right==550);
    CHECK(p.straight_percent==55);
    p.straight_percent=62;
    for(i=0;i<60;++i) Control_Step(&s,&p,0,1,0x18,2);
    CHECK(s.target==55 && s.left==550 && s.right==550 && p.base_percent==55);
    p.max_percent=61;
    Control_Step(&s,&p,0,1,0x18,2);
    CHECK(s.target==55 && s.left<=610 && s.right<=610);
    p.max_percent=85; p.straight_percent=20;
    Control_Step(&s,&p,0,1,0x18,2);
    CHECK(s.target==55 && s.speed_bonus==0); /* Legacy Straight is ignored even below Base. */
    p.straight_percent=60;
    Control_Step(&s,&p,1900,1,0x60,2);
    CHECK(s.target==55 && s.speed_reason==SPEED_BASE);
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
    CHECK(s.mode==CONTROL_TRACK && s.target==p.base_percent && s.left>=0 && s.right>=0);
    /* Both directions of a repeated square pattern retain bounded recovery
     * despite the faster pivot/recenter profile and brief central handoffs. */
    for(j=0;j<4;++j) {
        sign=(j&1U)?-1:1;
        for(i=0;i<4;++i) Control_Step(&s,&p,sign*1500,1,sign>0?0xF8:0x1F,2);
        for(i=0;i<6;++i) Control_Step(&s,&p,0,1,0x18,2);
        for(i=0;i<3;++i) Control_Step(&s,&p,0,0,0,2);
        CHECK((s.mode==CONTROL_TURN || s.mode==CONTROL_BRAKE) && s.turn_dir==sign && !s.may_cross_gap);
        /* A rolling entry can use the bounded arc; sustained loss must still
         * reach the symmetric pivot without renewing its search deadline. */
        CHECK(sign*(s.left-s.right)>0);
        if(s.recovery_arc) CHECK(s.left>0 && s.right>0);
        else CHECK(s.left==sign*500 && s.right==-sign*500);
        for(i=0;i<85;++i) Control_Step(&s,&p,0,0,0,2);
        CHECK(s.left==sign*500 && s.right==-sign*500);
        Control_Step(&s,&p,0,1,0x18,2);
        CHECK(s.mode==CONTROL_ALIGN && s.left==400 && s.right==400 && !s.brake_ms);
        for(i=0;i<5;++i) Control_Step(&s,&p,0,1,0x18,2);
        CHECK(s.mode==CONTROL_TRACK && s.post_turn_ms && s.speed_guard_ms && s.recovery_ms<1200);
        {
            float before=s.speed;
            Control_Step(&s,&p,0,1,0x18,2);
            CHECK(s.speed>=before && s.speed<=40 && !s.curve_valid);
            if(before<39.68f) CHECK(s.speed>before+0.319f && s.speed<=before+0.321f);
        }
    }
    Profile55(&p); p.base_percent=45; p.straight_percent=55; p.max_percent=75; Control_Start(&s);
    /* Fixed Base has a startup ramp; neither Straight nor Max is a cruise target. */
    for(i=0;i<1500;++i) {
        float before=s.speed;
        Control_Step(&s,&p,0,1,0x18,2);
        CHECK(s.speed<=before+0.161f && s.target==45 && s.speed_bonus==0);
        CHECK(s.left<=450 && s.right<=450 && !s.brake_count);
    }
    CHECK(s.left==450 && s.right==450 && p.max_percent==75);
    for(sign=-1;sign<=1;sign+=2) {
        for(i=0;i<800;++i) Control_Step(&s,&p,sign*500,1,sign>0?0x10:0x08,2);
        CHECK(s.speed==45 && s.target==45 && !s.brake_count);
    }
    /* Eye jitter and a broad central group do not throttle gentle curves. */
    for(i=0;i<1000;++i) {
        Control_Step(&s,&p,(i&1U)?100:-100,1,0x18,2);
        CHECK(s.mode==CONTROL_TRACK && s.target==45 && s.speed==45 && !s.brake_count);
    }
    for(i=0;i<100;++i) Control_Step(&s,&p,0,1,0x3C,2);
    CHECK(s.target==45 && s.speed==45 && s.mode==CONTROL_TRACK);
    /* A stable soft bend stays at Base without a full brake or stop. */
    for(i=0;i<500;++i) Control_Step(&s,&p,1200,1,0x30,2);
    CHECK(s.target==45 && s.left>0 && s.right>0 && !s.brake_count);
    /* Fixed-Base straight -> long dash: exact trusted commands throughout
     * the bounded GAP, without a repeated Start or hidden speed cycling. */
    for(i=0;i<500;++i) Control_Step(&s,&p,0,1,0x18,2);
    for(j=0;j<10;++j) {
        for(i=0;i<20;++i) Control_Step(&s,&p,0,0,0,2);
        Control_Step(&s,&p,0,1,0x18,2);
        CHECK(s.mode==CONTROL_TRACK && s.speed==45 && s.target==45 && !s.brake_count);
    }
    for(i=0;i<150;++i) {
        float before=s.speed;
        Control_Step(&s,&p,0,0,0,2);
        CHECK(s.mode==CONTROL_GAP && !s.brake_count && s.left==s.right);
        CHECK(s.speed==before && s.speed==45);
        CHECK(s.left==450);
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
    CHECK(s.speed==45);
    for(i=0;i<100;++i) Control_Step(&s,&p,1500,1,0xF8,2);
    CHECK(s.target==p.base_percent && s.speed==p.base_percent && s.mode==CONTROL_TRACK && s.speed_reason==SPEED_CORNER);
    /* Visible sharp curves also keep Base; the wheel mixer supplies steering. */
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
    CHECK(s.target==45 && s.speed==45 && s.mode==CONTROL_TRACK && !s.brake_count);
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
        CHECK(s.mode==CONTROL_TURN && !s.brake_count && !s.brake_ms);
        CHECK(sign*(s.left-s.right)>0);
        for(i=0;i<85;++i) {
            Control_Step(&s,&p,0,0,0,2);
            CHECK(s.mode==CONTROL_TURN && !s.brake_count && sign*(s.left-s.right)>0);
        }
        CHECK(s.left==sign*500 && s.right==-sign*500 && s.speed==50);
        Control_Step(&s,&p,0,1,0x18,2);
        CHECK(s.mode==CONTROL_ALIGN && s.left==400 && s.right==400);
        FastLoss(&s,&p,sign);
        Control_Step(&s,&p,sign*1500,1,sign>0?0x40:0x02,2);
        CHECK(s.mode==CONTROL_ALIGN && !s.brake_ms && s.left>=0 && s.right>=0);
        CHECK(s.left<=900 && s.right<=900);
        for(i=0;i<650;++i) Control_Step(&s,&p,sign*1500,i%4==0,i%4==0?0x40:0,2);
        CHECK(s.mode==CONTROL_FAULT && s.line_stop_reason==LINE_STOP_TURN_TIMEOUT);
    }
    /* Accepted frame intervals still obey bounded recovery and forward tracking. */
    for(dt=1;dt<=4;++dt) {
        CruiseAt(&s,&p,90);
        for(i=0;i<120/dt;++i) Control_Step(&s,&p,3500,1,0x80,dt);
        CHECK(s.speed==90 && s.target==90 && s.mode==CONTROL_TRACK);
    }
    Profile55(&p); p.base_percent=45; p.straight_percent=55; p.max_percent=75; Control_Start(&s); Control_Start(&mirror);
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
    Profile55(&p); p.base_percent=p.max_percent=15;
    for(sign=-1;sign<=1;sign+=2) {
        Control_Start(&s);
        for(i=0;i<100;++i) Control_Step(&s,&p,sign*3000,1,sign>0?0x80:0x01,2);
        for(i=0;i<100;++i) Control_Step(&s,&p,0,0,0,2);
        CHECK(s.mode==CONTROL_TURN && s.left==sign*150 && s.right==-sign*150 && !s.brake_count);
        Control_Step(&s,&p,0,1,0x18,2);
        CHECK(s.mode==CONTROL_ALIGN && s.left==150 && s.right==150 && s.target==15);
    }
}
