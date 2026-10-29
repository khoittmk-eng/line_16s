#include "blackbox.h"
#include "log_storage.h"
#include "log_serial.h"
#include "board.h"
#include <stddef.h>
#include <string.h>
#define LOG_MAGIC 0x32424C46UL
#define LOG_VERSION 1U
#define LOG_COMMIT 0xA55AU
typedef struct {
    uint32_t magic;
    uint16_t version, sample_bytes;
    uint32_t sequence;
    uint16_t count, trigger_index;
    uint32_t start_ms, trigger_ms, end_ms;
    float kp, ki, kd;
    uint16_t base, straight, maximum, align_ms;
    uint16_t low[8], high[8], white[8];
    char firmware[4];
    int8_t steering_sign;
    uint8_t polarity, trigger, end_fault;
    uint32_t crc;
} LogHeader;
typedef char SampleSizeCheck[sizeof(BlackboxSample)==48U?1:-1];
typedef char HeaderSizeCheck[sizeof(LogHeader)==108U?1:-1];
typedef char FirmwareTagSizeCheck[sizeof(BOARD_FIRMWARE_TAG)<=4U?1:-1];
typedef char SlotSizeCheck[sizeof(LogHeader)+sizeof(BlackboxSample)*BLACKBOX_CAPACITY<LOG_SLOT_BYTES-2U?1:-1];
static BlackboxSample samples[BLACKBOX_CAPACITY];
static LogHeader header;
static BlackboxState state;
static BlackboxTrigger trigger;
static uint16_t head, count, post_count;
static uint32_t last_sample_ms, trigger_ms;
static uint8_t have_sample, ram_saved;
static int saved_slot;
static uint32_t saved_sequence;
static uint32_t LogCRC(uint32_t crc, const void *ptr, unsigned bytes)
{
    const uint8_t *p=(const uint8_t *)ptr;
    unsigned i;
    while(bytes--) {
        crc^=*p++;
        for(i=0;i<8;++i) crc=(crc>>1)^((crc&1U)?0xEDB88320UL:0);
    }
    return crc;
}
static unsigned First(void) { return (head+BLACKBOX_CAPACITY-count)%BLACKBOX_CAPACITY; }
static const LogHeader *Saved(unsigned slot)
{
    return (const LogHeader *)LogStorage_Slot(slot);
}
static uint8_t ValidSlot(unsigned slot)
{
    const LogHeader *h=Saved(slot);
    const uint8_t *p=(const uint8_t *)h;
    uint32_t crc;
    if(!h || *(const uint16_t *)(p+LOG_SLOT_BYTES-2U)!=LOG_COMMIT) return 0;
    if(h->magic!=LOG_MAGIC || h->version!=LOG_VERSION || h->sample_bytes!=sizeof(BlackboxSample) ||
       !h->count || h->count>BLACKBOX_CAPACITY || h->firmware[3]!=0 ||
       (h->trigger_index!=0xFFFFU && h->trigger_index>=h->count)) return 0;
    crc=LogCRC(0xFFFFFFFFUL,h,offsetof(LogHeader,crc));
    crc=LogCRC(crc,p+sizeof(LogHeader),h->count*sizeof(BlackboxSample));
    return (crc^0xFFFFFFFFUL)==h->crc;
}
void Blackbox_Init(void)
{
    unsigned i;
    saved_slot=-1; saved_sequence=0;
    for(i=0;i<LOG_SLOT_COUNT;++i) if(ValidSlot(i)) {
        if(saved_slot<0 || (int32_t)(Saved(i)->sequence-saved_sequence)>0) {
            saved_slot=(int)i; saved_sequence=Saved(i)->sequence;
        }
    }
    Blackbox_Arm(LOG_TRIGGER_LOSS);
}
void Blackbox_Arm(BlackboxTrigger which)
{
    trigger=which==LOG_TRIGGER_CORNER?LOG_TRIGGER_CORNER:LOG_TRIGGER_LOSS;
    state=LOG_ARMED; head=count=post_count=0; have_sample=ram_saved=0;
    memset(&header,0,sizeof(header)); header.trigger_index=0xFFFFU;
}
void Blackbox_Start(uint32_t now, const ControlParameters *p, int8_t sign,
                    const uint16_t *low, const uint16_t *high, const uint16_t *white, uint8_t polarity)
{
    unsigned i;
    /* Preserve unsaved/frozen RAM across later runs until explicit ARM. */
    if(state!=LOG_ARMED) return;
    header.magic=LOG_MAGIC; header.version=LOG_VERSION; header.sample_bytes=sizeof(BlackboxSample);
    header.start_ms=now; header.kp=p->kp; header.ki=p->ki; header.kd=p->kd;
    header.base=p->base_percent; header.straight=p->straight_percent;
    header.maximum=p->max_percent; header.align_ms=p->align_ms;
    for(i=0;i<8;++i) { header.low[i]=low[i]; header.high[i]=high[i]; header.white[i]=white[i]; }
    memcpy(header.firmware,BOARD_FIRMWARE_TAG,sizeof(BOARD_FIRMWARE_TAG));
    header.steering_sign=sign; header.polarity=polarity; header.trigger=(uint8_t)trigger;
    state=LOG_RECORDING;
}
static int16_t Scaled(float f)
{
    if(f>32767.0f) return 32767;
    if(f< -32768.0f) return -32768;
    return (int16_t)f;
}
static void Push(const BlackboxSample *sample)
{
    samples[head]=*sample; head=(head+1U)%BLACKBOX_CAPACITY;
    if(count<BLACKBOX_CAPACITY) ++count;
    if(state==LOG_RECORDING && count>BLACKBOX_PRE_SAMPLES) count=BLACKBOX_PRE_SAMPLES;
}
void Blackbox_Record(uint32_t now, const uint16_t *raw, int16_t error, uint8_t mask,
                     uint8_t valid, const ControlState *s)
{
    BlackboxSample v;
    uint8_t event;
    unsigned i;
    if(state!=LOG_RECORDING && state!=LOG_POST) return;
    event=s->mode==CONTROL_BRAKE || s->mode==CONTROL_TURN || s->mode==CONTROL_FAULT;
    if(trigger==LOG_TRIGGER_CORNER) {
        /* Avoid freezing CORNER on a directionless startup crossing. */
        if(s->corner_pending_ms || (valid && (error>=1500 || error<=-1500))) event=1;
    } else if(!valid && s->mode!=CONTROL_GAP) event=1;
    /* Transition/trigger frames bypass downsampling; raw ADC order is electrical,
     * error/mask/side are canonical after the owner's steering sign. */
    if(have_sample && (uint32_t)(now-last_sample_ms)<BLACKBOX_SAMPLE_MS &&
       s->mode==samples[(head+BLACKBOX_CAPACITY-1U)%BLACKBOX_CAPACITY].mode &&
       !(event && state==LOG_RECORDING)) return;
    if(event && state==LOG_RECORDING) {
        state=LOG_POST; trigger_ms=now; header.trigger_ms=now;
        header.trigger_index=count; post_count=0;
    }
    v.ms=now;
    for(i=0;i<8;++i) v.raw[i]=raw[i];
    v.error=error; v.left=s->left; v.right=s->right;
    v.common_x10=Scaled(s->speed*10); v.target_x10=Scaled(s->target*10);
    v.correction_x10=Scaled(s->last_correction*10);
    v.derivative_x10=Scaled(s->derivative*10); v.planner_x10=Scaled(s->planner_rate*10);
    v.lost_ms=(uint16_t)(s->lost_ms>65535U?65535U:s->lost_ms);
    v.recovery_ms=(uint16_t)(s->recovery_ms>65535U?65535U:s->recovery_ms);
    v.mask=mask; v.valid=valid; v.mode=(uint8_t)s->mode; v.reason=(uint8_t)s->speed_reason;
    v.fault=(uint8_t)s->fault; v.stop_reason=(uint8_t)s->line_stop_reason;
    v.side=s->corner_pending_ms?s->corner_pending_dir:s->last_side_ms?s->last_side:s->post_turn_dir;
    v.flags=(s->corner_pending_ms?1U:0U)|(s->post_turn_ms?2U:0U)|
        (state==LOG_POST && now==trigger_ms?8U:0U)|
        (s->probe_continue && s->mode==CONTROL_PROBE?16U:0U);
    Push(&v); have_sample=1; last_sample_ms=now;
    if(state==LOG_POST && ++post_count>=BLACKBOX_POST_SAMPLES) {
        state=LOG_READY; header.end_ms=now;
    }
}
void Blackbox_Stop(uint32_t now, ControlFault fault)
{
    if(state!=LOG_RECORDING && state!=LOG_POST) return;
    if(count) {
        BlackboxSample v=samples[(head+BLACKBOX_CAPACITY-1U)%BLACKBOX_CAPACITY];
        /* Terminal row repeats the most recent sensors, not a fresh conversion. */
        v.ms=now; v.flags|=4U; v.flags&=(uint8_t)~(8U|16U);
        v.mode=fault==FAULT_NONE?CONTROL_STOPPED:CONTROL_FAULT; v.fault=(uint8_t)fault;
        v.left=v.right=v.common_x10=v.target_x10=v.correction_x10=0;
        if(count==BLACKBOX_CAPACITY) { if(header.trigger_index!=0xFFFFU && header.trigger_index) --header.trigger_index; }
        /* Do not discard prehistory for the terminal row. */
        state=LOG_POST; Push(&v);
    }
    header.end_ms=now; header.end_fault=(uint8_t)fault; state=count?LOG_READY:LOG_ARMED;
}
void Blackbox_GetStatus(BlackboxStatus *out)
{
    out->state=state; out->trigger=trigger; out->count=count;
    out->saved=saved_slot>=0; out->sequence=saved_sequence; out->ram_saved=ram_saved;
}
uint8_t Blackbox_GetSample(unsigned index, BlackboxSample *out)
{
    if(index>=count) return 0;
    *out=samples[(First()+index)%BLACKBOX_CAPACITY]; return 1;
}
uint8_t Blackbox_Save(void)
{
    unsigned slot=saved_slot==0?1U:0U, i;
    uint16_t commit=LOG_COMMIT;
    uint32_t crc;
    uint8_t ok;
    if(state!=LOG_READY || !count) return 0;
    if(ram_saved) return 1; /* No extra erase for repeated Save. */
    header.count=count; header.sequence=saved_sequence+1U;
    crc=LogCRC(0xFFFFFFFFUL,&header,offsetof(LogHeader,crc));
    for(i=0;i<count;++i) crc=LogCRC(crc,&samples[(First()+i)%BLACKBOX_CAPACITY],sizeof(BlackboxSample));
    header.crc=crc^0xFFFFFFFFUL;
    if(!LogStorage_Begin()) return 0;
    ok=LogStorage_Erase(slot) && LogStorage_Write(slot,0,&header,sizeof(header));
    for(i=0;ok && i<count;++i)
        ok=LogStorage_Write(slot,sizeof(header)+i*sizeof(BlackboxSample),
            &samples[(First()+i)%BLACKBOX_CAPACITY],sizeof(BlackboxSample));
    /* Commit last. A torn save never replaces the previously valid slot. */
    if(ok) ok=LogStorage_Write(slot,LOG_SLOT_BYTES-2U,&commit,2);
    LogStorage_End();
    if(ok && ValidSlot(slot)) { saved_slot=(int)slot; saved_sequence=header.sequence; ram_saved=1; return 1; }
    return 0;
}
static uint32_t export_crc;
static void ExportText(const char *text)
{
    export_crc=LogCRC(export_crc,text,(unsigned)strlen(text));
    LogSerial_Write(text);
}
static void Unsigned(uint32_t value)
{
    char text[11]; unsigned n=0,i=0; char out[11];
    do { text[n++]=(char)('0'+value%10U); value/=10U; } while(value);
    while(n) out[i++]=text[--n];
    out[i]=0; ExportText(out);
}
static void Number(int32_t value)
{
    char text[12]; unsigned n=0,i; uint32_t magnitude;
    char out[13];
    magnitude=value<0?0U-(uint32_t)value:(uint32_t)value;
    do { text[n++]=(char)('0'+magnitude%10U); magnitude/=10U; } while(magnitude);
    i=0; if(value<0) out[i++]='-';
    while(n) out[i++]=text[--n];
    out[i]=0; ExportText(out);
}
static void Field(int32_t value) { ExportText(","); Number(value); }
uint8_t Blackbox_Export(void)
{
    const LogHeader *h;
    const BlackboxSample *v;
    unsigned i,j;
    /* Do not silently export an older Flash run when a new RAM log is waiting. */
    if((state==LOG_READY && count && !ram_saved) || saved_slot<0 || !ValidSlot((unsigned)saved_slot)) return 0;
    h=Saved((unsigned)saved_slot); export_crc=0xFFFFFFFFUL;
    ExportText("# linefollower_blackbox_v1,firmware,"); ExportText(h->firmware);
    ExportText(",sequence,"); Unsigned(h->sequence);
    ExportText(",count,"); Number(h->count); ExportText("\r\n");
    ExportText("# profile,base,straight,max,kp_x1000,ki_x1000,kd_x1000,dir,polarity,align_ms\r\n# values");
    Field(h->base); Field(h->straight); Field(h->maximum);
    Field((int32_t)(h->kp*1000)); Field((int32_t)(h->ki*1000)); Field((int32_t)(h->kd*1000));
    Field(h->steering_sign); Field(h->polarity); Field(h->align_ms); ExportText("\r\n");
    ExportText("# window,start_ms,trigger_ms,end_ms,trigger_index,trigger_type,end_fault\r\n# values");
    ExportText(","); Unsigned(h->start_ms); ExportText(","); Unsigned(h->trigger_ms); ExportText(","); Unsigned(h->end_ms);
    Field(h->trigger_index); Field(h->trigger); Field(h->end_fault); ExportText("\r\n");
    for(j=0;j<3;++j) {
        ExportText(j==0?"# calib_low":j==1?"# calib_high":"# calib_white");
        for(i=0;i<8;++i) Field(j==0?h->low[i]:j==1?h->high[i]:h->white[i]);
        ExportText("\r\n");
    }
    ExportText("ms,raw0,raw1,raw2,raw3,raw4,raw5,raw6,raw7,error,left,right,common_x10,target_x10,correction_x10,derivative_x10,planner_x10,lost_ms,recovery_ms,mask,valid,mode,reason,fault,stop_reason,side,flags\r\n");
    v=(const BlackboxSample *)((const uint8_t *)h+sizeof(*h));
    for(i=0;i<h->count;++i) {
        Unsigned(v[i].ms);
        for(j=0;j<8;++j) Field(v[i].raw[j]);
        Field(v[i].error); Field(v[i].left); Field(v[i].right);
        Field(v[i].common_x10); Field(v[i].target_x10); Field(v[i].correction_x10);
        Field(v[i].derivative_x10); Field(v[i].planner_x10); Field(v[i].lost_ms); Field(v[i].recovery_ms);
        Field(v[i].mask); Field(v[i].valid); Field(v[i].mode); Field(v[i].reason);
        Field(v[i].fault); Field(v[i].stop_reason); Field(v[i].side); Field(v[i].flags);
        ExportText("\r\n");
    }
    {
        uint32_t checksum=export_crc^0xFFFFFFFFUL;
        ExportText("# END,count,"); Number(h->count);
        ExportText(",crc32,"); Unsigned(checksum); ExportText("\r\n");
    }
    return 1;
}
