#include <zephyr/kernel.h>
#include <stdlib.h>

/* Shows system-wide statistics, captured between every trace_utils_cache_report()
 */
#define CONTINOUS_PERFORMANCE_MEASUREMENT

/* Statistics/operations for specific code, i.e. interrupt
 */
// enum IRQn_Type - see SoC specific header file, i.e.
// modules/hal/nordic/nrfx/nrf54lm20a_application.h
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

/* Enable capturing for given FILTER_ID. Skip N hits on given FILTER_ID
 */
#define START_CAPTURE_AT_OCCURENCE_NO (4 * 3 * 8000)

/* if no cache fill-in encounted on hit on given FILTER_ID - continue capturing
 * useful to trace a code with variable execution paths
 */
#define CAPTURE_TILL_CACHE_NON_EMPTY

/* useful to emulate a system with high cache preasure - the worst
 * case scenario, where 'noisy neighbour' evicts cache lines
 * of observed code
 */
#define INVALIDATE_CACHE_ON_EVERY_FILTER_HIT

#define INVALIDATE_CACHE_ON_CAPTURING_FILTER_HIT

#define SHOW_CAPTURED_ADDRESSES

typedef struct {
	uint8_t duv;
	bool valid;
	uint32_t address;

} cache_line_info_t;

static cache_line_info_t filtered_cache_lines[CACHEDATA_SET_WAY_MaxCount * CACHEDATA_SET_MaxCount];

#ifdef CONTINOUS_PERFORMANCE_MEASUREMENT
static cache_line_info_t
	continous_capture_cache_lines[CACHEDATA_SET_WAY_MaxCount * CACHEDATA_SET_MaxCount];
#endif

static uint32_t captured_id;
static bool capture_data_to_be_displayed;
static uint32_t captured_hits;
static uint32_t captured_misses;

static uint32_t occurence_no = 0;
static bool capture_completed = false;

static uint32_t rd_reg(uint32_t addr)
{
	return *((volatile uint32_t *)addr);
}

static uint32_t dump_cache_line_info(cache_line_info_t *dst)
{
	uint32_t non_empty_lines = 0;
	for (int way = 0; way < CACHEDATA_SET_WAY_MaxCount; way++) {
		for (int set = 0; set < CACHEDATA_SET_MaxCount; set++) {
			cache_line_info_t *line_info = &dst[way * CACHEDATA_SET_MaxCount + set];

			uint32_t address_offset = set * 0x8 + way * 0x4;
			uint32_t info_raw = rd_reg(0x02F10000 + address_offset);

			line_info->duv = (info_raw >> 24) & 0x0F;
			line_info->valid = (info_raw >> 30) & 0x01;
			line_info->address = 0;

			if (line_info->valid && line_info->duv) {
				// tag
				line_info->address = info_raw & 0xFFFFF;
				line_info->address = line_info->address << 12;

				// index, line size = 32 bytes (4x64bits)
				line_info->address = line_info->address | (set * 32);

				if (line_info->duv & 0x01) {

				} else if (line_info->duv & 0x02) {
					line_info->address = line_info->address + 8;
				} else if (line_info->duv & 0x04) {
					line_info->address = line_info->address + 16;
				} else {
					line_info->address = line_info->address + 24;
				}

				non_empty_lines++;
			}
		}
	}

	return non_empty_lines;
}

static int compare_uint32_t(const void *a, const void *b)
{
	int aa = *(const uint32_t *)a;
	int bb = *(const uint32_t *)b;

	return (aa > bb) - (aa < bb);
}

