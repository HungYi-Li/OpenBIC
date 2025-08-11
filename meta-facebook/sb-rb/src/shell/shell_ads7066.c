#include <zephyr.h>
#include <device.h>
#include <stdlib.h>
#include <drivers/spi.h>
#include <shell/shell.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(ads7066, LOG_LEVEL_INF);

#define ADS7066_SPI_FREQ 6000000

static const struct device *spi_dev;
static struct spi_config spi_cfg = {
	.frequency = ADS7066_SPI_FREQ,
	.operation = SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8) | SPI_LINES_SINGLE,
	.slave = 0,
	.cs = NULL,
};

static int ads7066_read_device_id(uint8_t *data, uint8_t cmd)
{
	spi_dev = device_get_binding("SPIP");
	if (!spi_dev) {
		LOG_ERR("SPI device not find");
	}

	uint8_t tx_buf[3] = { 0x10, cmd, 0x00 }; // bit15=1: read
	uint8_t rx_buf[3] = { 0 };

	struct spi_buf tx = { .buf = tx_buf, .len = sizeof(tx_buf) };
	struct spi_buf rx = { .buf = rx_buf, .len = sizeof(rx_buf) };
	struct spi_buf_set tx_set = { .buffers = &tx, .count = 1 };
	struct spi_buf_set rx_set = { .buffers = &rx, .count = 1 };

	int ret = spi_write(spi_dev, &spi_cfg, &tx_set);
	if (ret < 0) {
		LOG_ERR("SPI write failed: %d", ret);
		return ret;
	}
	ret = spi_read(spi_dev, &spi_cfg, &rx_set);
	if (ret < 0) {
		LOG_ERR("SPI read failed: %d", ret);
		return ret;
	}

	memcpy(data, rx_buf, 3);
	return 0;
}

static int cmd_ads7066_id(const struct shell *shell, size_t argc, char **argv)
{
	uint8_t data[3] = { 0 };
	uint8_t cmd = strtoul(argv[1], NULL, 16);
	int ret = ads7066_read_device_id(data, cmd);
	if (ret < 0) {
		shell_error(shell, "read DEVICE_ID fail (err=%d)", ret);
		return ret;
	}

	shell_hexdump(shell, data, 3);
	return 0;
}

SHELL_CMD_REGISTER(ads7066_id, NULL, "read ADS7066 Device ID", cmd_ads7066_id);
