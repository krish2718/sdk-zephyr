/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief File containing sQSPI device interface specific definitions for the
 * Zephyr OS layer of the Wi-Fi driver.
 */

#include <errno.h>
#include <string.h>

#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/mspi.h>
#include <zephyr/drivers/mspi/devicetree.h>
#include <zephyr/drivers/wifi/nrf_wifi/bus/qspi_if.h>

#include <zephyr/drivers/wifi/nrf_wifi/bus/rpu_hw_if.h>
#include "spi_nor.h"

/* The NRF70 device node which is on the sQSPI bus */
#define SQSPI_IF_DEVICE_NODE DT_NODELABEL(nrf70)

/* Get MSPI bus device from devicetree */
#define SQSPI_IF_BUS_DEVICE DT_NODELABEL(sqspi)

static struct qspi_config *sqspi_cfg;
static const struct device *sqspi_bus = DEVICE_DT_GET(SQSPI_IF_BUS_DEVICE);
static struct mspi_dev_id sqspi_dev_id = MSPI_DEVICE_ID_DT(SQSPI_IF_DEVICE_NODE);
static struct mspi_dev_cfg sqspi_dev_cfg = MSPI_DEVICE_CONFIG_DT(SQSPI_IF_DEVICE_NODE);

LOG_MODULE_DECLARE(wifi_nrf_bus, CONFIG_WIFI_NRF70_BUSLIB_LOG_LEVEL);

struct sqspi_data {
#ifdef CONFIG_MULTITHREADING
	struct k_sem lock;
#endif
	struct mspi_xfer xfer;
	struct mspi_xfer_packet packet;
};

static struct sqspi_data sqspi_data;

static inline void set_up_xfer(enum mspi_xfer_direction dir, uint32_t addr,
				size_t size, void *data_buf)
{
	memset(&sqspi_data.xfer, 0, sizeof(sqspi_data.xfer));
	memset(&sqspi_data.packet, 0, sizeof(sqspi_data.packet));

	sqspi_data.xfer.xfer_mode = MSPI_PIO;
	sqspi_data.xfer.packets = &sqspi_data.packet;
	sqspi_data.xfer.num_packet = 1;
	sqspi_data.xfer.timeout = 1000;
	sqspi_data.xfer.tx_dummy = sqspi_dev_cfg.tx_dummy;
	sqspi_data.xfer.rx_dummy = sqspi_dev_cfg.rx_dummy;
	sqspi_data.xfer.cmd_length = sqspi_dev_cfg.cmd_length;
	sqspi_data.xfer.addr_length = sqspi_dev_cfg.addr_length;

	sqspi_data.packet.dir = dir;
	sqspi_data.packet.cmd = (dir == MSPI_RX) ? sqspi_dev_cfg.read_cmd :
				 sqspi_dev_cfg.write_cmd;
	sqspi_data.packet.address = addr;
	sqspi_data.packet.num_bytes = size;
	sqspi_data.packet.data_buf = data_buf;
}

static int sqspi_read_internal(unsigned int addr, void *dest, size_t size)
{
	int rc;

	if (!dest || size == 0) {
		return -EINVAL;
	}

#ifdef CONFIG_MULTITHREADING
	k_sem_take(&sqspi_data.lock, K_FOREVER);
#endif

	rc = mspi_dev_config(sqspi_bus, &sqspi_dev_id,
			     MSPI_DEVICE_CONFIG_NONE, &sqspi_dev_cfg);
	if (rc < 0) {
		LOG_ERR("mspi_dev_config() failed: %d", rc);
#ifdef CONFIG_MULTITHREADING
		k_sem_give(&sqspi_data.lock);
#endif
		return rc;
	}

	set_up_xfer(MSPI_RX, addr, size, dest);
	rc = mspi_transceive(sqspi_bus, &sqspi_dev_id, &sqspi_data.xfer);
	if (rc < 0) {
		LOG_ERR("mspi_transceive() failed: %d", rc);
	}

	(void)mspi_get_channel_status(sqspi_bus, 0);

#ifdef CONFIG_MULTITHREADING
	k_sem_give(&sqspi_data.lock);
#endif

	return rc;
}

