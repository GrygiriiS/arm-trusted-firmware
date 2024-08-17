// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2024, EPAM Systems
 */
#include <cdefs.h>
#include <string.h>
#include <errno.h>

#include <drivers/pinctrl.h>
#include <drivers/scmi-msg.h>
#include <drivers/scmi.h>
#include <lib/utils.h>
#include <lib/utils_def.h>

#include "common.h"

#pragma weak pinctrl_get_groups_count
#pragma weak pinctrl_get_functions_count
#pragma weak pinctrl_get_pins_count
#pragma weak pinctrl_get_pins
#pragma weak pinctrl_get_function_name
#pragma weak pinctrl_get_group_name
#pragma weak pinctrl_get_pin_name
#pragma weak pinctrl_get_group_pins
#pragma weak pinctrl_get_function_groups
#pragma weak pinctrl_request_group
#pragma weak pinctrl_free_group
#pragma weak pinctrl_request_pin
#pragma weak pinctrl_free_pin
#pragma weak pinctrl_set_mux_group
#pragma weak pinctrl_get_mux_group
#pragma weak pinctrl_set_mux_pin
#pragma weak pinctrl_get_mux_pin
#pragma weak pinctrl_config_get
#pragma weak pinctrl_config_set
#pragma weak pinctrl_config_group_get
#pragma weak pinctrl_config_group_set

uint32_t pinctrl_get_groups_count(void)
{
	return 0;
}

uint32_t pinctrl_get_functions_count(void)
{
	return 0;
}

uint32_t pinctrl_get_pins_count(void)
{
	return 0;
}

int pinctrl_get_pins(const struct pinctrl_pin **pins, uint32_t *num_pins)
{
	return -ENOTSUP;
}

const char *pinctrl_get_function_name(uint32_t func_id)
{
	return NULL;
}

const char *pinctrl_get_group_name(uint32_t group_id)
{
	return NULL;
}

const char *pinctrl_get_pin_name(uint32_t pin_id)
{
	return NULL;
}

int pinctrl_get_group_pins(uint32_t group_id, const uint32_t **pins,
			   uint32_t *num_pins)
{
	return -ENOTSUP;
}

int pinctrl_get_function_groups(uint32_t func_id, const uint32_t **groups,
				uint32_t *num_groups)
{
	return -ENOTSUP;
}

int pinctrl_request_group(uint32_t group_id)
{
	return -ENOTSUP;
}

int pinctrl_free_group(uint32_t group_id)
{
	return -ENOTSUP;
}

int pinctrl_request_pin(uint32_t pin_id)
{
	return -ENOTSUP;
}

int pinctrl_free_pin(uint32_t pin_id)
{
	return -ENOTSUP;
}

int pinctrl_set_mux_group(uint32_t group_id, uint32_t func_id)
{
	return -ENOTSUP;
}

int pinctrl_get_mux_group(uint32_t group_id, uint32_t *func_id)
{
	return -ENOTSUP;
}

int pinctrl_set_mux_pin(uint32_t pin_id, uint32_t func_id)
{
	return -ENOTSUP;
}

int pinctrl_get_mux_pin(uint32_t pin_id, uint32_t *func_id)
{
	return -ENOTSUP;
}

int pinctrl_config_get(uint32_t pin_id, uint32_t *config)
{
	return -ENOTSUP;
}

int pinctrl_config_set(uint32_t pin_id, uint32_t config)
{
	return -ENOTSUP;
}

int pinctrl_config_group_get(uint32_t group_id, uint32_t *config)
{
	return -ENOTSUP;
}

int pinctrl_config_group_set(uint32_t group_id, uint32_t config)
{
	return -ENOTSUP;
}

static bool message_id_is_supported(unsigned int message_id);

static void scmi_pinctrl_report_version(struct scmi_msg *msg)
{
	struct scmi_protocol_version_p2a return_values = {
		.status = SCMI_SUCCESS,
		.version = SCMI_PROTOCOL_VERSION_PINCTRL,
	};

	if (msg->in_size != 0U) {
		scmi_status_response(msg, SCMI_PROTOCOL_ERROR);
		return;
	}

	scmi_write_response(msg, &return_values, sizeof(return_values));
}

