/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2024, EPAM Systems
 */
#ifndef PLAT_RPI5_SCMI_DEVICE_H
#define PLAT_RPI5_SCMI_DEVICE_H

/* TODO: Update list */
enum rpi5_scmi_device_t {
	RPI5_SCMI_DEV_PCIE0 = 0,
	RPI5_SCMI_DEV_PCIE1 = 1,
	RPI5_SCMI_DEV_PCIE2 = 2,
	RPI5_SCMI_DEV_SDHCI0 = 3,
	RPI5_SCMI_DEV_SDHCI1 = 4,
	RPI5_SCMI_DEV_COUNT = 5,
};

#endif


