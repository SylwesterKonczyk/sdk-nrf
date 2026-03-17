/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <app_event_manager.h>

#define MODULE main
#include <caf/events/module_state_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE);

void trace_utils_init(void);
void trace_utils_report(void);

int main(void)
{
	if (app_event_manager_init()) {
		LOG_ERR("Application Event Manager not initialized");
	} else {
		module_set_state(MODULE_STATE_READY);
	}

	trace_utils_init();

	while (1) {
		k_msleep(1000);
		trace_utils_report();
	}

	return 0;
}
