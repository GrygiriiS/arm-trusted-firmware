// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2024, EPAM Systems
 */
#include <cdefs.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include <drivers/pinctrl.h>
#include <lib/utils.h>

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
