/*
 * Copyright (c) 2015-2021, Renesas Electronics Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MSSR_H
#define MSSR_H

void cpg_write_32(uint32_t addr, uint32_t val);

void rcar_mssr_clock(int n, uint32_t data, bool on, bool force);
bool rcar_mssr_clock_state(uint32_t reg, uint32_t bit);
void rcar_mssr_soft_reset(int n, uint32_t data, bool assert, bool force);
void rcar_mssr_setup(void);

#endif /* MSSR_H */
