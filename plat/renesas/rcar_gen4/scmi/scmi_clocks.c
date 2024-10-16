/*
 * Copyright 2024 EPAM Systems
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>

#include <common/debug.h>
#include <drivers/clk.h>
#include <drivers/delay_timer.h>
#include <drivers/scmi.h>
#include <lib/utils_def.h>
#include <platform_def.h>
#include <lib/mmio.h>

#include <cpg_registers.h>
#include <mssr.h>

#include "rcar_scmi_resources.h"
#include "r8a779g0-cpg-mssr.h"
#include "r8a779g0_clks_scmi.h"

#define CLK_CORE_INT_ID_FLAG BIT(31)

#define CLK_CORE_INT_GET_IDX(id) ((id) & ~CLK_CORE_INT_ID_FLAG)
#define CLK_CORE_INT_SET_IDX(id) ((id) | CLK_CORE_INT_ID_FLAG)

enum clk_core_int_ids {
	/* External Input Clocks */
	CLK_EXTAL,
	CLK_EXTALR,

	/* Internal Core Clocks */
	CLK_MAIN,
	CLK_PLL1,
	CLK_PLL4,
	CLK_PLL5,
	CLK_PLL1_DIV2,
	CLK_PLL2_DIV2,
	CLK_PLL4_DIV2,
	CLK_PLL5_DIV2,
	CLK_PLL5_DIV4,
	CLK_S0,
	CLK_S0_VIO,
	CLK_S0_VC,
	CLK_S0_HSC,
	CLK_SV_VIP,
	CLK_SV_IR,
	CLK_SDSRC,
	CLK_RPCSRC,
	CLK_OCO,
	CLK_VIO,
	CLK_VC,

	/* Module Clocks */
	MOD_CLK_BASE
};

static unsigned long r8a779g0_core_int_clks[MOD_CLK_BASE] = {
	[CLK_EXTAL] = 16666666,
	[CLK_EXTALR] = 32768,
	[CLK_MAIN] = 16666666,
	[CLK_PLL1] = 3200000000,
	[CLK_PLL4] = 2400000000,
	[CLK_PLL5] = 3200000000,
	[CLK_PLL1_DIV2] = 1600000000,
	[CLK_PLL4_DIV2] = 1200000000,
	[CLK_PLL5_DIV2] = 1600000000,
	[CLK_PLL5_DIV4] = 800000000,
	[CLK_S0] = 800000000,
	[CLK_S0_VIO] = 800000000,
	[CLK_S0_VC] = 800000000,
	[CLK_S0_HSC] = 800000000,
	[CLK_SV_VIP] = 640000000,
	[CLK_SV_IR] = 640000000,
	[CLK_SDSRC] = 800000000,
	[CLK_RPCSRC] = 640000000,
	[CLK_OCO] = 32768,
	[CLK_VC] = 533333333,
};

#define SCMI_CLOCK_NAME_LENGTH_MAX	16U

struct cpg_core_clk {
	char name[SCMI_CLOCK_NAME_LENGTH_MAX];
	uint32_t id;
	uint32_t parent;
	uint32_t div;
	uint32_t mult;
	unsigned long freq;
	bool valid:1;
};

#define DEF_FIXED(_name, _id, _parent, _div, _mult)	\
	[_id] = { .name = "c_"_name, .id = _id, .parent = _parent, .div = _div, .mult = _mult, .valid = true }

static struct cpg_core_clk r8a779g0_core_clks[R8A779G0_CLK_CORE_MAX] = {
	DEF_FIXED("zt",		R8A779G0_CLK_ZT,	CLK_CORE_INT_SET_IDX(CLK_PLL1_DIV2),	3, 1),
	DEF_FIXED(".rpc",	R8A779G0_CLK_RPC,	CLK_CORE_INT_SET_IDX(CLK_RPCSRC), 8, 1),
	DEF_FIXED("rpcd2",	R8A779G0_CLK_RPCD2,	R8A779G0_CLK_RPC, 2, 1),

