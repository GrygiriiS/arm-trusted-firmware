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
#include "rpi5_scmi_resources.h"

#define RPI_SHM_N_BASE(n)	(RPI_SCMI_SHMEM_BASE + n * PAGE_SIZE)

static struct scmi_msg_channel scmi_channel[SCMI_NUM_AGENTS] = {
	[0] = {
		.shm_addr = RPI_SHM_N_BASE(0),
		.shm_size = SMT_BUF_SLOT_SIZE,
		.agent_name = "agent-0",
	},
	[1] = {
		.shm_addr = RPI_SHM_N_BASE(1),
		.shm_size = SMT_BUF_SLOT_SIZE,
		.agent_name = "agent-1",
	},
	[2] = {
		.shm_addr = RPI_SHM_N_BASE(2),
		.shm_size = SMT_BUF_SLOT_SIZE,
		.agent_name = "agent-2",
	},
	[3] = {
		.shm_addr = RPI_SHM_N_BASE(3),
		.shm_size = SMT_BUF_SLOT_SIZE,
		.agent_name = "agent-3",
	},
	[4] = {
		.shm_addr = RPI_SHM_N_BASE(4),
		.shm_size = SMT_BUF_SLOT_SIZE,
		.agent_name = "agent-4",
	},
	[5] = {
		.shm_addr = RPI_SHM_N_BASE(5),
		.shm_size = SMT_BUF_SLOT_SIZE,
		.agent_name = "agent-5",
	},
	[6] = {
		.shm_addr = RPI_SHM_N_BASE(6),
		.shm_size = SMT_BUF_SLOT_SIZE,
		.agent_name = "agent-6",
	},
	[7] = {
		.shm_addr = RPI_SHM_N_BASE(7),
		.shm_size = SMT_BUF_SLOT_SIZE,
		.agent_name = "agent-7",
	},
};

struct scmi_msg_channel *plat_scmi_get_channel(unsigned int agent_id)
{
	assert(agent_id < ARRAY_SIZE(scmi_channel));

	return &scmi_channel[agent_id];
}

const char *plat_scmi_agent_get_name(unsigned int agent_id)
{
	assert(agent_id < ARRAY_SIZE(scmi_channel));

	return scmi_channel[agent_id].agent_name;
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

uint32_t plat_scmi_agent_count(void)
{
	return SCMI_NUM_AGENTS;
}

uint32_t plat_scmi_device_count(void)
{
	return RPI5_SCMI_DEV_COUNT;
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
