/* Exercise CPU-visible YM timers with the real bus and a controlled clock. */
#define main existing_bus_contract_main
#include "../upstream/segagenesisrecomp/tests/runtime/expansion_ram_test.c"
#undef main
void audio_event_push_latched(uint32_t t,uint8_t p,uint8_t a,uint8_t v)
{ (void)t;(void)p;(void)a;(void)v; }
int main(void)
{
    GenesisBus b; memset(&b,0,sizeof b);
    g_snd_frame=0; s_test_z80_stamp=105;
    gbus_z80_write(&b,0x4000,0x24);gbus_z80_write(&b,0x4001,0xff);
    gbus_z80_write(&b,0x4000,0x25);gbus_z80_write(&b,0x4001,3);
    gbus_z80_write(&b,0x4000,0x27);gbus_z80_write(&b,0x4001,0x15);
    s_test_z80_stamp=105+1007;
    CHECK(!(gbus_z80_read(&b,0x4000)&1));
    ++s_test_z80_stamp;
    CHECK(gbus_z80_read(&b,0x4000)&1);
    CHECK(gbus_read8(&b,0xA04000)&1);
    /* Acknowledgement must retain the running timer phase. */
    s_test_z80_stamp+=100;
    gbus_z80_write(&b,0x4001,0x15);
    CHECK(!(gbus_z80_read(&b,0x4000)&1));
    s_test_z80_stamp=105+2016;
    CHECK(gbus_z80_read(&b,0x4000)&1);
    /* Disabling the flag still runs the timer; stopping prevents overflow. */
    gbus_z80_write(&b,0x4001,0x11);
    s_test_z80_stamp+=1008;
    CHECK(!(gbus_z80_read(&b,0x4000)&1));
    gbus_z80_write(&b,0x4001,0x14);
    s_test_z80_stamp+=1008;
    CHECK(!(gbus_z80_read(&b,0x4000)&1));
    /* Register $25 uses only its low two bits. Reload from a stopped timer. */
    gbus_z80_write(&b,0x4000,0x25);gbus_z80_write(&b,0x4001,0xff);
    gbus_z80_write(&b,0x4000,0x27);gbus_z80_write(&b,0x4001,0x15);
    s_test_z80_stamp+=1008;
    CHECK(gbus_z80_read(&b,0x4000)&1);
    test_timer_b_acknowledge();
    puts("YM timer A and B CPU-visible overflow/acknowledgement PASS");
    return 0;
}