static void scmi_pinctrl_report_attributes(struct scmi_msg *msg)
{
	uint16_t nr_functions = pinctrl_get_functions_count();
	uint16_t nr_groups = pinctrl_get_groups_count();
	uint16_t nr_pins = pinctrl_get_pins_count();
	struct scmi_pinctrl_proro_attrs_a2p {
		int32_t status;
		uint32_t attrs_low;
		uint32_t attrs_high;
	} return_values = {.status = SCMI_SUCCESS};

	if (msg->in_size != 0U) {
		scmi_status_response(msg, SCMI_PROTOCOL_ERROR);
		return;
	}

	return_values.attrs_low =
		FLD(GENMASK(31, 16), nr_groups) | FLD(GENMASK(15, 0), nr_pins);
	return_values.attrs_high = FLD(GENMASK(15, 0), nr_functions);

	VERBOSE("scmi: pinctrl protocol_attrs group_max = %d, func_max = %d nr_pins = %d\n",
		nr_groups, nr_functions, nr_pins);

	scmi_write_response(msg, &return_values, sizeof(return_values));
}

static void scmi_pinctrl_report_message_attributes(struct scmi_msg *msg)
{
	struct scmi_protocol_message_attributes_a2p *in_args = (void *)msg->in;
	struct scmi_protocol_message_attributes_p2a return_values = {
		.status = SCMI_SUCCESS,
		/* For this protocol, attributes shall be zero */
		.attributes = 0U,
	};

	if (msg->in_size != sizeof(*in_args)) {
		scmi_status_response(msg, SCMI_PROTOCOL_ERROR);
		return;
	}

	if (!message_id_is_supported(in_args->message_id)) {
		scmi_status_response(msg, SCMI_NOT_FOUND);
		return;
	}

	scmi_write_response(msg, &return_values, sizeof(return_values));
}

static void scmi_pinctrl_attrs_rx_name(struct scmi_pinctrl_attrs_p2a *rx,
				       const char *name)
{
	assert(name != NULL);

	if (strlen(name) > sizeof(rx->name)) {
		rx->attributes |= SCMI_PINCTRL_ATTR_EXT_NAME;
	}
	strlcpy(rx->name, name, sizeof(rx->name));
}

static void scmi_pinctrl_attributes(struct scmi_msg *msg)
{
	struct scmi_pinctrl_attrs_a2p *in_args = (void *)msg->in;
	struct scmi_pinctrl_attrs_p2a return_values;
	const uint32_t *groups;
	const uint32_t *funcs;
	uint32_t selector;
	uint32_t nelems;
	int ret;

	if (msg->in_size != sizeof(*in_args) || in_args->flags >= TYPE_LAST) {
		ERROR("scmi: pinctrl pinctrl_attributes wrong format\n");
		scmi_status_response(msg, SCMI_PROTOCOL_ERROR);
		return;
	}

	VERBOSE("scmi: pinctrl attr id = %d flags = %d\n",
		in_args->identifier, in_args->flags);

	selector = in_args->identifier;

	return_values.status = SCMI_SUCCESS;

	switch (in_args->flags) {
	case PIN_TYPE:
		if (selector >= pinctrl_get_pins_count()) {
			ERROR("scmi: pinctrl get pin count failed with sel = %d\n",
			      selector);
			scmi_status_response(msg, SCMI_PROTOCOL_ERROR);
			return;
		};
		return_values.attributes = FLD(SCMI_PINCTRL_ATTR_NUM_ELEM, 1);
		scmi_pinctrl_attrs_rx_name(&return_values, pinctrl_get_pin_name(selector));
		break;
	case GROUP_TYPE:
		if (selector >= pinctrl_get_groups_count()) {
			ERROR("scmi: pinctrl_get_groups_count failed with sel = %d\n",
			      selector);
			scmi_status_response(msg, SCMI_PROTOCOL_ERROR);
			return;
		};

		ret = pinctrl_get_group_pins(selector, &groups, &nelems);
		if (ret) {
			ERROR("scmi: pinctrl_get_group_pins failed with ret = %d\n",
			      ret);
			scmi_status_response(msg, SCMI_PROTOCOL_ERROR);
			return;
		}

		return_values.attributes =
			FLD(SCMI_PINCTRL_ATTR_NUM_ELEM, nelems);
		scmi_pinctrl_attrs_rx_name(&return_values,
					   pinctrl_get_group_name(selector));
		break;
	case FUNCTION_TYPE:
		if (selector >= pinctrl_get_functions_count()) {
			ERROR("scmi: pinctrl_get_functions_count failed with sel = %d\n",
			      selector);
			scmi_status_response(msg, SCMI_PROTOCOL_ERROR);
			return;
		};

		ret = pinctrl_get_function_groups(selector, &funcs, &nelems);
		if (ret) {
			ERROR("scmi: pinctrl_get_fucntion_groups failed with ret = %d\n",
			      ret);
			scmi_status_response(msg, SCMI_PROTOCOL_ERROR);
			return;
		}

		return_values.attributes =
			FLD(SCMI_PINCTRL_ATTR_NUM_ELEM, nelems);
		scmi_pinctrl_attrs_rx_name(
			&return_values, pinctrl_get_function_name(selector));
		break;
	default:
		scmi_status_response(msg, SCMI_PROTOCOL_ERROR);
	}

	VERBOSE("scmi: pinctrl_attributes id=%d type=%u status=%08x attr=0x%08x name=%s\n",
		selector, in_args->flags, return_values.status,
		return_values.attributes, return_values.name);

	scmi_write_response(msg, &return_values, sizeof(return_values));
}

