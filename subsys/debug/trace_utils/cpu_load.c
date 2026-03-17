#include <zephyr/kernel.h>
#include <debug/cpu_load.h>

static uint32_t load;

void trace_utils_cpu_load_report_prepare(void)
{
	load = cpu_load_get();
	cpu_load_reset();
}

void trace_utils_cpu_load_report(void)
{
	printk("cpu load: %d.%02d%%\n", load / 1000, (load % 1000) / 10);
}
