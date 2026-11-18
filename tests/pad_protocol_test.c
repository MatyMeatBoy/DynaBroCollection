/* Real controller bus contracts; generous timing margins, not analog timing QA. */
#define main existing_bus_contract_main
#include "../upstream/segagenesisrecomp/tests/runtime/expansion_ram_test.c"
#undef main
void audio_event_push_latched(uint32_t t,uint8_t p,uint8_t a,uint8_t v)
{(void)t;(void)p;(void)a;(void)v;}
#define FRAME_MASTER (262ull*3420ull)
static uint64_t now;
static void clock_at(uint64_t t){
 now=t;g_snd_frame=(unsigned long)(t/FRAME_MASTER);
 uint32_t within=(uint32_t)(t%FRAME_MASTER);
 g_audio_cycle_counter=within/7;g_68k_stamp_rebase=within%7;
}
static void advance(uint64_t ticks){clock_at(now+ticks);}
static uint32_t address(int port){return 0xa10003u+2u*(unsigned)port;}
static uint8_t readpad(GenesisBus*b,int port){return gbus_read8(b,address(port));}
static uint8_t selectpad(GenesisBus*b,int port,int high){
 gbus_write8(b,address(port),high?64:0);return readpad(b,port);
}
static unsigned high_basic(unsigned p){return 64|((~p)&63);}
static unsigned low_basic(unsigned p){return ((p&GPAD_START)?0:32)|((p&GPAD_A)?0:16)|((~p)&3);}
static unsigned extras(unsigned p){return 64|((p&GPAD_C)?0:32)|((p&GPAD_B)?0:16)|
 ((p&GPAD_MODE)?0:8)|((p&GPAD_X)?0:4)|((p&GPAD_Y)?0:2)|((p&GPAD_Z)?0:1);}
static void fresh(GenesisBus*b,int six){
 clock_at(0);gbus_init(b,NULL);gbus_set_pad_type(b,0,six);gbus_set_pad_type(b,1,six);
}
/* DB2 PCs4B2..5A0, including the initialID read, signature and terminal high. */
static void rom_poll(GenesisBus*b,int port,unsigned buttons){
 unsigned h=selectpad(b,port,1),l=selectpad(b,port,0);
 unsigned id=((!!(h&12)*2+!!(h&3))*2+!!(l&12))*2+!!(l&3);
 CHECK(id==13);CHECK(h==high_basic(buttons));CHECK(l==low_basic(buttons));
 CHECK(selectpad(b,port,1)==high_basic(buttons));
 CHECK(selectpad(b,port,0)==low_basic(buttons));
 CHECK(selectpad(b,port,1)==high_basic(buttons));
 CHECK(selectpad(b,port,0)==(low_basic(buttons)&48));
 CHECK(selectpad(b,port,1)==extras(buttons));
 CHECK(selectpad(b,port,0)==((low_basic(buttons)&48)|15));
 CHECK(selectpad(b,port,1)==high_basic(buttons));
}
int main(void){
 GenesisBus b;
 const unsigned buttons[]={0,GPAD_UP,GPAD_DOWN,GPAD_LEFT,GPAD_RIGHT,GPAD_A,GPAD_B,GPAD_C,
 GPAD_START,GPAD_X,GPAD_Y,GPAD_Z,GPAD_MODE,GPAD_A|GPAD_B|GPAD_C|GPAD_START|GPAD_X|GPAD_Y|GPAD_Z|GPAD_MODE};
 for(unsigned i=0;i<sizeof buttons/sizeof buttons[0];i++){
  fresh(&b,1);gbus_set_pad(&b,0,(uint16_t)buttons[i]);
  rom_poll(&b,0,buttons[i]);rom_poll(&b,0,buttons[i]); /* No idle/host reset. */
 }
 fresh(&b,1);
 selectpad(&b,0,1);selectpad(&b,0,0);selectpad(&b,0,1);selectpad(&b,0,0);
 advance(108000); /* >2ms, still same frame; timeout restarts normal phase. */
 rom_poll(&b,0,0);
 /* A sealed host-frame/button refresh must not reset an incomplete burst. */
 fresh(&b,1);clock_at(FRAME_MASTER-1000);
 selectpad(&b,0,1);selectpad(&b,0,0);selectpad(&b,0,1);selectpad(&b,0,0);
 advance(2000);gbus_set_pad(&b,0,GPAD_START);gbus_set_pad_type(&b,0,1);
 CHECK(selectpad(&b,0,1)==127);CHECK(selectpad(&b,0,0)==16); /* Third low signature+Start. */
 /* Redundant TH writes/button changes never keep the timeout alive. */
 for(unsigned i=0;i<4;i++){advance(27000);gbus_write8(&b,address(0),0);gbus_set_pad(&b,0,GPAD_START);}
 CHECK(readpad(&b,0)==19); /* >2ms after transition, ordinarylow Start held. */
 /* Independent ports: one expires while the other continues a fresh burst. */
 fresh(&b,1);selectpad(&b,0,1);selectpad(&b,0,0);
 advance(108000);selectpad(&b,1,1);selectpad(&b,1,0);selectpad(&b,1,1);selectpad(&b,1,0);
 rom_poll(&b,0,0);CHECK(selectpad(&b,1,1)==127);CHECK(selectpad(&b,1,0)==48);
 /* Actual type changes reset; repeated type selection above did not. */
 gbus_set_pad_type(&b,1,0);gbus_set_pad_type(&b,1,1);rom_poll(&b,1,0);
 fresh(&b,0);gbus_set_pad(&b,0,GPAD_A|GPAD_START|GPAD_B|GPAD_C);
 for(unsigned i=0;i<8;i++){CHECK(selectpad(&b,0,1)==79);CHECK(selectpad(&b,0,0)==3);}
 fresh(&b,1);selectpad(&b,0,1);selectpad(&b,0,0);selectpad(&b,0,1);selectpad(&b,0,0);
 gbus_init(&b,NULL);gbus_set_pad_type(&b,0,1);rom_poll(&b,0,0);
 puts("Pad protocol: exact DB2 polls, terminal wrap, guest timeout, frame/input continuity, ports and button bits PASS");
 return 0;
}
