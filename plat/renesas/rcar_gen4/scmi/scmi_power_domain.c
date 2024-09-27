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
#include <lib/mmio.h>
#include <lib/spinlock.h>
#include <string.h>
#include <platform_def.h>

#include "rcar_scmi_resources.h"

#define R8A779G0_SYSC_BASE U(0xe6180000)

/* SYSC Common */
#define SYSCSR        0x000                 /* SYSC Status Register */
#define SYSCPONSR(x)  (0x800 + ((x) * 0x4)) /* Power-ON Status Register 0 */
#define SYSCPOFFSR(x) (0x808 + ((x) * 0x4)) /* Power-OFF Status Register */
#define SYSCISCR(x)   (0x810 + ((x) * 0x4)) /* Interrupt Status/Clear Reg */
#define SYSCIER(x)    (0x820 + ((x) * 0x4)) /* Interrupt Enable Register */
#define SYSCIMR(x)    (0x830 + ((x) * 0x4)) /* Interrupt Mask Register */

/* Power Domain Registers */
#define PDRSR(n)    (0x1000 + ((n) * 0x40))
#define PDRONCR(n)  (0x1004 + ((n) * 0x40))
#define PDROFFCR(n) (0x1008 + ((n) * 0x40))
#define PDRESR(n)   (0x100C + ((n) * 0x40))

/* PWRON/PWROFF */
#define PWRON_PWROFF BIT(0) /* Power-ON/OFF request */

/* PDRESR */
#define PDRESR_ERR BIT(0)

/* PDRSR */
#define PDRSR_OFF       BIT(0)  /* Power-OFF state */
#define PDRSR_ON        BIT(4)  /* Power-ON state */
#define PDRSR_OFF_STATE BIT(8)  /* Processing Power-OFF sequence */
#define PDRSR_ON_STATE  BIT(12) /* Processing Power-ON sequence */

#define SYSCSR_NOT_BUSY GENMASK(1, 0) /* All bit sets is not busy */

#define SYSCSR_TIMEOUT  10000
#define SYSCSR_DELAY_US 10

#define PDRESR_RETRIES  1000
#define PDRESR_DELAY_US 10

#define SYSCISR_TIMEOUT  10000
#define SYSCISR_DELAY_US 10

#define NUM_DOMAINS_EACH_REG   BITS_PER_TYPE(u32)
#define RCAR_GEN4_PD_ALWAYS_ON 64

/*
 * Power Domain flags
 */
#define PD_CPU   BIT(0) /* Area contains main CPU core */
#define PD_SCU   BIT(1) /* Area contains SCU and L2 cache */
#define PD_NO_CR BIT(2) /* Area lacks PWR{ON,OFF}CR registers */
#define PD_FORCE_ALWAYS_ON BIT(3) /* Power-on at init and remain always-on */

#define PD_CPU_NOCR  (PD_CPU | PD_NO_CR) /* CPU area lacks CR */
#define PD_ALWAYS_ON PD_NO_CR            /* Always-on area */
#define PD_ALWAYS_ON_ALL (PD_CPU | PD_SCU | PD_NO_CR | PD_FORCE_ALWAYS_ON)


#define SCMI_DEFAULT_STRING_LENGTH	16U

/*
 * Description of a Power Area
 */
struct rcar_gen4_pd {
	char name[SCMI_DEFAULT_STRING_LENGTH];
	int32_t parent; /* -1 if none */
	uint32_t flags; /* See PD_* */
	uint32_t use_cnt;
};

