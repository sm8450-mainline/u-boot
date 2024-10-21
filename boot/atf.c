// SPDX-License-Identifier: BSD-3-Clause
/*
 * Reference to the ARM TF Project,
 * plat/arm/common/arm_bl2_setup.c
 * Portions copyright (c) 2013-2016, ARM Limited and Contributors. All rights
 * reserved.
 * Copyright (C) 2016 Rockchip Electronic Co.,Ltd
 * Written by Kever Yang <kever.yang@rock-chips.com>
 * Copyright (C) 2017 Theobroma Systems Design und Consulting GmbH
 */

#include <atf_common.h>
#include <cpu_func.h>
#include <image.h>
#include <log.h>
#include <spl.h>
#include <asm/cache.h>

/* Holds all the structures we need for bl31 parameter passing */
struct bl2_to_bl31_params_mem {
	struct bl31_params bl31_params;
	struct atf_image_info bl31_image_info;
	struct atf_image_info bl32_image_info;
	struct atf_image_info bl33_image_info;
	struct entry_point_info bl33_ep_info;
	struct entry_point_info bl32_ep_info;
	struct entry_point_info bl31_ep_info;
};

struct bl2_to_bl31_params_mem_v2 {
	struct bl_params bl_params;
	struct bl_params_node bl31_params_node;
	struct bl_params_node bl32_params_node;
	struct bl_params_node bl33_params_node;
	struct atf_image_info bl31_image_info;
	struct atf_image_info bl32_image_info;
	struct atf_image_info bl33_image_info;
	struct entry_point_info bl33_ep_info;
	struct entry_point_info bl32_ep_info;
	struct entry_point_info bl31_ep_info;
};

struct bl31_params *bl2_plat_get_bl31_params_default(uintptr_t bl32_entry,
						     uintptr_t bl33_entry,
						     uintptr_t fdt_addr)
{
	static struct bl2_to_bl31_params_mem bl31_params_mem;
	struct bl31_params *bl2_to_bl31_params;
	struct entry_point_info *bl32_ep_info;
	struct entry_point_info *bl33_ep_info;

	/*
	 * Initialise the memory for all the arguments that needs to
	 * be passed to BL31
	 */
	memset(&bl31_params_mem, 0, sizeof(struct bl2_to_bl31_params_mem));

	/* Assign memory for TF related information */
	bl2_to_bl31_params = &bl31_params_mem.bl31_params;
	SET_PARAM_HEAD(bl2_to_bl31_params, ATF_PARAM_BL31, ATF_VERSION_1, 0);

	/* Fill BL31 related information */
	bl2_to_bl31_params->bl31_image_info = &bl31_params_mem.bl31_image_info;
	SET_PARAM_HEAD(bl2_to_bl31_params->bl31_image_info,
		       ATF_PARAM_IMAGE_BINARY, ATF_VERSION_1, 0);

	/* Fill BL32 related information */
	bl2_to_bl31_params->bl32_ep_info = &bl31_params_mem.bl32_ep_info;
	bl32_ep_info = &bl31_params_mem.bl32_ep_info;
	SET_PARAM_HEAD(bl32_ep_info, ATF_PARAM_EP, ATF_VERSION_1,
		       ATF_EP_SECURE);

	/* secure payload is optional, so set pc to 0 if absent */
	bl32_ep_info->args.arg3 = fdt_addr;
	bl32_ep_info->pc = bl32_entry ? bl32_entry : 0;
	bl32_ep_info->spsr = SPSR_64(MODE_EL1, MODE_SP_ELX,
				     DISABLE_ALL_EXECPTIONS);

	bl2_to_bl31_params->bl32_image_info = &bl31_params_mem.bl32_image_info;
	SET_PARAM_HEAD(bl2_to_bl31_params->bl32_image_info,
		       ATF_PARAM_IMAGE_BINARY, ATF_VERSION_1, 0);

	/* Fill BL33 related information */
	bl2_to_bl31_params->bl33_ep_info = &bl31_params_mem.bl33_ep_info;
	bl33_ep_info = &bl31_params_mem.bl33_ep_info;
	SET_PARAM_HEAD(bl33_ep_info, ATF_PARAM_EP, ATF_VERSION_1,
		       ATF_EP_NON_SECURE);

	/* BL33 expects to receive the primary CPU MPID (through x0) */
	bl33_ep_info->args.arg0 = 0xffff & read_mpidr();
	bl33_ep_info->pc = bl33_entry;
	bl33_ep_info->spsr = SPSR_64(MODE_EL2, MODE_SP_ELX,
				     DISABLE_ALL_EXECPTIONS);

	bl2_to_bl31_params->bl33_image_info = &bl31_params_mem.bl33_image_info;
	SET_PARAM_HEAD(bl2_to_bl31_params->bl33_image_info,
		       ATF_PARAM_IMAGE_BINARY, ATF_VERSION_1, 0);

	return bl2_to_bl31_params;
}

__weak struct bl31_params *bl2_plat_get_bl31_params(uintptr_t bl32_entry,
						    uintptr_t bl33_entry,
						    uintptr_t fdt_addr)
{
	return bl2_plat_get_bl31_params_default(bl32_entry, bl33_entry,
						fdt_addr);
}

