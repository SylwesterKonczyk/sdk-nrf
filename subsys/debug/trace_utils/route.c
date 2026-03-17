#include <zephyr/kernel.h>

void trace_utils_init(void);
void trace_utils_report(void);
void trace_utils_section_enter(uint32_t id);
void trace_utils_section_leave(void);
void trace_utils_event(uint32_t event_id);
void trace_utils_idle_enter(void);

#define TRACE_UTILS_EVNTS_MONITOR
#define TRACE_UTILS_DEPTH_LEVEL_MONITOR
#define TRACE_UTILS_CACHE_MONITOR

#ifdef TRACE_UTILS_EVNTS_MONITOR
void trace_utils_events_section_enter(uint32_t ts, uint32_t id);
void trace_utils_events_event(uint32_t event_id);
void trace_utils_events_idle_enter(uint32_t ts);
void trace_utils_events_report_prepare(void);
void trace_utils_events_report(void);
#endif

#ifdef TRACE_UTILS_DEPTH_LEVEL_MONITOR
void trace_utils_depth_level_section_enter(uint32_t ts, uint32_t id);
void trace_utils_depth_level_section_leave(uint32_t ts);
void trace_utils_depth_level_report_prepare(void);
void trace_utils_depth_level_report(void);
#endif

#ifdef TRACE_UTILS_CACHE_MONITOR
void trace_utils_cache_init(void);
void trace_utils_cache_section_enter(uint32_t ts, uint32_t id);
void trace_utils_cache_section_leave(uint32_t ts);
void trace_utils_cache_idle_enter(uint32_t ts);
void trace_utils_cache_report_prepare(void);
void trace_utils_cache_report(void);
#endif

#ifdef CONFIG_NRF_CPU_LOAD
void trace_utils_cpu_load_report_prepare(void);
void trace_utils_cpu_load_report(void);
#endif

void trace_utils_init(void)
{
	printk("\n#########################################\n");
	printk("Trace utils, build %s %s\n", __DATE__, __TIME__);

	uint32_t constlat = NRF_POWER->CONSTLATSTAT;
	printk("CONSTLATSTAT: %d\n", NRF_POWER->CONSTLATSTAT);

	if (!constlat) {
		printk("CONSTLATSTAT OFF !!!\n");
		printk("Consider CONFIG_SOC_NRF_FORCE_CONSTLAT=y and CONFIG_NRF_SYS_EVENT=y\n");
	}

	if ((uint32_t)trace_utils_init > 0x20000000) {
		printk("This code is executed from RAM\n");
	} else {
		printk("This code is executed from RRAM\n");
	}

#ifdef TRACE_UTILS_CACHE_MONITOR
	trace_utils_cache_init();
#endif

	printk("#########################################\n\n");
}

void trace_utils_report(void)
{

#ifdef CONFIG_NRF_CPU_LOAD
	trace_utils_cpu_load_report_prepare();
#endif

#ifdef TRACE_UTILS_EVNTS_MONITOR
	trace_utils_events_report_prepare();
#endif

#ifdef TRACE_UTILS_DEPTH_LEVEL_MONITOR
	trace_utils_depth_level_report_prepare();
#endif

#ifdef TRACE_UTILS_CACHE_MONITOR
	trace_utils_cache_report_prepare();
#endif

#ifdef CONFIG_NRF_CPU_LOAD
	trace_utils_cpu_load_report();
#endif

#ifdef TRACE_UTILS_EVNTS_MONITOR
	trace_utils_events_report();
#endif

#ifdef TRACE_UTILS_DEPTH_LEVEL_MONITOR
	trace_utils_depth_level_report();
#endif

#ifdef TRACE_UTILS_CACHE_MONITOR
	trace_utils_cache_report();
#endif
	printk("\n");
}

static uint32_t rd_reg(uint32_t addr)
{
	return *((volatile uint32_t *)addr);
}

static void wr_reg(uint32_t addr, uint32_t val)
{
	*((volatile uint32_t *)addr) = val;
}

#define ARM_CM_DEMCR	  0xE000EDFC
#define ARM_CM_DWT_CTRL	  0xE0001000
#define ARM_CM_DWT_CYCCNT 0xE0001004

void trace_utils_section_enter(uint32_t id)
{
	static atomic_t dwt_configured_mask;
	atomic_val_t dwt_configured_val = atomic_or(&dwt_configured_mask, 0x01);

	if (0 == dwt_configured_val) {
		uint32_t arm_cm_dwt_ctrl_val = rd_reg(ARM_CM_DWT_CTRL);
		if (arm_cm_dwt_ctrl_val) {
			if (0 == (arm_cm_dwt_ctrl_val & 0x01)) {
				wr_reg(ARM_CM_DEMCR, rd_reg(ARM_CM_DEMCR) | (1 << 24));
				wr_reg(ARM_CM_DWT_CYCCNT, 0);
				wr_reg(ARM_CM_DWT_CTRL, rd_reg(ARM_CM_DWT_CTRL) | (1 << 0));
			}
		}
	}

	uint32_t dwt_cycles = rd_reg(ARM_CM_DWT_CYCCNT);

#ifdef TRACE_UTILS_EVNTS_MONITOR
	trace_utils_events_section_enter(dwt_cycles, id);
#endif

#ifdef TRACE_UTILS_DEPTH_LEVEL_MONITOR
	trace_utils_depth_level_section_enter(dwt_cycles, id);
#endif

#ifdef TRACE_UTILS_CACHE_MONITOR
	trace_utils_cache_section_enter(dwt_cycles, id);
#endif
}

void trace_utils_section_leave(void)
{
	uint32_t dwt_cycles = rd_reg(ARM_CM_DWT_CYCCNT);

#ifdef TRACE_UTILS_DEPTH_LEVEL_MONITOR
	trace_utils_depth_level_section_leave(dwt_cycles);
#endif

#ifdef TRACE_UTILS_CACHE_MONITOR
	trace_utils_cache_section_leave(dwt_cycles);
#endif
}

void trace_utils_event(uint32_t event_id)
{
#ifdef TRACE_UTILS_EVNTS_MONITOR
	trace_utils_events_event(event_id);
#endif
}

void trace_utils_idle_enter(void)
{
	uint32_t dwt_cycles = rd_reg(ARM_CM_DWT_CYCCNT);

#ifdef TRACE_UTILS_EVNTS_MONITOR
	trace_utils_events_idle_enter(dwt_cycles);
#endif

#ifdef TRACE_UTILS_CACHE_MONITOR
	trace_utils_cache_idle_enter(dwt_cycles);
#endif
}