static struct rcar_gen4_pd r8a779g0_areas[R8A779G0_PD_MAX] = {
	[R8A779G0_PD_ALWAYS_ON] = {"always-on", -1, PD_ALWAYS_ON},
	[R8A779G0_PD_A3E0] = {"a3e0", -1, PD_SCU},
	[R8A779G0_PD_A2E0D0] = {"a2e0d0", R8A779G0_PD_A3E0, PD_SCU},
	[R8A779G0_PD_A2E0D1] = {"a2e0d1", R8A779G0_PD_A3E0, PD_SCU},
	[R8A779G0_PD_A1E0D0C0] = {"a1e0d0c0", R8A779G0_PD_A2E0D0, PD_CPU_NOCR},
	[R8A779G0_PD_A1E0D0C1] = {"a1e0d0c1", R8A779G0_PD_A2E0D0, PD_CPU_NOCR},
	[R8A779G0_PD_A1E0D1C0] = {"a1e0d1c0", R8A779G0_PD_A2E0D1, PD_CPU_NOCR},
	[R8A779G0_PD_A1E0D1C1] = {"a1e0d1c1", R8A779G0_PD_A2E0D1, PD_CPU_NOCR},
	[R8A779G0_PD_A33DGA] = {"3dg-a", -1},
	[R8A779G0_PD_A23DGB] = {"3dg-b", R8A779G0_PD_A33DGA},
	[R8A779G0_PD_A3VIP0] = {"a3vip0", -1},
	[R8A779G0_PD_A3VIP1] = {"a3vip1", -1},
	[R8A779G0_PD_A3VIP2] = {"a3vip2", -1},
	[R8A779G0_PD_A3ISP0] = {"a3isp0", -1},
	[R8A779G0_PD_A3ISP1] = {"a3isp1", -1},
	[R8A779G0_PD_A3IR] = {"a3ir", -1, PD_FORCE_ALWAYS_ON},
	[R8A779G0_PD_A2CN0] = {"a2cn0", R8A779G0_PD_A3IR, PD_FORCE_ALWAYS_ON},
	[R8A779G0_PD_A2IMP01] = {"a2imp01", R8A779G0_PD_A3IR},
	[R8A779G0_PD_A2IMP23] = {"a2imp23", R8A779G0_PD_A3IR},
	[R8A779G0_PD_A2PSC] = {"a2psc", R8A779G0_PD_A3IR},
	[R8A779G0_PD_A2DMA] = {"a2dma", R8A779G0_PD_A3IR},
	[R8A779G0_PD_A2CV0] = {"a2cv0", R8A779G0_PD_A3IR},
	[R8A779G0_PD_A2CV1] = {"a2cv1", R8A779G0_PD_A3IR},
	[R8A779G0_PD_A2CV2] = {"a2cv2", R8A779G0_PD_A3IR},
	[R8A779G0_PD_A2CV3] = {"a2cv3", R8A779G0_PD_A3IR},
	[R8A779G0_PD_A1CNN0] = {"a1cnn0", R8A779G0_PD_A2CN0,
				PD_FORCE_ALWAYS_ON},
	[R8A779G0_PD_A1DSP0] = {"a1dsp0", R8A779G0_PD_A2CN0},
	[R8A779G0_PD_A1DSP1] = {"a1dsp1", R8A779G0_PD_A2CN0},
	[R8A779G0_PD_A1DSP2] = {"a1dsp2", R8A779G0_PD_A2CN0},
	[R8A779G0_PD_A1DSP3] = {"a1dsp3", R8A779G0_PD_A2CN0},
	[R8A779G0_PD_A3DUL] = {"a3dul", -1},
};

#define SCMI_POWER_STATE_OFF	BIT(30)

scmi_perm_t rcar_scmi_perm_power_domains[R8A779G0_PD_MAX];

static spinlock_t rcar_pd_lock;