	DEF_FIXED("s0d2",	R8A779G0_CLK_S0D2,	CLK_CORE_INT_SET_IDX(CLK_S0),		2, 1),
	DEF_FIXED("s0d3",	R8A779G0_CLK_S0D3,	CLK_CORE_INT_SET_IDX(CLK_S0),		3, 1),
	DEF_FIXED("s0d4",	R8A779G0_CLK_S0D4,	CLK_CORE_INT_SET_IDX(CLK_S0),		4, 1),
	DEF_FIXED("cl16m",	R8A779G0_CLK_CL16M,	CLK_CORE_INT_SET_IDX(CLK_S0),		48, 1),
	DEF_FIXED("s0d2_rt",	R8A779G0_CLK_S0D2_RT,	CLK_CORE_INT_SET_IDX(CLK_S0),		2, 1),
	DEF_FIXED("s0d3_rt",	R8A779G0_CLK_S0D3_RT,	CLK_CORE_INT_SET_IDX(CLK_S0),		3, 1),
	DEF_FIXED("s0d4_rt",	R8A779G0_CLK_S0D4_RT,	CLK_CORE_INT_SET_IDX(CLK_S0),		4, 1),
	DEF_FIXED("s0d6_rt",	R8A779G0_CLK_S0D6_RT,	CLK_CORE_INT_SET_IDX(CLK_S0),		6, 1),
	DEF_FIXED("s0d24_rt",	R8A779G0_CLK_S0D24_RT,	CLK_CORE_INT_SET_IDX(CLK_S0),		24, 1),
	DEF_FIXED("cl16m_rt",	R8A779G0_CLK_CL16M_RT,	CLK_CORE_INT_SET_IDX(CLK_S0),		48, 1),
	DEF_FIXED("s0d2_per",	R8A779G0_CLK_S0D2_PER,	CLK_CORE_INT_SET_IDX(CLK_S0),		2, 1),
	DEF_FIXED("s0d3_per",	R8A779G0_CLK_S0D3_PER,	CLK_CORE_INT_SET_IDX(CLK_S0),		3, 1),
	DEF_FIXED("s0d4_per",	R8A779G0_CLK_S0D4_PER,	CLK_CORE_INT_SET_IDX(CLK_S0),		4, 1),
	DEF_FIXED("s0d6_per",	R8A779G0_CLK_S0D6_PER,	CLK_CORE_INT_SET_IDX(CLK_S0),		6, 1),
	DEF_FIXED("s0d24_per",	R8A779G0_CLK_S0D24_PER,	CLK_CORE_INT_SET_IDX(CLK_S0),		24, 1),
	DEF_FIXED("cl16m_per",	R8A779G0_CLK_CL16M_PER,	CLK_CORE_INT_SET_IDX(CLK_S0),		48, 1),
	DEF_FIXED("s0d2_mm",	R8A779G0_CLK_S0D2_MM,	CLK_CORE_INT_SET_IDX(CLK_S0),		2, 1),
	DEF_FIXED("s0d4_mm",	R8A779G0_CLK_S0D4_MM,	CLK_CORE_INT_SET_IDX(CLK_S0),		4, 1),
	DEF_FIXED("cl16m_mm",	R8A779G0_CLK_CL16M_MM,	CLK_CORE_INT_SET_IDX(CLK_S0),		48, 1),
	DEF_FIXED("s0d2_cc",	R8A779G0_CLK_S0D2_CC,	CLK_CORE_INT_SET_IDX(CLK_S0),		2, 1),
	DEF_FIXED("s0d2_u3dg",	R8A779G0_CLK_S0D2_U3DG,	CLK_CORE_INT_SET_IDX(CLK_S0),		2, 1),
	DEF_FIXED("s0d4_u3dg",	R8A779G0_CLK_S0D4_U3DG,	CLK_CORE_INT_SET_IDX(CLK_S0),		4, 1),
	DEF_FIXED("s0d1_vio",	R8A779G0_CLK_S0D1_VIO,	CLK_CORE_INT_SET_IDX(CLK_S0_VIO),	1, 1),
	DEF_FIXED("s0d2_vio",	R8A779G0_CLK_S0D2_VIO,	CLK_CORE_INT_SET_IDX(CLK_S0_VIO),	2, 1),
	DEF_FIXED("s0d4_vio",	R8A779G0_CLK_S0D4_VIO,	CLK_CORE_INT_SET_IDX(CLK_S0_VIO),	4, 1),
	DEF_FIXED("s0d1_vc",	R8A779G0_CLK_S0D1_VC,	CLK_CORE_INT_SET_IDX(CLK_S0_VC),	1, 1),
	DEF_FIXED("s0d2_vc",	R8A779G0_CLK_S0D2_VC,	CLK_CORE_INT_SET_IDX(CLK_S0_VC),	2, 1),
	DEF_FIXED("s0d4_vc",	R8A779G0_CLK_S0D4_VC,	CLK_CORE_INT_SET_IDX(CLK_S0_VC),	4, 1),
	DEF_FIXED("s0d1_hsc",	R8A779G0_CLK_S0D1_HSC,	CLK_CORE_INT_SET_IDX(CLK_S0_HSC),	1, 1),
	DEF_FIXED("s0d2_hsc",	R8A779G0_CLK_S0D2_HSC,	CLK_CORE_INT_SET_IDX(CLK_S0_HSC),	2, 1),
	DEF_FIXED("s0d4_hsc",	R8A779G0_CLK_S0D4_HSC,	CLK_CORE_INT_SET_IDX(CLK_S0_HSC),	4, 1),
	DEF_FIXED("s0d8_hsc",	R8A779G0_CLK_S0D8_HSC,	CLK_CORE_INT_SET_IDX(CLK_S0_HSC),	8, 1),
	DEF_FIXED("cl16m_hsc",	R8A779G0_CLK_CL16M_HSC,	CLK_CORE_INT_SET_IDX(CLK_S0_HSC),	48, 1),
	DEF_FIXED("svd1_vip",	R8A779G0_CLK_SVD1_VIP,	CLK_CORE_INT_SET_IDX(CLK_SV_VIP),	1, 1),
	DEF_FIXED("svd2_vip",	R8A779G0_CLK_SVD2_VIP,	CLK_CORE_INT_SET_IDX(CLK_SV_VIP),	2, 1),
	DEF_FIXED("svd1_ir",	R8A779G0_CLK_SVD1_IR,	CLK_CORE_INT_SET_IDX(CLK_SV_IR),	1, 1),
	DEF_FIXED("svd2_ir",	R8A779G0_CLK_SVD2_IR,	CLK_CORE_INT_SET_IDX(CLK_SV_IR),	2, 1),
	DEF_FIXED("cbfusa",	R8A779G0_CLK_CBFUSA,	CLK_CORE_INT_SET_IDX(CLK_EXTAL),	2, 1),
	DEF_FIXED("dsiref",	R8A779G0_CLK_DSIREF,	CLK_CORE_INT_SET_IDX(CLK_PLL5_DIV4),	48, 1),
	DEF_FIXED("zg",		R8A779G0_CLK_ZG,	CLK_CORE_INT_SET_IDX(CLK_PLL4_DIV2),	2, 1),
	DEF_FIXED("sd0",	R8A779G0_CLK_SD0,	CLK_CORE_INT_SET_IDX(CLK_SDSRC),	4,1),
	DEF_FIXED("mso",	R8A779G0_CLK_MSO,	CLK_CORE_INT_SET_IDX(CLK_PLL5_DIV4),	64, 1),
	DEF_FIXED("canfd",	R8A779G0_CLK_CANFD,	CLK_CORE_INT_SET_IDX(CLK_PLL5_DIV4),	10, 1),
	DEF_FIXED("csi",	R8A779G0_CLK_CSI,	CLK_CORE_INT_SET_IDX(CLK_PLL5_DIV4),	32, 1),
	DEF_FIXED("dsiext",	R8A779G0_CLK_DSIEXT,	CLK_CORE_INT_SET_IDX(CLK_PLL5_DIV4),	1, 1),
	DEF_FIXED("post",	R8A779G0_CLK_POST,	CLK_CORE_INT_SET_IDX(CLK_PLL5_DIV4),	12, 1),
	DEF_FIXED("post2",	R8A779G0_CLK_POST2,	CLK_CORE_INT_SET_IDX(CLK_PLL5_DIV4),	6, 1),
	DEF_FIXED("post3",	R8A779G0_CLK_POST3,	CLK_CORE_INT_SET_IDX(CLK_PLL5_DIV4),	12, 1),
	DEF_FIXED("post4",	R8A779G0_CLK_POST4,	CLK_CORE_INT_SET_IDX(CLK_PLL5_DIV4),	12, 1),
	DEF_FIXED("sasyncrt",	R8A779G0_CLK_SASYNCRT,	CLK_CORE_INT_SET_IDX(CLK_PLL5_DIV4),	48, 1),
	DEF_FIXED("sasyncper", R8A779G0_CLK_SASYNCPER, CLK_CORE_INT_SET_IDX(CLK_PLL5_DIV4), 3, 1),
	DEF_FIXED("sasyncperd1", R8A779G0_CLK_SASYNCPERD1, R8A779G0_CLK_SASYNCPER, 1, 1),
	DEF_FIXED("sasyncperd2", R8A779G0_CLK_SASYNCPERD2, R8A779G0_CLK_SASYNCPER, 2, 1),
	DEF_FIXED("sasyncperd4", R8A779G0_CLK_SASYNCPERD4, R8A779G0_CLK_SASYNCPER, 4, 1),
	DEF_FIXED("viobus",	R8A779G0_CLK_VIOBUS,	CLK_CORE_INT_SET_IDX(CLK_PLL5_DIV2),	1, 1),
	DEF_FIXED("viobusd2",	R8A779G0_CLK_VIOBUSD2,	CLK_CORE_INT_SET_IDX(CLK_PLL5_DIV2),	2, 1),
	DEF_FIXED("vcbus",	R8A779G0_CLK_VCBUS,	CLK_CORE_INT_SET_IDX(CLK_VC),		1, 1),
	DEF_FIXED("vcbusd2",	R8A779G0_CLK_VCBUSD2,	CLK_CORE_INT_SET_IDX(CLK_VC),		2, 1),
	DEF_FIXED("cpex",	R8A779G0_CLK_CPEX,	CLK_CORE_INT_SET_IDX(CLK_EXTAL),	2, 1),
	DEF_FIXED("osc",	R8A779G0_CLK_OSCCLK,	CLK_CORE_INT_SET_IDX(CLK_EXTAL),	128, 1),
	DEF_FIXED("r",		R8A779G0_CLK_RCLK, CLK_CORE_INT_SET_IDX(CLK_EXTALR), 1, 1),
};

