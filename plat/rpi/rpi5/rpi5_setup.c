/*
 * Copyright (c) 2024, Mario Bălănică <mariobalanica02@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <drivers/generic_delay_timer.h>
#include <rpi_shared.h>

#ifdef SCMI_SERVER_SUPPORT
extern void rpi5_init_scmi_server(void);
#endif

void plat_rpi_bl31_custom_setup(void)
{
	/* Enable arch timer */
	generic_delay_timer_init();

#if SCMI_SERVER_SUPPORT
	rpi5_init_scmi_server();
#endif
}