static void report_cache_lines_info(cache_line_info_t *src, bool report_full_sets,
				    bool report_all_addresses)
{
	/* As this function is not executed concurrently, it is safe to
	 * place table below on heap, limiting stack size needs
	 */
	static uint32_t addresses[CACHEDATA_SET_WAY_MaxCount * CACHEDATA_SET_MaxCount];

	uint32_t p0 = 0;
	uint32_t p25 = 0;
	uint32_t p50 = 0;
	uint32_t p75 = 0;
	uint32_t p100 = 0;

	for (int set = 0; set < CACHEDATA_SET_MaxCount; set++) {
		for (int way = 0; way < CACHEDATA_SET_WAY_MaxCount; way++) {

			cache_line_info_t *line_info = &src[way * CACHEDATA_SET_MaxCount + set];

			if (line_info->valid) {
				uint32_t bit_count = 0;
				uint32_t duv = line_info->duv;
				for (int i = 0; i < 4; i++) {
					if (duv & 0x01) {
						bit_count++;
					}
					duv = duv >> 1;
				}

				if (bit_count == 4) {
					p100++;
				} else if (bit_count == 3) {
					p75++;
				} else if (bit_count == 2) {
					p50++;
				} else if (bit_count == 1) {
					p25++;
				} else {
					p0++;
				}

				addresses[way * 128 + set] = line_info->address;
			} else {
				p0++;
				addresses[way * 128 + set] = 0;
			}
		}
	}

	printk("Lines, empty: %d, p25: %d, p50: %d, p75: %d, full: %d\n", p0, p25, p50, p75, p100);

	uint32_t sets_0 = 0;
	uint32_t sets_1 = 0;
	uint32_t sets_2 = 0;

	for (int set = 0; set < CACHEDATA_SET_MaxCount; set++) {
		uint32_t a1 = addresses[set];
		uint32_t a2 = addresses[128 + set];

		if (a1 == 0 && a2 == 0) {
			sets_0++;
		} else if (a1 && a2) {
			sets_2++;
		} else {
			sets_1++;
		}
	}

	printk("Sets, empty: %d, 1 way occupied: %d, two ways occupied: %d\n", sets_0, sets_1,
	       sets_2);

	if (report_full_sets) {
		for (int set = 0; set < CACHEDATA_SET_MaxCount; set++) {
			uint32_t a1 = addresses[set];
			uint32_t a2 = addresses[128 + set];

			if (a1 && a2) {
				printk("cached addr: %08x, %08x\n", a1, a2);
			}
		}
	}

	if (report_all_addresses) {
		printk("\n");
		qsort(addresses, ARRAY_SIZE(addresses), sizeof(uint32_t), compare_uint32_t);

		for (int i = 0; i < ARRAY_SIZE(addresses); i++) {
			if (addresses[i] != 0) {
				printk("cached addr: %08x\n", addresses[i]);
			}
		}
	}
}

static void report_cache_hit_miss_stats(void)
{

	uint32_t hits = NRF_ICACHE->PROFILING.HIT;
	uint32_t misses = NRF_ICACHE->PROFILING.MISS;

	uint32_t hit_rate = 0;

	if (misses + hits) {
		hit_rate = ((uint64_t)hits) * 10000 / (misses + hits);
		printk("Cache miss count: %d, hit count: %d, access count: %d,  hit rate: "
		       "%d.%02d%%\n",
		       misses, hits, misses + hits, hit_rate / 100, hit_rate % 100);
	}
}

void trace_utils_cache_report_prepare(void)
{
#ifdef CONTINOUS_PERFORMANCE_MEASUREMENT
	dump_cache_line_info(continous_capture_cache_lines);
#endif
}