#define DEF_MOD_CLK_IDX(_mod) (R8A779G0_CLK_MOD_##_mod - R8A779G0_CLK_MOD_FIRST)

#define GET_MOD_CLK_IDX(scmi_id) ((scmi_id) - R8A779G0_CLK_MOD_FIRST)

#define DEF_MOD(_name, _mod, _parent...)	\
	[DEF_MOD_CLK_IDX(_mod)] = { .name = "m_"_name, .id = _mod, .parent = _parent, .valid = true }

struct mssr_mod_clk {
	const char *name;
	uint32_t id;
	uint32_t parent;
	bool valid:1;
	bool enabled:1;
};

#define R8A779G0_REG_ID(id)     ((id) / 100)
#define R8A779G0_REG_BIT_ID(id) ((id) % 100)

static struct mssr_mod_clk r8a779g0_mod_clks[R8A779G0_CLK_MOD_NUM] = {
	DEF_MOD("rgx",			0,	R8A779G0_CLK_ZG),
	DEF_MOD("smpo0",		5,	R8A779G0_CLK_SVD1_VIP),
	DEF_MOD("smps0",		7,	R8A779G0_CLK_SVD1_VIP),
	DEF_MOD("umfl0",		9,	R8A779G0_CLK_SVD1_VIP),
	DEF_MOD("isp0",			16,	R8A779G0_CLK_S0D1_VIO),
	DEF_MOD("isp1",			17,	R8A779G0_CLK_S0D1_VIO),
	DEF_MOD("impcnn",		22,	R8A779G0_CLK_S0D6_PER),
	DEF_MOD("spmc",			23,	R8A779G0_CLK_S0D6_PER),
	DEF_MOD("imp0",			24,	R8A779G0_CLK_S0D6_PER),
	DEF_MOD("imp1",			25,	R8A779G0_CLK_S0D6_PER),
	DEF_MOD("impdmac0",		26,	R8A779G0_CLK_SVD1_IR),
	DEF_MOD("imppsc",		27,	R8A779G0_CLK_SVD1_IR),

