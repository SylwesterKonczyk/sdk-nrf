/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

void trace_utils_init(void);
void trace_utils_report(void);

int main(void)
{

	trace_utils_init();
	while (1) {

		k_msleep(10000);
		trace_utils_report();
	}

	return 0;
}