static int rcar_gen4_sysc_pwr_on_off(uint32_t pdr, bool on)
{
	uint32_t reg_offs, wait_mask;
	uint32_t val;
	uint32_t tmo = SYSCISR_TIMEOUT;
	int ret = SCMI_SUCCESS;

	if (on) {
		reg_offs = PDRONCR(pdr);
		wait_mask = PDRSR_ON_STATE;
	} else {
		reg_offs = PDROFFCR(pdr);
		wait_mask = PDRSR_OFF_STATE;
	}

	/* Wait until SYSC is ready to accept a power request */
	val = mmio_read_32(R8A779G0_SYSC_BASE + SYSCSR);
	if (!((val & SYSCSR_NOT_BUSY) == SYSCSR_NOT_BUSY)) {
		ERROR("domain:%d sysc busy SYSCSR:%08x\n", pdr, val);
		return SCMI_BUSY;
	}

	val = mmio_read_32(R8A779G0_SYSC_BASE + PDRESR(pdr));
	if (val & PDRESR_ERR) {
		ERROR("domain:%d err PDRESR:%08x\n", pdr, val);
		return SCMI_GENERIC_ERROR;
	}

	val = mmio_read_32(R8A779G0_SYSC_BASE + PDRSR(pdr));
	if (val & (PDRSR_OFF_STATE | PDRSR_ON_STATE)) {
		ERROR("domain:%d busy PDRSR:%08x\n", pdr, val);
		return SCMI_BUSY;
	}

	if (on && (val & PDRSR_ON)) {
		/* Already on */
		INFO("domain:%d already on PDRSR:%08x\n", pdr, val);
		return ret;
	} else if (!on && (val & PDRSR_OFF)) {
		/* Already off */
		INFO("domain:%d already off PDRSR:%08x\n", pdr, val);
		return ret;
	}

	/* Submit power shutoff or power resume request*/
	mmio_write_32(R8A779G0_SYSC_BASE + reg_offs, PWRON_PWROFF);

	val = mmio_read_32(R8A779G0_SYSC_BASE + PDRESR(pdr));
	if (val & PDRESR_ERR) {
		return SCMI_GENERIC_ERROR;
	}

	while (tmo) {
		val = mmio_read_32(R8A779G0_SYSC_BASE + PDRSR(pdr));
		if (!(val & wait_mask)) {
			break;
		}

		udelay(SYSCISR_DELAY_US);
		tmo -= SYSCISR_DELAY_US;
	}

	if (!tmo) {
		ret = SCMI_GENERIC_ERROR;
	}

	return ret;
}

static int rcar_gen4_sysc_power_on(uint32_t pdr)
{
	int ret = SCMI_SUCCESS;

	VERBOSE("start sysc power ON domain:%d status:%08x use_cnt:%u\n", pdr,
		mmio_read_32(R8A779G0_SYSC_BASE + PDRSR(pdr)),
		r8a779g0_areas[pdr].use_cnt);

	if (r8a779g0_areas[pdr].use_cnt) {
		r8a779g0_areas[pdr].use_cnt++;
		goto exit_ret;
	}

	if (r8a779g0_areas[pdr].parent >= 0) {
		ret = rcar_gen4_sysc_power_on(r8a779g0_areas[pdr].parent);
		if (ret) {
			ERROR("paernt_domain:%d on gen err (%d)\n",
			      r8a779g0_areas[pdr].parent, ret);
			goto exit_ret;
		}
	}

	ret = rcar_gen4_sysc_pwr_on_off(pdr, true);
	if (ret) {
		ERROR("domain:%d gen err (%d)\n", pdr, ret);
		goto exit_ret;
	}

	r8a779g0_areas[pdr].use_cnt++;

exit_ret:
	VERBOSE("sysc power ON domain:%d status:%08x (%d) use_cnt:%u\n", pdr,
		mmio_read_32(R8A779G0_SYSC_BASE + PDRSR(pdr)), ret,
		r8a779g0_areas[pdr].use_cnt);

	return ret;
}