	DEF_MOD("imp2",			103,	R8A779G0_CLK_S0D3),
	DEF_MOD("imp3",			104,	R8A779G0_CLK_S0D3),
	DEF_MOD("impdmac1",		105,	R8A779G0_CLK_S0D3),
	DEF_MOD("impdta",		116,	R8A779G0_CLK_S0D3),
	DEF_MOD("impslv",		118,	R8A779G0_CLK_S0D6_PER),
	DEF_MOD("spmi",			120,	R8A779G0_CLK_S0D6_PER),
	DEF_MOD("adg",			122,	R8A779G0_CLK_S0D6_PER),
	DEF_MOD("advfs",		123,	R8A779G0_CLK_S0D6_PER),

	DEF_MOD("avb0",			211,	R8A779G0_CLK_S0D8_HSC),
	DEF_MOD("avb1",			212,	R8A779G0_CLK_S0D8_HSC),
	DEF_MOD("avb2",			213,	R8A779G0_CLK_S0D8_HSC),

	DEF_MOD("can-fd",		328,	R8A779G0_CLK_CANFD),
	DEF_MOD("csitop0",		331,	R8A779G0_CLK_CSI),

	DEF_MOD("csitop1",		400,	R8A779G0_CLK_CSI),
	DEF_MOD("dis0",			411,	R8A779G0_CLK_S0D3),
	DEF_MOD("doc2ch",		414,	R8A779G0_CLK_S0D3),
	DEF_MOD("dsitxlink0",		415,	R8A779G0_CLK_DSIREF),
	DEF_MOD("dsitxlink1",		416,	R8A779G0_CLK_DSIREF),