static uint32_t scmi_pinctrl_get_group_pins(struct scmi_pinctrl_assoc_a2p *tx,
					    struct scmi_pinctrl_assoc_p2a *rx)
{
	uint32_t max_payload_size = SCMI_PLAYLOAD_MAX - sizeof(*rx);
	uint32_t rx_size = sizeof(rx->status);
	uint32_t counter = 0, lcounter = 0;
	const uint32_t *pins;
	uint32_t nr_pins;
	int ret;

	VERBOSE("scmi: pinctrl assoc group sel = %d skip = %d\n",
		tx->identifier, tx->index);

	ret = pinctrl_get_group_pins(tx->identifier, &pins, &nr_pins);
	if (ret) {
		ERROR("scmi: pinctrl_get_group_pins failed with ret = %d\n",
		      ret);
		rx->status = SCMI_GENERIC_ERROR;
		return rx_size;
	}

	rx_size = sizeof(*rx);

	for (counter = tx->index; counter < nr_pins; counter++, lcounter++) {
		if (lcounter * sizeof(uint16_t) >= max_payload_size) {
			break;
		}

		rx->array[lcounter] = pins[counter];
		rx_size += sizeof(uint16_t);
		VERBOSE("group pins [%d] = %d\n", lcounter,
			rx->array[lcounter]);
	}

	rx->flags = FLD(SCMI_PINCTRL_ASSOC_REM, nr_pins - counter) |
		    FLD(SCMI_PINCTRL_ASSOC_RET, lcounter);
	rx->status = SCMI_SUCCESS;

	rx_size = ALIGN_NEXT(rx_size, sizeof(uint32_t));

	VERBOSE("scmi: pinctrl assoc group sel = %d nr_pins= %d cnt_ret = %d sz=%u\n",
		tx->identifier, nr_pins, lcounter, rx_size);

	return rx_size;
}

static uint32_t
scmi_pinctrl_get_function_groups(struct scmi_pinctrl_assoc_a2p *tx,
				 struct scmi_pinctrl_assoc_p2a *rx)
{
	uint32_t max_payload_size = SCMI_PLAYLOAD_MAX - sizeof(*rx);
	uint32_t rx_size = sizeof(rx->status);
	uint32_t counter = 0, lcounter = 0;
	const uint32_t *groups;
	uint32_t nr_groups;
	int ret;

	VERBOSE("scmi: pinctrl assoc func sel = %d skip = %d\n", tx->identifier,
		tx->index);

