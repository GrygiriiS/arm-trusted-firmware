/*
 * Copyright 2024 EPAM Systems
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <assert.h>
#include <stdint.h>

#include <drivers/scmi-msg.h>
#include <drivers/scmi.h>

#include <platform_def.h>

#define RPI_SHM_BASE		RPI_SCMI_SHMEM_BASE
#define RPI_SHM0_BASE	RPI_SHM_BASE

static struct scmi_msg_channel scmi_channel[] = {
	[0] = {
		.shm_addr = RPI_SHM0_BASE,
		.shm_size = SMT_BUF_SLOT_SIZE,
	},
};

struct scmi_msg_channel *plat_scmi_get_channel(unsigned int agent_id)
{
	assert(agent_id < ARRAY_SIZE(scmi_channel));

	return &scmi_channel[agent_id];
}

static const char vendor[] = "EPAM";
static const char sub_vendor[] = "";

const char *plat_scmi_vendor_name(void)
{
	return vendor;
}

const char *plat_scmi_sub_vendor_name(void)
{
	return sub_vendor;
}

static const uint8_t plat_protocol_list[] = {
	SCMI_PROTOCOL_ID_RESET_DOMAIN,
	SCMI_PROTOCOL_ID_PINCTRL,
	0U /* Null termination */
};

size_t plat_scmi_protocol_count(void)
{
	return ARRAY_SIZE(plat_protocol_list) - 1U;
}

const uint8_t *plat_scmi_protocol_list(unsigned int agent_id __unused)
{
	return plat_protocol_list;
}

void rpi5_init_scmi_server(void)
{
	size_t i;

	for (i = 0U; i < ARRAY_SIZE(scmi_channel); i++) {
		scmi_smt_init_agent_channel(&scmi_channel[i]);
	}
}