	DEF_MOD("fcpcs",		507,	R8A779G0_CLK_S0D3),
	DEF_MOD("fcpvd0",		508,	R8A779G0_CLK_S0D3),
	DEF_MOD("fcpvd1",		509,	R8A779G0_CLK_S0D3),
	DEF_MOD("hscif0",		514,	R8A779G0_CLK_SASYNCPERD1),
	DEF_MOD("hscif1",		515,	R8A779G0_CLK_SASYNCPERD1),
	DEF_MOD("hscif2",		516,	R8A779G0_CLK_SASYNCPERD1),
	DEF_MOD("hscif3",		517,	R8A779G0_CLK_SASYNCPERD1),
	DEF_MOD("i2c0",			518,	R8A779G0_CLK_S0D6_PER),
	DEF_MOD("i2c1",			519,	R8A779G0_CLK_S0D6_PER),
	DEF_MOD("i2c2",			520,	R8A779G0_CLK_S0D6_PER),
	DEF_MOD("i2c3",			521,	R8A779G0_CLK_S0D6_PER),
	DEF_MOD("i2c4",			522,	R8A779G0_CLK_S0D6_PER),
	DEF_MOD("i2c5",			523,	R8A779G0_CLK_S0D6_PER),
	DEF_MOD("imr0",			525,	R8A779G0_CLK_S0D6_PER),
	DEF_MOD("imr1",			526,	R8A779G0_CLK_S0D6_PER),
	DEF_MOD("imr2",			527,	R8A779G0_CLK_S0D6_PER),
	DEF_MOD("ims0",			529,	R8A779G0_CLK_S0D6_PER),
	DEF_MOD("ims1",			530,	R8A779G0_CLK_S0D6_PER),

	DEF_MOD("ispcs0",		612,	R8A779G0_CLK_S0D2_VIO),
	DEF_MOD("ispcs1",		613,	R8A779G0_CLK_S0D2_VIO),
	DEF_MOD("ivcp1e",		616,	R8A779G0_CLK_VCBUS),
	DEF_MOD("msi0",			618,	R8A779G0_CLK_MSO),
	DEF_MOD("msi1",			619,	R8A779G0_CLK_MSO),
	DEF_MOD("msi2",			620,	R8A779G0_CLK_MSO),
	DEF_MOD("msi3",			621,	R8A779G0_CLK_MSO),
	DEF_MOD("msi4",			622,	R8A779G0_CLK_MSO),
	DEF_MOD("msi5",			623,	R8A779G0_CLK_MSO),
	DEF_MOD("pcie0",		624,	R8A779G0_CLK_S0D2),
	DEF_MOD("pcie1",		625,	R8A779G0_CLK_S0D2),
	DEF_MOD("pwm",			628,	R8A779G0_CLK_SASYNCPERD4),
	DEF_MOD("rpc",			629,	R8A779G0_CLK_RPCD2),
	DEF_MOD("rtdm0",		630,	R8A779G0_CLK_S0D4_RT),
	DEF_MOD("rtdm1",		631,	R8A779G0_CLK_S0D4_RT),

	DEF_MOD("rtdm2",		700,	R8A779G0_CLK_S0D4_RT),
	DEF_MOD("rtdm3",		701,	R8A779G0_CLK_S0D4_RT),
	DEF_MOD("scif0",		702,	R8A779G0_CLK_SASYNCPERD4),
	DEF_MOD("scif1",		703,	R8A779G0_CLK_SASYNCPERD4),
	DEF_MOD("scif3",		704,	R8A779G0_CLK_SASYNCPERD4),
	DEF_MOD("scif4",		705,	R8A779G0_CLK_SASYNCPERD4),
	DEF_MOD("sdhi0",		706,	R8A779G0_CLK_SD0),
	DEF_MOD("sydm1",		709,	R8A779G0_CLK_S0D6_PER),
	DEF_MOD("sydm2",		710,	R8A779G0_CLK_S0D6_PER),
	DEF_MOD("tmu0",			713,	R8A779G0_CLK_SASYNCRT),
	DEF_MOD("tmu1",			714,	R8A779G0_CLK_SASYNCPERD2),
	DEF_MOD("tmu2",			715,	R8A779G0_CLK_SASYNCPERD2),
	DEF_MOD("tmu3",			716,	R8A779G0_CLK_SASYNCPERD2),
	DEF_MOD("tmu4",			717,	R8A779G0_CLK_SASYNCPERD2),
	DEF_MOD("tpu",			718,	R8A779G0_CLK_SASYNCPERD4),
	DEF_MOD("caiplite_wrp0",	720,    R8A779G0_CLK_S0D2),
	DEF_MOD("caiplite0",            721,    R8A779G0_CLK_S0D1_VIO),
	DEF_MOD("caiplite1",            722,    R8A779G0_CLK_S0D1_VIO),
	DEF_MOD("caiplite2",            723,    R8A779G0_CLK_S0D1_VIO),
	DEF_MOD("caiplite3",            724,    R8A779G0_CLK_S0D1_VIO),
	DEF_MOD("caiplite4",            725,    R8A779G0_CLK_S0D1_VIO),
	DEF_MOD("caiplite5",            726,    R8A779G0_CLK_S0D1_VIO),
	DEF_MOD("caiplite6",            727,    R8A779G0_CLK_S0D1_VIO),
	DEF_MOD("caiplite7",            728,    R8A779G0_CLK_S0D1_VIO),
	DEF_MOD("vcp4l",		729,	R8A779G0_CLK_S0D6_RT),
	DEF_MOD("vin0",			730,	R8A779G0_CLK_S0D1_VIO),
	DEF_MOD("vin1",			731,	R8A779G0_CLK_S0D1_VIO),

