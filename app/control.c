#include "control.h"
#include "control_tuning.h"
static float Abs(float x) { return x < 0.0f ? -x : x; }
static float Limit(float x, float low, float high) { return x < low ? low : x > high ? high : x; }
static float Approach(float current, float target, float step)
{
    if (current < target) return current + step > target ? target : current + step;
    return current - step < target ? target : current - step;
}
void Control_Defaults(ControlParameters *p)
{
    /* Trial gains: error is in sensor pitches, derivative is per second. */
    p->kp=12.00f; p->ki=0.0f; p->kd=0.12f;
    p->base_percent=45; p->straight_percent=45; p->max_percent=85;
    p->advance_ms=0; p->align_ms=CONTROL_REACQUIRE_DEFAULT_MS;
}
void Control_Stop(ControlState *s, ControlFault fault)
{
    s->mode=fault==FAULT_NONE?CONTROL_STOPPED:CONTROL_FAULT; s->fault=fault;
    s->speed=s->integral=s->derivative=s->last_error=s->last_correction=s->saturation_error=0;
    s->target=s->planner_rate=0; s->speed_reason=SPEED_IDLE;
    s->speed_bonus=0; s->cruise_ms=s->speed_guard_ms=0;
    s->curve_valid=0; s->curve_age_ms=1000U; s->curve_left=s->curve_right=0;
    s->stable_ms=s->lost_ms=s->recovery_ms=s->reacquire_ms=s->ambiguous_ms=0;
    s->split_cue_ms=0; s->split_cue_dir=0;
    s->corner_hint_ms=s->saturation_ms=0; s->corner_dir=s->turn_dir=0;
    s->brake_ms=0; s->brake_count=0;
    s->guide_age_ms=CONTROL_GUIDE_MAX_AGE_MS+1U;
    s->guide_left=s->guide_right=0; s->guide_valid=0;
    s->recovery_seen_line=0;
    s->last_side=0; s->last_side_ms=0;
    s->post_turn_ms=0; s->post_turn_dir=0;
    s->shape_ms=s->corner_pending_ms=0; s->shape_dir=s->corner_pending_dir=0;
    s->left=s->right=0; s->history=s->lost=s->may_cross_gap=0;
    s->probe_continue=0;
    s->recovery_arc_speed=0; s->recovery_arc=0;
    s->gap_correction=0;
    s->corner_gain=0; s->search_near_line=0;
    s->corner_inward_ms=s->corner_outward_ms=0; s->corner_releasing=0;
    s->line_stop_reason=LINE_STOP_NONE;
}
void Control_Start(ControlState *s) { Control_Stop(s,FAULT_NONE); s->mode=CONTROL_TRACK; }
static void LineFault(ControlState *s, LineStopReason reason)
{
    Control_Stop(s,FAULT_LINE); s->line_stop_reason=reason;
}
static void RememberSide(ControlState *s, int8_t side)
{
    s->last_side=side; s->last_side_ms=CONTROL_LAST_SIDE_MS;
}
static void TurnOutput(ControlState *s, float maximum)
{
    float duty=Limit(s->search_near_line?CONTROL_PIVOT_PERCENT:CONTROL_SEARCH_FAST_PERCENT,0,maximum);
    /* No brake/reversal stage on initial confirmed loss: reuse the actual
     * entering common PWM within Max and create yaw without wheel reversal.
     * B33 restores B30's 0.8 arc: both wheels keep a forward request,
     * with entry common PWM preserved whenever Max provides headroom.
     * Count this short arc inside the existing continuous recovery deadline.
     * Valid line cancels the maneuver immediately; there is no minimum turn. */
    if(s->recovery_arc_speed>0 && s->recovery_ms<CONTROL_RECOVERY_ARC_MS) {
        float common=Limit(s->recovery_arc_speed,0,maximum/(1.0f+CONTROL_RECOVERY_ARC_STEERING));
        float correction=s->turn_dir*common*CONTROL_RECOVERY_ARC_STEERING;
        s->left=(int16_t)((common+correction)*10.0f);
        s->right=(int16_t)((common-correction)*10.0f);
        s->speed=s->target=common; s->speed_reason=SPEED_CORNER;
        s->last_correction=correction; s->recovery_arc=1;
        return;
    }
    s->recovery_arc=0;
    /* Confirmed loss: request rotation now, with no forward-common ramp or
     * brake dwell. The motor driver retains its per-wheel reversal guard.
     * A zero common PWM request does not mean zero physical forward speed. */
    s->target=duty; s->speed_reason=SPEED_RECOVERY;
    s->left=(int16_t)((s->turn_dir>0?duty:-duty)*10.0f);
    s->right=-s->left;
    /* Track the requested PWM magnitude; do not carry straight-line duty into RECENTER. */
    s->speed=(Abs(s->left)+Abs(s->right))/20.0f;
}
static void BeginRecovery(ControlState *s, int8_t direction)
{
    /* Post-turn retries keep their old episode budget and pivot fallback.
     * Never renew the forward arc through centered reacquisition flicker. */
    s->recovery_arc_speed=!s->post_turn_ms && s->left>=0 && s->right>=0 &&
        (s->left+s->right)>=CONTROL_RECOVERY_ARC_MIN_PERCENT*20.0f?
        (s->left+s->right)/20.0f:0;
    s->recovery_arc=0;
    /* B24: operator requested direct steering, without a brake-before-turn pulse. */
    s->mode=CONTROL_TURN; s->turn_dir=direction;
    s->speed_bonus=0; s->cruise_ms=0; s->speed_guard_ms=CONTROL_SPEED_GUARD_MS;
    /* A loss just after reacquisition belongs to the same bounded episode. */
    if(!s->post_turn_ms) s->recovery_ms=0;
    s->reacquire_ms=0;
    s->post_turn_ms=0; s->post_turn_dir=0;
    s->guide_valid=0;
    s->probe_continue=0;
    s->curve_valid=0;
    s->corner_pending_ms=0; s->corner_pending_dir=0;
    s->recovery_seen_line=0; s->split_cue_ms=0; s->split_cue_dir=0;
    s->integral=s->derivative=s->planner_rate=0; s->lost=1; s->saturation_ms=0;
    s->brake_ms=0;
    s->corner_gain=0;
    s->corner_inward_ms=s->corner_outward_ms=0; s->corner_releasing=0;
}
static void RecoveryOutput(ControlState *s, float maximum)
{
    /* BRAKE enum is retained for decoding historical logs, never emitted now. */
    s->brake_ms=0; s->mode=CONTROL_TURN;
    TurnOutput(s,maximum);
}
static void SearchLastSideOrStop(ControlState *s, float maximum, LineStopReason reason)
{
    if(s->last_side_ms && s->last_side) {
        s->may_cross_gap=0;
        BeginRecovery(s,s->last_side);
        TurnOutput(s,maximum);
    } else LineFault(s,reason);
}
static void ForwardHold(ControlState *s, float base, float maximum, float dt)
{
    float correction;
    s->target=Limit(base,0,s->speed);
    s->speed=Approach(s->speed,s->target,CONTROL_DECEL_PERCENT_PER_SECOND*dt);
    s->last_correction*=0.85f;
    correction=Limit(s->last_correction,-s->speed,s->speed);
    s->left=(int16_t)(Limit(s->speed+correction,0,maximum)*10.0f);
    s->right=(int16_t)(Limit(s->speed-correction,0,maximum)*10.0f);
}
static void AmbiguousHold(ControlState *s, float base, float maximum, float dt)
{
    float average=(s->guide_left+s->guide_right)/20.0f, scale;
    /* Keep only the last trusted wheel ratio, never the invalid centroid.
     * Shed high straight duty toward the existing tight-curve cap; no boost. */
    s->speed=Limit(s->speed,0,average);
    s->target=Limit(base,0,CONTROL_AMBIGUOUS_CAP_PERCENT);
    s->target=Limit(s->target,0,s->speed);
    s->speed=Approach(s->speed,s->target,CONTROL_DECEL_PERCENT_PER_SECOND*dt);
    scale=average>0?s->speed/average:0;
    s->mode=CONTROL_PROBE; s->speed_reason=SPEED_PROBE;
    s->left=(int16_t)Limit(s->guide_left*scale,0,maximum*10.0f);
    s->right=(int16_t)Limit(s->guide_right*scale,0,maximum*10.0f);
    s->last_correction=(s->left-s->right)/20.0f;
}
static void UpdateDerivative(ControlState *s, float e, float dt, uint32_t dt_ms)
{
    /* Consecutive observed inward movement can lead the filtered planner.
     * This only releases extra corner P; it never changes PID direction,
     * speed, sensor validity or the corner/recovery memory. */
#if CONTROL_CORNER_RETURN_CONFIRM_MS > 0
    if(s->mode==CONTROL_TRACK &&
       s->shape_ms>=CONTROL_SHAPE_CONFIRM_MS && e*s->shape_dir>=1.0f && e*s->last_error>0) {
        float change=Abs(e)-Abs(s->last_error);
        if(change < -CONTROL_CORNER_TREND_STEP_PITCH) {
            s->corner_outward_ms=0;
            if(s->corner_inward_ms<CONTROL_CORNER_RETURN_CONFIRM_MS) s->corner_inward_ms+=(uint16_t)dt_ms;
            if(s->corner_inward_ms>=CONTROL_CORNER_RETURN_CONFIRM_MS) s->corner_releasing=1;
        } else if(change > CONTROL_CORNER_TREND_STEP_PITCH) {
            s->corner_inward_ms=0;
            if(s->corner_outward_ms<CONTROL_CORNER_RETURN_CONFIRM_MS) s->corner_outward_ms+=(uint16_t)dt_ms;
            if(s->corner_outward_ms>=CONTROL_CORNER_RETURN_CONFIRM_MS) s->corner_releasing=0;
        } else s->corner_inward_ms=s->corner_outward_ms=0;
    } else {
        s->corner_inward_ms=s->corner_outward_ms=0; s->corner_releasing=0;
    }
#else
    s->corner_inward_ms=s->corner_outward_ms=0; s->corner_releasing=0;
#endif
    s->derivative+=(dt/(0.015f+dt))*((e-s->last_error)/dt-s->derivative);
    /* Planner rate is separate from the PD derivative. */
    s->planner_rate+=(dt/(CONTROL_SPEED_RATE_FILTER_SECONDS+dt))*(s->derivative-s->planner_rate);
    s->last_error=e;
}
static void RememberGuide(ControlState *s, float e, uint8_t mask, uint32_t dt_ms, float maximum)
{
    /* A genuine edge/outward drift invalidates straight-gap continuation immediately. */
    if(Abs(e)>1.2f || (mask&0xC3U) || Abs(s->planner_rate)>CONTROL_GUIDE_RATE_MAX) {
        s->guide_valid=0; s->stable_ms=0;
        return;
    }
    if(Abs(e)<=CONTROL_GUIDE_CENTER_PITCH && s->left>0 && s->right>0) {
        float correction=(s->left-s->right)/20.0f;
        float previous=s->guide_valid && s->guide_age_ms<=4U?s->gap_correction:0;
        /* Filter only the stored gap direction, not live TRACK PID. A single
         * central-eye D kick must not be held for a whole white dash. */
        if(Abs(correction-previous)>=0.05f) {
            float mean=(s->left+s->right)/20.0f;
            float bound=Limit(mean,0,maximum-mean);
            correction=previous+CONTROL_GAP_STEER_ALPHA_PER_MS*dt_ms*(correction-previous);
            correction=Limit(correction,-bound,bound);
        }
        if(Abs(correction)<0.05f) correction=0; /* Below half one PWM permille. */
        s->gap_correction=correction;
        s->guide_left=s->left; s->guide_right=s->right;
        s->guide_age_ms=0; s->guide_valid=1;
        if(s->stable_ms<1000U) s->stable_ms+=dt_ms;
        if(s->stable_ms>=40U) { s->corner_dir=0; s->corner_hint_ms=0; }
        if(s->stable_ms>=CONTROL_LAST_SIDE_CLEAR_MS) { s->last_side=0; s->last_side_ms=0; }
        if(s->stable_ms>=CONTROL_CORNER_CLEAR_MS) {
            s->corner_pending_ms=0; s->corner_pending_dir=0;
        }
        if(s->stable_ms>=CONTROL_POST_TURN_CLEAR_MS && s->post_turn_ms) {
            s->post_turn_ms=0; s->post_turn_dir=0; s->recovery_ms=0;
        }
    } else s->stable_ms=0;
}
static void HoldGuide(ControlState *s, float base, float maximum)
{
    float average=(s->guide_left+s->guide_right)/20.0f, scale=1.0f;
    /* Short gaps keep exact commands. Longer gaps shed only the excess above
     * Base, preserving the remembered wheel ratio and never accelerating. */
    if(average>base && s->lost_ms>CONTROL_GAP_BONUS_HOLD_MS) {
        float drop=(s->lost_ms-CONTROL_GAP_BONUS_HOLD_MS)*0.001f*CONTROL_GAP_BONUS_DECEL;
        scale=(average-Limit(drop,0,average-base))/average;
    }
    s->mode=CONTROL_GAP; s->speed_reason=SPEED_GAP;
    s->left=(int16_t)Limit((average+s->gap_correction)*10.0f*scale,0,maximum*10.0f);
    s->right=(int16_t)Limit((average-s->gap_correction)*10.0f*scale,0,maximum*10.0f);
    s->speed=(s->left+s->right)/20.0f; s->target=s->speed;
    s->last_correction=(s->left-s->right)/20.0f;
}
static void Follow(ControlState *s, const ControlParameters *p, float e, float target, float maximum, float dt, uint8_t smooth)
{
    float steering_limit, candidate, correction, common, proportional_gain;
    float reverse_limit=0;
    float accel=CONTROL_ACCEL_PERCENT_PER_SECOND;
    /* Startup/rejoin ramp only; ordinary tracking has one fixed Base target.
     * The mixer can lower common duty to preserve steering within Max. */
    float decel=smooth && target>=p->base_percent?
        CONTROL_SOFT_DECEL_PERCENT_PER_SECOND:CONTROL_DECEL_PERCENT_PER_SECOND;
    if(s->post_turn_ms && !s->corner_pending_ms && Abs(e)<=1.0f &&
       s->speed<CONTROL_EXIT_ACCEL_CEILING_PERCENT) accel=CONTROL_EXIT_ACCEL_PERCENT_PER_SECOND;
    s->target=target;
    s->speed=Approach(s->speed,target,(target>s->speed?accel:decel)*dt);
    steering_limit=Limit(s->speed,0,maximum*0.5f);
    /* Start steering harder while the line is still visible, without B24's
     * abrupt correction floor. A cue is heuristic, not proof of a 90deg turn.
     * Ordinary soft curves, improving error and RECENTER keep ordinary PD. */
    if(s->mode==CONTROL_TRACK && s->shape_ms>=CONTROL_SHAPE_CONFIRM_MS &&
       e*s->shape_dir>=1.0f && e*s->planner_rate>=0) {
        float desired=s->corner_releasing?0:CONTROL_CORNER_GAIN_MAX*Limit(Abs(e)-1.0f,0,1);
        float rate=s->corner_releasing?CONTROL_CORNER_GAIN_RELEASE_PER_MS:CONTROL_CORNER_GAIN_RISE_PER_MS;
        s->corner_gain=Approach(s->corner_gain,desired,rate*dt*1000.0f);
    } else s->corner_gain=0;
    proportional_gain=p->kp;
    if(s->corner_gain>0) proportional_gain*=1.0f+s->corner_gain;
    candidate=p->ki>0?Limit(s->integral+e*dt,-10.0f/p->ki,10.0f/p->ki):0;
    correction=proportional_gain*e+p->ki*candidate+p->kd*s->derivative;
    /* B34: owner permits the inner wheel to reverse on sharp visible corners.
     * This extends live PD headroom, without a fixed spin pulse or steering
     * floor. Returning/cleared cues and ALIGN keep the forward-only mixer.
     * Bound both wheels even if Base or Max changes during the maneuver. */
    if(s->mode==CONTROL_TRACK && s->shape_ms>=CONTROL_SHAPE_CONFIRM_MS &&
       e*s->shape_dir>=CONTROL_CORNER_REVERSE_ERROR_PITCH &&
       e*s->planner_rate>=0 && !s->corner_releasing && correction*e>0) {
        reverse_limit=Limit(s->corner_gain,0,1)*Limit(CONTROL_CORNER_REVERSE_MAX_PERCENT,0,maximum);
        steering_limit=Limit(s->speed+reverse_limit,0,(maximum+reverse_limit)*0.5f);
    }
    if(Abs(correction)<=steering_limit || correction*e<0) s->integral=candidate;
    correction=Limit(proportional_gain*e+p->ki*s->integral+p->kd*s->derivative,-steering_limit,steering_limit);
    if(reverse_limit>0 && correction*e<=0) {
        reverse_limit=0;
        steering_limit=Limit(s->speed,0,maximum*0.5f);
        correction=Limit(correction,-steering_limit,steering_limit);
    }
    /* B26: visible line gets its P correction in this frame. D already has a
     * 15 ms low-pass; a second slew on the whole PD delayed ordinary bends. */
    s->last_correction=correction;
    common=Limit(s->speed,0,maximum-Abs(correction));
    s->left=(int16_t)(Limit(common+correction,-reverse_limit,maximum)*10.0f);
    s->right=(int16_t)(Limit(common-correction,-reverse_limit,maximum)*10.0f);
}
/* A split pattern may still supply direction, never a trustworthy centroid.
 * Require three outer-side eyes and at most one opposite-side eye. */
