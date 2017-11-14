/* Copyright 2017 IBM Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * 	http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <phys-map.h>
#include <chip.h>
#include <skiboot.h>
#include <opal-api.h>
#include <stack.h>
#include <inttypes.h>

struct phys_map_entry {
	enum phys_map_type type;
	int index;
	uint64_t addr;
	uint64_t size;
};

struct phys_map_info {
	int chip_select_shift;
	const struct phys_map_entry *table;
};

static const struct phys_map_info *phys_map;

static const struct phys_map_entry phys_map_table_nimbus[] = {

	/* System memory upto 4TB minus GPU memory */
	{ SYSTEM_MEM,      0, 0x0000000000000000ull, 0x0000034000000000ull },
	/* GPU memory from 4TB - 128GB*GPU */
	{ GPU_MEM_4T_DOWN, 5, 0x0000034000000000ull, 0x0000002000000000ull },
	{ GPU_MEM_4T_DOWN, 4, 0x0000036000000000ull, 0x0000002000000000ull },
	{ GPU_MEM_4T_DOWN, 3, 0x0000038000000000ull, 0x0000002000000000ull },
	{ GPU_MEM_4T_DOWN, 2, 0x000003a000000000ull, 0x0000002000000000ull },
	{ GPU_MEM_4T_DOWN, 1, 0x000003c000000000ull, 0x0000002000000000ull },
	{ GPU_MEM_4T_DOWN, 0, 0x000003e000000000ull, 0x0000002000000000ull },
	/* GPU memory from 4TB + 128GB*GPU. 4 GPUs only */
	{ GPU_MEM_4T_UP,   0, 0x0000040000000000ull, 0x0000002000000000ull },
	{ GPU_MEM_4T_UP,   1, 0x0000042000000000ull, 0x0000002000000000ull },
	{ GPU_MEM_4T_UP,   2, 0x0000044000000000ull, 0x0000002000000000ull },
	{ GPU_MEM_4T_UP,   3, 0x0000046000000000ull, 0x0000002000000000ull },

	/* 0 TB offset @ MMIO 0x0006000000000000ull */
	{ PHB4_64BIT_MMIO, 0, 0x0006000000000000ull, 0x0000004000000000ull },
	{ PHB4_64BIT_MMIO, 1, 0x0006004000000000ull, 0x0000004000000000ull },
	{ PHB4_64BIT_MMIO, 2, 0x0006008000000000ull, 0x0000004000000000ull },
	{ PHB4_32BIT_MMIO, 0, 0x000600c000000000ull, 0x0000000080000000ull },
	{ PHB4_32BIT_MMIO, 1, 0x000600c080000000ull, 0x0000000080000000ull },
	{ PHB4_32BIT_MMIO, 2, 0x000600c100000000ull, 0x0000000080000000ull },
	{ PHB4_32BIT_MMIO, 3, 0x000600c180000000ull, 0x0000000080000000ull },
	{ PHB4_32BIT_MMIO, 4, 0x000600c200000000ull, 0x0000000080000000ull },
	{ PHB4_32BIT_MMIO, 5, 0x000600c280000000ull, 0x0000000080000000ull },
	{ PHB4_XIVE_ESB  , 0, 0x000600c300000000ull, 0x0000000020000000ull },
	{ PHB4_XIVE_ESB  , 1, 0x000600c320000000ull, 0x0000000020000000ull },
	{ PHB4_XIVE_ESB  , 2, 0x000600c340000000ull, 0x0000000020000000ull },
	{ PHB4_XIVE_ESB  , 3, 0x000600c360000000ull, 0x0000000020000000ull },
	{ PHB4_XIVE_ESB  , 4, 0x000600c380000000ull, 0x0000000020000000ull },
	{ PHB4_XIVE_ESB  , 5, 0x000600c3a0000000ull, 0x0000000020000000ull },
	{ PHB4_REG_SPC   , 0, 0x000600c3c0000000ull, 0x0000000000100000ull },
	{ PHB4_REG_SPC   , 1, 0x000600c3c0100000ull, 0x0000000000100000ull },
	{ PHB4_REG_SPC   , 2, 0x000600c3c0200000ull, 0x0000000000100000ull },
	{ PHB4_REG_SPC   , 3, 0x000600c3c0300000ull, 0x0000000000100000ull },
	{ PHB4_REG_SPC   , 4, 0x000600c3c0400000ull, 0x0000000000100000ull },
	{ PHB4_REG_SPC   , 5, 0x000600c3c0500000ull, 0x0000000000100000ull },
	{ RESV	         , 0, 0x000600c3c0600000ull, 0x0000000c3fa00000ull },
	{ NPU_OCAPI_MMIO , 0, 0x000600d000000000ull, 0x0000000800000000ull },
	{ NPU_OCAPI_MMIO , 1, 0x000600d800000000ull, 0x0000000800000000ull },
	{ NPU_OCAPI_MMIO , 2, 0x000600e000000000ull, 0x0000000800000000ull },
	{ NPU_OCAPI_MMIO , 3, 0x000600e800000000ull, 0x0000000800000000ull },
	{ NPU_OCAPI_MMIO , 4, 0x000600f000000000ull, 0x0000000800000000ull },
	{ NPU_OCAPI_MMIO , 5, 0x000600f800000000ull, 0x0000000800000000ull },

	/* 1 TB offset @ MMIO 0x0006000000000000ull */
	{ XIVE_VC        , 0, 0x0006010000000000ull, 0x0000008000000000ull },
	{ XIVE_PC        , 0, 0x0006018000000000ull, 0x0000001000000000ull },
	{ VAS_USER_WIN   , 0, 0x0006019000000000ull, 0x0000000100000000ull },
	{ VAS_HYP_WIN    , 0, 0x0006019100000000ull, 0x0000000002000000ull },
	{ RESV	         , 1, 0x0006019102000000ull, 0x000000001e000000ull },
	{ OCAB_XIVE_ESB  , 0, 0x0006019120000000ull, 0x0000000020000000ull },
	{ RESV	         , 3, 0x0006019140000000ull, 0x0000006ec0000000ull },

	/* 2 TB offset @ MMIO 0x0006000000000000ull */
	{ PHB4_64BIT_MMIO, 3, 0x0006020000000000ull, 0x0000004000000000ull },
	{ PHB4_64BIT_MMIO, 4, 0x0006024000000000ull, 0x0000004000000000ull },
	{ PHB4_64BIT_MMIO, 5, 0x0006028000000000ull, 0x0000004000000000ull },
	{ RESV	         , 4, 0x000602c000000000ull, 0x0000004000000000ull },

	/* 3 TB offset @ MMIO 0x0006000000000000ull */
	{ LPC_BUS        , 0, 0x0006030000000000ull, 0x0000000100000000ull },
	{ FSP_MMIO       , 0, 0x0006030100000000ull, 0x0000000100000000ull },
	{ NPU_REGS       , 0, 0x0006030200000000ull, 0x0000000001000000ull },
	{ NPU_USR        , 0, 0x0006030201000000ull, 0x0000000000200000ull },
	{ NPU_PHY        , 0, 0x0006030201200000ull, 0x0000000000200000ull },
	{ NPU_PHY	 , 1, 0x0006030201400000ull, 0x0000000000200000ull },
	{ NPU_NTL	 , 0, 0x0006030201600000ull, 0x0000000000020000ull },
	{ NPU_NTL	 , 1, 0x0006030201620000ull, 0x0000000000020000ull },
	{ NPU_NTL	 , 2, 0x0006030201640000ull, 0x0000000000020000ull },
	{ NPU_NTL	 , 3, 0x0006030201660000ull, 0x0000000000020000ull },
	{ NPU_NTL	 , 4, 0x0006030201680000ull, 0x0000000000020000ull },
	{ NPU_NTL	 , 5, 0x00060302016a0000ull, 0x0000000000020000ull },
	{ NPU_GENID	 , 0, 0x00060302016c0000ull, 0x0000000000020000ull },
	{ NPU_GENID	 , 1, 0x00060302016e0000ull, 0x0000000000020000ull },
	{ NPU_GENID	 , 2, 0x0006030201700000ull, 0x0000000000020000ull },
	{ RESV	         , 5, 0x0006030201720000ull, 0x00000000018e0000ull },
	{ PSIHB_REG      , 0, 0x0006030203000000ull, 0x0000000000100000ull },
	{ XIVE_IC        , 0, 0x0006030203100000ull, 0x0000000000080000ull },
	{ XIVE_TM        , 0, 0x0006030203180000ull, 0x0000000000040000ull },
	{ PSIHB_ESB      , 0, 0x00060302031c0000ull, 0x0000000000010000ull },
	{ NX_RNG         , 0, 0x00060302031d0000ull, 0x0000000000010000ull },
	{ RESV           , 6, 0x00060302031e0000ull, 0x000000001ce20000ull },
	{ CENTAUR_SCOM   , 0, 0x0006030220000000ull, 0x0000000020000000ull },
	{ RESV           , 7, 0x0006030240000000ull, 0x000000f9c0000000ull },
	{ XSCOM          , 0, 0x000603fc00000000ull, 0x0000000400000000ull },

	/* NULL entry at end */
	{ NULL_MAP, 0, 0, 0 },
};

