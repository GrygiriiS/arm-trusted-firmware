/*
 * Copyright 2024 EPAM Systems
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef RCAR_SCMI_RESOURCES_H
#define RCAR_SCMI_RESOURCES_H

#include <assert.h>
#include <drivers/scmi-msg.h>

typedef uint16_t scmi_umask_t;

typedef scmi_umask_t scmi_perm_t;

static inline bool scmi_permission_granted(scmi_perm_t perm, uint32_t agent_id)
{
	assert(agent_id < plat_scmi_agent_count());
	return perm & (1 << agent_id);
}

enum rcar_scmi_power_domains {
	R8A779G0_PD_A1E0D0C0 = 0,
	R8A779G0_PD_A1E0D0C1 = 1,
	R8A779G0_PD_A1E0D1C0 = 2,
	R8A779G0_PD_A1E0D1C1 = 3,
	R8A779G0_PD_A2E0D0 = 16,
	R8A779G0_PD_A2E0D1 = 17,
	R8A779G0_PD_A3E0 = 20,
	R8A779G0_PD_A33DGA = 24,
	R8A779G0_PD_A23DGB = 25,
	R8A779G0_PD_A1DSP0 = 33,
	R8A779G0_PD_A2IMP01 = 34,
	R8A779G0_PD_A2PSC = 35,
	R8A779G0_PD_A2CV0 = 36,
	R8A779G0_PD_A2CV1 = 37,
	R8A779G0_PD_A1CNN0 = 41,
	R8A779G0_PD_A2CN0 = 42,
	R8A779G0_PD_A3IR = 43,
	R8A779G0_PD_A1DSP1 = 45,
	R8A779G0_PD_A2IMP23 = 46,
	R8A779G0_PD_A2DMA = 47,
	R8A779G0_PD_A2CV2 = 48,
	R8A779G0_PD_A2CV3 = 49,
	R8A779G0_PD_A1DSP2 = 53,
	R8A779G0_PD_A1DSP3 = 54,
	R8A779G0_PD_A3VIP0 = 56,
	R8A779G0_PD_A3VIP1 = 57,
	R8A779G0_PD_A3VIP2 = 58,
	R8A779G0_PD_A3VIP3 = 59,
	R8A779G0_PD_A3ISP0 = 60,
	R8A779G0_PD_A3ISP1 = 61,
	R8A779G0_PD_A3DUL = 62,

	/* Always-on power area */
	R8A779G0_PD_ALWAYS_ON = 64,
	R8A779G0_PD_MAX = R8A779G0_PD_ALWAYS_ON + 1
};

extern scmi_perm_t rcar_scmi_perm_power_domains[];

#endif /* RCAR_SCMI_RESOURCES_H */
