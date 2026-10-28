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
    p->base_percent=55; p->straight_percent=60; p->max_percent=85;
    p->advance_ms=0; p->align_ms=CONTROL_REACQUIRE_DEFAULT_MS;
}
void Control_Stop(ControlState *s, ControlFault fault)
{
    s->mode=fault==FAULT_NONE?CONTROL_STOPPED:CONTROL_FAULT; s->fault=fault;
    s->speed=s->integral=s->derivative=s->last_error=s->last_correction=s->saturation_error=0;
    s->target=s->planner_rate=0; s->speed_reason=SPEED_IDLE;
    s->speed_bonus=0; s->cruise_ms=s->speed_guard_ms=0;
    s->fast_history_ms=s->entry_brake_ms=0; s->entry_latched=0;
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
static void Pivot(ControlState *s, float maximum, float dt)
{
    float duty=Limit(CONTROL_PIVOT_PERCENT,0,maximum);
    float left=s->turn_dir>0?duty:-duty;
    float right=-left;
    float step=CONTROL_TURN_RAMP_PERCENT_PER_SECOND*dt;
    s->target=duty; s->speed_reason=SPEED_RECOVERY;
    s->left=(int16_t)(Approach(s->left/10.0f,left,step)*10.0f);
    s->right=(int16_t)(Approach(s->right/10.0f,right,step)*10.0f);
    /* Track the requested PWM magnitude; do not carry straight-line duty into RECENTER. */
    s->speed=(Abs(s->left)+Abs(s->right))/20.0f;
}
static void BeginRecovery(ControlState *s, int8_t direction)
{
    uint8_t fast=s->speed>CONTROL_BRAKE_ENTRY_PERCENT ||
                 (s->left+s->right)/20.0f>CONTROL_BRAKE_ENTRY_PERCENT;
    s->mode=fast?CONTROL_BRAKE:CONTROL_TURN; s->turn_dir=direction;
    s->fast_history_ms=s->entry_brake_ms=0;
    s->speed_bonus=0; s->cruise_ms=0; s->speed_guard_ms=CONTROL_SPEED_GUARD_MS;
    /* A loss just after reacquisition belongs to the same bounded episode. */
    if(!s->post_turn_ms) s->recovery_ms=0;
    s->reacquire_ms=0;
    s->post_turn_ms=0; s->post_turn_dir=0;
    s->guide_valid=0;
    s->curve_valid=0;
    s->corner_pending_ms=0; s->corner_pending_dir=0;
    s->recovery_seen_line=0; s->split_cue_ms=0; s->split_cue_dir=0;
    s->integral=s->derivative=s->planner_rate=0; s->lost=1; s->saturation_ms=0;
    s->brake_ms=fast?CONTROL_BRAKE_PULSE_MS:0;
    if(fast) {
        if(s->brake_count<65535U) ++s->brake_count;
        s->left=s->right=0; s->speed=0;
        s->target=0; s->speed_reason=SPEED_BRAKE;
    }
}
static void RecoveryOutput(ControlState *s, float maximum, uint32_t dt_ms)
{
    if(s->mode==CONTROL_BRAKE && s->brake_ms>dt_ms) {
        s->brake_ms-=dt_ms;
        s->left=s->right=0;
        return;
    }
    s->brake_ms=0; s->mode=CONTROL_TURN;
    Pivot(s,maximum,dt_ms*0.001f);
}
static void SearchLastSideOrStop(ControlState *s, float maximum, float dt, LineStopReason reason)
{
    if(s->last_side_ms && s->last_side) {
        s->may_cross_gap=0;
        BeginRecovery(s,s->last_side);
        if(s->mode!=CONTROL_BRAKE) Pivot(s,maximum,dt);
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
static void UpdateDerivative(ControlState *s, float e, float dt)
{
    s->derivative+=(dt/(0.015f+dt))*((e-s->last_error)/dt-s->derivative);
    /* Planner rate is separate from the PD derivative. */
    s->planner_rate+=(dt/(CONTROL_SPEED_RATE_FILTER_SECONDS+dt))*(s->derivative-s->planner_rate);
    s->last_error=e;
}
static float CruiseTarget(ControlState *s, float e, float base, float straight, uint8_t mask, uint32_t dt_ms)
{
    float desired=0, available, confidence, outward, risk, target;
    float blend=Limit((Abs(e)-CONTROL_CURVE_BLEND_START_PITCH)/
                      (CONTROL_CURVE_BLEND_END_PITCH-CONTROL_CURVE_BLEND_START_PITCH),0,1);
    float tight=Limit(base,0,CONTROL_TIGHT_CURVE_CAP_PERCENT);
    /* Stable off-center tracking is different from rapidly escaping outward.
     * Keep moderate curves fast; react sooner to outward motion without a
     * full brake or a timed pivot while a valid line is visible. */
    outward=Abs(e)>CONTROL_GUIDE_CENTER_PITCH && e*s->planner_rate>0?Abs(s->planner_rate):0;
    risk=0;
    if(outward>CONTROL_OUTWARD_RATE_START)
        risk=Limit((outward-CONTROL_OUTWARD_RATE_START)/CONTROL_OUTWARD_RATE_SPAN,0,1)*
             Limit((Abs(e)-CONTROL_GUIDE_CENTER_PITCH)/1.2f,0,1);
    /* Quantized central eyes (+/-0.5 pitch) remain eligible. No hard rate gate
     * on each frame: only persistent visible tracking earns the bonus. */
    if(s->speed_guard_ms || s->corner_pending_ms || s->post_turn_ms ||
       (mask&0xC3U) || Abs(e)>CONTROL_STRAIGHT_CENTER_PITCH) s->cruise_ms=0;
    else if(s->cruise_ms<CONTROL_CRUISE_CONFIRM_MS) s->cruise_ms+=dt_ms;
    if(s->cruise_ms>=CONTROL_CRUISE_CONFIRM_MS) {
        available=straight-base;
        confidence=1.0f-Limit((Abs(e)-1.0f),0,1);
        confidence*=1.0f-Limit((outward-4.0f)/8.0f,0,1);
        desired=available*confidence;
    }
    s->speed_bonus=Approach(s->speed_bonus,desired,
        (desired>s->speed_bonus?CONTROL_BONUS_RISE_PER_SECOND:CONTROL_BONUS_FALL_PER_SECOND)*dt_ms*0.001f);
    s->speed_bonus=Limit(s->speed_bonus,0,straight-base);
    /* Corner protection removes extra speed immediately; the existing output
     * ramp still governs duty. Do not modify B14's forward PD/recovery choice. */
    if(s->speed_guard_ms || s->corner_pending_ms || s->post_turn_ms || (mask&0x81U)) s->speed_bonus=0;
    target=base+(tight-base)*blend+s->speed_bonus*(1.0f-blend);
    if(risk>0) target=Limit(target,0,base+(tight-base)*risk);
    s->speed_reason=blend>0 || risk>0?SPEED_CURVE:SPEED_BASE;
    if(s->speed_bonus>0 && !blend && !risk) s->speed_reason=SPEED_BOOST;
    return target;
}
static void RememberGuide(ControlState *s, float e, uint8_t mask, uint32_t dt_ms)
{
    /* A genuine edge/outward drift invalidates straight-gap continuation immediately. */
    if(Abs(e)>1.2f || (mask&0xC3U) || Abs(s->planner_rate)>CONTROL_GUIDE_RATE_MAX) {
        s->guide_valid=0; s->stable_ms=0;
        return;
    }
    if(Abs(e)<=CONTROL_GUIDE_CENTER_PITCH && s->left>0 && s->right>0) {
        s->guide_left=s->left; s->guide_right=s->right;
        s->guide_age_ms=0; s->guide_valid=1;
        if(s->stable_ms<1000U) s->stable_ms+=dt_ms;
        if(s->stable_ms>=40U) { s->corner_dir=0; s->corner_hint_ms=0; }
        if(s->stable_ms>=CONTROL_LAST_SIDE_CLEAR_MS) { s->last_side=0; s->last_side_ms=0; }
        if(s->stable_ms>=CONTROL_CORNER_CLEAR_MS) {
            s->corner_pending_ms=0; s->corner_pending_dir=0; s->entry_latched=0;
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
    s->left=(int16_t)Limit(s->guide_left*scale,0,maximum*10.0f);
    s->right=(int16_t)Limit(s->guide_right*scale,0,maximum*10.0f);
    s->speed=(s->left+s->right)/20.0f; s->target=s->speed;
    s->last_correction=(s->left-s->right)/20.0f;
}
static void Follow(ControlState *s, const ControlParameters *p, float e, float target, float maximum, float dt, uint8_t smooth)
{
    float steering_limit, candidate, correction, common;
    float accel=CONTROL_ACCEL_PERCENT_PER_SECOND;
    if(s->post_turn_ms && !s->corner_pending_ms && Abs(e)<=1.0f &&
       s->speed<CONTROL_EXIT_ACCEL_CEILING_PERCENT) accel=CONTROL_EXIT_ACCEL_PERCENT_PER_SECOND;
    s->target=target;
    s->speed=Approach(s->speed,target,(target>s->speed?accel:CONTROL_DECEL_PERCENT_PER_SECOND)*dt);
    steering_limit=Limit(s->speed,0,maximum*0.5f);
    candidate=p->ki>0?Limit(s->integral+e*dt,-10.0f/p->ki,10.0f/p->ki):0;
    correction=p->kp*e+p->ki*candidate+p->kd*s->derivative;
    if(Abs(correction)<=steering_limit || correction*e<0) s->integral=candidate;
    correction=Limit(p->kp*e+p->ki*s->integral+p->kd*s->derivative,-steering_limit,steering_limit);
    /* Limit brief steering kicks only inside a trusted moderate track.
     * Edge/outward/corner recovery keeps immediate full PD authority. */
    if(smooth) {
        /* Release excess steering immediately as the line returns. A sign
         * reversal starts from zero rather than holding the old turn direction. */
        float previous=correction*s->last_correction<0?0:s->last_correction;
        if(Abs(correction)>Abs(previous))
            correction=Approach(previous,correction,CONTROL_SOFT_STEERING_SLEW*dt);
        correction=Limit(correction,-steering_limit,steering_limit);
    }
    s->last_correction=correction;
    common=Limit(s->speed,0,maximum-Abs(correction));
    s->left=(int16_t)(Limit(common+correction,0,maximum)*10.0f);
    s->right=(int16_t)(Limit(common-correction,0,maximum)*10.0f);
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
    if(s->fast_history_ms>dt_ms) s->fast_history_ms-=dt_ms;
    else s->fast_history_ms=0;
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
    if(s->mode==CONTROL_ENTRY_BRAKE) {
        if(!valid) {
            /* Carry only the remainder into normal recovery, not a second
             * brake pulse. The entry pulse already consumed part of the budget. */
            uint32_t remaining=s->entry_brake_ms;
            int8_t side=s->corner_pending_dir?s->corner_pending_dir:s->last_side;
            if(!side) { LineFault(s,LINE_STOP_NO_HISTORY); return; }
            BeginRecovery(s,side);
            s->recovery_ms=CONTROL_ENTRY_BRAKE_MS-remaining+dt_ms;
            s->mode=CONTROL_BRAKE; s->brake_ms=remaining;
            RecoveryOutput(s,maximum,dt_ms); return;
        }
        if(s->shape_ms>=CONTROL_SHAPE_CONFIRM_MS && s->shape_dir==s->turn_dir && s->entry_brake_ms>dt_ms) {
            s->entry_brake_ms-=dt_ms;
            s->left=s->right=0; s->speed=s->target=0;
            return;
        }
        /* Visible ordinary line cancels early; persistent shape resumes PD
         * after the bounded pulse. Never pivot on valid line data. */
        s->entry_brake_ms=0; s->mode=CONTROL_TRACK;
        s->speed=Limit(base,0,CONTROL_CORNER_CAP_PERCENT);
        s->last_error=e; s->integral=s->derivative=s->planner_rate=s->last_correction=0;
    }
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
            s->recovery_seen_line=1;
            if(s->mode!=CONTROL_ALIGN) {
                s->last_error=e; s->derivative=s->integral=s->planner_rate=0;
                s->speed=Limit(base,0,CONTROL_RECENTER_PERCENT);
            }
            s->brake_ms=0; /* Visible line immediately cancels a brake pulse as well as a pivot. */
            s->mode=CONTROL_ALIGN; /* RECENTER: immediately follow the observed line with forward PD. */
            if(Abs(e)<=1.0f && (mask&0x3CU)) s->reacquire_ms+=dt_ms;
            else s->reacquire_ms=0;
            if(s->reacquire_ms>=(uint32_t)Limit(p->align_ms,CONTROL_REACQUIRE_MIN_MS,CONTROL_REACQUIRE_MAX_MS)) {
                s->last_error=e; s->derivative=s->integral=s->planner_rate=0;
            }
            UpdateDerivative(s,e,dt);
            s->speed_reason=SPEED_RECOVERY;
            Follow(s,p,e,Limit(base,0,CONTROL_RECENTER_PERCENT),maximum,dt,0);
            if(s->reacquire_ms>=(uint32_t)Limit(p->align_ms,CONTROL_REACQUIRE_MIN_MS,CONTROL_REACQUIRE_MAX_MS)) {
                s->mode=CONTROL_TRACK; s->lost=0; s->lost_ms=0;
                s->post_turn_ms=CONTROL_POST_TURN_MS;
                s->post_turn_dir=s->turn_dir;
                s->saturation_ms=0;
                s->corner_hint_ms=0; s->corner_dir=0; s->stable_ms=0;
                s->ambiguous_ms=0; s->may_cross_gap=0;
                /* Publish current forward commands at the handoff, not one frame later. */
                RememberGuide(s,e,mask,dt_ms);
            }
            return;
        }
        /* A short trusted dash does not discard already earned speed on black
         * reacquisition. Longer gaps have already removed the excess. */
        if(s->mode==CONTROL_GAP && s->speed>base && Abs(e)<=CONTROL_GUIDE_CENTER_PITCH && !(mask&0xC3U)) {
            s->speed_bonus=Limit(s->speed-base,0,Limit((float)p->straight_percent,base,maximum)-base);
            s->cruise_ms=CONTROL_CRUISE_CONFIRM_MS;
        }
        if(s->lost || !s->history) { s->last_error=e; s->derivative=s->integral=s->planner_rate=0; }
        s->lost=0; s->lost_ms=s->ambiguous_ms=0; s->history=1; s->mode=CONTROL_TRACK;
        if(Abs(e)>=1.2f && (mask&0x81U)) {
            s->corner_dir=e>0?1:-1; s->corner_hint_ms=CONTROL_CORNER_MEMORY_MS;
        }
        UpdateDerivative(s,e,dt);
        {
            float target=CruiseTarget(s,e,base,Limit((float)p->straight_percent,base,maximum),mask,dt_ms);
            if(s->shape_ms>=CONTROL_SHAPE_CONFIRM_MS) {
                target=Limit(target,0,CONTROL_CORNER_CAP_PERCENT);
                s->speed_reason=SPEED_CORNER;
                if(!s->entry_latched && s->fast_history_ms && !s->post_turn_ms &&
                   (s->left+s->right)/20.0f>=CONTROL_ENTRY_BRAKE_PERCENT) {
                    s->entry_latched=1; s->entry_brake_ms=CONTROL_ENTRY_BRAKE_MS;
                    s->turn_dir=s->shape_dir; s->mode=CONTROL_ENTRY_BRAKE;
                    s->fast_history_ms=0; s->guide_valid=s->curve_valid=s->may_cross_gap=0;
                    s->stable_ms=s->cruise_ms=0; s->speed_bonus=0;
                    s->left=s->right=0; s->speed=s->target=0;
                    s->speed_reason=SPEED_BRAKE;
                    if(s->brake_count<65535U) ++s->brake_count;
                    return;
                }
            }
            if(s->post_turn_ms && s->stable_ms<CONTROL_REJOIN_CENTER_MS) {
                target=Limit(target,0,CONTROL_REJOIN_CAP_PERCENT);
                s->speed_reason=SPEED_RECOVERY;
            }
            Follow(s,p,e,target,maximum,dt,
                   !s->corner_pending_ms && !s->post_turn_ms && !(mask&0x81U) &&
                   Abs(e)<=1.5f && Abs(s->planner_rate)<=4.0f);
        }
        if(s->cruise_ms>=CONTROL_CRUISE_CONFIRM_MS && Abs(e)<=CONTROL_STRAIGHT_CENTER_PITCH &&
           (s->left+s->right)/20.0f>=CONTROL_ENTRY_BRAKE_PERCENT)
            s->fast_history_ms=CONTROL_FAST_HISTORY_MS;
        RememberGuide(s,e,mask,dt_ms);
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
    s->cruise_ms=0; s->speed_bonus=0;
    s->stable_ms=0;
    split_direction=ConfirmSplit(s,mask,dt_ms);
    if(recovering) {
        if(s->mode==CONTROL_ALIGN && Abs(s->last_error)>0.35f) s->turn_dir=s->last_error>0?1:-1;
        /* A new side after visible reacquisition may be the next bend.
         * Retarget within this recovery episode; never restart its deadline. */
        if(s->recovery_seen_line && split_direction) {
            RememberSide(s,split_direction);
            s->turn_dir=split_direction;
            s->corner_dir=split_direction; s->corner_hint_ms=CONTROL_CORNER_MEMORY_MS;
        }
        if(s->mode!=CONTROL_BRAKE) s->mode=CONTROL_TURN;
        s->reacquire_ms=0;
        RecoveryOutput(s,maximum,dt_ms); return;
    }
    if(!s->lost) {
        s->lost=1; s->lost_ms=0;
        s->may_cross_gap=!s->post_turn_ms && !s->corner_pending_ms && s->guide_valid && s->guide_age_ms<=CONTROL_GUIDE_MAX_AGE_MS;
    }
    s->lost_ms+=dt_ms;
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
            if(s->mode!=CONTROL_BRAKE) Pivot(s,maximum,dt);
            return;
        }
        if(s->corner_pending_ms && s->lost_ms>=CONTROL_LOSS_CONFIRM_MS) {
            s->may_cross_gap=0;
            BeginRecovery(s,s->corner_pending_dir);
            if(s->mode!=CONTROL_BRAKE) Pivot(s,maximum,dt);
            return;
        }
        /* Brief central split masks at a dash boundary must not collapse duty to PROBE 15%. */
        if(s->may_cross_gap && !(mask&0x81U) &&
           s->ambiguous_ms<=CONTROL_GAP_TRANSITION_MS && s->lost_ms<=CONTROL_GAP_MAX_MS) {
            HoldGuide(s,base,maximum); return;
        }
        s->may_cross_gap=0; s->guide_valid=0;
        if(s->ambiguous_ms>=CONTROL_AMBIGUOUS_MAX_MS) {
            if(s->corner_hint_ms && s->corner_dir) {
                BeginRecovery(s,s->corner_dir);
                if(s->mode!=CONTROL_BRAKE) Pivot(s,maximum,dt);
            } else SearchLastSideOrStop(s,maximum,dt,LINE_STOP_AMBIGUOUS);
        } else { s->mode=CONTROL_PROBE; s->speed_reason=SPEED_PROBE; ForwardHold(s,Limit(base,0,15.0f),maximum,dt); }
        return;
    }
    s->ambiguous_ms=0;
    if(s->may_cross_gap) {
        if(s->lost_ms<=CONTROL_GAP_MAX_MS) HoldGuide(s,base,maximum);
        else SearchLastSideOrStop(s,maximum,dt,LINE_STOP_GAP_TIMEOUT);
        return;
    }
    if(s->lost_ms<CONTROL_LOSS_CONFIRM_MS) {
        s->mode=CONTROL_PROBE; s->speed_reason=SPEED_PROBE; ForwardHold(s,base,maximum,dt); return;
    }
    {
        int8_t direction=Abs(s->last_error)>0.35f?(s->last_error>0?1:-1):
                         s->corner_hint_ms?s->corner_dir:0;
        if(!direction && s->corner_pending_ms) direction=s->corner_pending_dir;
        if(s->post_turn_ms) {
            if(s->recovery_ms>=CONTROL_RECOVERY_MAX_MS) {
                LineFault(s,LINE_STOP_TURN_TIMEOUT); return;
            }
            if(!direction) direction=s->last_side_ms?s->last_side:s->post_turn_dir;
        }
        if(!direction) { SearchLastSideOrStop(s,maximum,dt,LINE_STOP_GAP_TIMEOUT); return; }
        BeginRecovery(s,direction);
        if(s->mode!=CONTROL_BRAKE) Pivot(s,maximum,dt);
    }
}
