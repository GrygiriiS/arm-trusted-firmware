/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2023, EPAM Systems
 */
#ifndef SCMI_MSG_PINCTRL_H
#define SCMI_MSG_PINCTRL_H

#include <stdbool.h>
#include <stdint.h>

#include <lib/utils_def.h>

#define SCMI_PROTOCOL_VERSION_PINCTRL 0x10000U

#define SCMI_PINCTRL_ATTR_NAME_SZ 16U
#define SCMI_PINCTRL_NAME_MAX_SZ  64U

#define ALIGN_NEXT(VALUE, INTERVAL)                                            \
	((((VALUE) + (INTERVAL)-1) / (INTERVAL)) * (INTERVAL))

/*
 * Identifiers of the SCMI Pinctrl Domain Management Protocol commands
 */
enum scmi_pinctrl_domain_command_id {
	SCMI_PINCTRL_ATTRIBUTES = 0x03,
	SCMI_PINCTRL_LIST_ASSOCIATIONS = 0x04,
	SCMI_PINCTRL_SETTINGS_GET = 0x05,
	SCMI_PINCTRL_SETTINGS_CONFIGURE = 0x6,
	SCMI_PINCTRL_REQUEST = 0x7,
	SCMI_PINCTRL_RELEASE = 0x8,
	SCMI_PINCTRL_NAME_GET = 0x9,
	SCMI_PINCTRL_SET_PERMISSIONS = 0xa,
};

#define FLD(mask, val)     (((val) << (__builtin_ffsll(mask) - 1) & (mask)))
#define FLD_GET(mask, val) (((val) & (mask)) >> (__builtin_ffsll(mask) - 1))

enum scmi_pinctrl_selector_type {
	PIN_TYPE = 0,
	GROUP_TYPE,
	FUNCTION_TYPE,
	TYPE_LAST,
};

enum scmi_pinctrl_conf_type {
	SCMI_PIN_DEFAULT = 0,
	SCMI_PIN_BIAS_BUS_HOLD = 1,
	SCMI_PIN_BIAS_DISABLE = 2,
	SCMI_PIN_BIAS_HIGH_IMPEDANCE = 3,
	SCMI_PIN_BIAS_PULL_UP = 4,
	SCMI_PIN_BIAS_PULL_DEFAULT = 5,
	SCMI_PIN_BIAS_PULL_DOWN = 6,
	SCMI_PIN_DRIVE_OPEN_DRAIN = 7,
	SCMI_PIN_DRIVE_OPEN_SOURCE = 8,
	SCMI_PIN_DRIVE_PUSH_PULL = 9,
	SCMI_PIN_DRIVE_STRENGTH = 10,
	SCMI_PIN_INPUT_DEBOUNCE = 11,
	SCMI_PIN_INPUT_MODE = 12,
	SCMI_PIN_PULL_MODE = 13,
	SCMI_PIN_INPUT_VALUE = 14,
	SCMI_PIN_INPUT_SCHMITT = 15,
	SCMI_PIN_LOW_POWER_MODE = 16,
	SCMI_PIN_OUTPUT_MODE = 17,
	SCMI_PIN_OUTPUT_VALUE = 18,
	SCMI_PIN_POWER_SOURCE = 19,
	SCMI_PIN_SLEW_RATE = 20,
	SCMI_PIN_OEM_START = 192,
	SCMI_PIN_OEM_END = 255,
};

/*
 * PROTOCOL_ATTRIBUTES
 */

struct scmi_pinctrl_proro_attrs_a2p {
	int32_t status;
	uint32_t attrs_low;
	uint32_t attrs_high;
};

/*
 * PINCTRL_ATTRIBUTES
 */

struct scmi_pinctrl_attrs_a2p {
	uint32_t identifier;
	uint32_t flags;
};

#define SCMI_PINCTRL_ATTR_EXT_NAME BIT(31)
#define SCMI_PINCTRL_ATTR_NUM_ELEM GENMASK(15, 0)

struct scmi_pinctrl_attrs_p2a {
	int32_t status;
	uint32_t attributes;
	char name[SCMI_PINCTRL_ATTR_NAME_SZ];
};

/*
 * PINCTRL_LIST_ASSOCIATIONS
 */

#define SCMI_PINCTRL_ASSOC_TYPE GENMASK(1, 0)

struct scmi_pinctrl_assoc_a2p {
	uint32_t identifier;
	uint32_t flags;
	uint32_t index;
};

#define SCMI_PINCTRL_ASSOC_REM GENMASK(31, 16)
#define SCMI_PINCTRL_ASSOC_RET GENMASK(11, 0)

struct scmi_pinctrl_assoc_p2a {
	int32_t status;
	uint32_t flags;
	uint16_t array[];
};

/*
 * PINCTRL_SETTINGS_GET
 */

#define SCMI_PINCTRL_CFGG_FLAG      GENMASK(19, 18)
#define SCMI_PINCTRL_CFGG_ONE       0
#define SCMI_PINCTRL_CFGG_ALL       1
#define SCMI_PINCTRL_CFGG_ONLY_FUNC 2
#define SCMI_PINCTRL_CFGG_SEL_TYPE  GENMASK(17, 16)
#define SCMI_PINCTRL_CFGG_SKIP      GENMASK(15, 8)
#define SCMI_PINCTRL_CFGG_TYPE      GENMASK(7, 0)

struct scmi_pinctrl_settings_get_a2p {
	uint32_t identifier;
	uint32_t attributes;
};

struct scmi_pinctrl_settings_get_p2a {
	int32_t status;
	uint32_t function_selected;
	uint32_t num_configs;
	uint32_t configs[];
};

/*
 * PINCTRL_SETTINGS_CONFIGURE
 */

#define SCMI_PINCTRL_CFG_NO_FUNC    0xFFFFFFFF
#define SCMI_PINCTRL_CFG_FUNC_VALID BIT(10)
#define SCMI_PINCTRL_CFG_NUM        GENMASK(9, 2)
#define SCMI_PINCTRL_CFG_SEL_TYPE       GENMASK(1, 0)

struct scmi_pinctrl_settings_conf_a2p {
	uint32_t identifier;
	uint32_t function_id;
	uint32_t attributes;
	uint32_t configs[];
};

/*
 * PINCTRL_REQUEST
 */

#define SCMI_PINCTRL_REQ_SEL_TYPE       GENMASK(1, 0)

struct scmi_pinctrl_request_a2p {
	uint32_t identifier;
	uint32_t flags;
};

/*
 * PINCTRL_RELEASE
 */

#define SCMI_PINCTRL_REL_SEL_TYPE       GENMASK(1, 0)

struct scmi_pinctrl_release_a2p {
	uint32_t identifier;
	uint32_t flags;
};

/*
 * PINCTRL_NAME_GET
 */

struct scmi_pinctrl_name_a2p {
	uint32_t identifier;
	uint32_t flags;
};

struct scmi_pinctrl_name_p2a {
	int32_t status;
	uint32_t flags;
	char name[SCMI_PINCTRL_NAME_MAX_SZ];
};

#endif /* SCMI_MSG_PINCTRL_H */
