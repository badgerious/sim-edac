// SPDX-License-Identifier: GPL-2.0-only
#include <linux/module.h>
#include <linux/init.h>
#include <linux/edac.h>
#include <linux/bits.h>
#include <asm/mce.h>

#include "edac_module.h"

#define EDAC_MOD_STR "sim_edac"

/*
 *  * Get a bit field at register value <v>, from bit <lo> to bit <hi>
 *   */
#define GET_BITFIELD(v, lo, hi) (((v)&GENMASK_ULL((hi), (lo))) >> (lo))

struct sim_edac_pvt {
};

static struct mem_ctl_info *mci;

static int sim_mce_check_error(struct notifier_block *nb, unsigned long val,
			       void *data)
{
	struct mce *mce = (struct mce *)data;
	u32 channel = 0;
	u32 core_err_cnt = GET_BITFIELD(mce->status, 38, 52);
	const char *optype = "memory read error";
	const char *msg = "";
	int dimm = 0;

	edac_mc_handle_error(HW_EVENT_ERR_CORRECTED, mci, core_err_cnt,
			     mce->addr >> PAGE_SHIFT, mce->addr & ~PAGE_MASK, 0,
			     channel, dimm, -1, optype, msg);

	mce->kflags |= MCE_HANDLED_EDAC;
	return NOTIFY_OK;
}

static struct notifier_block sim_mce_dec = {
	.notifier_call	= sim_mce_check_error,
	.priority	= MCE_PRIO_EDAC,
};

static int sim_register_mci(void)
{
	struct edac_mc_layer layers[2];
	struct sim_edac_pvt *pvt;
	int rc;

	layers[0].type = EDAC_MC_LAYER_CHANNEL;
	layers[0].size = 1;
	layers[0].is_virt_csrow = false;
	layers[1].type = EDAC_MC_LAYER_SLOT;
	layers[1].size = 1;
	layers[1].is_virt_csrow = true;

	mci = edac_mc_alloc(0, ARRAY_SIZE(layers), layers,
			    sizeof(*pvt));
	if (unlikely(!mci))
		return -ENOMEM;

	mci->mtype_cap = MEM_FLAG_DDR4;
	mci->edac_ctl_cap = EDAC_FLAG_NONE;
	mci->edac_cap = EDAC_FLAG_NONE;
	mci->mod_name = EDAC_MOD_STR;
	mci->dev_name = EDAC_MOD_STR;
	mci->ctl_page_to_phys = NULL;
	mci->ctl_name = EDAC_MOD_STR;

	// TODO:
	mci->pdev = NULL;

	if (unlikely(edac_mc_add_mc(mci))) {
		edac_dbg(0, "MC: failed edac_mc_add_mc()\n");
		rc = -EINVAL;
		goto fail;
	}

	return 0;

fail:
	edac_mc_free(mci);
	mci = NULL;
	return rc;
}

static void sim_unregister_mci(void)
{
	edac_mc_del_mc(mci->pdev);
	edac_mc_free(mci);
	mci = NULL;
}

static int __init sim_edac_init(void)
{
	const char *owner;
	int rc;

	edac_dbg(2, "\n");

	owner = edac_get_owner();
	if (owner && strncmp(owner, EDAC_MOD_STR, sizeof(EDAC_MOD_STR)))
		return -EBUSY;

	rc = sim_register_mci();
	if (rc)
		return rc;

	mce_register_decode_chain(&sim_mce_dec);

	return 0;
}

static void __exit sim_edac_exit(void)
{
	edac_dbg(2, "\n");
	mce_unregister_decode_chain(&sim_mce_dec);
	sim_unregister_mci();
}

module_init(sim_edac_init);
module_exit(sim_edac_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MC Driver for VM testing");