static int sqspi_write_internal(unsigned int addr, const void *src, size_t size)
{
	int rc;

	if (!src || size == 0) {
		return -EINVAL;
	}

	/* address must be 4-byte aligned */
	if ((addr % 4U) != 0) {
		return -EINVAL;
	}

	/* write size must be less than 4, or a multiple of 4 */
	if ((size > 4) && ((size % 4U) != 0)) {
		return -EINVAL;
	}

#ifdef CONFIG_MULTITHREADING
	k_sem_take(&sqspi_data.lock, K_FOREVER);
#endif

	rc = mspi_dev_config(sqspi_bus, &sqspi_dev_id,
			     MSPI_DEVICE_CONFIG_NONE, &sqspi_dev_cfg);
	if (rc < 0) {
		LOG_ERR("mspi_dev_config() failed: %d", rc);
#ifdef CONFIG_MULTITHREADING
		k_sem_give(&sqspi_data.lock);
#endif
		return rc;
	}

	set_up_xfer(MSPI_TX, addr, size, (void *)src);
	rc = mspi_transceive(sqspi_bus, &sqspi_dev_id, &sqspi_data.xfer);
	if (rc < 0) {
		LOG_ERR("mspi_transceive() failed: %d", rc);
	}

	(void)mspi_get_channel_status(sqspi_bus, 0);

#ifdef CONFIG_MULTITHREADING
	k_sem_give(&sqspi_data.lock);
#endif

	return rc;
}

static int sqspi_send_cmd(uint8_t op_code, const void *tx_buf, size_t tx_len,
			  void *rx_buf, size_t rx_len)
{
	int rc;

#ifdef CONFIG_MULTITHREADING
	k_sem_take(&sqspi_data.lock, K_FOREVER);
#endif

	rc = mspi_dev_config(sqspi_bus, &sqspi_dev_id,
			     MSPI_DEVICE_CONFIG_NONE, &sqspi_dev_cfg);
	if (rc < 0) {
		LOG_ERR("mspi_dev_config() failed: %d", rc);
#ifdef CONFIG_MULTITHREADING
		k_sem_give(&sqspi_data.lock);
#endif
		return rc;
	}

	memset(&sqspi_data.xfer, 0, sizeof(sqspi_data.xfer));
	memset(&sqspi_data.packet, 0, sizeof(sqspi_data.packet));

	sqspi_data.xfer.xfer_mode = MSPI_PIO;
	sqspi_data.xfer.packets = &sqspi_data.packet;
	sqspi_data.xfer.num_packet = 1;
	sqspi_data.xfer.timeout = 1000;
	sqspi_data.xfer.cmd_length = 1;
	sqspi_data.xfer.addr_length = 0;

	if (tx_len > 0) {
		sqspi_data.packet.dir = MSPI_TX;
		sqspi_data.packet.cmd = op_code;
		sqspi_data.packet.num_bytes = tx_len;
		sqspi_data.packet.data_buf = (void *)tx_buf;
	} else if (rx_len > 0) {
		sqspi_data.packet.dir = MSPI_RX;
		sqspi_data.packet.cmd = op_code;
		sqspi_data.packet.num_bytes = rx_len;
		sqspi_data.packet.data_buf = rx_buf;
	} else {
		sqspi_data.packet.dir = MSPI_TX;
		sqspi_data.packet.cmd = op_code;
		sqspi_data.packet.num_bytes = 0;
		sqspi_data.packet.data_buf = NULL;
	}

	rc = mspi_transceive(sqspi_bus, &sqspi_dev_id, &sqspi_data.xfer);
	if (rc < 0) {
		LOG_ERR("mspi_transceive() failed: %d", rc);
	}

	(void)mspi_get_channel_status(sqspi_bus, 0);

#ifdef CONFIG_MULTITHREADING
	k_sem_give(&sqspi_data.lock);
#endif

	return rc;
}

int sqspi_RDSR1(const struct device *dev, uint8_t *rdsr1)
{
	uint8_t sr = 0;
	int ret;

	ARG_UNUSED(dev);

	ret = sqspi_send_cmd(0x1f, NULL, 0, &sr, sizeof(sr));
	if (ret == 0) {
		*rdsr1 = sr;
		LOG_DBG("RDSR1 = 0x%x", sr);
	}

	return ret;
}

int sqspi_RDSR2(const struct device *dev, uint8_t *rdsr2)
{
	uint8_t sr = 0;
	int ret;

	ARG_UNUSED(dev);

	ret = sqspi_send_cmd(0x2f, NULL, 0, &sr, sizeof(sr));
	if (ret == 0) {
		*rdsr2 = sr;
		LOG_DBG("RDSR2 = 0x%x", sr);
	}

	return ret;
}

int sqspi_WRSR2(const struct device *dev, uint8_t data)
{
	int ret;

	ARG_UNUSED(dev);

	ret = sqspi_send_cmd(0x3f, &data, sizeof(data), NULL, 0);
	if (ret < 0) {
		LOG_ERR("WRSR2 failed: %d", ret);
	}

	return ret;
}

