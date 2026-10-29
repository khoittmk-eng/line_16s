#include "control.h"
#include "control_tuning.h"
extern volatile unsigned test_count, failure_line;
void test_done(void);
static void Check(int ok,unsigned line)
{
    ++test_count;
    if(!ok) { failure_line=90000U+line; test_done(); }
}
#define CHECK(x) Check((x),__LINE__)
/* Keep the released 55/85 profile covered independently of new defaults. */
static void Profile55(ControlParameters *p)
{
    Control_Defaults(p); p->base_percent=p->straight_percent=55;
}

static float Abs(float x) { return x<0?-x:x; }
static void Warm(ControlState *s,const ControlParameters *p)
{
    unsigned i;
    Control_Start(s);
    for(i=0;i<800;++i) Control_Step(s,p,0,1,0x18,2);
}
static void Corner(ControlState *s,const ControlParameters *p,int sign)
{
    unsigned i;
    for(i=0;i<3;++i) Control_Step(s,p,sign*1500,1,sign>0?0xF8:0x1F,2);
    CHECK(s->corner_pending_ms && s->corner_pending_dir==sign);
}
static void EnterTurn(ControlState *s,const ControlParameters *p,int sign)
{
    unsigned i;
    Warm(s,p);
    Control_Step(s,p,sign*2500,1,sign>0?0x40:0x02,2);
    for(i=0;i<3;++i) Control_Step(s,p,0,0,0,2);
    CHECK(s->mode==CONTROL_TURN && s->turn_dir==sign);
}
void RunFixedBaseTests(void)
{
    ControlState s;
    ControlParameters p;
    unsigned i,j,dt;
    int sign;

    /* Real B24 regression first: a weak opposite central-eye sample must not
     * overwrite a confirmed corner. Wait the old 6 ms so this tests direction,
     * independently of B25's earlier pending-corner entry policy. */
    Profile55(&p); p.straight_percent=p.base_percent;
    for(sign=-1;sign<=1;sign+=2) {
        Warm(&s,&p); Corner(&s,&p,sign);
        Control_Step(&s,&p,-sign*500,1,sign>0?0x08:0x10,2);
        CHECK(s.corner_pending_dir==sign);
        for(i=0;i<3;++i) Control_Step(&s,&p,0,0,0,2);
        CHECK(s.mode==CONTROL_TURN && s.turn_dir==sign);
        CHECK(sign*(s.left-s.right)>0 && !s.may_cross_gap);

        /* The same weak-sample reversal used to occur inside RECENTER before
         * it could complete; established turn intent must survive this reloss. */
        EnterTurn(&s,&p,sign);
        Control_Step(&s,&p,-sign*500,1,sign>0?0x08:0x10,2);
        CHECK(s.mode==CONTROL_ALIGN);
        Control_Step(&s,&p,0,0,0,2);
        CHECK(s.mode==CONTROL_TURN && s.turn_dir==sign);

        /* Strong new opposite observations still identify the next square
         * corner, both before and during a recovery episode. */
        Warm(&s,&p); Corner(&s,&p,sign);
        Control_Step(&s,&p,-sign*2500,1,sign>0?0x02:0x40,2);
        for(i=0;i<3;++i) Control_Step(&s,&p,0,0,0,2);
        CHECK(s.mode==CONTROL_TURN && s.turn_dir==-sign);
        EnterTurn(&s,&p,sign);
        Control_Step(&s,&p,-sign*2500,1,sign>0?0x02:0x40,2);
        CHECK(s.mode==CONTROL_ALIGN);
        Control_Step(&s,&p,0,0,0,2);
        CHECK(s.mode==CONTROL_TURN && s.turn_dir==-sign);

        /* Completed RECENTER retains direction through weak center noise and
         * another loss; its search budget is not reset on this retry. */
        EnterTurn(&s,&p,sign);
        for(i=0;i<6;++i) Control_Step(&s,&p,0,1,0x18,2);
        CHECK(s.mode==CONTROL_TRACK && s.post_turn_ms);
        {
            unsigned elapsed=s.recovery_ms;
            Control_Step(&s,&p,-sign*500,1,sign>0?0x08:0x10,2);
            for(i=0;i<3;++i) Control_Step(&s,&p,0,0,0,2);
            CHECK(s.mode==CONTROL_TURN && s.turn_dir==sign);
            CHECK(s.recovery_ms==elapsed+8U && !s.may_cross_gap);
        }
    }

    /* A newer strong side wins over an older extreme-edge hint, including
     * when a weak opposite center sample follows it before the next loss. */
    Profile55(&p);
    for(sign=-1;sign<=1;sign+=2) {
        Warm(&s,&p);
        Control_Step(&s,&p,sign*3500,1,sign>0?0x80:0x01,2);
        Control_Step(&s,&p,-sign*2500,1,sign>0?0x02:0x40,2);
        for(i=0;i<3;++i) Control_Step(&s,&p,0,0,0,2);
        CHECK(s.mode==CONTROL_TURN && s.turn_dir==-sign);
        EnterTurn(&s,&p,sign);
        Control_Step(&s,&p,-sign*2500,1,sign>0?0x02:0x40,2);
        Control_Step(&s,&p,sign*500,1,sign>0?0x10:0x08,2);
        Control_Step(&s,&p,0,0,0,2);
        CHECK(s.mode==CONTROL_TURN && s.turn_dir==-sign);
        Warm(&s,&p);
        Control_Step(&s,&p,sign*3500,1,sign>0?0x80:0x01,2);
        Control_Step(&s,&p,-sign*2500,1,sign>0?0x02:0x40,2);
        for(i=0;i<40;++i) Control_Step(&s,&p,0,0,0x81,2);
        CHECK(s.mode==CONTROL_TURN && s.turn_dir==-sign);
    }

    /* A new opposite split can be the next square corner. Pending memory must
     * not consume its first frame and erase the split confirmation history. */
    for(sign=-1;sign<=1;sign+=2) {
        Warm(&s,&p); Corner(&s,&p,sign);
        for(i=0;i<2;++i) {
            Control_Step(&s,&p,0,0,sign>0?0x17:0xE8,2);
            CHECK(s.mode==CONTROL_PROBE && s.corner_pending_dir==sign);
        }
        Control_Step(&s,&p,0,0,sign>0?0x17:0xE8,2);
        CHECK(s.mode==CONTROL_TURN && s.turn_dir==-sign);
        CHECK(-sign*(s.left-s.right)>0);
        if(s.recovery_arc) CHECK(s.left>0 && s.right>0);
        else CHECK(s.left==-sign*500 && s.right==sign*500);
        /* One conflicting sample followed by white is not a new direction. */
        Warm(&s,&p); Corner(&s,&p,sign);
        Control_Step(&s,&p,0,0,sign>0?0x17:0xE8,2);
        Control_Step(&s,&p,0,0,0,2);
        CHECK(s.mode==CONTROL_TURN && s.turn_dir==sign);
    }

    /* Straight is retained for old settings/log layouts, never a boost input.
     * Startup still ramps; once cruising, center/soft/edge target stays Base. */
    for(j=0;j<3;++j) {
        Profile55(&p);
        CHECK(p.base_percent==55 && p.max_percent==85);
        p.straight_percent=j==0?0:j==1?55:100;
        Control_Start(&s);
        Control_Step(&s,&p,0,1,0x18,2);
        CHECK(s.speed>0 && s.speed<=0.161f && s.target==55);
        for(i=0;i<800;++i) {
            Control_Step(&s,&p,0,1,0x18,2);
            CHECK(s.target==55 && s.speed<=55 && s.mode==CONTROL_TRACK);
            CHECK(s.speed_bonus==0 && s.speed_reason!=SPEED_BOOST);
        }
        CHECK(s.left==550 && s.right==550);
        for(sign=-1;sign<=1;sign+=2) {
            Control_Step(&s,&p,sign*1000,1,sign>0?0x30:0x0C,2);
            CHECK(s.target==55 && s.mode==CONTROL_TRACK);
            Control_Step(&s,&p,sign*2500,1,sign>0?0x40:0x02,2);
            CHECK(s.target==55 && s.mode==CONTROL_TRACK);
            Control_Step(&s,&p,sign*3500,1,sign>0?0x80:0x01,2);
            CHECK(s.target==55 && s.mode==CONTROL_TRACK && !s.brake_count);
            CHECK(s.left>=0 && s.right>=0 && s.left<=850 && s.right<=850);
        }
    }

    /* A steady confirmed broad shape must not insert B24's 0.8-limit kick at
     * 6 ms. Its ordinary PID correction is 12 * 1.5 = 18 in either direction. */
    Profile55(&p);
    for(sign=-1;sign<=1;sign+=2) {
        Warm(&s,&p);
        s.last_error=sign*1.5f; s.derivative=s.planner_rate=0;
        s.last_correction=sign*18.0f;
        for(i=0;i<10;++i) {
            float before=Abs(s.last_correction);
            Control_Step(&s,&p,sign*1500,1,sign>0?0xF8:0x1F,2);
            CHECK(s.mode==CONTROL_TRACK && s.target==55);
            CHECK(sign*s.last_correction>=18.0f && sign*s.last_correction<=27.001f);
            CHECK(Abs(s.last_correction)<=before+2.881f && s.left>0 && s.right>0);
        }
        CHECK(s.corner_pending_ms && s.corner_pending_dir==sign);
    }

    /* Confirmed pending direction does not wait for another loss timer; the
     * first white frame turns immediately (arc if rolling, pivot otherwise).
     * These are controller requests, not measured motor velocity or yaw. */
    for(dt=1;dt<=4;++dt) for(j=0;j<3;++j) for(sign=-1;sign<=1;sign+=2) {
        unsigned elapsed=0;
        int duty;
        Profile55(&p);
        p.max_percent=j==0?20:j==1?55:85;
        p.base_percent=p.max_percent;
        Warm(&s,&p);
        do {
            Control_Step(&s,&p,sign*1500,1,sign>0?0xF8:0x1F,dt);
            elapsed+=dt;
        } while(elapsed<6U);
        CHECK(s.corner_pending_ms);
        Control_Step(&s,&p,0,0,0,dt);
        duty=p.max_percent<50?p.max_percent:50;
        CHECK(s.mode==CONTROL_TURN && s.turn_dir==sign && !s.brake_ms && !s.brake_count);
        if(s.recovery_arc) CHECK(s.left>0 && s.right>0 && sign*(s.left-s.right)>0);
        else CHECK(s.left==sign*duty*10 && s.right==-sign*duty*10);
        CHECK(!s.may_cross_gap);
        p.max_percent=10;
        Control_Step(&s,&p,0,0,0,dt);
        CHECK(s.left<=100 && s.left>=-100 && s.right<=100 && s.right>=-100);
        CHECK(sign*(s.left-s.right)>0);
        Control_Step(&s,&p,0,1,0x18,dt);
        CHECK(s.mode==CONTROL_ALIGN && s.left>=0 && s.right>=0);
        CHECK(s.target<=10 && s.left<=100 && s.right<=100);
    }

    /* An unconfirmed side still needs the existing 6 ms loss confirmation. */
    Profile55(&p);
    for(sign=-1;sign<=1;sign+=2) {
        Warm(&s,&p);
        Control_Step(&s,&p,sign*2500,1,sign>0?0x40:0x02,2);
        Control_Step(&s,&p,0,0,0,2);
        CHECK(s.mode==CONTROL_PROBE);
        Control_Step(&s,&p,0,0,0,2);
        CHECK(s.mode==CONTROL_PROBE);
        Control_Step(&s,&p,0,0,0,2);
        CHECK(s.mode==CONTROL_TURN && s.turn_dir==sign);
    }

    /* The fixed-Base change must not remove low-duty rejoin protection or
     * explicit fault latching. A fresh start is still an operator action. */
    EnterTurn(&s,&p,1);
    for(i=0;i<6;++i) Control_Step(&s,&p,0,1,0x18,2);
    CHECK(s.mode==CONTROL_TRACK && s.post_turn_ms && s.recovery_arc && s.speed>40 && s.speed<=55);
    Control_Step(&s,&p,0,1,0x18,2);
    CHECK(s.target==55 && s.speed<=55);
    Control_Stop(&s,FAULT_ADC);
    for(i=0;i<10;++i) Control_Step(&s,&p,0,1,0x18,2);
    CHECK(s.mode==CONTROL_FAULT && !s.left && !s.right);
}