	ret = pinctrl_get_function_groups(tx->identifier, &groups, &nr_groups);
	if (ret) {
		ERROR("scmi: pinctrl_get_fucntion_groups failed with ret = %d\n",
		      ret);
		rx->status = SCMI_GENERIC_ERROR;
		return rx_size;
	}

	rx_size = sizeof(*rx);

	for (counter = tx->index; counter < nr_groups; counter++, lcounter++) {
		if (lcounter * sizeof(uint16_t) >= max_payload_size) {
			break;
		}

		rx->array[lcounter] = groups[counter];
		rx_size += sizeof(uint16_t);
		VERBOSE("func groups [%d] = %d\n", lcounter, groups[counter]);
	}

	rx->flags = FLD(SCMI_PINCTRL_ASSOC_REM, nr_groups - counter) |
		    FLD(SCMI_PINCTRL_ASSOC_RET, lcounter);
	rx->status = SCMI_SUCCESS;

	rx_size = ALIGN_NEXT(rx_size, sizeof(uint32_t));

	VERBOSE("scmi: pinctrl assoc func sel = %d nr_pins= %d cnt_ret = %d sz=%u\n",
		tx->identifier, nr_groups, lcounter, rx_size);

	return rx_size;
};

static void scmi_pinctrl_list_associations(struct scmi_msg *msg)
{
	struct scmi_pinctrl_assoc_a2p *tx =
		(struct scmi_pinctrl_assoc_a2p *)msg->in;
	struct scmi_pinctrl_assoc_p2a *rx =
		(struct scmi_pinctrl_assoc_p2a *)msg->out;

	VERBOSE("scmi: pinctrl_list_assoc id = %d flags = 0x%08x idx = %d\n",
		tx->identifier, tx->flags, tx->index);

	if (msg->in_size != sizeof(*tx) || (tx->flags >= TYPE_LAST)) {
		ERROR("scmi: pinctrl assoc wrong format\n");
		scmi_status_response(msg, SCMI_PROTOCOL_ERROR);
		return;
	}

	switch (tx->flags) {
	case GROUP_TYPE:
		msg->out_size_out = scmi_pinctrl_get_group_pins(tx, rx);
		break;
	case FUNCTION_TYPE:
		msg->out_size_out = scmi_pinctrl_get_function_groups(tx, rx);
		break;
	default:
		scmi_status_response(msg, SCMI_PROTOCOL_ERROR);
		return;
	}
}

static int scmi_pinctrl_cfg_scmi2pinconf(enum scmi_pinctrl_conf_type param,
					 enum pin_config_param  *type)
{
	switch (param) {
	case SCMI_PIN_BIAS_BUS_HOLD:
		*type = PIN_CONFIG_BIAS_BUS_HOLD;
		break;
	case SCMI_PIN_BIAS_DISABLE:
		*type = PIN_CONFIG_BIAS_DISABLE;
		break;
	case SCMI_PIN_BIAS_HIGH_IMPEDANCE:
		*type = PIN_CONFIG_BIAS_HIGH_IMPEDANCE;
		break;
	case SCMI_PIN_BIAS_PULL_DOWN:
		*type = PIN_CONFIG_BIAS_PULL_DOWN;
		break;
	case SCMI_PIN_BIAS_PULL_DEFAULT:
		*type = PIN_CONFIG_BIAS_PULL_PIN_DEFAULT;
		break;
	case SCMI_PIN_BIAS_PULL_UP:
		*type = PIN_CONFIG_BIAS_PULL_UP;
		break;
	case SCMI_PIN_DRIVE_OPEN_DRAIN:
		*type = PIN_CONFIG_DRIVE_OPEN_DRAIN;
		break;
	case SCMI_PIN_DRIVE_OPEN_SOURCE:
		*type = PIN_CONFIG_DRIVE_OPEN_SOURCE;
		break;
	case SCMI_PIN_DRIVE_PUSH_PULL:
		*type = PIN_CONFIG_DRIVE_PUSH_PULL;
		break;
	case SCMI_PIN_DRIVE_STRENGTH:
		*type = PIN_CONFIG_DRIVE_STRENGTH;
		/* PIN_CONFIG_DRIVE_STRENGTH_UA */
		break;
	case SCMI_PIN_INPUT_DEBOUNCE:
		*type = PIN_CONFIG_INPUT_DEBOUNCE;
		break;
	case SCMI_PIN_INPUT_MODE:
		*type = PIN_CONFIG_INPUT_ENABLE;
		/* PIN_CONFIG_INPUT_SCHMITT_ENABLE */
		break;
	case SCMI_PIN_INPUT_SCHMITT:
		*type = PIN_CONFIG_INPUT_SCHMITT;
		break;
	case SCMI_PIN_LOW_POWER_MODE:
		*type = PIN_CONFIG_MODE_LOW_POWER;
		break;
	case SCMI_PIN_OUTPUT_VALUE:
		*type = PIN_CONFIG_OUTPUT;
		/* PIN_CONFIG_OUTPUT_IMPEDANCE_OHMS */
		break;
	case SCMI_PIN_OUTPUT_MODE:
		*type = PIN_CONFIG_OUTPUT_ENABLE;
		break;
	case SCMI_PIN_POWER_SOURCE:
		*type = PIN_CONFIG_POWER_SOURCE;
		break;
	case SCMI_PIN_SLEW_RATE:
		*type = PIN_CONFIG_SLEW_RATE;
		break;
	default:
		return -SCMI_INVALID_PARAMETERS;
	}

