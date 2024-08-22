/*
 * Copyright 2024 EPAM Systems
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef RPI5_SCMI_RESOURCES_H
#define RPI5_SCMI_RESOURCES_H

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

#endif /* RPI5_SCMI_RESOURCES_H */