static int8_t SplitDirection(uint8_t mask)
{
    unsigned i, left=0, right=0;
    for(i=0;i<4;++i) { left+=(mask>>i)&1U; right+=(mask>>(i+4U))&1U; }
    if((mask&0x80U) && right>=3 && left<=1) return 1;
    if((mask&0x01U) && left>=3 && right<=1) return -1;
    return 0;
}
static int8_t ConfirmSplit(ControlState *s, uint8_t mask, uint32_t dt_ms)
{
    int8_t cue=SplitDirection(mask);
    if(cue && cue==s->split_cue_dir) {
        if(s->split_cue_ms<CONTROL_LOSS_CONFIRM_MS) s->split_cue_ms+=dt_ms;
    } else { s->split_cue_dir=cue; s->split_cue_ms=cue?dt_ms:0; }
    return s->split_cue_ms>=CONTROL_LOSS_CONFIRM_MS?cue:0;
}
static void ObserveValidShape(ControlState *s, uint8_t valid, uint8_t mask, uint32_t dt_ms)
{
    /* An edge-reaching broad group, not width alone; keep centroid PD authority. */
    int8_t side=(valid && (mask&0x18U))?SplitDirection(mask):0;
    if(side && side==s->shape_dir) {
        if(s->shape_ms<CONTROL_SHAPE_CONFIRM_MS) s->shape_ms+=dt_ms;
    } else { s->shape_dir=side; s->shape_ms=side?dt_ms:0; }
    if(side && s->shape_ms>=CONTROL_SHAPE_CONFIRM_MS) {
        s->corner_pending_dir=side; s->corner_pending_ms=CONTROL_CORNER_PENDING_MS;
        RememberSide(s,side);
    }
}
void Control_Step(ControlState *s, const ControlParameters *p, int16_t error, uint8_t valid, uint8_t mask, uint32_t dt_ms)
{
    float e=error/1000.0f, dt=dt_ms*0.001f, base, maximum;
    uint8_t recovering;
    int8_t split_direction;
    if(s->mode==CONTROL_STOPPED || s->mode==CONTROL_FAULT) return;
    if(dt_ms<1U || dt_ms>4U) { Control_Stop(s,FAULT_TIMING); return; }
    maximum=Limit((float)p->max_percent,0,100);
    base=Limit((float)p->base_percent,0,maximum);
    if(base==0 || maximum==0) { Control_Stop(s,FAULT_NONE); return; }
    s->search_near_line=mask!=0U;
    if(s->curve_age_ms<1000U) s->curve_age_ms+=dt_ms;
    if(s->speed_guard_ms>dt_ms) s->speed_guard_ms-=dt_ms;
    else s->speed_guard_ms=0;
    if(s->guide_age_ms<1000U) s->guide_age_ms+=dt_ms;
    if(s->corner_pending_ms>dt_ms) s->corner_pending_ms-=dt_ms;
    else { s->corner_pending_ms=0; s->corner_pending_dir=0; }
    if(s->post_turn_ms) {
        s->recovery_ms+=dt_ms;
        if(s->post_turn_ms>dt_ms) s->post_turn_ms-=dt_ms;
        else { s->post_turn_ms=0; s->post_turn_dir=0; s->recovery_ms=0; }
    }
    if(s->last_side_ms>dt_ms) s->last_side_ms-=dt_ms;
    else { s->last_side_ms=0; s->last_side=0; }
    if(s->corner_hint_ms>dt_ms) s->corner_hint_ms-=dt_ms;
    else { s->corner_hint_ms=0; s->corner_dir=0; }
    recovering=s->mode==CONTROL_TURN || s->mode==CONTROL_SEARCH || s->mode==CONTROL_ALIGN || s->mode==CONTROL_BRAKE;
    if(recovering) {
        s->recovery_ms+=dt_ms;
        if(s->recovery_ms>=CONTROL_RECOVERY_MAX_MS) {
            LineFault(s,LINE_STOP_TURN_TIMEOUT); return;
        }
    }
    if(valid || mask==0U) { s->split_cue_ms=0; s->split_cue_dir=0; }
    ObserveValidShape(s,valid,mask,dt_ms);
    if(recovering || s->shape_ms>=CONTROL_SHAPE_CONFIRM_MS || (valid && Abs(e)>=2.4f))
        s->speed_guard_ms=CONTROL_SPEED_GUARD_MS;
    if(valid) {
        /* Keep observed side separate from PID error, which can pass through zero. */
        if(Abs(e)>=1.2f) {
            RememberSide(s,e>0?1:-1);
            if(s->corner_pending_ms && !s->shape_dir) s->corner_pending_dir=e>0?1:-1;
        }
        /* A valid extreme error is not a command to run a timed corner routine. */
        if(recovering) {
            float rejoin=s->recovery_arc?base:Limit(base,0,CONTROL_RECENTER_PERCENT);
            s->recovery_seen_line=1;
            /* Retain a genuinely new side even if a weak center sample follows
             * it before the next loss. Weak readings must not undo this cue. */
            if(Abs(e)>=1.2f) s->turn_dir=e>0?1:-1;
            if(s->mode!=CONTROL_ALIGN) {
                s->last_error=e; s->derivative=s->integral=s->planner_rate=0;
                /* Do not discard the current command magnitude by resetting
                 * every reacquisition to 30%. Keep a bounded forward handoff;
                 * a low-duty approach must not jump up to the rejoin ceiling. */
                s->speed=Limit(s->speed,0,rejoin);
            }
            s->brake_ms=0; /* Visible line immediately cancels a brake pulse as well as a pivot. */
            s->mode=CONTROL_ALIGN; /* RECENTER: immediately follow the observed line with forward PD. */
            if(Abs(e)<=1.0f && (mask&0x3CU)) s->reacquire_ms+=dt_ms;
            else s->reacquire_ms=0;
            if(s->reacquire_ms>=(uint32_t)Limit(p->align_ms,CONTROL_REACQUIRE_MIN_MS,CONTROL_REACQUIRE_MAX_MS)) {
                s->last_error=e; s->derivative=s->integral=s->planner_rate=0;
            }
            UpdateDerivative(s,e,dt,dt_ms);
            s->speed_reason=SPEED_RECOVERY;
            Follow(s,p,e,rejoin,maximum,dt,0);
            if(s->reacquire_ms>=(uint32_t)Limit(p->align_ms,CONTROL_REACQUIRE_MIN_MS,CONTROL_REACQUIRE_MAX_MS)) {
                s->mode=CONTROL_TRACK; s->lost=0; s->lost_ms=0;
                s->post_turn_ms=CONTROL_POST_TURN_MS;
                s->post_turn_dir=s->turn_dir;
                s->saturation_ms=0;
                s->corner_hint_ms=0; s->corner_dir=0; s->stable_ms=0;
                s->ambiguous_ms=0; s->may_cross_gap=0;
                /* Publish current forward commands at the handoff, not one frame later. */
                RememberGuide(s,e,mask,dt_ms,maximum);
            }
            return;
        }
        /* Fixed Base: there is no straight bonus to restore after a dash. */
        s->speed_bonus=0; s->cruise_ms=0;
        if(s->lost || !s->history) { s->last_error=e; s->derivative=s->integral=s->planner_rate=0; }
        s->lost=0; s->lost_ms=s->ambiguous_ms=0; s->history=1; s->mode=CONTROL_TRACK;
        s->probe_continue=0;
        if(Abs(e)>=1.2f && (mask&0x81U)) {
            s->corner_dir=e>0?1:-1; s->corner_hint_ms=CONTROL_CORNER_MEMORY_MS;
        }
        UpdateDerivative(s,e,dt,dt_ms);
        {
            float target=base;
            s->speed_reason=SPEED_BASE;
            if(s->shape_ms>=CONTROL_SHAPE_CONFIRM_MS) {
                /* Visible corners keep the same Base target and ordinary PD. */
                s->speed_reason=SPEED_CORNER;
            }
            if(s->post_turn_ms && !s->recovery_arc && s->stable_ms<CONTROL_REJOIN_CENTER_MS) {
                target=Limit(target,0,CONTROL_REJOIN_CAP_PERCENT);
                s->speed_reason=SPEED_RECOVERY;
            }
            Follow(s,p,e,target,maximum,dt,
                   !s->corner_pending_ms && !s->post_turn_ms && !(mask&0x81U) &&
                   Abs(e)<=1.5f && Abs(s->planner_rate)<=4.0f);
        }
        RememberGuide(s,e,mask,dt_ms,maximum);
        /* Separate from the long straight-gap guide: this snapshot tolerates
         * only a few milliseconds of missing/fragmented moderate curve data. */
        s->curve_valid=!s->corner_pending_ms && !s->post_turn_ms && !s->corner_hint_ms &&
            !(mask&0x81U) && Abs(e)<=2.0f && Abs(s->planner_rate)<=4.0f && s->left>0 && s->right>0;
        if(s->curve_valid) {
            s->curve_left=s->left; s->curve_right=s->right; s->curve_age_ms=0;
        }
        /* Long saturation with no improving edge error is different from a normal curve sample. */
        if(Abs(e)>=2.4f && (mask&0x81U) && (s->left==0 || s->right==0) && s->speed>=5.0f) {
            if(!s->saturation_ms || Abs(e)<s->saturation_error-0.2f) {
                s->saturation_ms=dt_ms; s->saturation_error=Abs(e);
            } else s->saturation_ms+=dt_ms;
            if(s->saturation_ms>5000U) s->saturation_ms=5000U;
            /* Diagnostic only: a sustained valid curve must not be reclassified as a corner. */
        } else s->saturation_ms=0;
        return;
    }
    if(!s->history) { LineFault(s,LINE_STOP_NO_HISTORY); return; }
    if(s->post_turn_ms && s->recovery_ms>=CONTROL_RECOVERY_MAX_MS) {
        LineFault(s,LINE_STOP_TURN_TIMEOUT); return;
    }
    s->integral=0;
    s->corner_gain=0;
    s->corner_inward_ms=s->corner_outward_ms=0; s->corner_releasing=0;
    s->cruise_ms=0; s->speed_bonus=0;
    if(!s->lost) {
        /* Latch once per continuous loss. Center flicker must earn a new
         * trusted 40 ms guide before another allowance; corner chains bypass. */
        s->probe_continue=!recovering && !s->post_turn_ms && !s->corner_pending_ms &&
            !s->corner_hint_ms && !s->speed_guard_ms && s->guide_valid &&
            s->guide_age_ms<=CONTROL_AMBIGUOUS_GUIDE_AGE_MS &&
            s->stable_ms>=CONTROL_AMBIGUOUS_CENTER_MS && Abs(s->last_error)<=CONTROL_GUIDE_CENTER_PITCH &&
            Abs(s->planner_rate)<=4.0f && s->guide_left>0 && s->guide_right>0 &&
            (mask&0x18U)==0x18U && !SplitDirection(mask);
    }
    s->stable_ms=0;
    split_direction=ConfirmSplit(s,mask,dt_ms);
    if(recovering) {
        if(s->mode==CONTROL_ALIGN && Abs(s->last_error)>=1.2f) s->turn_dir=s->last_error>0?1:-1;
        /* A new side after visible reacquisition may be the next bend.
         * Retarget within this recovery episode; never restart its deadline. */
        if(s->recovery_seen_line && split_direction) {
            RememberSide(s,split_direction);
            s->turn_dir=split_direction;
            s->corner_dir=split_direction; s->corner_hint_ms=CONTROL_CORNER_MEMORY_MS;
        }
        s->mode=CONTROL_TURN;
        s->reacquire_ms=0;
        RecoveryOutput(s,maximum); return;
    }
    if(!s->lost) {
        s->lost=1; s->lost_ms=0;
        s->may_cross_gap=!s->post_turn_ms && !s->corner_pending_ms && s->guide_valid && s->guide_age_ms<=CONTROL_GUIDE_MAX_AGE_MS;
    }
    s->lost_ms+=dt_ms;
    if((mask&0x18U)!=0x18U || s->lost_ms>CONTROL_AMBIGUOUS_BRIDGE_MS)
        s->probe_continue=0;
    /* ConfirmSplit has already observed this frame. Edge/corner/post-turn data
     * bypass this bridge, and the ordinary loss/deadline counters never reset. */
    if(!s->may_cross_gap && s->curve_valid && !s->corner_pending_ms && !s->post_turn_ms &&
       !s->corner_hint_ms && !split_direction && !(mask&0x81U) &&
       s->curve_age_ms<=CONTROL_CURVE_SNAPSHOT_MAX_AGE_MS &&
       s->lost_ms<=(mask?CONTROL_CURVE_BRIDGE_MS:CONTROL_CURVE_WHITE_BRIDGE_MS)) {
        s->mode=CONTROL_PROBE; s->speed_reason=SPEED_PROBE;
        s->left=(int16_t)Limit(s->curve_left,0,maximum*10.0f);
        s->right=(int16_t)Limit(s->curve_right,0,maximum*10.0f);
        s->speed=(s->left+s->right)/20.0f; s->target=s->speed;
        if(mask) s->ambiguous_ms+=dt_ms;
        else s->ambiguous_ms=0;
        return;
    }
    if(mask!=0U) {
        s->ambiguous_ms+=dt_ms;
        if(split_direction) {
            RememberSide(s,split_direction);
            s->may_cross_gap=0; s->guide_valid=0;
            s->corner_dir=split_direction; s->corner_hint_ms=CONTROL_CORNER_MEMORY_MS;
            BeginRecovery(s,split_direction);
            TurnOutput(s,maximum);
            return;
        }
        /* Opposite split evidence may be the next corner: let its existing
         * 6 ms confirmation finish before committing to the older heading. */
        if(s->corner_pending_ms &&
           (!s->split_cue_dir || s->split_cue_dir==s->corner_pending_dir)) {
            s->may_cross_gap=0;
            BeginRecovery(s,s->corner_pending_dir);
            TurnOutput(s,maximum);
            return;
        }
        /* Brief central split masks at a dash boundary must not collapse duty to PROBE 15%. */
        if(s->may_cross_gap && !(mask&0x81U) &&
           s->ambiguous_ms<=CONTROL_GAP_TRANSITION_MS && s->lost_ms<=CONTROL_GAP_MAX_MS) {
            HoldGuide(s,base,maximum); return;
        }
        s->may_cross_gap=0; s->guide_valid=0;
        if(s->probe_continue) {
            if(s->lost_ms<=CONTROL_AMBIGUOUS_BRIDGE_MS && (mask&0x18U)==0x18U &&
               !s->post_turn_ms && !s->corner_pending_ms && !s->corner_hint_ms) {
                AmbiguousHold(s,base,maximum,dt); return;
            }
            s->probe_continue=0;
        }
        if(s->ambiguous_ms>=CONTROL_AMBIGUOUS_MAX_MS) {
            if(s->last_side_ms && s->last_side) {
                SearchLastSideOrStop(s,maximum,LINE_STOP_AMBIGUOUS);
            } else if(s->corner_hint_ms && s->corner_dir) {
                BeginRecovery(s,s->corner_dir);
                TurnOutput(s,maximum);
            } else SearchLastSideOrStop(s,maximum,LINE_STOP_AMBIGUOUS);
        } else { s->mode=CONTROL_PROBE; s->speed_reason=SPEED_PROBE; ForwardHold(s,Limit(base,0,15.0f),maximum,dt); }
        return;
    }
    s->probe_continue=0; /* White loss never borrows the ambiguous-pattern bridge. */
    s->ambiguous_ms=0;
    if(s->may_cross_gap) {
        if(s->lost_ms<=CONTROL_GAP_MAX_MS) HoldGuide(s,base,maximum);
        else SearchLastSideOrStop(s,maximum,LINE_STOP_GAP_TIMEOUT);
        return;
    }
    /* A corner already confirmed on visible line needs no second loss delay.
     * A fresh strong opposite observation updates pending_dir above. */
    if(s->corner_pending_ms) {
        s->may_cross_gap=0;
        BeginRecovery(s,s->corner_pending_dir);
        TurnOutput(s,maximum);
        return;
    }
    if(s->lost_ms<CONTROL_LOSS_CONFIRM_MS) {
        s->mode=CONTROL_PROBE; s->speed_reason=SPEED_PROBE; ForwardHold(s,base,maximum,dt); return;
    }
    {
        int8_t direction=0;
        /* A central eye at +/-0.5 pitch must not reverse a remembered turn.
         * Strong new visible side or a confirmed split can still retarget. */
        if(s->post_turn_ms) {
            direction=s->last_side_ms?s->last_side:s->post_turn_dir;
        } else if(Abs(s->last_error)>=1.2f) direction=s->last_error>0?1:-1;
        else if(s->last_side_ms) direction=s->last_side;
        else if(s->corner_hint_ms) direction=s->corner_dir;
        else if(Abs(s->last_error)>0.35f) direction=s->last_error>0?1:-1;
        if(!direction) { SearchLastSideOrStop(s,maximum,LINE_STOP_GAP_TIMEOUT); return; }
        BeginRecovery(s,direction);
        TurnOutput(s,maximum);
    }
}
