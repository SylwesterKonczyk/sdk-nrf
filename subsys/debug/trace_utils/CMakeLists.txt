#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_sources(route.c)
zephyr_library_sources_ifdef(CONFIG_NRF_CPU_LOAD cpu_load.c)
zephyr_library_sources(depth.c)
zephyr_library_sources(events.c)
zephyr_library_sources(cache.c)

zephyr_code_relocate(FILES route.c LOCATION RAM)
zephyr_code_relocate(FILES cpu_load.c LOCATION RAM)
zephyr_code_relocate(FILES depth.c LOCATION RAM)
zephyr_code_relocate(FILES events.c LOCATION RAM)
zephyr_code_relocate(FILES cache.c LOCATION RAM)

