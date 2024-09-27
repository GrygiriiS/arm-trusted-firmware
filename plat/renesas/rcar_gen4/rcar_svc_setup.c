/*
 * Copyright 2024 EPAM Systems
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdbool.h>
#include <stdint.h>

#include <common/debug.h>
#include <common/runtime_svc.h>
#include <drivers/scmi-msg.h>

#include <rcar_smc.h>

/* Setup Standard Services */
static int32_t rcar_svc_setup(void)
{
	return 0;
}

/*
 * Top-level Standard Service SMC handler.
 */
static uintptr_t rcar_svc_smc_handler(uint32_t smc_fid, u_register_t x1,
				      u_register_t x2, u_register_t x3,
				      u_register_t x4, void *cookie,
				      void *handle, u_register_t flags)
{
	uint32_t ret1 = 0U;

	switch (smc_fid) {
	case RCAR_SIP_SMC_SCMI:
		scmi_smt_fastcall_smc_entry(0);
		break;
	case RCAR_SIP_SMC_SCMI + 1:
		scmi_smt_fastcall_smc_entry(1);
		break;
	case RCAR_SIP_SMC_SCMI + 2:
		scmi_smt_fastcall_smc_entry(2);
		break;
	case RCAR_SIP_SMC_SCMI + 3:
		scmi_smt_fastcall_smc_entry(3);
		break;
	case RCAR_SIP_SMC_SCMI + 4:
		scmi_smt_fastcall_smc_entry(4);
		break;
	case RCAR_SIP_SMC_SCMI + 5:
		scmi_smt_fastcall_smc_entry(5);
		break;
	case RCAR_SIP_SMC_SCMI + 6:
		scmi_smt_fastcall_smc_entry(6);
		break;
	case RCAR_SIP_SMC_SCMI + 7:
		scmi_smt_fastcall_smc_entry(7);
		break;

	default:
		WARN("Unknown RCAR Service Call: 0x%x\n", smc_fid);
		ret1 = SMC_UNK;
		break;
	}

	SMC_RET1(handle, ret1);
}

/* Register Standard Service Calls as runtime service */
DECLARE_RT_SVC(rcar_sip_svc,
	       OEN_SIP_START,
	       OEN_SIP_END,
	       SMC_TYPE_FAST,
	       rcar_svc_setup,
	       rcar_svc_smc_handler
);