static int rcar_gen4_sysc_power_off(uint32_t pdr)
{
	int ret = SCMI_SUCCESS;

	VERBOSE("start sysc power OFF domain:%d status:%08x use_cnt:%u\n", pdr,
		mmio_read_32(R8A779G0_SYSC_BASE + PDRSR(pdr)),
		r8a779g0_areas[pdr].use_cnt);

	if (!r8a779g0_areas[pdr].use_cnt) {
		VERBOSE("domain:%d off use_cnt(0) err\n", pdr);
		ret = SCMI_BUSY;
		goto exit_ret;
	}

	if (r8a779g0_areas[pdr].use_cnt > 1) {
		r8a779g0_areas[pdr].use_cnt--;
		goto exit_ret;
	}

	if (r8a779g0_areas[pdr].flags & PD_ALWAYS_ON_ALL) {
		/* always on PD */
		return SCMI_BUSY;
	}

	ret = rcar_gen4_sysc_pwr_on_off(pdr, false);
	if (ret) {
		ERROR("domain:%d gen err (%d)\n", pdr, ret);
		goto exit_ret;
	}

	r8a779g0_areas[pdr].use_cnt--;

	if (r8a779g0_areas[pdr].parent >= 0) {
		ret = rcar_gen4_sysc_power_off(r8a779g0_areas[pdr].parent);
		if (ret) {
			VERBOSE("paernt_domain:%d off gen err (%d)\n",
				r8a779g0_areas[pdr].parent, ret);
			goto exit_ret;
		}
	}

exit_ret:
	VERBOSE("sysc power OFF domain:%d status:%08x (%d) use_cnt:%u\n", pdr,
		mmio_read_32(R8A779G0_SYSC_BASE + PDRSR(pdr)), ret,
		r8a779g0_areas[pdr].use_cnt);

	return ret;
}

size_t plat_scmi_pd_count(unsigned int agent_id __unused)
{
	return R8A779G0_PD_MAX;
}

const char *plat_scmi_pd_get_name(unsigned int agent_id __unused,
				  unsigned int pd_id)
{
	assert(pd_id < plat_scmi_pd_count(agent_id));

	return r8a779g0_areas[pd_id].name;
}

unsigned int plat_scmi_pd_get_state(unsigned int agent_id,
				    unsigned int pd_id)
{
	struct rcar_gen4_pd *pd;
	uint32_t status;

	assert(pd_id < plat_scmi_pd_count(agent_id));
	pd = &r8a779g0_areas[pd_id];

	if (pd->flags & PD_ALWAYS_ON_ALL) {
		/* always on PD */
		return 0;
	}

	status = mmio_read_32(R8A779G0_SYSC_BASE + PDRSR(pd_id));

	if (status & PDRSR_OFF)
		return SCMI_POWER_STATE_OFF;

	return 0;
}

int32_t plat_scmi_pd_set_state(unsigned int agent_id,
			       unsigned int flags __unused,
			       unsigned int pd_id,
			       unsigned int state)
{
	int32_t ret;
	bool on;

	assert(pd_id < plat_scmi_pd_count(agent_id));

	on = !(state & SCMI_POWER_STATE_OFF);

	spin_lock(&rcar_pd_lock);
	if (on) {
		ret = rcar_gen4_sysc_power_on(pd_id);
	} else {
		ret = rcar_gen4_sysc_power_off(pd_id);
	}
	spin_unlock(&rcar_pd_lock);

	return ret;
}

void rcar_init_scmi_power_domains(void)
{
	struct rcar_gen4_pd *pd;
	int32_t ret;
	int i;

	INFO("sysc power ON boot status status0:%08x status1:%08x\n",
	     mmio_read_32(R8A779G0_SYSC_BASE + SYSCPONSR(0)),
	     mmio_read_32(R8A779G0_SYSC_BASE + SYSCPONSR(1)));
	INFO("sysc power OFF boot status status0:%08x status1:%08x\n",
	     mmio_read_32(R8A779G0_SYSC_BASE + SYSCPOFFSR(0)),
	     mmio_read_32(R8A779G0_SYSC_BASE + SYSCPOFFSR(1)));

	for (i = 0; i < R8A779G0_PD_MAX; i++) {
		pd = &r8a779g0_areas[i];

		if (!strnlen(pd->name, SCMI_DEFAULT_STRING_LENGTH)) {
			pd->flags = PD_NO_CR;
			snprintf(pd->name, SCMI_DEFAULT_STRING_LENGTH, "reserved-%d", i);
			continue;
		}

		if (pd->flags & (PD_CPU | PD_SCU | PD_NO_CR)) {
			continue;
		}

		if (pd->flags & PD_FORCE_ALWAYS_ON) {
			ret = rcar_gen4_sysc_power_on(i);
			if (ret) {
				ERROR("domain:%d gen err (%d)\n", i, ret);
			}
		}
	}
}
