#include "blackbox.h"
#include "log_storage.h"
#include "log_serial.h"
#include "robot.h"
#include "timebase.h"
#include <string.h>
extern volatile unsigned test_count, failure_line;
void test_done(void);
void TestRobot_Prepare(void);
void TestRobot_EmitMask(uint8_t mask);
static void Check(int ok,unsigned line)
{
    ++test_count;
    if(!ok) { failure_line=50000U+line; test_done(); }
}
#define CHECK(x) Check((x),__LINE__)
/* Explicit test-only external regions, not additional memory on the MCU. */
#define FAKE_FLASH ((uint8_t *)0x60000000UL)
#define SERIAL_TEXT ((char *)0x60010000UL)
volatile unsigned blackbox_export_bytes;
#define output_bytes blackbox_export_bytes
static unsigned erase_count, pause_count, resume_count;
static int fail_after=-1;
static uint8_t opened, check_busy;
const void *LogStorage_Slot(unsigned slot) { return slot<2?FAKE_FLASH+slot*LOG_SLOT_BYTES:0; }
uint8_t LogStorage_Begin(void)
{
    CHECK(!opened);
    if(check_busy) CHECK(!Robot_Start());
    opened=1; return 1;
}
uint8_t LogStorage_Erase(unsigned slot)
{
    CHECK(opened && slot<2); ++erase_count;
    memset(FAKE_FLASH+slot*LOG_SLOT_BYTES,255,LOG_SLOT_BYTES); return 1;
}
uint8_t LogStorage_Write(unsigned slot,unsigned offset,const void *data,unsigned bytes)
{
    const uint8_t *p=(const uint8_t *)data;
    unsigned i;
    CHECK(opened && slot<2 && !(offset&1U) && !(bytes&1U) && offset+bytes<=LOG_SLOT_BYTES);
    for(i=0;i<bytes;i+=2) {
        if(!fail_after) return 0;
        if(fail_after>0) --fail_after;
        CHECK((FAKE_FLASH[slot*LOG_SLOT_BYTES+offset+i]&p[i])==p[i]);
        CHECK((FAKE_FLASH[slot*LOG_SLOT_BYTES+offset+i+1]&p[i+1])==p[i+1]);
        FAKE_FLASH[slot*LOG_SLOT_BYTES+offset+i]=p[i];
        FAKE_FLASH[slot*LOG_SLOT_BYTES+offset+i+1]=p[i+1];
    }
    return 1;
}
void LogStorage_End(void) { CHECK(opened); opened=0; }
void LogSerial_Init(void) {}
void LogSerial_Write(const char *text)
{
    while(*text) {
        CHECK(output_bytes<65534U);
        SERIAL_TEXT[output_bytes++]=*text++;
    }
    SERIAL_TEXT[output_bytes]=0;
}
void LineADC_PauseForLog(void) { ++pause_count; }
void LineADC_ResumeAfterLog(void) { ++resume_count; }
static int Contains(const char *text)
{
    return strstr(SERIAL_TEXT,text)!=0;
}
static void Capture(BlackboxTrigger trigger)
{
    ControlState s;
    ControlParameters p;
    uint16_t raw[8], low[8], high[8], white[8];
    unsigned i,j;
    Control_Defaults(&p); Control_Start(&s);
    for(j=0;j<8;++j) { low[j]=200; high[j]=3800; white[j]=200; raw[j]=(uint16_t)(300+j); }
    Blackbox_Arm(trigger);
    Blackbox_Start(1000,&p,-1,low,high,white,0xFF);
    s.left=600; s.right=600; s.speed=s.target=60;
    for(i=0;i<300;++i) Blackbox_Record(1000+i*2,raw,0,0x18,1,&s);
    if(trigger==LOG_TRIGGER_CORNER) {
        s.corner_pending_ms=200; s.corner_pending_dir=-1;
    } else s.mode=CONTROL_BRAKE;
    s.left=0; s.right=0; s.target=s.speed=0;
    for(i=0;i<80;++i) Blackbox_Record(1600+i*4,raw,-1500,0x1F,1,&s);
}
void RunBlackboxTests(void)
{
    BlackboxStatus status;
    BlackboxSample first,event,last;
    ControlParameters p;
    ControlState s;
    uint16_t raw[8]={200,200,200,3800,3800,200,200,200};
    unsigned i,before,lines;
    int failures[]={0,5,80,3894};
    memset(FAKE_FLASH,255,2*LOG_SLOT_BYTES);
    Blackbox_Init(); Blackbox_GetStatus(&status);
    CHECK(status.state==LOG_ARMED && !status.saved && !Blackbox_Save() && !Blackbox_Export());
    Capture(LOG_TRIGGER_CORNER); Blackbox_GetStatus(&status);
    CHECK(status.state==LOG_READY && status.count==160 && !status.saved);
    CHECK(sizeof(BlackboxSample)==48);
    CHECK(Blackbox_GetSample(0,&first) && first.ms==1280 && first.raw[7]==307);
    CHECK(Blackbox_GetSample(80,&event) && event.ms==1600 && event.flags==9 && event.side==-1);
    CHECK(Blackbox_GetSample(159,&last) && last.ms==1916);
    CHECK(!Blackbox_GetSample(160,&last));
    Blackbox_Stop(2000,FAULT_LINE);
    CHECK(Blackbox_GetSample(159,&last) && last.ms==1916); /* Frozen window survives later Stop. */
    CHECK(Blackbox_Save()); Blackbox_GetStatus(&status);
    CHECK(status.saved && status.ram_saved && status.sequence==1);
    before=erase_count; CHECK(Blackbox_Save() && erase_count==before);
    output_bytes=0; CHECK(Blackbox_Export());
    CHECK(Contains("# linefollower_blackbox_v1,firmware,B29,sequence,1,count,160\r\n"));
    CHECK(Contains("# values,45,45,85,12000,0,120,-1,255,12\r\n"));
    CHECK(Contains("# END,count,160,crc32,"));
    for(i=0,lines=0;i<output_bytes;++i) if(SERIAL_TEXT[i]=='\n') ++lines;
    CHECK(lines==170); /* Metadata + CSV header + samples + end marker. */
    Blackbox_Init(); Blackbox_GetStatus(&status);
    CHECK(status.saved && status.sequence==1 && status.state==LOG_ARMED && !status.count);
    /* Torn saves before/within payload/at commit preserve the older slot on reboot. */
    for(i=0;i<sizeof(failures)/sizeof(failures[0]);++i) {
        Capture(LOG_TRIGGER_LOSS); fail_after=failures[i];
        CHECK(!Blackbox_Export()); /* Do not export stale Flash instead of pending RAM. */
        CHECK(!Blackbox_Save()); fail_after=-1;
        Blackbox_Init(); Blackbox_GetStatus(&status);
        CHECK(status.saved && status.sequence==1);
    }
    Capture(LOG_TRIGGER_LOSS); CHECK(Blackbox_Save());
    Blackbox_Init(); Blackbox_GetStatus(&status); CHECK(status.sequence==2);
    /* CRC corruption rejects the newest payload and falls back to slot 0. */
    FAKE_FLASH[LOG_SLOT_BYTES+200]^=1;
    Blackbox_Init(); Blackbox_GetStatus(&status); CHECK(status.sequence==1);
    /* Manual stop keeps tail history and adds a clearly marked stale-sensor terminal row. */
    Blackbox_Arm(LOG_TRIGGER_LOSS); Control_Defaults(&p); Control_Start(&s);
    Blackbox_Start(0,&p,1,raw,raw,raw,0);
    s.left=s.right=450;
    for(i=0;i<300;++i) Blackbox_Record(i*2,raw,0,0x18,1,&s);
    Blackbox_Stop(600,FAULT_UI); Blackbox_GetStatus(&status);
    CHECK(status.count==81 && status.state==LOG_READY);
    CHECK(Blackbox_GetSample(80,&last) && last.ms==600 && (last.flags&4) && last.fault==FAULT_UI && !last.left && !last.right);
    Blackbox_Start(700,&p,1,raw,raw,raw,0);
    CHECK(Blackbox_GetSample(80,&last) && last.ms==600);
    /* GAP is not a LOSS trigger; a real non-GAP loss bypasses the 4 ms sampling gate. */
    Blackbox_Arm(LOG_TRIGGER_LOSS); Blackbox_Start(0,&p,1,raw,raw,raw,0);
    s.mode=CONTROL_GAP; Blackbox_Record(0,raw,0,0,0,&s);
    Blackbox_GetStatus(&status); CHECK(status.state==LOG_RECORDING);
    s.mode=CONTROL_PROBE; Blackbox_Record(2,raw,0,0,0,&s);
    Blackbox_GetStatus(&status); CHECK(status.state==LOG_POST && status.count==2);
    Blackbox_Stop(3,FAULT_ADC);
    /* Real supervisor: APIs cannot pause/erase/export/arm during RUN. */
    TestRobot_Prepare(); before=erase_count;
    CHECK(!Robot_LogArm(1) && !Robot_LogSave() && !Robot_LogExport() && erase_count==before);
    for(i=0;i<100;++i) { Robot_UIAlive(Time_Millis()); TestRobot_EmitMask(0x18); }
    Robot_Stop(FAULT_NONE);
    check_busy=1; CHECK(Robot_LogSave()); check_busy=0;
    output_bytes=0; before=pause_count;
    CHECK(Robot_LogExport() && pause_count==before+1 && resume_count==pause_count);
    CHECK(!Robot_Start()); /* A frame from before Flash/UART maintenance is stale. */
    TestRobot_EmitMask(0x18); Robot_UIAlive(Time_Millis());
    CHECK(Robot_Start()); Robot_Stop(FAULT_NONE);
    CHECK(Robot_LogArm(1)); Blackbox_GetStatus(&status);
    CHECK(status.state==LOG_ARMED && status.saved && status.trigger==LOG_TRIGGER_CORNER);
    /* B23 flag bit4 remains compatible with B29 CSV. */
    Blackbox_Start(1000,&p,1,raw,raw,raw,0);
    Control_Start(&s); s.mode=CONTROL_PROBE; s.probe_continue=1;
    Blackbox_Record(1002,raw,0,0xFF,0,&s);
    CHECK(Blackbox_GetSample(0,&last) && (last.flags&16));
    Blackbox_GetStatus(&status);
    CHECK(status.state==LOG_RECORDING); /* CORNER ignores directionless PROBE. */
    Blackbox_Stop(1003,FAULT_NONE);
    CHECK(Blackbox_GetSample(1,&last) && (last.flags&4) && !(last.flags&16));
    /* Startup wide/split patterns like hardware seq3 do not freeze CORNER.
     * A directional observation later in the run must still trigger promptly. */
    Blackbox_Arm(LOG_TRIGGER_CORNER); Blackbox_Start(2000,&p,1,raw,raw,raw,0);
    Control_Start(&s); s.mode=CONTROL_PROBE;
    for(i=0;i<100;++i) Blackbox_Record(2000+i*4,raw,40,i&1?0xFF:0x99,0,&s);
    Blackbox_GetStatus(&status); CHECK(status.state==LOG_RECORDING && status.count==80);
    s.mode=CONTROL_TRACK; s.corner_pending_ms=200; s.corner_pending_dir=1;
    Blackbox_Record(2400,raw,1500,0xF8,1,&s);
    Blackbox_GetStatus(&status); CHECK(status.state==LOG_POST && status.count==81);
    CHECK(Blackbox_GetSample(80,&last) && (last.flags&8) && last.side==1);
    /* A fault always captures CORNER even without a direction. */
    Blackbox_Arm(LOG_TRIGGER_CORNER); Blackbox_Start(3000,&p,1,raw,raw,raw,0);
    Control_Stop(&s,FAULT_LINE); Blackbox_Record(3002,raw,0,0,0,&s);
    Blackbox_GetStatus(&status); CHECK(status.state==LOG_POST);
    CHECK(Blackbox_GetSample(0,&last) && (last.flags&8) && last.fault==FAULT_LINE);
}
