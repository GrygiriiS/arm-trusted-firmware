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

#include "rcar_scmi_resources.h"
#include "r8a779g0-cpg-mssr.h"
#include "r8a779g0_clks_scmi.h"
#include "r8a779g0_resets_scmi.h"

enum scmi_device_ids_t {
	GEN4_SCMI_DEV_I2C1_E6508000 = 0,
	GEN4_SCMI_DEV_FCP0_fea10000 = 1,
	GEN4_SCMI_DEV_FCP1_fea11000 = 2,
	GEN4_SCMI_DEV_vspd0_fea20000 = 3,
	GEN4_SCMI_DEV_vspd1_fea28000 = 4,
	GEN4_SCMI_DEV_isp0_fed00000 = 5,
	GEN4_SCMI_DEV_isp1_fed20000 = 6,
	GEN4_SCMI_DEV_vin00_e6ef0000 = 7,
	GEN4_SCMI_DEV_vin01_e6ef1000 = 8,
	GEN4_SCMI_DEV_dsi0_fed80000 = 9,
	GEN4_SCMI_DEV_vin02_e6ef2000 = 10,
	GEN4_SCMI_DEV_vin03_e6ef3000 = 11,
	GEN4_SCMI_DEV_vin10_e6ef8000 = 12,
	GEN4_SCMI_DEV_vin11_e6ef9000 = 13,
	GEN4_SCMI_DEV_vin12_e6efa000 = 14,
	GEN4_SCMI_DEV_vin13_e6efb000 = 15,
	GEN4_SCMI_DEV_csi40_fe500000 = 16,
	GEN4_SCMI_DEV_csi41_fe540000 = 17,
	GEN4_SCMI_DEV_cisp0_fec00000 = 18,
	GEN4_SCMI_DEV_cisp1_fee00000 = 19,
	GEN4_SCMI_DEV_vspx0_fedd0000 = 20,
	GEN4_SCMI_DEV_vspx1_fedd8000 = 21,
	GEN4_SCMI_DEV_fcpvx0_fedb0000 = 22,
	GEN4_SCMI_DEV_fcpvx1_fedb8000 = 23,
	GEN4_SCMI_DEV_ivcp1e_00_fea00000 = 24,
	GEN4_SCMI_DEV_vcp4_00_fe910000 = 25,
	GEN4_SCMI_DEV_fcpc_00_fe90f000 = 26,
	GEN4_SCMI_DEV_LAST,
};

struct scmi_device {
	int *rsts;
	int *clocks;
	int *pd;
};

//static const struct scmi_device default_dev = {
//		.rsts = (int[]){-1},
//		.clocks = (int[]){-1},
//		.pd = (int[]){-1},
//};