	DEF_MOD("vin2",			800,	R8A779G0_CLK_S0D1_VIO),
	DEF_MOD("vin3",			801,	R8A779G0_CLK_S0D1_VIO),
	DEF_MOD("vin4",			802,	R8A779G0_CLK_S0D1_VIO),
	DEF_MOD("vin5",			803,	R8A779G0_CLK_S0D1_VIO),
	DEF_MOD("vin6",			804,	R8A779G0_CLK_S0D1_VIO),
	DEF_MOD("vin7",			805,	R8A779G0_CLK_S0D1_VIO),
	DEF_MOD("vin10",		806,	R8A779G0_CLK_S0D1_VIO),
	DEF_MOD("vin11",		807,	R8A779G0_CLK_S0D1_VIO),
	DEF_MOD("vin12",		808,	R8A779G0_CLK_S0D1_VIO),
	DEF_MOD("vin13",		809,	R8A779G0_CLK_S0D1_VIO),
	DEF_MOD("vin14",		810,	R8A779G0_CLK_S0D1_VIO),
	DEF_MOD("vin15",		811,	R8A779G0_CLK_S0D1_VIO),
	DEF_MOD("vin16",		812,	R8A779G0_CLK_S0D1_VIO),
	DEF_MOD("vin17",		813,	R8A779G0_CLK_S0D1_VIO),
	DEF_MOD("vspd0",		830,	R8A779G0_CLK_S0D1_VIO),
	DEF_MOD("vspd1",		831,	R8A779G0_CLK_S0D1_VIO),

	DEF_MOD("wcrc0",		903,	R8A779G0_CLK_S0D2_RT),
	DEF_MOD("wcrc1",		904,	R8A779G0_CLK_S0D2_RT),
	DEF_MOD("wcrc2",		905,	R8A779G0_CLK_S0D2_RT),
	DEF_MOD("wcrc3",		906,	R8A779G0_CLK_S0D2_RT),
	DEF_MOD("wdt1:wdt0",		907,	R8A779G0_CLK_RCLK),
	DEF_MOD("cmt0",			910,	R8A779G0_CLK_RCLK),
	DEF_MOD("cmt1",			911,	R8A779G0_CLK_RCLK),
	DEF_MOD("cmt2",			912,	R8A779G0_CLK_RCLK),
	DEF_MOD("cmt3",			913,	R8A779G0_CLK_RCLK),
	DEF_MOD("pfc0",			915,	R8A779G0_CLK_CL16M),
	DEF_MOD("pfc1",			916,	R8A779G0_CLK_CL16M),
	DEF_MOD("pfc2",			917,	R8A779G0_CLK_CL16M),
	DEF_MOD("pfc3",			918,	R8A779G0_CLK_CL16M),
	DEF_MOD("thermal",		919,	R8A779G0_CLK_CL16M),

	DEF_MOD("vspx0",		1028,	R8A779G0_CLK_S0D1_VIO),
	DEF_MOD("vspx1",		1029,	R8A779G0_CLK_S0D1_VIO),

	DEF_MOD("fcpvx0",		1100,	R8A779G0_CLK_S0D2_VC),
	DEF_MOD("fcpvx1",		1101,	R8A779G0_CLK_S0D2_VC),

	DEF_MOD("cve0",			2706,	R8A779G0_CLK_SVD1_IR),
	DEF_MOD("cve1",			2707,	R8A779G0_CLK_SVD1_IR),
	DEF_MOD("cve2",			2708,	R8A779G0_CLK_SVD1_IR),
	DEF_MOD("cve3",			2709,	R8A779G0_CLK_SVD1_IR),
	DEF_MOD("impsdmac0",		2712,	R8A779G0_CLK_SVD1_VIP),
	DEF_MOD("impsdmac1",		2713,	R8A779G0_CLK_SVD1_VIP),
	DEF_MOD("tsn",			2723,	R8A779G0_CLK_S0D8_HSC),

