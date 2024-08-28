/*
 * Copyright 2024 EPAM Systems
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>

#include <common/debug.h>
#include <drivers/delay_timer.h>
#include <drivers/scmi.h>
#include <lib/utils_def.h>
#include <platform_def.h>
#include <lib/mmio.h>

#include "rpi5_scmi_resources.h"

#define SW_INIT_SET		0x00
#define SW_INIT_CLEAR		0x04
#define SW_INIT_STATUS		0x08

#define SW_INIT_BIT(id)		BIT((id) & 0x1f)
#define SW_INIT_BANK(id)	((id) >> 5)

#define SW_INIT_BASE U(0x1001504318)
/* A full bank contains extra registers that we are not utilizing but still
 * qualify as a single bank.
 */
#define SW_INIT_BANK_SIZE	0x18

struct scmi_reset rpi5_resets[RPI5_SCMIRST_MAX] = {
	[RPI5_SCMIRST_PCIE1_1] = {
		.name = "swinit_pcie1",
		.id = 7,
	},
	[RPI5_SCMIRST_PCIE1_2] = {
		.name = "bridge_pcie1",
		.id = 43,
	},
	[RPI5_SCMIRST_PCIE2_1] = {
		.name = "swinit_pcie2",
		.id = 32,
	},
	[RPI5_SCMIRST_PCIE2_2] = {
		.name = "bridge_pcie2",
		.id = 44,
	},
};

scmi_perm_t rpi_scmi_perm_resets[RPI5_SCMIRST_MAX];

static int brcmstb_reset_assert(unsigned int scmi_id)
{
	unsigned int off = SW_INIT_BANK(rpi5_resets[scmi_id].id) * SW_INIT_BANK_SIZE;

	mmio_write_32(SW_INIT_BASE + off + SW_INIT_SET,
			SW_INIT_BIT(rpi5_resets[scmi_id].id));

	return 0;
}

static int brcmstb_reset_deassert(unsigned int scmi_id)
{
	unsigned int off = SW_INIT_BANK(rpi5_resets[scmi_id].id) * SW_INIT_BANK_SIZE;

	mmio_write_32(SW_INIT_BASE + off + SW_INIT_CLEAR,
			SW_INIT_BIT(rpi5_resets[scmi_id].id));

	/* Maximum reset delay after de-asserting a line and seeing block
	 * operation is typically 14us for the worst case, build some slack
	 * here.
	 */
	udelay(14);

	return 0;
}

static int brcmstb_reset_status(unsigned int scmi_id)
{
	unsigned int off = SW_INIT_BANK(rpi5_resets[scmi_id].id) * SW_INIT_BANK_SIZE;

	return mmio_read_32(SW_INIT_BASE + off + SW_INIT_STATUS) &
			     SW_INIT_BIT(rpi5_resets[scmi_id].id);
}

size_t plat_scmi_rstd_count(unsigned int agent_id __unused)
{
	return RPI5_SCMIRST_MAX;
}

const char *plat_scmi_rstd_get_name(unsigned int agent_id __unused,
				  unsigned int scmi_id)
{
	if (scmi_id >= RPI5_SCMIRST_MAX) {
		ERROR("Unable to get reset name, id %d is invalid", scmi_id);
		return NULL;
	}

	return rpi5_resets[scmi_id].name;
}

int32_t plat_scmi_rstd_autonomous(unsigned int agent_id __unused,
				unsigned int scmi_id,
				unsigned int state __unused)
{
	int rc;
	/*
	 * According to the information from the following Linux kernel commit:
	 * 95a15d80aa0d (firmware: arm_scmi: Add RESET protocol in SCMI v2.0, 2019-07-08)
	 * autonomous reset stands for assert and then deassert, performed by ARM-TF.
	 * So if reset was asserted then just do deassert, if deasserted
	 * then assert/deassert.
	 */
	if (scmi_id >= RPI5_SCMIRST_MAX) {
		ERROR("Unable to set autonomous reset, id %d is invalid", scmi_id);
		return SCMI_INVALID_PARAMETERS;
	}

	if (!brcmstb_reset_status(scmi_id)) {
		rc = brcmstb_reset_assert(scmi_id);
		if (rc) {
			ERROR("Unable to assert %d", scmi_id);
			return rc;
		}
	}

	rc = brcmstb_reset_deassert(scmi_id);
	if (rc) {
		ERROR("Unable to deassert %d", scmi_id);
		return rc;
	}

	return SCMI_SUCCESS;
}

int32_t plat_scmi_rstd_set_state(unsigned int agent_id __unused,
			       unsigned int scmi_id,
			       bool assert_not_deassert)
{
	int rc;

	if (scmi_id >= RPI5_SCMIRST_MAX) {
		ERROR("Unable to set reset state, id %d is invalid", scmi_id);
		return SCMI_INVALID_PARAMETERS;
	}

	rc = (assert_not_deassert) ? brcmstb_reset_assert(scmi_id) :
			 brcmstb_reset_deassert(scmi_id);

	return (rc == 0) ? SCMI_SUCCESS : rc;
}

bool plat_scmi_rstd_permitted(uint32_t agent_id, uint32_t domain_id)
{
	assert(agent_id < plat_scmi_agent_count());

	if (!agent_id)
		return true;

	return scmi_permission_granted(rpi_scmi_perm_resets[domain_id],
				       agent_id);
}
