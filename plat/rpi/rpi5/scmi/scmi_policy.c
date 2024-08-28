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

_Static_assert(sizeof(scmi_umask_t) * 8 > SCMI_NUM_AGENTS);

static const struct scmi_device default_dev = {
		.rsts = (int[]){-1},
		.pins = (int[]){-1},
};

static const struct scmi_device rpi_scmi_devices[RPI5_SCMI_DEV_COUNT] = {
	[RPI5_SCMI_DEV_PCIE0] = default_dev,
	[RPI5_SCMI_DEV_PCIE1] = {
		.rsts = (int[]){RPI5_SCMIRST_PCIE1_1, RPI5_SCMIRST_PCIE1_2, -1},
		.pins = (int[]){-1},
	},
	[RPI5_SCMI_DEV_PCIE2] = {
		.rsts = (int[]){RPI5_SCMIRST_PCIE2_1, RPI5_SCMIRST_PCIE2_2, -1},
		.pins = (int[]){-1},
	},
	[RPI5_SCMI_DEV_SDHCI0] = default_dev,
	[RPI5_SCMI_DEV_SDHCI1] = default_dev,
};

static spinlock_t scmi_lock;

static uint32_t rpi_scmi_device_owner[RPI5_SCMI_DEV_COUNT];

static void __scmi_permit(scmi_perm_t *perm, uint32_t agent_id)
{
	scmi_perm_t mask = 1 << agent_id;

	*perm |= mask;
}

static void __scmi_deny(scmi_perm_t *perm, uint32_t agent_id)
{
	scmi_perm_t mask = 1 << agent_id;

	*perm &= ~mask;
}

static void __mangle_dev_perm(uint32_t agent_id, uint32_t dev_id,
			      void (*fn)(scmi_perm_t *, uint32_t))
{
#define MANGLE(attr, res_perms)                                                \
	{                                                                      \
		int *index = rpi_scmi_devices[dev_id].attr;                    \
		while (index && *index >= 0) {                                 \
			fn(&res_perms[*index++], agent_id);                    \
		}                                                              \
	}
	MANGLE(rsts, rpi_scmi_perm_resets);
#undef MANGLE
}

static void scmi_permit(uint32_t agent_id, uint32_t dev_id)
{
	__mangle_dev_perm(agent_id, dev_id, __scmi_permit);
}

static void scmi_deny(uint32_t agent_id, uint32_t dev_id)
{
	__mangle_dev_perm(agent_id, dev_id, __scmi_deny);
}

int32_t plat_scmi_device_permission(uint32_t agent_id, uint32_t device_id,
				    bool allow)
{
	uint32_t agent_id_int = agent_id + 1;
	int32_t ret = SCMI_SUCCESS;

	assert(agent_id < plat_scmi_agent_count());
	assert(device_id < plat_scmi_device_count());

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
		scmi_permit(agent_id, device_id);
		rpi_scmi_device_owner[device_id] = agent_id_int;
	} else {
		scmi_deny(agent_id, device_id);
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
				scmi_deny(agent_id, device_id);
				rpi_scmi_device_owner[device_id] = 0;
			}
		}
	}
	spin_unlock(&scmi_lock);

	return SCMI_SUCCESS;
}