	return 0;
}

static void scmi_pinctrl_config_get(struct scmi_msg *msg)
{
	struct scmi_pinctrl_settings_get_a2p *tx =
		(struct scmi_pinctrl_settings_get_a2p *)msg->in;
	struct scmi_pinctrl_settings_get_p2a *rx =
		(struct scmi_pinctrl_settings_get_p2a *)msg->out;
	enum scmi_pinctrl_selector_type type;
	enum scmi_pinctrl_conf_type scmi_conf_type;
	enum pin_config_param param;
	uint32_t pin_config;
	uint32_t rx_size = sizeof(*rx);
	uint8_t cfg_flag;
	int ret;

	VERBOSE("scmi: pinctrl cfg get id = %d attr=0x%08x\n", tx->identifier,
		tx->attributes);

	type = FLD_GET(SCMI_PINCTRL_CFGG_SEL_TYPE, tx->attributes);
	cfg_flag = FLD_GET(SCMI_PINCTRL_CFGG_FLAG, tx->attributes);
	scmi_conf_type = FLD_GET(SCMI_PINCTRL_CFGG_TYPE, tx->attributes);

	if (msg->in_size < sizeof(*tx) || (type > GROUP_TYPE)) {
		ERROR("scmi: pinctrl cfg get wrong format\n");
		scmi_status_response(msg, SCMI_PROTOCOL_ERROR);
		return;
	}

	if (cfg_flag > SCMI_PINCTRL_CFGG_ONLY_FUNC) {
		ERROR("scmi: pinctrl cfg get wrong config flag format %u\n",
		      cfg_flag);
		scmi_status_response(msg, SCMI_PROTOCOL_ERROR);
		return;
	}

	if (cfg_flag == SCMI_PINCTRL_CFGG_ALL) {
		ERROR("scmi: pinctrl cfg get config flag=2 (GET ALL) not supported\n");
		scmi_status_response(msg, SCMI_NOT_SUPPORTED);
		return;
	}

	if (FLD_GET(SCMI_PINCTRL_CFGG_SKIP, tx->attributes)) {
		ERROR("scmi: pinctrl cfg get only single config is supported\n");
		scmi_status_response(msg, SCMI_NOT_SUPPORTED);
		return;
	}

	if (type == GROUP_TYPE) {
		ret = pinctrl_get_mux_group(tx->identifier,
					    &rx->function_selected);
	} else {
		ret = pinctrl_get_mux_pin(tx->identifier,
					  &rx->function_selected);
	}

	if (ret) {
		ERROR("scmi: pinctrl cfg get mux failed (%d)\n", ret);
		scmi_status_response(msg, SCMI_GENERIC_ERROR);
		return;
	}

	rx->status = SCMI_SUCCESS;
	rx->num_configs = 0;

	if (cfg_flag == SCMI_PINCTRL_CFGG_ONLY_FUNC) {
		goto exit;
	}

	ret = scmi_pinctrl_cfg_scmi2pinconf(scmi_conf_type, &param);
	if (ret) {
		ERROR("scmi: pinctrl cfg get invalid cfg_type=%d\n",
		      scmi_conf_type);
		scmi_status_response(msg, ret);
		return;
	}

	pin_config = pinconf_to_config_packed(param, 0);
	if (type == GROUP_TYPE) {
		ret = pinctrl_config_group_get(tx->identifier, &pin_config);
	} else {
		ret = pinctrl_config_get(tx->identifier, &pin_config);
	}

	if (ret) {
		if (ret != -ENOTSUP) {
			ERROR("scmi: pinctrl cfg get config failed (%d)\n",
			      ret);
			scmi_status_response(msg, SCMI_GENERIC_ERROR);
		} else {
			scmi_status_response(msg, SCMI_NOT_SUPPORTED);
		}
		return;
	}

	rx->num_configs = 1;
	rx->configs[0] = scmi_conf_type;
	rx->configs[1] = pinconf_to_config_argument(pin_config);
	rx_size += 2 * sizeof(uint32_t);

	VERBOSE("scmi: pinctrl cfg get id=%d func=0x%08x num_configs=0x%08x cfg=%u cfg_arg=%u\n",
		tx->identifier, rx->function_selected, rx->num_configs,
		rx->configs[0], rx->configs[1]);
exit:
	msg->out_size_out = rx_size;
}

