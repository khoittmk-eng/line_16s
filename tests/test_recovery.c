#include "control.h"
#include "control_tuning.h"
extern volatile unsigned test_count, failure_line;
void test_done(void);
static void Check(int ok, unsigned line)
{
    ++test_count;
    if(!ok) { failure_line=30000U+line; test_done(); }
}
#define CHECK(x) Check((x),__LINE__)
static void Warm(ControlState *s, ControlParameters *p)
{
    unsigned i; Control_Start(s);
    for(i=0;i<600;++i) Control_Step(s,p,0,1,0x18,2);
}
static void Lose(ControlState *s, ControlParameters *p, int sign)
{
    unsigned i;
    for(i=0;i<5;++i) Control_Step(s,p,sign*3000,1,sign>0?0x80:0x01,2);
    for(i=0;i<3;++i) Control_Step(s,p,sign*3000,0,0,2);
    CHECK(s->mode==CONTROL_TURN);
}
void RunRecoveryTests(void)
{
    ControlState s;
    ControlParameters p;
    unsigned i,j;
    int sign;
    /* A broad valid bend can return briefly to center without any TURN episode. */
    Control_Defaults(&p); p.base_percent=35; p.straight_percent=45; p.max_percent=75;
    for(sign=-1;sign<=1;sign+=2) {
        Warm(&s,&p);
        for(i=0;i<4;++i) Control_Step(&s,&p,sign*1500,1,sign>0?0xF8:0x1F,2);
        CHECK(s.mode==CONTROL_TRACK && s.left>=0 && s.right>=0 && s.target<=p.base_percent);
        for(i=0;i<6;++i) Control_Step(&s,&p,0,1,0x18,2);
        for(i=0;i<3;++i) Control_Step(&s,&p,0,0,0,2);
        CHECK((s.mode==CONTROL_TURN || s.mode==CONTROL_BRAKE) && s.turn_dir==sign && !s.may_cross_gap);
    }
    /* Brief center after a turn is not enough evidence for a 300 ms straight gap. */
    /* A wide ambiguous frame after a confirmed bend need not wait PROBE 80 ms. */
    Warm(&s,&p);
    for(i=0;i<3;++i) Control_Step(&s,&p,-1500,1,0x1F,2);
    for(i=0;i<3;++i) Control_Step(&s,&p,0,0,0xFF,2);
    CHECK((s.mode==CONTROL_TURN || s.mode==CONTROL_BRAKE) && s.turn_dir==-1);
    /* One broad sample and alternating side noise do not confirm a bend. */
    Warm(&s,&p);
    Control_Step(&s,&p,1500,1,0xF8,2);
    CHECK(!s.corner_pending_ms && s.target<=35 && s.mode==CONTROL_TRACK && s.left>=0 && s.right>=0);
    for(i=0;i<20;++i) Control_Step(&s,&p,(i&1)?1500:-1500,1,(i&1)?0xF8:0x1F,2);
    CHECK(!s.corner_pending_ms && s.mode==CONTROL_TRACK);
    /* Stable center clears pending intent; real straight dashes still hold commands. */
    Warm(&s,&p);
    for(i=0;i<3;++i) Control_Step(&s,&p,1500,1,0xF8,2);
    for(i=0;i<100;++i) Control_Step(&s,&p,0,1,0x18,2);
    CHECK(!s.corner_pending_ms);
    {
        int16_t left=s.left,right=s.right;
        for(i=0;i<120;++i) Control_Step(&s,&p,0,0,0,2);
        CHECK(s.mode==CONTROL_GAP && s.left>=left-1 && s.left<=left+1 && s.right>=right-1 && s.right<=right+1);
    }
    /* Pending expires without a forced turn while the line remains visible. */
    Warm(&s,&p);
    for(i=0;i<3;++i) Control_Step(&s,&p,1500,1,0xF8,2);
    for(i=0;i<100;++i) Control_Step(&s,&p,1000,1,0x30,2);
    CHECK(!s.corner_pending_ms && s.mode==CONTROL_TRACK);
    /* Brief center after a turn is not enough evidence for a 300 ms straight gap. */
    Control_Defaults(&p); p.kp=12; p.ki=0; p.kd=0.12f;
    p.base_percent=35; p.straight_percent=45; p.max_percent=75;
    for(sign=-1;sign<=1;sign+=2) {
        unsigned deadline;
        Warm(&s,&p); Lose(&s,&p,sign);
        for(i=0;i<40;++i) Control_Step(&s,&p,0,0,0,2);
        for(i=0;i<6;++i) Control_Step(&s,&p,0,1,0x18,2);
        CHECK(s.mode==CONTROL_TRACK);
        deadline=s.recovery_ms;
        for(i=0;i<3;++i) Control_Step(&s,&p,0,0,0,2);
        CHECK(s.mode==CONTROL_TURN && s.turn_dir==sign);
        CHECK(s.recovery_ms==deadline+6 && !s.may_cross_gap);
    }
    /* Stable centered tracking releases the guard and restores normal dashed GAP. */
    Warm(&s,&p); Lose(&s,&p,1);
    for(i=0;i<6;++i) Control_Step(&s,&p,0,1,0x18,2);
    CHECK(s.post_turn_ms && s.guide_valid);
    for(i=0;i<CONTROL_POST_TURN_CLEAR_MS/2U;++i) Control_Step(&s,&p,0,1,0x18,2);
    CHECK(!s.post_turn_ms && !s.recovery_ms);
    {
        int16_t left=s.left,right=s.right;
        for(i=0;i<120;++i) Control_Step(&s,&p,0,0,0,2);
        CHECK(s.mode==CONTROL_GAP && s.left>=left-1 && s.left<=left+1 && s.right>=right-1 && s.right<=right+1);
    }
    /* Brief successful center windows cannot repeatedly reset the search deadline. */
    Warm(&s,&p); Lose(&s,&p,1);
    for(j=0;j<80;++j) {
        for(i=0;i<6;++i) Control_Step(&s,&p,0,1,0x18,2);
        for(i=0;i<3;++i) Control_Step(&s,&p,0,0,0,2);
    }
    CHECK(s.mode==CONTROL_FAULT && s.line_stop_reason==LINE_STOP_TURN_TIMEOUT);
    CHECK(!s.left && !s.right && !s.post_turn_ms);
    /* Remember completed turn even if last-side TTL expired during a long search. */
    Warm(&s,&p); Lose(&s,&p,-1);
    for(i=0;i<350;++i) Control_Step(&s,&p,0,0,0,2);
    for(i=0;i<6;++i) Control_Step(&s,&p,0,1,0x18,2);
    CHECK(!s.last_side_ms && s.post_turn_dir==-1);
    for(i=0;i<3;++i) Control_Step(&s,&p,0,0,0,2);
    CHECK(s.mode==CONTROL_TURN && s.turn_dir==-1 && s.recovery_ms==718);
    /* Fresh opposite line still has authority over the completed-turn direction. */
    for(i=0;i<6;++i) Control_Step(&s,&p,0,1,0x18,2);
    Control_Step(&s,&p,2500,1,0x40,2);
    for(i=0;i<3;++i) Control_Step(&s,&p,0,0,0,2);
    CHECK(s.mode==CONTROL_TURN && s.turn_dir==1);
    /* Guard expires on visible off-center tracking without forcing a timed turn. */
    Warm(&s,&p); Lose(&s,&p,1);
    for(i=0;i<6;++i) Control_Step(&s,&p,0,1,0x18,2);
    for(i=0;i<CONTROL_POST_TURN_MS/2U;++i) Control_Step(&s,&p,1000,1,0x30,2);
    CHECK(s.mode==CONTROL_TRACK && !s.post_turn_ms);
    /* A brief centered glimpse must not erase the last observed side for search. */
    Control_Defaults(&p); p.base_percent=35; p.straight_percent=45; p.max_percent=75;
    for(sign=-1;sign<=1;sign+=2) {
        Warm(&s,&p);
        Control_Step(&s,&p,sign*2500,1,sign>0?0x40:0x02,2);
        for(i=0;i<10;++i) Control_Step(&s,&p,0,1,0x18,2);
        for(i=0;i<CONTROL_GAP_MAX_MS/2U+1U;++i) Control_Step(&s,&p,0,0,0,2);
        CHECK(s.mode==CONTROL_TURN && s.turn_dir==sign);
        Control_Step(&s,&p,0,1,0x18,2);
        CHECK(s.mode==CONTROL_ALIGN && s.left>=0 && s.right>=0);
    }
    /* Fresh opposite side replaces old direction, even if PID then passes through zero. */
    Warm(&s,&p);
    Control_Step(&s,&p,2500,1,0x40,2);
    Control_Step(&s,&p,-2500,1,0x02,2);
    for(i=0;i<10;++i) Control_Step(&s,&p,0,1,0x18,2);
    for(i=0;i<151;++i) Control_Step(&s,&p,0,0,0,2);
    CHECK(s.mode==CONTROL_TURN && s.turn_dir==-1);
    for(i=0;i<600;++i) Control_Step(&s,&p,0,0,0,2);
    CHECK(s.mode==CONTROL_FAULT && s.line_stop_reason==LINE_STOP_TURN_TIMEOUT);
    Control_Step(&s,&p,0,1,0x18,2);
    CHECK(s.mode==CONTROL_FAULT && !s.left && !s.right && !s.last_side);
    /* Once the side observation expires, do not turn on stale memory. */
    Warm(&s,&p);
    Control_Step(&s,&p,2500,1,0x40,2);
    for(i=0;i<CONTROL_LAST_SIDE_MS/2U;++i) Control_Step(&s,&p,1000,1,0x30,2);
    Control_Step(&s,&p,0,1,0x18,2);
    for(i=0;i<151;++i) Control_Step(&s,&p,0,0,0,2);
    CHECK(s.mode==CONTROL_FAULT && s.line_stop_reason==LINE_STOP_GAP_TIMEOUT);
    /* Sustained straight tracking clears the turn sooner than TTL. */
    Warm(&s,&p);
    Control_Step(&s,&p,2500,1,0x40,2);
    for(i=0;i<100;++i) Control_Step(&s,&p,0,1,0x18,2);
    CHECK(!s.last_side && !s.last_side_ms);
    for(i=0;i<151;++i) Control_Step(&s,&p,0,0,0,2);
    CHECK(s.mode==CONTROL_FAULT && s.line_stop_reason==LINE_STOP_GAP_TIMEOUT);
    /* Ambiguous timeout may search a fresh side, but cannot invent one. */
    Warm(&s,&p);
    Control_Step(&s,&p,-2500,1,0x02,2);
    for(i=0;i<10;++i) Control_Step(&s,&p,0,1,0x18,2);
    for(i=0;i<40;++i) Control_Step(&s,&p,0,0,0x81,2);
    CHECK(s.mode==CONTROL_TURN && s.turn_dir==-1);
    /* Watchdog Stop still discards memory and cannot be bypassed by sensor frames. */
    Control_Stop(&s,FAULT_ADC);
    Control_Step(&s,&p,0,1,0x18,2);
    CHECK(s.mode==CONTROL_FAULT && !s.left && !s.right && !s.last_side_ms);
    /* Next corner can arrive before RECENTER completes: accept fresh direction. */
    Control_Defaults(&p); p.base_percent=35; p.straight_percent=45; p.max_percent=75;
    for(sign=-1;sign<=1;sign+=2) {
        Warm(&s,&p); Lose(&s,&p,sign);
        Control_Step(&s,&p,0,1,0x18,2);
        CHECK(s.mode==CONTROL_ALIGN);
        for(i=0;i<3;++i) Control_Step(&s,&p,0,0,sign>0?0x17:0xE8,2);
        CHECK(s.mode==CONTROL_TURN && s.turn_dir==-sign && s.recovery_ms==8);
        for(i=0;i<80;++i) Control_Step(&s,&p,0,0,0,2);
        CHECK(s.left*sign<0 && s.right*sign>0);
        for(i=0;i<6;++i) Control_Step(&s,&p,0,1,0x18,2);
        CHECK(s.mode==CONTROL_TRACK);
        /* B13: immediate reloss retries the recent turn, not a long forward gap. */
        {
            unsigned elapsed=s.recovery_ms;
            for(i=0;i<20;++i) Control_Step(&s,&p,0,0,0,2);
            CHECK(s.mode==CONTROL_TURN && s.turn_dir==-sign && s.recovery_ms==elapsed+40);
        }
    }
    /* No seen line yet: opposite split noise must not reverse the first turn. */
    Warm(&s,&p); Lose(&s,&p,1);
    for(i=0;i<10;++i) Control_Step(&s,&p,0,0,0x17,2);
    CHECK(s.turn_dir==1 && s.mode==CONTROL_TURN);
    Control_Step(&s,&p,0,1,0x18,2);
    for(i=0;i<20;++i) Control_Step(&s,&p,0,0,(i&1U)?0x17:0xE8,2);
    CHECK(s.turn_dir==1 && s.mode==CONTROL_TURN);
    /* Repeated retargets without completed reacquisition do not renew 1200 ms. */
    for(i=0;i<650;++i) Control_Step(&s,&p,0,0,((i/4U)&1U)?0x17:0xE8,2);
    CHECK(s.mode==CONTROL_FAULT && s.line_stop_reason==LINE_STOP_TURN_TIMEOUT);
    CHECK(!s.left && !s.right);
    /* No 200 ms straight between alternating turns: exercise the transition itself. */
    for(j=0;j<2;++j) {
        p.base_percent=j?45:35; p.max_percent=75; Warm(&s,&p);
        for(i=0;i<8;++i) {
            unsigned k;
            sign=(i&1U)?-1:1;
            for(k=0;k<3;++k) Control_Step(&s,&p,0,0,sign>0?0xE8:0x17,2);
            CHECK((s.mode==CONTROL_TURN || s.mode==CONTROL_BRAKE) && s.turn_dir==sign);
            for(k=0;k<40;++k) Control_Step(&s,&p,0,0,0,2);
            for(k=0;k<6;++k) Control_Step(&s,&p,0,1,0x18,2);
            CHECK(s.mode==CONTROL_TRACK && s.guide_valid && s.guide_age_ms==0);
            CHECK(s.left>=0 && s.right>=0 && s.left<=750 && s.right<=750);
        }
    }
    /* A central eye at a dash edge can kick D; keep the last moving commands through white. */
    Control_Defaults(&p); p.base_percent=45; p.straight_percent=45; p.max_percent=60; Warm(&s,&p);
    Control_Step(&s,&p,500,1,0x10,2);
    {
        int before_diff=s.left-s.right;
        Control_Step(&s,&p,0,0,0,2);
        CHECK(s.mode==CONTROL_GAP && s.left>0 && s.right>0);
        CHECK(s.left-s.right<before_diff); /* Filter the dash-edge D kick. */
    }
    /* Asymmetric split corner after centered tracking has no valid extreme centroid. */
    for(sign=-1;sign<=1;sign+=2) {
        Control_Defaults(&p); p.base_percent=20; p.straight_percent=20; p.max_percent=30; Warm(&s,&p);
        for(i=0;i<3;++i) Control_Step(&s,&p,0,0,sign>0?0xE8:0x17,2);
        CHECK(s.mode==CONTROL_TURN && s.turn_dir==sign && !s.may_cross_gap);
        for(i=0;i<40;++i) Control_Step(&s,&p,0,0,0,2);
        CHECK(s.mode==CONTROL_TURN && s.left*sign>0 && s.right*sign<0);
        for(i=0;i<6;++i) Control_Step(&s,&p,0,1,0x18,2);
        CHECK(s.mode==CONTROL_TRACK);
        Warm(&s,&p);
        for(i=0;i<10;++i) Control_Step(&s,&p,0,0,(i&1)?0xE8:0x17,2);
        CHECK(s.mode==CONTROL_PROBE); /* Alternating noise cannot confirm a side. */
    }
    Control_Defaults(&p);
    CHECK(p.base_percent==45 && p.max_percent==85 && p.advance_ms==0 && p.align_ms==12);
    p.base_percent=20; p.straight_percent=20; p.max_percent=30;
    /* Golden invariant: visible single line never enters a geometry/timed pivot, even at an edge. */
    p.advance_ms=800; /* Wrong legacy geometry must have no effect. */
    for(sign=-1;sign<=1;sign+=2) {
        Warm(&s,&p);
        for(i=0;i<1000;++i) {
            Control_Step(&s,&p,sign*3500,1,sign>0?0x80:0x01,2);
            CHECK(s.mode==CONTROL_TRACK && s.left>=0 && s.right>=0);
        }
        Warm(&s,&p);
        for(i=0;i<500;++i) {
            Control_Step(&s,&p,sign*1500,1,sign>0?0xF8:0x1F,2);
            CHECK(s.mode==CONTROL_TRACK && s.left>=0 && s.right>=0);
        }
    }
    Control_Defaults(&p);
    p.base_percent=20; p.straight_percent=20; p.max_percent=30;
    for(sign=-1;sign<=1;sign+=2) {
        Warm(&s,&p); Lose(&s,&p,sign);
        for(i=0;i<100;++i) Control_Step(&s,&p,sign*3000,0,0,2);
        CHECK(s.left==sign*300 && s.right==-sign*300); /* Symmetric pivot remains capped at Max. */
        Control_Step(&s,&p,sign*2500,1,sign>0?0x40:0x02,2);
        CHECK(s.mode==CONTROL_ALIGN && s.left>=0 && s.right>=0); /* Do not keep spinning past visible line. */
        for(i=0;i<6;++i) Control_Step(&s,&p,0,1,0x18,2);
        CHECK(s.mode==CONTROL_TRACK && s.derivative==0);
    }
    /* A centered line can be taken back immediately; no mandatory white-clear or minimum spin gate. */
    Warm(&s,&p);
    Control_Step(&s,&p,3000,1,0x80,2);
    for(i=0;i<40;++i) Control_Step(&s,&p,3000,0,0xFF,2);
    CHECK(s.mode==CONTROL_TURN);
    Control_Step(&s,&p,0,1,0x18,2);
    CHECK(s.mode==CONTROL_ALIGN && s.left>=0 && s.right>=0);
    for(i=0;i<6;++i) Control_Step(&s,&p,0,1,0x18,2);
    CHECK(s.mode==CONTROL_TRACK);
    /* Repeated straight dashes retain the successful bounded gap behavior. */
    Warm(&s,&p);
    for(j=0;j<20;++j) {
        for(i=0;i<20;++i) Control_Step(&s,&p,0,1,0x18,2);
        for(i=0;i<60;++i) {
            float speed=s.speed;
            Control_Step(&s,&p,0,0,0,2);
            CHECK(s.mode==CONTROL_GAP && s.left>0 && s.right>0 && s.speed<=speed);
        }
        Control_Step(&s,&p,0,1,0x18,2);
        CHECK(s.mode==CONTROL_TRACK && s.derivative==0);
    }
    /* Short-lived wide noise does not force a corner. */
    Warm(&s,&p);
    for(i=0;i<10;++i) Control_Step(&s,&p,0,0,0xFF,2);
    CHECK(s.mode==CONTROL_PROBE);
    Control_Step(&s,&p,0,1,0x18,2);
    CHECK(s.mode==CONTROL_TRACK);
    /* Reacquisition flicker must not extend the recovery timeout forever. */
    Warm(&s,&p); Lose(&s,&p,1);
    for(i=0;i<650;++i) Control_Step(&s,&p,3000,(i%8U)<4U,(i%8U)<4U?0x80:0,2);
    CHECK(s.mode==CONTROL_FAULT && !s.left && !s.right && s.line_stop_reason==LINE_STOP_TURN_TIMEOUT);
    Warm(&s,&p);
    for(i=0;i<CONTROL_GAP_MAX_MS/2U+5U;++i) Control_Step(&s,&p,0,0,0,2);
    CHECK(s.mode==CONTROL_FAULT && s.line_stop_reason==LINE_STOP_GAP_TIMEOUT);
    Warm(&s,&p);
    for(i=0;i<40;++i) Control_Step(&s,&p,0,0,0x81,2);
    CHECK(s.mode==CONTROL_FAULT && s.line_stop_reason==LINE_STOP_AMBIGUOUS);
    /* Consecutive bends can recover without using any 16 cm map/distance estimate. */
    Warm(&s,&p);
    for(j=0;j<8;++j) {
        sign=(j&1U)?-1:1;
        Lose(&s,&p,sign);
        for(i=0;i<50;++i) Control_Step(&s,&p,0,0,0,2);
        for(i=0;i<6;++i) Control_Step(&s,&p,0,1,0x18,2);
        CHECK(s.mode==CONTROL_TRACK);
        for(i=0;i<100;++i) Control_Step(&s,&p,sign*1000,1,sign>0?0x30:0x0C,2);
        CHECK(s.mode==CONTROL_TRACK);
    }
    /* A short central split-mask transition keeps the same remembered rolling commands. */
    Control_Defaults(&p); p.base_percent=45; p.straight_percent=45; p.max_percent=60; Warm(&s,&p);
    Control_Step(&s,&p,500,1,0x10,2);
    {
        int16_t left=(int16_t)((s.guide_left+s.guide_right)/2.0f+s.gap_correction*10.0f);
        int16_t right=(int16_t)((s.guide_left+s.guide_right)/2.0f-s.gap_correction*10.0f);
        for(i=0;i<4;++i) {
            Control_Step(&s,&p,0,0,0x24,2);
            CHECK(s.mode==CONTROL_GAP && s.left>=left-1 && s.left<=left+1 && s.right>=right-1 && s.right<=right+1 && !s.brake_count);
        }
        for(i=0;i<120;++i) {
            Control_Step(&s,&p,0,0,0,2);
            CHECK(s.mode==CONTROL_GAP && s.left>=left-1 && s.left<=left+1 && s.right>=right-1 && s.right<=right+1 && !s.brake_count);
        }
        Control_Step(&s,&p,0,1,0x18,2);
        CHECK(s.mode==CONTROL_TRACK && s.derivative==0 && s.target==45);
    }
    /* A sustained ambiguous pattern must not inherit a white-gap allowance indefinitely. */
    Warm(&s,&p);
    for(i=0;i<CONTROL_GAP_TRANSITION_MS/2U+1U;++i) Control_Step(&s,&p,0,0,0x24,2);
    CHECK(s.mode==CONTROL_PROBE && !s.may_cross_gap && !s.guide_valid);
    /* A stale guide or a real outside edge cannot authorize a straight continuation. */
    Warm(&s,&p);
    for(i=0;i<CONTROL_GUIDE_MAX_AGE_MS/2U+1U;++i) Control_Step(&s,&p,1000,1,0x30,2);
    Control_Step(&s,&p,0,0,0,2);
    CHECK(s.mode!=CONTROL_GAP && !s.may_cross_gap);
    Warm(&s,&p);
    Control_Step(&s,&p,3000,1,0x80,2);
    Control_Step(&s,&p,0,0,0,2);
    CHECK(s.mode==CONTROL_PROBE && !s.may_cross_gap && !s.guide_valid);
    /* Exact white-span boundary: hold through the allowance, then latch Stop. */
    Warm(&s,&p);
    for(i=0;i<CONTROL_GAP_MAX_MS/2U;++i) {
        Control_Step(&s,&p,0,0,0,2);
        CHECK(s.mode==CONTROL_GAP && s.left>0 && s.right>0);
    }
    Control_Step(&s,&p,0,0,0,2);
    CHECK(s.mode==CONTROL_FAULT && s.line_stop_reason==LINE_STOP_GAP_TIMEOUT);
    Control_Step(&s,&p,0,1,0x18,2);
    CHECK(s.mode==CONTROL_FAULT && !s.left && !s.right);
}