void trace_utils_cache_report(void)
{
#ifdef CONTINOUS_PERFORMANCE_MEASUREMENT
	report_cache_hit_miss_stats();
	report_cache_lines_info(continous_capture_cache_lines, false, false);
	NRF_ICACHE->PROFILING.CLEAR = 1U;
#endif

#ifdef START_CAPTURE_AT_OCCURENCE_NO
	if (capture_data_to_be_displayed) {
		printk("****************************************\n");
		printk("Cache info for source %d\n", captured_id);

		uint32_t hit_rate = 0;

		if (captured_misses + captured_hits) {
			hit_rate = ((uint64_t)captured_hits) * 10000 /
				   (captured_misses + captured_hits);
			printk("Cache miss count: %d, hit count: %d, access count: %d,  hit rate: "
			       "%d.%02d%%\n",
			       captured_misses, captured_hits, captured_misses + captured_hits,
			       hit_rate / 100, hit_rate % 100);
		}
		capture_data_to_be_displayed = false;

#ifdef SHOW_CAPTURED_ADDRESSES
		report_cache_lines_info(filtered_cache_lines, false, true);
#else
		report_cache_lines_info(filtered_cache_lines, false, false);
#endif

		printk("****************************************\n");

	} else if (occurence_no == START_CAPTURE_AT_OCCURENCE_NO && !capture_completed) {
		printk("****************************************\n");
		printk("Capturing in progress - no cache access encountered\n");
		printk("****************************************\n");
	}
#endif
}

void trace_utils_cache_init(void)
{

	//	uint32_t prefetchconfig_val = 0x107;
	//	uint32_t prefetchconfig_val = 0x07;
	//	uint32_t prefetchconfig_val = 0x0;
	//	wr_reg(0xE0082000+0x478, prefetchconfig_val);

	printk("PREFETCHCONFIG: 0x%08X\n", rd_reg(0xE0082000 + 0x478));

	NRF_ICACHE->PROFILING.ENABLE = 1U;
}

static void prv_trace_utils_cache_section_enter(int current_depth, uint32_t ts, uint32_t id)
{
	if (current_depth == 0) {
#ifdef INVALIDATE_CACHE_ON_CAPTURING_FILTER_HIT
		NRF_ICACHE->TASKS_INVALIDATECACHE = 1U;
#endif
		captured_id = id;
		NRF_ICACHE->PROFILING.CLEAR = 1U;
	}
}

static void prv_trace_utils_cache_section_leave(uint32_t ts)
{
	captured_hits = NRF_ICACHE->PROFILING.HIT;
	captured_misses = NRF_ICACHE->PROFILING.MISS;

	uint32_t non_empty_lines = dump_cache_line_info(filtered_cache_lines);

#ifdef CAPTURE_TILL_CACHE_NON_EMPTY
	if (non_empty_lines) {
		capture_completed = true;
		capture_data_to_be_displayed = true;
	}
#else
	capture_completed = true;
	capture_data_to_be_displayed = true;
#endif
}

static volatile int capture_start_level = 0;
static volatile bool capture_in_progress = false;
static atomic_t depth_level;

void trace_utils_cache_section_enter(uint32_t ts, uint32_t id)
{
	atomic_val_t current_depth = atomic_inc(&depth_level);
	bool capture_starting = false;

#ifdef FILTER_ID
	if (id == FILTER_ID && !capture_in_progress) {
#ifdef START_CAPTURE_AT_OCCURENCE_NO

		if (occurence_no < START_CAPTURE_AT_OCCURENCE_NO) {
			occurence_no++;

		} else if (!capture_completed) {
			capture_start_level = current_depth;
			capture_starting = true;
		}
#endif

#ifdef INVALIDATE_CACHE_ON_EVERY_FILTER_HIT
		NRF_ICACHE->TASKS_INVALIDATECACHE = 1U;
#endif
	}
#endif

	if (capture_starting || capture_in_progress) {
		prv_trace_utils_cache_section_enter(current_depth - capture_start_level, ts, id);
	}

	if (capture_starting) {
		capture_in_progress = true;
	}
}

void trace_utils_cache_section_leave(uint32_t ts)
{
	atomic_val_t current_depth = atomic_get(&depth_level);

	if (capture_in_progress && current_depth - 1 == capture_start_level) {
		prv_trace_utils_cache_section_leave(ts);
		capture_in_progress = false;
	}
	atomic_dec(&depth_level);
}

void trace_utils_cache_idle_enter(uint32_t ts)
{
}
