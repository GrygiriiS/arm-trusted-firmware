/*
 * Copyright 2024 EPAM Systems
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>

#include <common/debug.h>
#include <drivers/delay_timer.h>
#include <drivers/scmi.h>
#include <lib/utils_def.h>
#include <platform_def.h>
#include <lib/mmio.h>

#include "mssr.h"

#include "rcar_scmi_resources.h"
#include "r8a779g0_resets_scmi.h"

#define R8A779G0_REG_ID(id)     ((id) / 100)
#define R8A779G0_REG_BIT_ID(id) ((id) % 100)

struct gen4_reset {
	const char *name;
	uint32_t id;
};

#define RESET_NAME(n) "reset_" #n

struct gen4_reset r8a779g0_resets[R8A779G0_RESET_MAX] = {
	[R8A779G0_RESET_0] = {.name = RESET_NAME(0), .id = 0},
	[R8A779G0_RESET_5] = {.name = RESET_NAME(5), .id = 5},
	[R8A779G0_RESET_7] = {.name = RESET_NAME(7), .id = 7},
	[R8A779G0_RESET_9] = {.name = RESET_NAME(9), .id = 9},
	[R8A779G0_RESET_16] = {.name = RESET_NAME(16), .id = 16},
	[R8A779G0_RESET_17] = {.name = RESET_NAME(17), .id = 17},
	[R8A779G0_RESET_22] = {.name = RESET_NAME(22), .id = 22},
	[R8A779G0_RESET_23] = {.name = RESET_NAME(23), .id = 23},
	[R8A779G0_RESET_24] = {.name = RESET_NAME(24), .id = 24},
	[R8A779G0_RESET_25] = {.name = RESET_NAME(25), .id = 25},
	[R8A779G0_RESET_26] = {.name = RESET_NAME(26), .id = 26},
	[R8A779G0_RESET_27] = {.name = RESET_NAME(27), .id = 27},
	[R8A779G0_RESET_103] = {.name = RESET_NAME(103), .id = 103},
	[R8A779G0_RESET_104] = {.name = RESET_NAME(104), .id = 104},
	[R8A779G0_RESET_105] = {.name = RESET_NAME(105), .id = 105},
	[R8A779G0_RESET_116] = {.name = RESET_NAME(116), .id = 116},
	[R8A779G0_RESET_118] = {.name = RESET_NAME(118), .id = 118},
	[R8A779G0_RESET_120] = {.name = RESET_NAME(120), .id = 120},
	[R8A779G0_RESET_123] = {.name = RESET_NAME(123), .id = 123},
	[R8A779G0_RESET_211] = {.name = RESET_NAME(211), .id = 211},
	[R8A779G0_RESET_212] = {.name = RESET_NAME(212), .id = 212},
	[R8A779G0_RESET_213] = {.name = RESET_NAME(213), .id = 213},
	[R8A779G0_RESET_328] = {.name = RESET_NAME(328), .id = 328},
	[R8A779G0_RESET_331] = {.name = RESET_NAME(331), .id = 331},
	[R8A779G0_RESET_400] = {.name = RESET_NAME(400), .id = 400},
	[R8A779G0_RESET_411] = {.name = RESET_NAME(411), .id = 411},
	[R8A779G0_RESET_414] = {.name = RESET_NAME(414), .id = 414},
	[R8A779G0_RESET_415] = {.name = RESET_NAME(415), .id = 415},
	[R8A779G0_RESET_416] = {.name = RESET_NAME(416), .id = 416},
	[R8A779G0_RESET_507] = {.name = RESET_NAME(507), .id = 507},
	[R8A779G0_RESET_508] = {.name = RESET_NAME(508), .id = 508},
	[R8A779G0_RESET_509] = {.name = RESET_NAME(509), .id = 509},
	[R8A779G0_RESET_514] = {.name = RESET_NAME(514), .id = 514},
	[R8A779G0_RESET_515] = {.name = RESET_NAME(515), .id = 515},
	[R8A779G0_RESET_516] = {.name = RESET_NAME(516), .id = 516},
	[R8A779G0_RESET_517] = {.name = RESET_NAME(517), .id = 517},
	[R8A779G0_RESET_518] = {.name = RESET_NAME(518), .id = 518},
	[R8A779G0_RESET_519] = {.name = RESET_NAME(519), .id = 519},
	[R8A779G0_RESET_520] = {.name = RESET_NAME(520), .id = 520},
	[R8A779G0_RESET_521] = {.name = RESET_NAME(521), .id = 521},
	[R8A779G0_RESET_522] = {.name = RESET_NAME(522), .id = 522},
	[R8A779G0_RESET_523] = {.name = RESET_NAME(523), .id = 523},
	[R8A779G0_RESET_525] = {.name = RESET_NAME(525), .id = 525},
	[R8A779G0_RESET_526] = {.name = RESET_NAME(526), .id = 526},
	[R8A779G0_RESET_527] = {.name = RESET_NAME(527), .id = 527},
	[R8A779G0_RESET_529] = {.name = RESET_NAME(529), .id = 529},
	[R8A779G0_RESET_530] = {.name = RESET_NAME(530), .id = 530},
	[R8A779G0_RESET_612] = {.name = RESET_NAME(612), .id = 612},
	[R8A779G0_RESET_613] = {.name = RESET_NAME(613), .id = 613},
	[R8A779G0_RESET_616] = {.name = RESET_NAME(616), .id = 616},
	[R8A779G0_RESET_618] = {.name = RESET_NAME(618), .id = 618},
	[R8A779G0_RESET_619] = {.name = RESET_NAME(619), .id = 619},
	[R8A779G0_RESET_620] = {.name = RESET_NAME(620), .id = 620},
	[R8A779G0_RESET_621] = {.name = RESET_NAME(621), .id = 621},
	[R8A779G0_RESET_622] = {.name = RESET_NAME(622), .id = 622},
	[R8A779G0_RESET_623] = {.name = RESET_NAME(623), .id = 623},
	[R8A779G0_RESET_624] = {.name = RESET_NAME(624), .id = 624},
	[R8A779G0_RESET_625] = {.name = RESET_NAME(625), .id = 625},
	[R8A779G0_RESET_628] = {.name = RESET_NAME(628), .id = 628},
	[R8A779G0_RESET_629] = {.name = RESET_NAME(629), .id = 629},
	[R8A779G0_RESET_630] = {.name = RESET_NAME(630), .id = 630},
	[R8A779G0_RESET_631] = {.name = RESET_NAME(631), .id = 631},
	[R8A779G0_RESET_700] = {.name = RESET_NAME(700), .id = 700},
	[R8A779G0_RESET_701] = {.name = RESET_NAME(701), .id = 701},
	[R8A779G0_RESET_702] = {.name = RESET_NAME(702), .id = 702},
	[R8A779G0_RESET_703] = {.name = RESET_NAME(703), .id = 703},
	[R8A779G0_RESET_704] = {.name = RESET_NAME(704), .id = 704},
	[R8A779G0_RESET_705] = {.name = RESET_NAME(705), .id = 705},
	[R8A779G0_RESET_706] = {.name = RESET_NAME(706), .id = 706},
	[R8A779G0_RESET_709] = {.name = RESET_NAME(709), .id = 709},
	[R8A779G0_RESET_710] = {.name = RESET_NAME(710), .id = 710},
	[R8A779G0_RESET_713] = {.name = RESET_NAME(713), .id = 713},
	[R8A779G0_RESET_714] = {.name = RESET_NAME(714), .id = 714},
	[R8A779G0_RESET_715] = {.name = RESET_NAME(715), .id = 715},
	[R8A779G0_RESET_716] = {.name = RESET_NAME(716), .id = 716},
	[R8A779G0_RESET_717] = {.name = RESET_NAME(717), .id = 717},
	[R8A779G0_RESET_718] = {.name = RESET_NAME(718), .id = 718},
	[R8A779G0_RESET_720] = {.name = RESET_NAME(720), .id = 720},
	[R8A779G0_RESET_721] = {.name = RESET_NAME(721), .id = 721},
	[R8A779G0_RESET_722] = {.name = RESET_NAME(722), .id = 722},
	[R8A779G0_RESET_723] = {.name = RESET_NAME(723), .id = 723},
	[R8A779G0_RESET_724] = {.name = RESET_NAME(724), .id = 724},
	[R8A779G0_RESET_725] = {.name = RESET_NAME(725), .id = 725},
	[R8A779G0_RESET_726] = {.name = RESET_NAME(726), .id = 726},
	[R8A779G0_RESET_727] = {.name = RESET_NAME(727), .id = 727},
	[R8A779G0_RESET_728] = {.name = RESET_NAME(728), .id = 728},
	[R8A779G0_RESET_729] = {.name = RESET_NAME(729), .id = 729},
	[R8A779G0_RESET_730] = {.name = RESET_NAME(730), .id = 730},
	[R8A779G0_RESET_731] = {.name = RESET_NAME(731), .id = 731},
	[R8A779G0_RESET_800] = {.name = RESET_NAME(800), .id = 800},
	[R8A779G0_RESET_801] = {.name = RESET_NAME(801), .id = 801},
	[R8A779G0_RESET_802] = {.name = RESET_NAME(802), .id = 802},
	[R8A779G0_RESET_803] = {.name = RESET_NAME(803), .id = 803},
	[R8A779G0_RESET_804] = {.name = RESET_NAME(804), .id = 804},
	[R8A779G0_RESET_805] = {.name = RESET_NAME(805), .id = 805},
	[R8A779G0_RESET_806] = {.name = RESET_NAME(806), .id = 806},
	[R8A779G0_RESET_807] = {.name = RESET_NAME(807), .id = 807},
	[R8A779G0_RESET_808] = {.name = RESET_NAME(808), .id = 808},
	[R8A779G0_RESET_809] = {.name = RESET_NAME(809), .id = 809},
	[R8A779G0_RESET_810] = {.name = RESET_NAME(810), .id = 810},
	[R8A779G0_RESET_811] = {.name = RESET_NAME(811), .id = 811},
	[R8A779G0_RESET_812] = {.name = RESET_NAME(812), .id = 812},
	[R8A779G0_RESET_813] = {.name = RESET_NAME(813), .id = 813},
	[R8A779G0_RESET_830] = {.name = RESET_NAME(830), .id = 830},
	[R8A779G0_RESET_831] = {.name = RESET_NAME(831), .id = 831},
	[R8A779G0_RESET_903] = {.name = RESET_NAME(903), .id = 903},
	[R8A779G0_RESET_904] = {.name = RESET_NAME(904), .id = 904},
	[R8A779G0_RESET_905] = {.name = RESET_NAME(905), .id = 905},
	[R8A779G0_RESET_906] = {.name = RESET_NAME(906), .id = 906},
	[R8A779G0_RESET_907] = {.name = RESET_NAME(907), .id = 907},
	[R8A779G0_RESET_910] = {.name = RESET_NAME(910), .id = 910},
	[R8A779G0_RESET_911] = {.name = RESET_NAME(911), .id = 911},
	[R8A779G0_RESET_912] = {.name = RESET_NAME(912), .id = 912},
	[R8A779G0_RESET_913] = {.name = RESET_NAME(913), .id = 913},
	[R8A779G0_RESET_915] = {.name = RESET_NAME(915), .id = 915},
	[R8A779G0_RESET_916] = {.name = RESET_NAME(916), .id = 916},
	[R8A779G0_RESET_917] = {.name = RESET_NAME(917), .id = 917},
	[R8A779G0_RESET_918] = {.name = RESET_NAME(918), .id = 918},
	[R8A779G0_RESET_919] = {.name = RESET_NAME(919), .id = 919},
	[R8A779G0_RESET_1028] = {.name = RESET_NAME(1028), .id = 1028},
	[R8A779G0_RESET_1029] = {.name = RESET_NAME(1029), .id = 1029},
	[R8A779G0_RESET_1100] = {.name = RESET_NAME(1100), .id = 1100},
	[R8A779G0_RESET_1101] = {.name = RESET_NAME(1101), .id = 1101},
	[R8A779G0_RESET_2706] = {.name = RESET_NAME(2706), .id = 2706},
	[R8A779G0_RESET_2707] = {.name = RESET_NAME(2707), .id = 2707},
	[R8A779G0_RESET_2708] = {.name = RESET_NAME(2708), .id = 2708},
	[R8A779G0_RESET_2709] = {.name = RESET_NAME(2709), .id = 2709},
	[R8A779G0_RESET_2712] = {.name = RESET_NAME(2712), .id = 2712},
	[R8A779G0_RESET_2713] = {.name = RESET_NAME(2713), .id = 2713},
	[R8A779G0_RESET_2723] = {.name = RESET_NAME(2723), .id = 2723},
	[R8A779G0_RESET_2806] = {.name = RESET_NAME(2806), .id = 2806},
	[R8A779G0_RESET_2807] = {.name = RESET_NAME(2807), .id = 2807},
	[R8A779G0_RESET_2819] = {.name = RESET_NAME(2819), .id = 2819},
	[R8A779G0_RESET_2821] = {.name = RESET_NAME(2821), .id = 2821},
	[R8A779G0_RESET_2822] = {.name = RESET_NAME(2822), .id = 2822},
	[R8A779G0_RESET_2823] = {.name = RESET_NAME(2823), .id = 2823},
	[R8A779G0_RESET_2824] = {.name = RESET_NAME(2824), .id = 2824},
	[R8A779G0_RESET_2825] = {.name = RESET_NAME(2825), .id = 2825},
	[R8A779G0_RESET_2826] = {.name = RESET_NAME(2826), .id = 2826},
	[R8A779G0_RESET_2827] = {.name = RESET_NAME(2827), .id = 2827},
	[R8A779G0_RESET_2828] = {.name = RESET_NAME(2828), .id = 2828},
	[R8A779G0_RESET_2830] = {.name = RESET_NAME(2830), .id = 2830},
	[R8A779G0_RESET_2831] = {.name = RESET_NAME(2831), .id = 2831},
	[R8A779G0_RESET_2900] = {.name = RESET_NAME(2900), .id = 2900},
	[R8A779G0_RESET_2901] = {.name = RESET_NAME(2901), .id = 2901},
	[R8A779G0_RESET_2902] = {.name = RESET_NAME(2902), .id = 2902},
	[R8A779G0_RESET_2903] = {.name = RESET_NAME(2903), .id = 2903},
	[R8A779G0_RESET_2904] = {.name = RESET_NAME(2904), .id = 2904},
	[R8A779G0_RESET_2905] = {.name = RESET_NAME(2905), .id = 2905},
	[R8A779G0_RESET_2907] = {.name = RESET_NAME(2907), .id = 2907},
	[R8A779G0_RESET_2908] = {.name = RESET_NAME(2908), .id = 2908},
	[R8A779G0_RESET_2909] = {.name = RESET_NAME(2909), .id = 2909},
	[R8A779G0_RESET_2910] = {.name = RESET_NAME(2910), .id = 2910},
	[R8A779G0_RESET_2911] = {.name = RESET_NAME(2911), .id = 2911},
	[R8A779G0_RESET_2912] = {.name = RESET_NAME(2912), .id = 2912},
	[R8A779G0_RESET_2913] = {.name = RESET_NAME(2913), .id = 2913},
	[R8A779G0_RESET_2914] = {.name = RESET_NAME(2914), .id = 2914},
	[R8A779G0_RESET_2916] = {.name = RESET_NAME(2916), .id = 2916},
	[R8A779G0_RESET_2917] = {.name = RESET_NAME(2917), .id = 2917},
	[R8A779G0_RESET_2918] = {.name = RESET_NAME(2918), .id = 2918},
	[R8A779G0_RESET_2919] = {.name = RESET_NAME(2919), .id = 2919},
	[R8A779G0_RESET_2920] = {.name = RESET_NAME(2920), .id = 2920},
	[R8A779G0_RESET_2921] = {.name = RESET_NAME(2921), .id = 2921},
	[R8A779G0_RESET_2922] = {.name = RESET_NAME(2922), .id = 2922},
	[R8A779G0_RESET_2923] = {.name = RESET_NAME(2923), .id = 2923},
	[R8A779G0_RESET_2926] = {.name = RESET_NAME(2926), .id = 2926},
	[R8A779G0_RESET_2927] = {.name = RESET_NAME(2927), .id = 2927},
};

scmi_perm_t gen4_scmi_perm_resets[R8A779G0_RESET_MAX];

size_t plat_scmi_rstd_count(unsigned int agent_id __unused)
{
	return R8A779G0_RESET_MAX;
}

const char *plat_scmi_rstd_get_name(unsigned int agent_id,
				  unsigned int scmi_id)
{
	assert(scmi_id < plat_scmi_rstd_count(agent_id));

	return r8a779g0_resets[scmi_id].name;
}

int32_t plat_scmi_rstd_autonomous(unsigned int agent_id __unused,
				unsigned int scmi_id,
				unsigned int state __unused)
{
	struct gen4_reset *reset;
	uint32_t reg_ofs;
	uint32_t bit_ofs;

	assert(scmi_id < plat_scmi_rstd_count(agent_id));

	reset = &r8a779g0_resets[scmi_id];
	reg_ofs = R8A779G0_REG_ID(reset->id);
	bit_ofs = R8A779G0_REG_BIT_ID(reset->id);

	rcar_mssr_soft_reset(reg_ofs, BIT(bit_ofs), true, false);

	/* Wait for at least one cycle of the RCLK clock (@ ca. 32 kHz) */
	udelay(35);

	rcar_mssr_soft_reset(reg_ofs, BIT(bit_ofs), false, false);

	VERBOSE("scmi: reset:%u autonomous reg:%u bit:%u\n", scmi_id, reg_ofs,
			bit_ofs);
	return SCMI_SUCCESS;
}

int32_t plat_scmi_rstd_set_state(unsigned int agent_id __unused,
			       unsigned int scmi_id,
			       bool assert_not_deassert)
{
	struct gen4_reset *reset;
	uint32_t reg_ofs;
	uint32_t bit_ofs;

	assert(scmi_id < plat_scmi_rstd_count(agent_id));

	reset = &r8a779g0_resets[scmi_id];
	reg_ofs = R8A779G0_REG_ID(reset->id);
	bit_ofs = R8A779G0_REG_BIT_ID(reset->id);

	if (assert_not_deassert) {
		rcar_mssr_soft_reset(reg_ofs, BIT(bit_ofs), true, false);
	} else {
		rcar_mssr_soft_reset(reg_ofs, BIT(bit_ofs), false, false);
	}

	VERBOSE("scmi: reset:%u %s reg:%u bit:%u\n", scmi_id, (assert_not_deassert) ? "assert" : "deassert", reg_ofs,
			bit_ofs);
	return SCMI_SUCCESS;
}
