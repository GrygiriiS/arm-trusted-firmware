/*
 * Copyright (c) 2015-2022, Renesas Electronics Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <lib/mmio.h>
#include <lib/utils_def.h>

#include <cpg_registers.h>

void cpg_write_32(uint32_t addr, uint32_t val)
{
	mmio_write_32(CPG_CPGWPR, ~val);
	mmio_write_32(addr, val);
}

void rcar_mssr_clock(int n, uint32_t data, bool on, bool force)
{
	uint32_t prev_status, next_status;

	prev_status = mmio_read_32(MSSR_MSTPSR(n));

	if (on)
		next_status = prev_status & ~data;
	else
		next_status = prev_status | data;

	if (!force && (prev_status == next_status))
		return;

	cpg_write_32(MSSR_MSTPCR(n), next_status);

	if (on)
		while (data & mmio_read_32(MSSR_MSTPSR(n)));
	else
		while (!(data & mmio_read_32(MSSR_MSTPSR(n))));
}

bool rcar_mssr_clock_state(uint32_t reg, uint32_t bit)
{
	uint32_t status;

	status = mmio_read_32(MSSR_MSTPSR(reg));

	return !(status & BIT(bit));
}

void rcar_mssr_soft_reset(int n, uint32_t data, bool assert, bool force)
{
	uint32_t prev_status, next_status;

	prev_status = mmio_read_32(MSSR_SRCR(n));

	if (assert)
		next_status = prev_status | data;
	else
		next_status = prev_status & ~data;

	if (!force && (prev_status == next_status))
		return;

	if (assert)
		cpg_write_32(MSSR_SRCR(n), data);
	else
		cpg_write_32(MSSR_SRSTCLR(n), data);
}

void rcar_mssr_setup(void)
{
	/* INTC-AP de-assert */
	rcar_mssr_soft_reset(5,  BIT(31), 0, 0);
	rcar_mssr_soft_reset(11, BIT(19), 0, 0);

	/* INTC-AP clock on */
	rcar_mssr_clock(5, BIT(31), 1, 0);
}