static int scmi_pinctrl_config_mux(struct scmi_pinctrl_settings_conf_a2p *tx,
				   enum scmi_pinctrl_selector_type type)
{
	int ret;

	if (!(tx->attributes & SCMI_PINCTRL_CFG_FUNC_VALID)) {
		return SCMI_SUCCESS;
	}

	if (tx->function_id == SCMI_PINCTRL_CFG_NO_FUNC) {
		ERROR("scmi: pinctrl cfg func valid but not set\n");
		return SCMI_PROTOCOL_ERROR;
	}

	if (tx->function_id >= pinctrl_get_functions_count()) {
		ERROR("scmi: pinctrl cfg group id invalid if d %d\n",
		      tx->function_id);
		return SCMI_PROTOCOL_ERROR;
	}

	VERBOSE("scmi: pinctrl cfg mux id=%d func=%u\n", tx->identifier,
		tx->function_id);

	if (type == GROUP_TYPE) {
		ret = pinctrl_set_mux_group(tx->identifier, tx->function_id);
	} else {
		ret = pinctrl_set_mux_pin(tx->identifier, tx->function_id);
	}

	if (ret) {
		return SCMI_GENERIC_ERROR;
	}

	return SCMI_SUCCESS;
}

static int scmi_pinctrl_config_cfg(unsigned int selector,
				   enum scmi_pinctrl_selector_type type,
				   uint32_t nr_cfg, uint32_t *cfgs)
{
	int ret;
	int i;

	if (!nr_cfg) {
		return SCMI_SUCCESS;
	}

	for (i = 0; i < nr_cfg; i++) {
		enum scmi_pinctrl_conf_type scmi_type = cfgs[i * 2];
		int scmi_arg = cfgs[i * 2 + 1];
		enum pin_config_param pinconf_type;
		unsigned long config;

		VERBOSE("scmi: pinctrl set sel:%u cfg:%i  %08x:%08x\n",
			selector, i, cfgs[i * 2], cfgs[i * 2 + 1]);

		ret = scmi_pinctrl_cfg_scmi2pinconf(scmi_type, &pinconf_type);
		if (ret) {
			ERROR("scmi: pinctrl set cfg=%d invalid cfg_type=%d\n",
			      i, scmi_type);
			return ret;
		}

		config = pinconf_to_config_packed(pinconf_type, scmi_arg);

		if (type == GROUP_TYPE) {
			ret = pinctrl_config_group_set(selector, config);
		} else {
			ret = pinctrl_config_set(selector, config);
		}

		if (ret) {
			ERROR("scmi: pinctrl set cfg=%d cfg_type=%d (%d)\n", i,
			      scmi_type, ret);
			return SCMI_GENERIC_ERROR;
		}
	}

	return 0;
}