	DEF_MOD("vdsp0_bus",		2801,	R8A779G0_CLK_SVD1_IR),
	DEF_MOD("vdsp1_bus",		2802,	R8A779G0_CLK_SVD1_IR),
	DEF_MOD("vdsp2_bus",		2803,	R8A779G0_CLK_SVD1_IR),
	DEF_MOD("vdsp3_bus",		2804,	R8A779G0_CLK_SVD1_IR),
	DEF_MOD("paptop",               2806,    R8A779G0_CLK_S0D6_PER),
	DEF_MOD("papsdma",              2807,    R8A779G0_CLK_S0D6_PER),
	DEF_MOD("dsc",			2819,	R8A779G0_CLK_VIOBUSD2),
	DEF_MOD("vdsp0_csb",		2821,	R8A779G0_CLK_SVD1_IR),
	DEF_MOD("vdsp1_csb",		2830,	R8A779G0_CLK_SVD1_IR),

	DEF_MOD("vdsp2_csb",		2907,	R8A779G0_CLK_SVD1_IR),
	DEF_MOD("vdsp3_csb",		2916,	R8A779G0_CLK_SVD1_IR),
	DEF_MOD("ssiu",			2926,	R8A779G0_CLK_S0D6_PER),
	DEF_MOD("ssi",			2927,	R8A779G0_CLK_S0D6_PER),
};

static void cpg_clk_core_recalc_rate(uint32_t core_id)
{
	uint32_t parent = r8a779g0_core_clks[core_id].parent;
	unsigned long parent_freq;
	uint64_t cur_freq;

	if (!r8a779g0_core_clks[core_id].valid) {
		return;
	}

	if (r8a779g0_core_clks[core_id].freq) {
		return;
	}

	if (parent & CLK_CORE_INT_ID_FLAG) {
		parent_freq =
			r8a779g0_core_int_clks[CLK_CORE_INT_GET_IDX(parent)];
	} else {
		cpg_clk_core_recalc_rate(parent);
		parent_freq = r8a779g0_core_clks[parent].freq;
	}

	cur_freq = (parent_freq * r8a779g0_core_clks[core_id].mult) /
		   r8a779g0_core_clks[core_id].div;

	r8a779g0_core_clks[core_id].freq = cur_freq;
}

static unsigned long cpg_clk_core_get_rate(uint32_t core_id)
{
	unsigned long freq;

	if (!r8a779g0_core_clks[core_id].valid) {
		return 0;
	}

	freq = r8a779g0_core_clks[core_id].freq;
	if (!freq) {
		ERROR("scmi: clk: recalc error\n");
	}

	return freq;
}

static unsigned long cpg_clk_mod_get_rate(uint32_t mod_id)
{
	return cpg_clk_core_get_rate(r8a779g0_mod_clks[mod_id].parent);
}

static void cpg_clk_mod_set_state(uint32_t mod_id, bool on)
{
	struct mssr_mod_clk *mod = &r8a779g0_mod_clks[mod_id];
	uint32_t reg_ofs;
	uint32_t bit_ofs;

	if (!mod->valid) {
		ERROR("scmi: clk: invalid mod clk %u\n", mod_id);
		return;
	}

	if (on == mod->enabled) {
		return;
	}

	VERBOSE("scmi: mod_clk:%u %s id:%u %s\n", mod_id, mod->name, mod->id,
		on ? "enable" : "disable");

	reg_ofs = R8A779G0_REG_ID(mod->id);
	bit_ofs = R8A779G0_REG_BIT_ID(mod->id);
	rcar_mssr_clock(reg_ofs, BIT(bit_ofs), on, false);
	mod->enabled = on;
}

size_t plat_scmi_clock_count(unsigned int agent_id)
{
	return R8A779G0_CLK_MOD_MAX;
}

const char *plat_scmi_clock_get_name(unsigned int agent_id,
				     unsigned int scmi_id)
{
	assert(scmi_id < plat_scmi_clock_count(agent_id));

	if (scmi_id < R8A779G0_CLK_MOD_FIRST) {
		return r8a779g0_core_clks[scmi_id].name;
	}

	return r8a779g0_mod_clks[GET_MOD_CLK_IDX(scmi_id)].name;
}

int32_t plat_scmi_clock_set_rate(unsigned int agent_id __unused,
				 unsigned int scmi_id __unused,
				 unsigned long rate __unused)
{
	VERBOSE("scmi: mod_clk: set rate id:%u \n", scmi_id);

	return SCMI_SUCCESS;
}

