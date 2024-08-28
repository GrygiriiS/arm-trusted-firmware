/*
 * Copyright 2024 EPAM Systems
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef RPI5_SCMI_RESOURCES_H
#define RPI5_SCMI_RESOURCES_H

#include <assert.h>
#include <drivers/scmi-msg.h>

#include "rpi5_scmi_devices.h"

struct scmi_reset {
	const char *name;
	uint32_t id;
};
enum rcar_scmi_rst_offset {
	RPI5_SCMIRST_PCIE1_1,	/* 0 */
	RPI5_SCMIRST_PCIE1_2,	/* 1 */
	RPI5_SCMIRST_PCIE2_1,	/* 2 */
	RPI5_SCMIRST_PCIE2_2,	/* 3 */
	RPI5_SCMIRST_MAX
};

struct scmi_device {
	int *rsts;
	int *pins;
};

typedef uint16_t scmi_umask_t;

typedef scmi_umask_t scmi_perm_t;

static inline bool scmi_permission_granted(scmi_perm_t perm, uint32_t agent_id)
{
	assert(agent_id < plat_scmi_agent_count());
	return perm & (1 << agent_id);
}

extern scmi_perm_t rpi_scmi_perm_resets[];

#endif /* RPI5_SCMI_RESOURCES_H */
