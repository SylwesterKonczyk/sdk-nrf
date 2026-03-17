#include <zephyr/kernel.h>

#define MAX_INTERRUPTS 256

#define SECTIONS_OFFSET 1000
#define MAX_SECTIONS	64

#define EVENTS_OFFSET 2000
#define MAX_EVENTS    64

static atomic_t per_report_cpu_idle_entry_count;
static uint32_t r_cpu_idle_entry_count;

static atomic_t per_report_interrupts_count[MAX_INTERRUPTS];
static uint32_t r_interrupts_count[MAX_INTERRUPTS];

static atomic_t per_report_sections_count[MAX_SECTIONS];
static uint32_t r_sections_count[MAX_SECTIONS];

static atomic_t per_report_events_count[MAX_EVENTS];
static uint32_t r_events_count[MAX_EVENTS];

void trace_utils_events_report_prepare(void)
{
	atomic_val_t v;

	v = atomic_clear(&per_report_cpu_idle_entry_count);
	r_cpu_idle_entry_count = v;

	for (int i = 0; i < MAX_INTERRUPTS; i++) {
		v = atomic_clear(&per_report_interrupts_count[i]);
		r_interrupts_count[i] = v;
	}

	for (int i = 0; i < MAX_SECTIONS; i++) {
		v = atomic_clear(&per_report_sections_count[i]);
		r_sections_count[i] = v;
	}

	for (int i = 0; i < MAX_EVENTS; i++) {
		v = atomic_clear(&per_report_events_count[i]);
		r_events_count[i] = v;
	}
}

void trace_utils_events_report(void)
{
	int i;
	printk("Events: [CPU IDLE]: %d, ", r_cpu_idle_entry_count);

	for (i = 0; i < MAX_INTERRUPTS; i++) {
		if (r_interrupts_count[i]) {

			if (GRTC_2_IRQn == i) {
				printk("[GRTC_2_IRQn (%d)]: %d, ", i, r_interrupts_count[i]);
				continue;
			}

			if (GRTC_3_IRQn == i) {
				printk("[GRTC_3_IRQn (%d)]: %d, ", i, r_interrupts_count[i]);
				continue;
			}

			if (RADIO_0_IRQn == i) {
				printk("[RADIO_0_IRQn (%d)]: %d, ", i, r_interrupts_count[i]);
				continue;
			}

			if (TIMER10_IRQn == i) {
				printk("[TIMER10_IRQn (%d)]: %d, ", i, r_interrupts_count[i]);
				continue;
			}

			if (CRACEN_IRQn == i) {
				printk("[CRACEN_IRQn (%d)]: %d, ", i, r_interrupts_count[i]);
				continue;
			}

			if (SERIAL00_IRQn == i) {
				printk("[SERIAL00_IRQn (%d)]: %d, ", i, r_interrupts_count[i]);
				continue;
			}

			if (USBHS_IRQn == i) {
				printk("[USBHS_IRQn (%d)]: %d, ", i, r_interrupts_count[i]);
				continue;
			}

			if (SWI00_IRQn == i) {
				printk("[SWI00_IRQn (%d)]: %d, ", i, r_interrupts_count[i]);
				continue;
			}

			printk("[%d]: %d, ", i, r_interrupts_count[i]);
		}
	}

	for (i = 0; i < MAX_SECTIONS; i++) {
		if (r_sections_count[i]) {
			printk("[%d]: %d, ", i + SECTIONS_OFFSET, r_sections_count[i]);
		}
	}

	for (i = 0; i < MAX_EVENTS; i++) {
		if (r_events_count[i]) {
			printk("[%d]: %d, ", i + EVENTS_OFFSET, r_events_count[i]);
		}
	}

	printk("\n");
}

void trace_utils_events_event(uint32_t event_id)
{
	if (event_id >= EVENTS_OFFSET && event_id < EVENTS_OFFSET + MAX_EVENTS) {
		atomic_inc(&per_report_events_count[event_id - EVENTS_OFFSET]);
	}
}

void trace_utils_events_section_enter(uint32_t ts, uint32_t id)
{
	if (id < MAX_INTERRUPTS) {
		atomic_inc(&per_report_interrupts_count[id]);
	} else if (id >= SECTIONS_OFFSET && id < SECTIONS_OFFSET + MAX_SECTIONS) {
		atomic_inc(&per_report_sections_count[id - SECTIONS_OFFSET]);
	}
}

void trace_utils_events_idle_enter(uint32_t ts)
{
	atomic_inc(&per_report_cpu_idle_entry_count);
}
