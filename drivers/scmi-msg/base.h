/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2015-2019, Arm Limited and Contributors. All rights reserved.
 * Copyright (c) 2019-2020, Linaro Limited
 */

#ifndef SCMI_MSG_BASE_H
#define SCMI_MSG_BASE_H

#include <stdint.h>

#define SCMI_PROTOCOL_VERSION_BASE	0x20000U

#define SCMI_DEFAULT_STRING_LENGTH	16U

enum scmi_base_message_id {
	SCMI_BASE_DISCOVER_VENDOR			= 0x003,
	SCMI_BASE_DISCOVER_SUB_VENDOR			= 0x004,
	SCMI_BASE_DISCOVER_IMPLEMENTATION_VERSION	= 0x005,
	SCMI_BASE_DISCOVER_LIST_PROTOCOLS		= 0x006,
	SCMI_BASE_DISCOVER_AGENT			= 0x007,
	SCMI_BASE_NOTIFY_ERRORS				= 0x008,
	SCMI_BASE_SET_DEVICE_PERMISSIONS		= 0x009,
	SCMI_BASE_SET_PROTOCOL_PERMISSIONS		= 0x00A,
	SCMI_BASE_RESET_AGENT_CONFIGURATION		= 0x00B,
};

/*
 * PROTOCOL_ATTRIBUTES
 */

#define SCMI_BASE_PROTOCOL_ATTRS_NUM_PROTOCOLS_POS	0
#define SCMI_BASE_PROTOCOL_ATTRS_NUM_AGENTS_POS		8

#define SCMI_BASE_PROTOCOL_ATTRS_NUM_PROTOCOLS_MASK	0xFFU
#define SCMI_BASE_PROTOCOL_ATTRS_NUM_AGENTS_MASK	0xFF00U

#define SCMI_BASE_PROTOCOL_ATTRIBUTES(NUM_PROTOCOLS, NUM_AGENTS) \
	((((NUM_PROTOCOLS) << SCMI_BASE_PROTOCOL_ATTRS_NUM_PROTOCOLS_POS) & \
	  SCMI_BASE_PROTOCOL_ATTRS_NUM_PROTOCOLS_MASK) | \
	(((NUM_AGENTS) << SCMI_BASE_PROTOCOL_ATTRS_NUM_AGENTS_POS) & \
	 SCMI_BASE_PROTOCOL_ATTRS_NUM_AGENTS_MASK))

/* Value for scmi_base_set_device_permissions_p2a:flags */
#define SCMI_BASE_ACCESS_TYPE			BIT(0)

/* Value for scmi_base_reset_agent_configuration_p2a:flags */
#define SCMI_BASE_PERMISSIONS_RESET		BIT(0)


/*
 * BASE_DISCOVER_VENDOR
 */
struct scmi_base_discover_vendor_p2a {
	int32_t status;
	char vendor_identifier[SCMI_DEFAULT_STRING_LENGTH];
};

/*
 * BASE_DISCOVER_SUB_VENDOR
 */
struct scmi_base_discover_sub_vendor_p2a {
	int32_t status;
	char sub_vendor_identifier[SCMI_DEFAULT_STRING_LENGTH];
};

/*
 * BASE_DISCOVER_IMPLEMENTATION_VERSION
 * No special structure right now, see protocol_version.
 */

/*
 * BASE_DISCOVER_LIST_PROTOCOLS
 */
struct scmi_base_discover_list_protocols_a2p {
	uint32_t skip;
};

struct scmi_base_discover_list_protocols_p2a {
	int32_t status;
	uint32_t num_protocols;
	uint32_t protocols[];
};

/*
 * BASE_DISCOVER_AGENT
 */
#define SCMI_BASE_AGENT_ID_OWN 0xFFFFFFFF

struct scmi_base_discover_agent_a2p {
	uint32_t agent_id;
};

struct scmi_base_discover_agent_p2a {
	int32_t status;
	uint32_t agent_id;
	char name[SCMI_DEFAULT_STRING_LENGTH];
};


/*
 * BASE_SET_DEVICE_PERMISSIONS
 */
struct scmi_base_set_device_permissions_a2p {
	uint32_t agent_id;
	uint32_t device_id;
	uint32_t flags;
};

struct scmi_base_set_device_permissions_p2a {
	int32_t status;
};

/*
 * BASE_RESET_AGENT_CONFIGURATION
 */
struct scmi_base_reset_agent_configuration_a2p {
	uint32_t agent_id;
	uint32_t flags;
};

struct scmi_base_reset_agent_configuration_p2a {
	int32_t status;
};

#endif /* SCMI_MSG_BASE_H */