static void scmi_pinctrl_config_set(struct scmi_msg *msg)
{
	struct scmi_pinctrl_settings_conf_a2p *tx =
		(struct scmi_pinctrl_settings_conf_a2p *)msg->in;
	enum scmi_pinctrl_selector_type type;
	uint32_t nr_cfg;
	int ret;

	VERBOSE("scmi: pinctrl cfg id = %d func=0x%08x attr=0x%08x\n",
		tx->identifier, tx->function_id, tx->attributes);

	type = FLD_GET(SCMI_PINCTRL_CFG_SEL_TYPE, tx->attributes);
	nr_cfg = FLD_GET(SCMI_PINCTRL_CFG_NUM, tx->attributes);

	if (msg->in_size < sizeof(*tx) || (type > GROUP_TYPE)) {
		ERROR("scmi: pinctrl cfg assoc wrong format\n");
		scmi_status_response(msg, SCMI_PROTOCOL_ERROR);
		return;
	}

	if (type == PIN_TYPE && tx->identifier >= pinctrl_get_pins_count()) {
		ERROR("scmi: pinctrl cfg pin id invalid id = %d\n",
		      tx->identifier);
		scmi_status_response(msg, SCMI_PROTOCOL_ERROR);
		return;
	}

	if (type == GROUP_TYPE && tx->identifier >= pinctrl_get_groups_count()) {
		ERROR("scmi: pinctrl cfg group id invalid if d %d\n",
		      tx->identifier);
		scmi_status_response(msg, SCMI_PROTOCOL_ERROR);
		return;
	}

	ret = scmi_pinctrl_config_mux(tx, type);
	if (ret) {
		scmi_status_response(msg, ret);
		return;
	}

	ret = scmi_pinctrl_config_cfg(tx->identifier, type, nr_cfg, tx->configs);
	if (ret) {
		scmi_status_response(msg, ret);
		return;
	}

	scmi_status_response(msg, SCMI_SUCCESS);
}

static void scmi_pinctrl_request(struct scmi_msg *msg)
{
	struct scmi_pinctrl_request_a2p *tx = (void *)msg->in;
	enum scmi_pinctrl_selector_type type;
	int ret;

	if (msg->in_size != sizeof(*tx)) {
		ERROR("scmi: pinctrl req wrong format\n");
		scmi_status_response(msg, SCMI_PROTOCOL_ERROR);
		return;
	}

	type = FLD_GET(SCMI_PINCTRL_REQ_SEL_TYPE, tx->flags);

	if (type > GROUP_TYPE) {
		ERROR("scmi: pinctrl req wrong sel_type %d\n", type);
		scmi_status_response(msg, SCMI_INVALID_PARAMETERS);
		return;
	}

	if (type == GROUP_TYPE) {
		ret = pinctrl_request_group(tx->identifier);
	} else {
		ret = pinctrl_request_pin(tx->identifier);
	}

	if (ret) {
		ERROR("scmi: pinctrl_req failed with ret = %d\n", ret);
		scmi_status_response(msg, SCMI_GENERIC_ERROR);
		return;
	}

	scmi_status_response(msg, SCMI_SUCCESS);
}

static void scmi_pinctrl_release(struct scmi_msg *msg)
{
	struct scmi_pinctrl_release_a2p *tx = (void *)msg->in;
	enum scmi_pinctrl_selector_type type;
	int ret;

	if (msg->in_size != sizeof(*tx)) {
		ERROR("scmi: pinctrl free wrong format\n");
		scmi_status_response(msg, SCMI_PROTOCOL_ERROR);
		return;
	}

	type = FLD_GET(SCMI_PINCTRL_REL_SEL_TYPE, tx->flags);

	if (type > GROUP_TYPE) {
		ERROR("scmi: pinctrl free wrong sel_type %d\n", type);
		scmi_status_response(msg, SCMI_INVALID_PARAMETERS);
		return;
	}

	if (type == GROUP_TYPE) {
		ret = pinctrl_free_group(tx->identifier);
	} else {
		ret = pinctrl_free_pin(tx->identifier);
	}

	if (ret) {
		ERROR("scmi: pinctrl_free_pin failed with ret = %d\n", ret);
		scmi_status_response(msg, SCMI_GENERIC_ERROR);
		return;
	}

	scmi_status_response(msg, SCMI_SUCCESS);
}

