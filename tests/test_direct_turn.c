#include "control.h"
#include "blackbox.h"
#include "control_tuning.h"
extern volatile unsigned test_count, failure_line;
void test_done(void);
static void Check(int ok,unsigned line)
{
    ++test_count;
    if(!ok) { failure_line=80000U+line; test_done(); }
}
#define CHECK(x) Check((x),__LINE__)
/* Keep the released 55/85 profile covered independently of new defaults. */
static void Profile55(ControlParameters *p)
{
    Control_Defaults(p); p->base_percent=p->straight_percent=55;
}

void RunDirectTurnTests(void)
{
    ControlState s;
    ControlParameters p;
    unsigned i,j,dt;
    int sign;
    for(sign=-1;sign<=1;sign+=2) {
        Profile55(&p);
        Control_Start(&s);
        /* A recent side observation with high equal wheel commands. */
        s.history=1; s.speed=80; s.left=s.right=800; s.last_error=(float)sign;
        for(i=0;i<3;++i) Control_Step(&s,&p,0,0,0,2);
        CHECK(s.mode==CONTROL_TURN && !s.brake_count && !s.brake_ms);
        CHECK(s.recovery_arc && s.left>0 && s.right>0 && sign*(s.left-s.right)>0);
        CHECK(s.left<=850 && s.right<=850);
        /* After the arc allowance, TURN removes forward common duty and
         * requests counter-rotation without renewing the episode budget. */
        s.mode=CONTROL_TURN; s.turn_dir=(int8_t)sign;
        s.left=s.right=800; s.speed=80; s.recovery_ms=CONTROL_RECOVERY_ARC_MS;
        Control_Step(&s,&p,0,0,0,2);
        CHECK(s.left==sign*500 && s.right==-sign*500 && !s.brake_count);
    }
    /* First recovery update must create yaw, across accepted intervals and Max. */
    for(dt=1;dt<=4;++dt) for(j=0;j<3;++j) for(sign=-1;sign<=1;sign+=2) {
        unsigned elapsed=0;
        Profile55(&p); p.max_percent=j==0?20:j==1?55:85;
        p.base_percent=p.straight_percent=p.max_percent;
        Control_Start(&s); s.history=1;
        s.speed=p.max_percent; s.left=s.right=p.max_percent*10;
        s.last_error=(float)sign;
        do { Control_Step(&s,&p,0,0,0,dt); elapsed+=dt; } while(elapsed<6);
        CHECK(s.mode==CONTROL_TURN && !s.brake_ms && !s.brake_count);
        if(s.recovery_arc) CHECK(s.left>0 && s.right>0 && sign*(s.left-s.right)>0);
        else CHECK(s.left==sign*(p.max_percent<50?p.max_percent:50)*10 && s.right==-s.left);
        for(i=0;i<140/dt;++i) {
            Control_Step(&s,&p,0,0,0,dt);
            CHECK(s.mode==CONTROL_TURN && !s.brake_count);
            CHECK(s.left<=p.max_percent*10 && s.left>=-(int)p.max_percent*10);
            CHECK(s.right<=p.max_percent*10 && s.right>=-(int)p.max_percent*10);
        }
        CHECK(s.left==sign*(p.max_percent<50?p.max_percent:50)*10);
        CHECK(s.right==-s.left);
        /* New Max while turning cannot create inverted mixer bounds. */
        p.base_percent=p.straight_percent=p.max_percent=10;
        Control_Step(&s,&p,0,0,0,dt);
        CHECK(s.left<=100 && s.left>=-100 && s.right<=100 && s.right>=-100);
        Control_Step(&s,&p,0,1,0x18,dt);
        CHECK(s.mode==CONTROL_ALIGN && s.left>=0 && s.right>=0);
        Control_Stop(&s,FAULT_UI); Control_Step(&s,&p,0,0,0,dt);
        CHECK(s.mode==CONTROL_FAULT && !s.left && !s.right);
    }
    for(sign=-1;sign<=1;sign+=2) {
        Profile55(&p); Control_Start(&s);
        s.history=1; s.speed=55; s.left=s.right=550; s.last_error=sign*1.5f;
        for(i=0;i<3;++i) Control_Step(&s,&p,sign*1500,1,sign>0?0xF8:0x1F,2);
        CHECK(s.mode==CONTROL_TRACK && s.target==55 && !s.brake_count);
        CHECK(sign*(s.left-s.right)>=416 && sign*(s.left-s.right)<=419 && s.left>0 && s.right>0);
        CHECK(s.left<=850 && s.right<=850);
        /* Confirming a broad cue never injects a steering floor; returning
         * error remains under ordinary PD control even while the cue persists. */
        Control_Step(&s,&p,sign*1400,1,sign>0?0xF8:0x1F,2);
        CHECK(sign*s.last_correction<18 && s.mode==CONTROL_TRACK);
        Control_Step(&s,&p,0,1,0x18,2);
        CHECK(s.left>0 && s.right>0 && s.mode==CONTROL_TRACK);
    }
}