int sqspi_validate_rpu_wake_writecmd(const struct device *dev)
{
	int ret;
	uint8_t rdsr2 = 0;

	for (int ii = 0; ii < 1; ii++) {
		ret = sqspi_RDSR2(dev, &rdsr2);
		if (!ret && (rdsr2 & RPU_WAKEUP_NOW)) {
			return 0;
		}
	}

	return -1;
}

int sqspi_wait_while_rpu_awake(const struct device *dev)
{
	int ret;
	uint8_t val = 0;

	for (int ii = 0; ii < 10; ii++) {
		ret = sqspi_RDSR1(dev, &val);

		LOG_DBG("RDSR1 = 0x%x", val);

		if (!ret && (val & RPU_AWAKE_BIT)) {
			return val;
		}

		k_msleep(1);
	}

	LOG_ERR("RPU is not awake even after 10ms");
	return -1;
}

int sqspi_cmd_wakeup_rpu(const struct device *dev, uint8_t data)
{
	return sqspi_WRSR2(dev, data);
}

int sqspi_cmd_sleep_rpu(const struct device *dev)
{
	return sqspi_WRSR2(dev, 0x0);
}

int sqspi_read_reg(const struct device *dev, uint8_t reg_addr, uint8_t *reg_value)
{
	uint8_t sr = 0;
	int ret;

	ARG_UNUSED(dev);

	ret = sqspi_send_cmd(reg_addr, NULL, 0, &sr, sizeof(sr));
	if (ret == 0) {
		*reg_value = sr;
		LOG_DBG("sQSPI read reg 0x%02x = 0x%02x", reg_addr, sr);
	}

	return ret;
}

int sqspi_write_reg(const struct device *dev, uint8_t reg_addr, uint8_t reg_value)
{
	int ret;

	ARG_UNUSED(dev);

	ret = sqspi_send_cmd(reg_addr, &reg_value, sizeof(reg_value), NULL, 0);
	if (ret < 0) {
		LOG_ERR("sQSPI write reg 0x%02x failed: %d", reg_addr, ret);
	} else {
		LOG_DBG("sQSPI write reg 0x%02x = 0x%02x", reg_addr, reg_value);
	}

	return ret;
}

struct device sqspi_perip = {
	.data = NULL,
};

int sqspi_deinit(void)
{
	return 0;
}

int sqspi_init(struct qspi_config *config)
{
	sqspi_cfg = config;

	if (!device_is_ready(sqspi_bus)) {
		LOG_ERR("sQSPI bus device not ready");
		return -ENODEV;
	}

#ifdef CONFIG_MULTITHREADING
	k_sem_init(&sqspi_data.lock, 1, 1);
#endif

	k_sem_init(&sqspi_cfg->lock, 1, 1);

	return 0;
}

int sqspi_write(unsigned int addr, const void *data, int len)
{
	int status;

	addr |= sqspi_cfg->addrmask;

	k_sem_take(&sqspi_cfg->lock, K_FOREVER);

	status = sqspi_write_internal(addr, data, len);

	k_sem_give(&sqspi_cfg->lock);

	return status;
}

int sqspi_read(unsigned int addr, void *data, int len)
{
	int status;

	addr |= sqspi_cfg->addrmask;

	k_sem_take(&sqspi_cfg->lock, K_FOREVER);

	status = sqspi_read_internal(addr, data, len);

	k_sem_give(&sqspi_cfg->lock);

	return status;
}

int sqspi_hl_readw(unsigned int addr, void *data)
{
	int status;
	uint32_t len = 4;
	uint8_t rxb[4 + (NRF_WIFI_QSPI_SLAVE_MAX_LATENCY * 4)];

	len += (4 * sqspi_cfg->qspi_slave_latency);

	if (len > sizeof(rxb)) {
		LOG_ERR("%s: len exceeded, check NRF_WIFI_QSPI_SLAVE_MAX_LATENCY (len=%u, rxb=%zu)",
			__func__, (unsigned int)len, sizeof(rxb));
		return -ENOMEM;
	}

	memset(rxb, 0, len);

	k_sem_take(&sqspi_cfg->lock, K_FOREVER);

	status = sqspi_read_internal(addr, rxb, len);

	k_sem_give(&sqspi_cfg->lock);

	*(uint32_t *)data = *(uint32_t *)(rxb + (len - 4));

	return status;
}

int sqspi_hl_read(unsigned int addr, void *data, int len)
{
	int count = 0;

	while (count < (len / 4)) {
		sqspi_hl_readw(addr + (4 * count), ((char *)data + (4 * count)));
		count++;
	}

	return 0;
}