static const struct scmi_device scmi_devices[GEN4_SCMI_DEV_LAST] = {
	[GEN4_SCMI_DEV_I2C1_E6508000] = {
		.rsts = (int[]){R8A779G0_RESET_519, -1},
		.clocks = (int[]){R8A779G0_CLK_MOD_519, -1},
	},
	[GEN4_SCMI_DEV_FCP0_fea10000] = {
		.rsts = (int[]){R8A779G0_RESET_508, -1},
		.clocks = (int[]){R8A779G0_CLK_MOD_508, -1},
	},
	[GEN4_SCMI_DEV_FCP1_fea11000] = {
		.rsts = (int[]){R8A779G0_RESET_509, -1},
		.clocks = (int[]){R8A779G0_CLK_MOD_509, -1},
	},
	[GEN4_SCMI_DEV_vspd0_fea20000] = {
		.rsts = (int[]){R8A779G0_RESET_830, -1},
		.clocks = (int[]){R8A779G0_CLK_MOD_830, -1},
	},
	[GEN4_SCMI_DEV_vspd1_fea28000] = {
		.rsts = (int[]){R8A779G0_RESET_831, -1},
		.clocks = (int[]){R8A779G0_CLK_MOD_831, -1},
	},
	[GEN4_SCMI_DEV_isp0_fed00000] = {
		.rsts = (int[]){R8A779G0_RESET_612, -1},
		.clocks = (int[]){R8A779G0_CLK_MOD_612, -1},
		.pd = (int[]){R8A779G0_PD_A3ISP0, -1},
	},
	[GEN4_SCMI_DEV_isp1_fed20000] = {
		.rsts = (int[]){R8A779G0_RESET_613, -1},
		.clocks = (int[]){R8A779G0_CLK_MOD_613, -1},
		.pd = (int[]){R8A779G0_PD_A3ISP1, -1},
	},

	[GEN4_SCMI_DEV_vin00_e6ef0000] = {
		.rsts = (int[]){R8A779G0_RESET_730, -1},
		.clocks = (int[]){R8A779G0_CLK_MOD_730, -1},
	},
	[GEN4_SCMI_DEV_vin01_e6ef1000] = {
		.rsts = (int[]){R8A779G0_RESET_731, -1},
		.clocks = (int[]){R8A779G0_CLK_MOD_731, -1},
	},
	[GEN4_SCMI_DEV_vin02_e6ef2000] = {
		.rsts = (int[]){R8A779G0_RESET_800, -1},
		.clocks = (int[]){R8A779G0_CLK_MOD_800, -1},
	},
	[GEN4_SCMI_DEV_vin03_e6ef3000] = {
		.rsts = (int[]){R8A779G0_RESET_801, -1},
		.clocks = (int[]){R8A779G0_CLK_MOD_801, -1},
	},
	[GEN4_SCMI_DEV_vin10_e6ef8000] = {
		.rsts = (int[]){R8A779G0_RESET_806, -1},
		.clocks = (int[]){R8A779G0_CLK_MOD_806, -1},
	},
	[GEN4_SCMI_DEV_vin11_e6ef9000] = {
		.rsts = (int[]){R8A779G0_RESET_807, -1},
		.clocks = (int[]){R8A779G0_CLK_MOD_807, -1},
	},
	[GEN4_SCMI_DEV_vin12_e6efa000] = {
		.rsts = (int[]){R8A779G0_RESET_808, -1},
		.clocks = (int[]){R8A779G0_CLK_MOD_808, -1},
	},
	[GEN4_SCMI_DEV_vin13_e6efb000] = {
		.rsts = (int[]){R8A779G0_RESET_809, -1},
		.clocks = (int[]){R8A779G0_CLK_MOD_809, -1},
	},

	[GEN4_SCMI_DEV_csi40_fe500000] = {
		.rsts = (int[]){R8A779G0_RESET_331, -1},
		.clocks = (int[]){R8A779G0_CLK_MOD_331, -1},
	},
	[GEN4_SCMI_DEV_csi41_fe540000] = {
		.rsts = (int[]){R8A779G0_RESET_400, -1},
		.clocks = (int[]){R8A779G0_CLK_MOD_400, -1},
	},

	[GEN4_SCMI_DEV_cisp0_fec00000] = {
		.rsts = (int[]){R8A779G0_RESET_16, -1},
		.clocks = (int[]){R8A779G0_CLK_MOD_16, -1},
		.pd = (int[]){R8A779G0_PD_A3ISP0, -1},
	},
	[GEN4_SCMI_DEV_cisp1_fee00000] = {
		.rsts = (int[]){R8A779G0_RESET_17, -1},
		.clocks = (int[]){R8A779G0_CLK_MOD_17, -1},
		.pd = (int[]){R8A779G0_PD_A3ISP1, -1},
	},

	[GEN4_SCMI_DEV_vspx0_fedd0000] = {
		.rsts = (int[]){R8A779G0_RESET_1028, -1},
		.clocks = (int[]){R8A779G0_CLK_MOD_1028, -1},
		.pd = (int[]){R8A779G0_PD_A3ISP0, -1},
	},
	[GEN4_SCMI_DEV_vspx1_fedd8000] = {
		.rsts = (int[]){R8A779G0_RESET_1029, -1},
		.clocks = (int[]){R8A779G0_CLK_MOD_1029, -1},
		.pd = (int[]){R8A779G0_PD_A3ISP1, -1},
	},

	[GEN4_SCMI_DEV_fcpvx0_fedb0000] = {
		.rsts = (int[]){R8A779G0_RESET_1100, -1},
		.clocks = (int[]){R8A779G0_CLK_MOD_1100, -1},
		.pd = (int[]){R8A779G0_PD_A3ISP0, -1},
	},
	[GEN4_SCMI_DEV_fcpvx1_fedb8000] = {
		.rsts = (int[]){R8A779G0_RESET_1101, -1},
		.clocks = (int[]){R8A779G0_CLK_MOD_1101, -1},
		.pd = (int[]){R8A779G0_PD_A3ISP1, -1},
	},

	[GEN4_SCMI_DEV_ivcp1e_00_fea00000] = {
		.rsts = (int[]){R8A779G0_RESET_616, -1},
		.clocks = (int[]){R8A779G0_CLK_MOD_616, -1},
	},


	[GEN4_SCMI_DEV_vcp4_00_fe910000] = {
		.rsts = (int[]){R8A779G0_RESET_729, -1},
		.clocks = (int[]){R8A779G0_CLK_MOD_729, -1},
	},

	[GEN4_SCMI_DEV_fcpc_00_fe90f000] = {
		.rsts = (int[]){R8A779G0_RESET_507, -1},
		.clocks = (int[]){R8A779G0_CLK_MOD_507, -1},
	},
};