struct bl_params *bl2_plat_get_bl31_params_v2_default(uintptr_t bl32_entry,
						      uintptr_t bl33_entry,
						      uintptr_t fdt_addr)
{
	static struct bl2_to_bl31_params_mem_v2 bl31_params_mem;
	struct bl_params *bl_params;
	struct bl_params_node *bl_params_node;

	/*
	 * Initialise the memory for all the arguments that needs to
	 * be passed to BL31
	 */
	memset(&bl31_params_mem, 0, sizeof(bl31_params_mem));

	/* Assign memory for TF related information */
	bl_params = &bl31_params_mem.bl_params;
	SET_PARAM_HEAD(bl_params, ATF_PARAM_BL_PARAMS, ATF_VERSION_2, 0);
	bl_params->head = &bl31_params_mem.bl31_params_node;

	/* Fill BL31 related information */
	bl_params_node = &bl31_params_mem.bl31_params_node;
	bl_params_node->image_id = ATF_BL31_IMAGE_ID;
	bl_params_node->image_info = &bl31_params_mem.bl31_image_info;
	bl_params_node->ep_info = &bl31_params_mem.bl31_ep_info;
	bl_params_node->next_params_info = &bl31_params_mem.bl32_params_node;
	SET_PARAM_HEAD(bl_params_node->image_info, ATF_PARAM_IMAGE_BINARY,
		       ATF_VERSION_2, 0);

	/* Fill BL32 related information */
	bl_params_node = &bl31_params_mem.bl32_params_node;
	bl_params_node->image_id = ATF_BL32_IMAGE_ID;
	bl_params_node->image_info = &bl31_params_mem.bl32_image_info;
	bl_params_node->ep_info = &bl31_params_mem.bl32_ep_info;
	bl_params_node->next_params_info = &bl31_params_mem.bl33_params_node;
	SET_PARAM_HEAD(bl_params_node->ep_info, ATF_PARAM_EP,
		       ATF_VERSION_2, ATF_EP_SECURE);

	/* secure payload is optional, so set pc to 0 if absent */
	bl_params_node->ep_info->args.arg3 = fdt_addr;
	bl_params_node->ep_info->pc = bl32_entry ? bl32_entry : 0;
	bl_params_node->ep_info->spsr = SPSR_64(MODE_EL1, MODE_SP_ELX,
						DISABLE_ALL_EXECPTIONS);
	SET_PARAM_HEAD(bl_params_node->image_info, ATF_PARAM_IMAGE_BINARY,
		       ATF_VERSION_2, 0);

	/* Fill BL33 related information */
	bl_params_node = &bl31_params_mem.bl33_params_node;
	bl_params_node->image_id = ATF_BL33_IMAGE_ID;
	bl_params_node->image_info = &bl31_params_mem.bl33_image_info;
	bl_params_node->ep_info = &bl31_params_mem.bl33_ep_info;
	bl_params_node->next_params_info = NULL;
	SET_PARAM_HEAD(bl_params_node->ep_info, ATF_PARAM_EP,
		       ATF_VERSION_2, ATF_EP_NON_SECURE);

	/* BL33 expects to receive the primary CPU MPID (through x0) */
	bl_params_node->ep_info->args.arg0 = 0xffff & read_mpidr();
	bl_params_node->ep_info->pc = bl33_entry;
	bl_params_node->ep_info->spsr = SPSR_64(MODE_EL2, MODE_SP_ELX,
						DISABLE_ALL_EXECPTIONS);
	SET_PARAM_HEAD(bl_params_node->image_info, ATF_PARAM_IMAGE_BINARY,
		       ATF_VERSION_2, 0);

	return bl_params;
}

__weak struct bl_params *bl2_plat_get_bl31_params_v2(uintptr_t bl32_entry,
						     uintptr_t bl33_entry,
						     uintptr_t fdt_addr)
{
	return bl2_plat_get_bl31_params_v2_default(bl32_entry, bl33_entry,
						   fdt_addr);
}

static inline void raw_write_daif(unsigned int daif)
{
	__asm__ __volatile__("msr DAIF, %x0\n\t" : : "r" (daif) : "memory");
}

typedef void __noreturn (*atf_entry_t)(struct bl31_params *params, void *plat_params);

void __noreturn bl31_entry(uintptr_t atf_addr, uintptr_t bl32_entry,
				  uintptr_t bl33_entry, uintptr_t fdt_addr)
{
	atf_entry_t  atf_entry = (atf_entry_t)atf_addr;
	void *bl31_params;

	if (CONFIG_IS_ENABLED(ATF_LOAD_IMAGE_V2))
		bl31_params = bl2_plat_get_bl31_params_v2(bl32_entry,
							  bl33_entry,
							  fdt_addr);
	else
		bl31_params = bl2_plat_get_bl31_params(bl32_entry, bl33_entry,
						       fdt_addr);

	raw_write_daif(SPSR_EXCEPTION_MASK);
	if (!CONFIG_IS_ENABLED(SYS_DCACHE_OFF))
		dcache_disable();

	atf_entry(bl31_params, (void *)fdt_addr);
}