unsigned long plat_scmi_clock_get_rate(unsigned int agent_id,
				       unsigned int scmi_id)
{
	assert(scmi_id < plat_scmi_clock_count(agent_id));

	VERBOSE("scmi: mod_clk: get rate id:%u \n", scmi_id);

	if (scmi_id < R8A779G0_CLK_MOD_FIRST) {
		return cpg_clk_core_get_rate(scmi_id);
	}

	return cpg_clk_mod_get_rate(GET_MOD_CLK_IDX(scmi_id));
}

int32_t plat_scmi_clock_rates_array(unsigned int agent_id, unsigned int scmi_id,
				    unsigned long *array, size_t *nb_elts,
				    uint32_t start_idx)
{
	assert(scmi_id < plat_scmi_clock_count(agent_id));

	if (start_idx > 0) {
		return SCMI_OUT_OF_RANGE;
	}

	if (array == NULL) {
		*nb_elts = 1U;
	} else if (*nb_elts == 1U) {
		*array = plat_scmi_clock_get_rate(agent_id, scmi_id);
	} else {
		return SCMI_GENERIC_ERROR;
	}

	return SCMI_SUCCESS;
}

int32_t plat_scmi_clock_get_state(unsigned int agent_id, unsigned int scmi_id)
{
	assert(scmi_id < plat_scmi_clock_count(agent_id));

	if (scmi_id < R8A779G0_CLK_MOD_FIRST) {
		return (int32_t) true;
	}

	if (!r8a779g0_mod_clks[GET_MOD_CLK_IDX(scmi_id)].valid) {
		return 0;
	}

	return (int32_t)r8a779g0_mod_clks[GET_MOD_CLK_IDX(scmi_id)].enabled;
}

int32_t plat_scmi_clock_set_state(unsigned int agent_id, unsigned int scmi_id,
				  bool enable_not_disable)
{
	assert(scmi_id < plat_scmi_clock_count(agent_id));

	if (scmi_id < R8A779G0_CLK_MOD_FIRST) {
		return SCMI_SUCCESS;
	}

	cpg_clk_mod_set_state(GET_MOD_CLK_IDX(scmi_id), enable_not_disable);

	return SCMI_SUCCESS;
}

static uint32_t cpg_init_regs[][2] = {
	{SD0CKCR, 0x00000001},   {RPCCKCR, 0x00000017},
	{CANFDCKCR, 0x00000009}, {MSOCKCR, 0x0000003F},
	{CSICKCR, 0x0000001F},   {DSIEXTCKCR, 0x00000000},
	{POSTCKCR, 0x0000000B},  {POST2CKCR, 0x00000005},
	{POST3CKCR, 0x0000000B}, {POST4CKCR, 0x0000000B},
};

void rcar_init_scmi_clocks(void)
{
	uint32_t i;

	/*
	 * TODO: Here should be default cpg settings cfg:
	 * - read MDx and update freqs/parent/dividers in core(internal) clocks
	 * - (probably not needed) PLLs cfg, but not plls 2(A), 6(RT), 3(DDR)
	 * - Clock Frequency Control Registers
	 */

	/* setup Clock Frequency Control Registers */
	for (i = 0; i < ARRAY_SIZE(cpg_init_regs); i++) {
		VERBOSE("scmi: clk: init reg:%08x val:%08x\n",
			cpg_init_regs[i][0], cpg_init_regs[i][1]);
		cpg_write_32(cpg_init_regs[i][0], cpg_init_regs[i][1]);
	}

	for (i = 0; i < R8A779G0_CLK_CORE_MAX; i++) {
		if (!r8a779g0_core_clks[i].valid) {
			snprintf(r8a779g0_core_clks[i].name,
				 SCMI_CLOCK_NAME_LENGTH_MAX, "reserved-%d", i);
			continue;
		}

		cpg_clk_core_recalc_rate(i);
		VERBOSE("scmi: clk:%u core init freq:%lu\n", i,
			r8a779g0_core_clks[i].freq);
	}

	for (i = 0; i < R8A779G0_CLK_MOD_NUM; i++) {
		struct mssr_mod_clk *mod = &r8a779g0_mod_clks[i];
		uint32_t reg_ofs;
		uint32_t bit_ofs;

		if (!mod->valid) {
			continue;
		}

		reg_ofs = R8A779G0_REG_ID(mod->id);
		bit_ofs = R8A779G0_REG_BIT_ID(mod->id);
		mod->enabled = rcar_mssr_clock_state(reg_ofs, bit_ofs);
	}
}