static spinlock_t scmi_lock;

static const int *scmi_device_owner[SCMI_NUM_AGENTS] =
{
	[0] = (int[]){-1},
	[1] = (int[]){-1},
	[2] = (int[]){
		GEN4_SCMI_DEV_FCP0_fea10000,
		GEN4_SCMI_DEV_FCP1_fea11000,
		GEN4_SCMI_DEV_vspd0_fea20000,
		GEN4_SCMI_DEV_vspd1_fea28000,
		GEN4_SCMI_DEV_isp0_fed00000,
		GEN4_SCMI_DEV_isp1_fed20000,
		GEN4_SCMI_DEV_vin00_e6ef0000,
		GEN4_SCMI_DEV_vin01_e6ef1000,
		GEN4_SCMI_DEV_vin02_e6ef2000,
		GEN4_SCMI_DEV_vin03_e6ef3000,
		GEN4_SCMI_DEV_vin10_e6ef8000,
		GEN4_SCMI_DEV_vin11_e6ef9000,
		GEN4_SCMI_DEV_vin12_e6efa000,
		GEN4_SCMI_DEV_vin13_e6efb000,
		GEN4_SCMI_DEV_csi40_fe500000,
		GEN4_SCMI_DEV_csi41_fe540000,
		GEN4_SCMI_DEV_cisp0_fec00000,
		GEN4_SCMI_DEV_cisp1_fee00000,
		GEN4_SCMI_DEV_vspx0_fedd0000,
		GEN4_SCMI_DEV_vspx1_fedd8000,
		GEN4_SCMI_DEV_fcpvx0_fedb0000,
		GEN4_SCMI_DEV_fcpvx1_fedb8000,
		GEN4_SCMI_DEV_ivcp1e_00_fea00000,
		GEN4_SCMI_DEV_vcp4_00_fe910000,
		GEN4_SCMI_DEV_fcpc_00_fe90f000,
		GEN4_SCMI_DEV_I2C1_E6508000,
		-1,
	},
	[3] = (int[]){-1},
	[4] = (int[]){-1},
	[5] = (int[]){-1},
	[6] = (int[]){-1},
	[7] = (int[]){-1},
};

#define SCMI_POWER_STATE_OFF	BIT(30)

static void gen4_scmi_reset_dev(uint32_t agent_id,
				const struct scmi_device *scmi_dev)
{
	int *scmi_ids = scmi_dev->rsts;
	int i = 0;

	while (scmi_ids[i] != -1) {
		plat_scmi_rstd_set_state(agent_id, scmi_ids[i], true);
		INFO("scmi: agent:%u reset:%d assert\n", agent_id, scmi_ids[i]);
		i++;
	}

	scmi_ids = scmi_dev->clocks;
	i = 0;

	while (scmi_ids[i] != -1) {
		plat_scmi_clock_set_state(agent_id, scmi_ids[i], false);
		INFO("scmi: agent:%u clock:%d disable\n", agent_id,
		     scmi_ids[i]);
		i++;
	}

	scmi_ids = scmi_dev->pd;
	if (!scmi_ids) {
		return;
	}
	i = 0;

	while (scmi_ids[i] != -1) {
		plat_scmi_pd_set_state(agent_id, 0, scmi_ids[i],
				       SCMI_POWER_STATE_OFF);
		INFO("scmi: agent:%u pd:%d off\n", agent_id, scmi_ids[i]);
		i++;
	}
}

int32_t plat_scmi_reset_agent_cfg(uint32_t agent_id, bool reset_perm)
{
	const int *agent_devs;
	int i = 0;

	assert(agent_id < plat_scmi_agent_count());

	VERBOSE("scmi: reset agent:%u cfg\n", agent_id);

	spin_lock(&scmi_lock);
	agent_devs = scmi_device_owner[agent_id];

	while (agent_devs[i] != -1) {
		INFO("scmi: reset agent:%u device:%d\n", agent_id,
		     agent_devs[i]);

		gen4_scmi_reset_dev(agent_id, &scmi_devices[agent_devs[i]]);
		i++;
	}
	spin_unlock(&scmi_lock);

	return SCMI_SUCCESS;
}
