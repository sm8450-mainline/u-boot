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
#include <errno.h>
#include <image.h>
#include <log.h>
#include <spl.h>
#include <asm/cache.h>

static int spl_fit_images_find(void *blob, int os)
{
	int parent, node, ndepth = 0;
	const void *data;

	if (!blob)
		return -FDT_ERR_BADMAGIC;

	parent = fdt_path_offset(blob, "/fit-images");
	if (parent < 0)
		return -FDT_ERR_NOTFOUND;

	for (node = fdt_next_node(blob, parent, &ndepth);
	     (node >= 0) && (ndepth > 0);
	     node = fdt_next_node(blob, node, &ndepth)) {
		if (ndepth != 1)
			continue;

		data = fdt_getprop(blob, node, FIT_OS_PROP, NULL);
		if (!data)
			continue;

		if (genimg_get_os_id(data) == os)
			return node;
	};

	return -FDT_ERR_NOTFOUND;
}

uintptr_t spl_fit_images_get_entry(void *blob, int node)
{
	ulong  val;
	int ret;

	ret = fit_image_get_entry(blob, node, &val);
	if (ret)
		ret = fit_image_get_load(blob, node, &val);

	debug("%s: entry point 0x%lx\n", __func__, val);
	return val;
}

void __noreturn spl_invoke_atf(struct spl_image_info *spl_image)
{
	uintptr_t  bl32_entry = 0;
	uintptr_t  bl33_entry = CONFIG_TEXT_BASE;
	void *blob = spl_image->fdt_addr;
	uintptr_t platform_param = (uintptr_t)blob;
	int node;

	/*
	 * Find the OP-TEE binary (in /fit-images) load address or
	 * entry point (if different) and pass it as the BL3-2 entry
	 * point, this is optional.
	 */
	node = spl_fit_images_find(blob, IH_OS_TEE);
	if (node >= 0)
		bl32_entry = spl_fit_images_get_entry(blob, node);

	/*
	 * Find the U-Boot binary (in /fit-images) load addreess or
	 * entry point (if different) and pass it as the BL3-3 entry
	 * point.
	 * This will need to be extended to support Falcon mode.
	 */

	node = spl_fit_images_find(blob, IH_OS_U_BOOT);
	if (node >= 0)
		bl33_entry = spl_fit_images_get_entry(blob, node);

	/*
	 * If ATF_NO_PLATFORM_PARAM is set, we override the platform
	 * parameter and always pass 0.  This is a workaround for
	 * older ATF versions that have insufficiently robust (or
	 * overzealous) argument validation.
	 */
	if (CONFIG_IS_ENABLED(ATF_NO_PLATFORM_PARAM))
		platform_param = 0;

	/*
	 * We don't provide a BL3-2 entry yet, but this will be possible
	 * using similar logic.
	 */
	bl31_entry(spl_image->entry_point, bl32_entry,
		   bl33_entry, platform_param);
}