static const struct phys_map_info phys_map_nimbus = {
	.chip_select_shift = 42,
	.table = phys_map_table_nimbus,
};

static inline bool phys_map_entry_null(const struct phys_map_entry *e)
{
	if (e->type == NULL_MAP)
		return true;
	return false;
}


/* This crashes skiboot on error as any bad calls here are almost
 *  certainly a developer error
 */
void phys_map_get(uint64_t gcid, enum phys_map_type type,
		  int index, uint64_t *addr, uint64_t *size) {
	const struct phys_map_entry *e;
	uint64_t a;

	if (!phys_map)
		goto error;

	/* Find entry in table */
	for (e = phys_map->table; ; e++) {

		/* End of table */
		if (phys_map_entry_null(e))
			goto error;

		/* Is this our entry? */
		if (e->type != type)
			continue;
		if (e->index != index)
			continue;

		/* Found entry! */
		break;
	}
	a = e->addr;
	a += gcid << phys_map->chip_select_shift;

	if (addr)
		*addr = a;
	if (size)
		*size = e->size;

	prlog(PR_TRACE, "Assigning BAR [%"PRIx64"] type:%02i index:%x "
	      "0x%016"PRIx64" for 0x%016"PRIx64"\n",
	      gcid, type, index, a, e->size);

	return;

error:
	/* Something has gone really wrong */
	prlog(PR_EMERG, "ERROR: Failed to lookup BAR type:%i index:%i",
	      type, index);
	assert(0);
}

void phys_map_init(void)
{
	const char *name = "unused";

	phys_map = NULL;

	if (proc_gen == proc_gen_p9) {
		name = "nimbus";
		phys_map = &phys_map_nimbus;
	}

	prlog(PR_DEBUG, "Assigning physical memory map table for %s\n", name);

}

