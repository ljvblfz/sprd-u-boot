#include <config.h>
#include <asm/io.h>
#include <asm/arch/chip_drv_config_extern.h>
#include <asm/arch/bits.h>
#include <linux/types.h>

void start_watchdog(uint32_t init_time_ms)
{
	WDG_ClockOn();
    WDG_ResetMCU();
}

void stop_watchdog(void)
{
    WDG_TimerStop();
	
}

void init_watchdog(void)
{
    WDG_TimerInit();
}

void feed_watchdog(void)
{
}

void load_watchdog(uint32_t time_ms)
{
    WDG_TimerLoad(time_ms);
}

void hw_watchdog_reset(void)
{

}
int fatal_dump_enabled(void)
{
	return *(volatile u32 *) (0x1ffc);
}