static void scmi_pinctrl_name_get(struct scmi_msg *msg)
{
	struct scmi_pinctrl_name_a2p *tx = (void *)msg->in;
	struct scmi_pinctrl_name_p2a rx = {
		.status = SCMI_SUCCESS,
		.flags = 0,
	};
	const struct pinctrl_pin *pins;
	const char *name;
	unsigned int nr_pins;
	int ret;

	if (msg->in_size != sizeof(*tx) || (tx->flags >= TYPE_LAST)) {
		scmi_status_response(msg, SCMI_PROTOCOL_ERROR);
		return;
	}

	VERBOSE("scmi: pinctrl_name_get id = %d flags = %d\n", tx->identifier,
		tx->flags);

	switch (tx->flags) {
	case PIN_TYPE:
		ret = pinctrl_get_pins(&pins, &nr_pins);
		if (ret) {
			ERROR("scmi_name_get: unable to get pins: %d\n", ret);
			scmi_status_response(msg, SCMI_GENERIC_ERROR);
			return;
		}

		name = pins[tx->identifier].name;
		break;
	case GROUP_TYPE:
		name = pinctrl_get_group_name(tx->identifier);
		break;
	case FUNCTION_TYPE:
		name = pinctrl_get_function_name(tx->identifier);
		break;
	default:
		scmi_status_response(msg, SCMI_PROTOCOL_ERROR);
		return;
	}

	strlcpy(rx.name, name, sizeof(rx.name));

	scmi_write_response(msg, &rx, sizeof(rx));

	VERBOSE("scmi: pinctrl_name_get id = %d name = %s\n", tx->identifier,
		rx.name);
}

static void scmi_pinctrl_set_permissions(struct scmi_msg *msg)
{
	scmi_status_response(msg, SCMI_NOT_SUPPORTED);
}

static const scmi_msg_handler_t scmi_pinctrl_handler_table[] = {
	[SCMI_PROTOCOL_VERSION] = scmi_pinctrl_report_version,
	[SCMI_PROTOCOL_ATTRIBUTES] = scmi_pinctrl_report_attributes,
	[SCMI_PROTOCOL_MESSAGE_ATTRIBUTES] =
		scmi_pinctrl_report_message_attributes,
	[SCMI_PINCTRL_ATTRIBUTES] = scmi_pinctrl_attributes,
	[SCMI_PINCTRL_LIST_ASSOCIATIONS] = scmi_pinctrl_list_associations,
	[SCMI_PINCTRL_SETTINGS_GET] = scmi_pinctrl_config_get,
	[SCMI_PINCTRL_SETTINGS_CONFIGURE] = scmi_pinctrl_config_set,
	[SCMI_PINCTRL_REQUEST] = scmi_pinctrl_request,
	[SCMI_PINCTRL_RELEASE] = scmi_pinctrl_release,
	[SCMI_PINCTRL_NAME_GET] = scmi_pinctrl_name_get,
	[SCMI_PINCTRL_SET_PERMISSIONS] = scmi_pinctrl_set_permissions
};

static bool message_id_is_supported(unsigned int message_id)
{
	return (message_id < ARRAY_SIZE(scmi_pinctrl_handler_table)) &&
	       (scmi_pinctrl_handler_table[message_id] != NULL);
}

scmi_msg_handler_t scmi_msg_get_pinctrl_handler(struct scmi_msg *msg)
{
	unsigned int message_id = SPECULATION_SAFE_VALUE(msg->message_id);

	if (message_id >= ARRAY_SIZE(scmi_pinctrl_handler_table)) {
		VERBOSE("Pinctrl domain handle not found %u\n",
			msg->message_id);
		return NULL;
	}

	return scmi_pinctrl_handler_table[message_id];
}
