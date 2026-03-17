#include <zephyr/kernel.h>

#define MAX_DEPTH 10

static uint32_t per_report_max_depth;
static uint32_t per_report_occurences_count;
static uint32_t per_report_l0_occurences_count;
static uint32_t per_report_dwt_cycles;
static uint32_t per_report_max_dwt_cycles;
static uint32_t per_report_min_dwt_cycles = 0xFFFFFFFF;
static uint32_t current_stack[MAX_DEPTH];
static uint32_t per_report_max_stack[MAX_DEPTH];

static uint32_t r_max_depth;
static uint32_t r_occurences_count;
static uint32_t r_l0_occurences_count;
static uint32_t r_dwt_cycles;
static uint32_t r_max_dwt_cycles;
static uint32_t r_min_dwt_cycles;
static uint32_t r_max_stack[MAX_DEPTH];

// enum IRQn_Type - see SoC specific header file, i.e.
// modules/hal/nordic/nrfx/nrf54lm20a_application.h
// modules/hal/nordic/nrfx/bsp/stable/mdk/nrf54lm20a_application.h

// #define FILTER_ID CRACEN_IRQn
// #define FILTER_ID SERIAL00_IRQn
// #define FILTER_ID RADIO_0_IRQn
// #define FILTER_ID TIMER10_IRQn
// #define FILTER_ID CRACEN_IRQn
// #define FILTER_ID GRTC_2_IRQn
// #define FILTER_ID GRTC_3_IRQn
// #define FILTER_ID USBHS_IRQn

// #define FILTER_ID 1000 	// zephyr/drivers/usb/udc/udc_dwc2.c udc_dwc2_isr_handler()
// #define FILTER_ID 1001	// zephyr/drivers/usb/udc/udc_dwc2.c dwc2_thread_handler()
#define FILTER_ID 1010 // nrf/subsys/app_event_manager/app_event_manager.c event_processor_fn()

void trace_utils_depth_level_report_prepare(void)
{
	r_max_depth = per_report_max_depth;
	r_occurences_count = per_report_occurences_count;
	r_l0_occurences_count = per_report_l0_occurences_count;
	r_dwt_cycles = per_report_dwt_cycles;
	r_max_dwt_cycles = per_report_max_dwt_cycles;
	r_min_dwt_cycles = per_report_min_dwt_cycles;

	for (int i = 0; i < MAX_DEPTH && i < r_max_depth; i++) {
		r_max_stack[i] = per_report_max_stack[i];
	}

	per_report_max_depth = 0;
	per_report_occurences_count = 0;
	per_report_l0_occurences_count = 0;
	per_report_dwt_cycles = 0;
	per_report_max_dwt_cycles = 0;
	per_report_min_dwt_cycles = 0xFFFFFFFF;
}

void trace_utils_depth_level_report(void)
{

	printk("Execution stack: ");
	for (int i = 0; i < MAX_DEPTH && i < r_max_depth; i++) {
		printk("%d, ", r_max_stack[i]);
	}
	printk("\n");

	if (r_l0_occurences_count) {
		printk("l0_occ: %4u, occ: %4u, max_depth: %u, cycles avg: %4u, min: %4u, max: "
		       "%4u\n",
		       r_l0_occurences_count, r_occurences_count, r_max_depth,
		       r_dwt_cycles / r_l0_occurences_count, r_min_dwt_cycles, r_max_dwt_cycles);
	}
}

static uint32_t entry_ts;

static void prv_trace_utils_depth_level_section_enter(int current_depth, uint32_t ts, uint32_t id)
{
	if (current_depth < MAX_DEPTH) {
		current_stack[current_depth] = id;
	}

	if (current_depth == 0) {
		entry_ts = ts;
		per_report_l0_occurences_count++;
	}

	per_report_occurences_count++;

	if ((current_depth + 1) > per_report_max_depth) {
		per_report_max_depth = current_depth + 1;

		for (int i = 0; i < MAX_DEPTH && i < per_report_max_depth; i++) {
			per_report_max_stack[i] = current_stack[i];
		}
	}
}

static void prv_trace_utils_depth_level_section_leave(uint32_t ts)
{

	uint32_t dwt_cycles = ts - entry_ts;

	per_report_dwt_cycles += dwt_cycles;

	if (dwt_cycles > per_report_max_dwt_cycles) {
		per_report_max_dwt_cycles = dwt_cycles;
	}

	if (dwt_cycles < per_report_min_dwt_cycles) {
		per_report_min_dwt_cycles = dwt_cycles;
	}
}

static volatile int capture_start_level = 0;
static volatile bool capture_in_progress = false;
static atomic_t depth_level;

void trace_utils_depth_level_section_enter(uint32_t ts, uint32_t id)
{
	atomic_val_t current_depth = atomic_inc(&depth_level);
	bool capture_starting = false;

#ifdef FILTER_ID
	if (id == FILTER_ID && !capture_in_progress) {
		capture_start_level = current_depth;
		capture_starting = true;
	}
#endif

	if (capture_starting || capture_in_progress) {
		prv_trace_utils_depth_level_section_enter(current_depth - capture_start_level, ts,
							  id);
	}

	if (capture_starting) {
		capture_in_progress = true;
	}
}

void trace_utils_depth_level_section_leave(uint32_t ts)
{
	atomic_val_t current_depth = atomic_get(&depth_level);

	if (capture_in_progress && current_depth - 1 == capture_start_level) {
		prv_trace_utils_depth_level_section_leave(ts);
		capture_in_progress = false;
	}
	atomic_dec(&depth_level);
}
