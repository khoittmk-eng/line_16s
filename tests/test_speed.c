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
void RunSpeedTests(void)
{
    ControlParameters p;
    ControlState s, mirror;
    unsigned i,j,dt;
    int sign;
    Control_Defaults(&p); Control_Start(&s);
    /* Constant Base cruise: Max must never become a target just because the line is centered. */
    for(i=0;i<1500;++i) {
        float before=s.speed;
        Control_Step(&s,&p,0,1,0x18,2);
        CHECK(s.speed<=before+0.161f && s.target==45 && s.speed_reason==SPEED_BASE);
        CHECK(s.left<=450 && s.right<=450 && !s.brake_count);
    }
    CHECK(s.left==450 && s.right==450 && p.max_percent==75);
    for(sign=-1;sign<=1;sign+=2) {
        for(i=0;i<800;++i) Control_Step(&s,&p,sign*500,1,sign>0?0x10:0x08,2);
        CHECK(s.speed==45 && s.target==45 && s.speed_reason==SPEED_BASE && !s.brake_count);
    }
    /* Eye jitter and a broad central group do not throttle gentle curves. */
    for(i=0;i<1000;++i) {
        Control_Step(&s,&p,(i&1U)?100:-100,1,0x18,2);
        CHECK(s.mode==CONTROL_TRACK && s.target==45 && s.speed==45 && !s.brake_count);
    }
    for(i=0;i<100;++i) Control_Step(&s,&p,0,1,0x3C,2);
    CHECK(s.target==45 && s.speed==45 && s.mode==CONTROL_TRACK);
    for(i=0;i<100;++i) Control_Step(&s,&p,1500,1,0xF8,2);
    CHECK(s.target==28 && s.speed==28 && s.mode==CONTROL_TRACK && s.speed_reason==SPEED_CORNER);
    /* Sharp-curve target changes continuously rather than stepping down at error 1.0. */
    Control_Step(&s,&p,1501,1,0x30,2);
    CHECK(s.target>44.9f && s.target<45);
    for(i=0;i<100;++i) Control_Step(&s,&p,2000,1,0x60,2);
    CHECK(s.target>39.3f && s.target<39.4f && s.speed_reason==SPEED_CURVE);
    for(i=0;i<100;++i) Control_Step(&s,&p,3000,1,0x80,2);
    CHECK(s.target==28 && s.speed==28 && s.mode==CONTROL_TRACK && !s.brake_count);
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
        CHECK(s.mode==CONTROL_TURN && s.left==sign*9 && s.right==-sign*9);
        for(i=0;i<35;++i) Control_Step(&s,&p,0,0,0,2);
        CHECK(s.left==sign*300 && s.right==-sign*300 && s.speed==30);
        Control_Step(&s,&p,0,1,0x18,2);
        CHECK(s.mode==CONTROL_ALIGN && s.left==260 && s.right==260);
        FastLoss(&s,&p,sign);
        Control_Step(&s,&p,sign*1500,1,sign>0?0x40:0x02,2);
        CHECK(s.mode==CONTROL_ALIGN && !s.brake_ms && s.left>=0 && s.right>=0);
        CHECK(s.left<=520 && s.right<=520);
        for(i=0;i<650;++i) Control_Step(&s,&p,sign*1500,i%4==0,i%4==0?0x40:0,2);
        CHECK(s.mode==CONTROL_FAULT && s.line_stop_reason==LINE_STOP_TURN_TIMEOUT);
    }
    /* Accepted frame intervals still obey bounded braking and forward tracking. */
    for(dt=1;dt<=4;++dt) {
        CruiseAt(&s,&p,90);
        for(i=0;i<120/dt;++i) Control_Step(&s,&p,3500,1,0x80,dt);
        CHECK(s.speed<=28 && s.mode==CONTROL_TRACK);
    }
    Control_Defaults(&p); Control_Start(&s); Control_Start(&mirror);
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
