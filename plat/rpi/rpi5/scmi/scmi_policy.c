/*
 * Copyright 2024 EPAM Systems
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <stdint.h>
#include <drivers/scmi-msg.h>
#include <drivers/scmi.h>
#include <lib/spinlock.h>

#include "rpi5_scmi_resources.h"

static spinlock_t scmi_lock;

static uint32_t rpi_scmi_device_owner[RPI5_SCMI_DEV_COUNT];

int32_t plat_scmi_device_permission(uint32_t agent_id, uint32_t device_id,
				    bool allow)
{
	uint32_t agent_id_int = agent_id + 1;
	int32_t ret = SCMI_SUCCESS;

	assert(agent_id < plat_scmi_agent_count());
	assert(device_id < ARRAY_SIZE(rpi_scmi_device_owner));

	spin_lock(&scmi_lock);

	/* The 0 is used as unassigned/denied and (agent_id + 1) as allowed */
	if (allow && rpi_scmi_device_owner[device_id] == agent_id_int) {
		goto exit;
	}

	if (allow && rpi_scmi_device_owner[device_id]) {
		/* already assigned to other agent */
		ret = SCMI_DENIED;
		goto exit;
	}

	VERBOSE("scmi: device permission agent:%u device_id:%u allow:%d\n",
		agent_id, device_id, allow);

	if (allow) {
		rpi_scmi_device_owner[device_id] = agent_id_int;
	} else {
		rpi_scmi_device_owner[device_id] = 0;
	}

exit:
	spin_unlock(&scmi_lock);
	return ret;
}

int32_t plat_scmi_reset_agent_cfg(uint32_t agent_id, bool reset_perm)
{
	uint32_t agent_id_int = agent_id + 1;
	uint32_t device_id;

	assert(agent_id < plat_scmi_agent_count());

	VERBOSE("scmi: reset agent:%u cfg\n", agent_id);

	spin_lock(&scmi_lock);
	for (device_id = 0; device_id < ARRAY_SIZE(rpi_scmi_device_owner);
	     device_id++) {
		if (rpi_scmi_device_owner[device_id] == agent_id_int) {
			VERBOSE("scmi: reset agent:%u device:%u cfg\n",
				agent_id, device_id);

			/* perform device specific reset actions if required */
			if (reset_perm) {
				rpi_scmi_device_owner[device_id] = 0;
			}
		}
	}
	spin_unlock(&scmi_lock);

	return SCMI_SUCCESS;
}
