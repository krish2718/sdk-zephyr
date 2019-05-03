/*
 * Copyright (c) 2021, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_IPC_RPMSG_BACKEND_H
#define ZEPHYR_SUBSYS_IPC_RPMSG_BACKEND_H

#include <openamp/rpmsg_virtio.h>

#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_PARTITION_MANAGER_ENABLED

#include "pm_config.h"

#if defined(PM_RPMSG_NRF53_SRAM_ADDRESS) || defined(PM__RPMSG_NRF53_SRAM_ADDRESS)

#if defined(PM_RPMSG_NRF53_SRAM_ADDRESS)
#define VDEV_START_ADDR PM_RPMSG_NRF53_SRAM_ADDRESS
#define VDEV_SIZE PM_RPMSG_NRF53_SRAM_SIZE
#else
/* The current image is a child image in a different domain than the image
 * which defined the required values. To reach the values of the parent domain
 * we use the 'PM__' variant of the define.
 */
#define VDEV_START_ADDR PM__RPMSG_NRF53_SRAM_ADDRESS
#define VDEV_SIZE PM__RPMSG_NRF53_SRAM_SIZE
#endif /* defined(PM_RPMSG_NRF53_SRAM_ADDRESS) */

#else
#define VDEV_START_ADDR		DT_REG_ADDR(DT_CHOSEN(zephyr_ipc_shm))
#define VDEV_SIZE		DT_REG_SIZE(DT_CHOSEN(zephyr_ipc_shm))
#endif /* defined(PM_RPMSG_NRF53_SRAM_ADDRESS) || defined(PM__RPMSG_NRF53_SRAM_ADDRESS) */

#else

#define VDEV_START_ADDR		DT_REG_ADDR(DT_CHOSEN(zephyr_ipc_shm))
#define VDEV_SIZE		DT_REG_SIZE(DT_CHOSEN(zephyr_ipc_shm))

#endif /* CONFIG_PARTITION_MANAGER_ENABLED */

#define VDEV_STATUS_ADDR	VDEV_START_ADDR
#define VDEV_STATUS_SIZE	0x400

#define SHM_START_ADDR		(VDEV_START_ADDR + VDEV_STATUS_SIZE)
#define SHM_SIZE		    (VDEV_SIZE - VDEV_STATUS_SIZE)
#define SHM_DEVICE_NAME		"sramx.shm"

/*
 * @brief Initialize RPMsg backend
 *
 * @param io   Shared memory IO region. This is an output parameter providing
 *             a pointer to an actual shared memory IO region structure.
 *             Caller of this function shall pass an address at which the
 *             pointer to the shared memory IO region structure is stored.
 * @param vdev Pointer to the virtio device initialized by this function.
 *
 * @retval 0 Initialization successful
 * @retval <0 Initialization error reported by OpenAMP
 */
int rpmsg_backend_init(struct metal_io_region **io, struct virtio_device *vdev);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SUBSYS_IPC_RPMSG_BACKEND_H */
