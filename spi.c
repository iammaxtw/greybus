/*
 * SPI bridge PHY driver.
 *
 * Copyright 2014-2016 Google Inc.
 * Copyright 2014-2016 Linaro Ltd.
 *
 * Released under the GPLv2 only.
 */

#include <linux/module.h>

#include "greybus.h"
#include "gbphy.h"
#include "spilib.h"

#ifndef SPI_CORE_SUPPORT_PM
static int gbphy_spi_prepare_transfer_hardware(struct device *dev)
{
	return gbphy_runtime_get_sync(to_gbphy_dev(dev));
}

static void gbphy_spi_unprepare_transfer_hardware(struct device *dev)
{
	gbphy_runtime_put_autosuspend(to_gbphy_dev(dev));
}

static struct spilib_ops __spilib_ops = {
	.prepare_transfer_hardware = gbphy_spi_prepare_transfer_hardware,
	.unprepare_transfer_hardware = gbphy_spi_unprepare_transfer_hardware,
};

static struct spilib_ops *spilib_ops = &__spilib_ops;
#else
static struct spilib_ops *spilib_ops = NULL;
#endif

static int gb_spi_probe(struct gbphy_device *gbphy_dev,
			const struct gbphy_device_id *id)
{
	struct gb_connection *connection;
	int ret;

	connection = gb_connection_create(gbphy_dev->bundle,
					  le16_to_cpu(gbphy_dev->cport_desc->id),
					  NULL);
	if (IS_ERR(connection))
		return PTR_ERR(connection);

	ret = gb_connection_enable(connection);
	if (ret)
		goto exit_connection_destroy;

	ret = gb_spilib_master_init(connection, &gbphy_dev->dev, spilib_ops);
	if (ret)
		goto exit_connection_disable;

	gb_gbphy_set_data(gbphy_dev, connection);

	gbphy_runtime_put_autosuspend(gbphy_dev);
	return 0;

exit_connection_disable:
	gb_connection_disable(connection);
exit_connection_destroy:
	gb_connection_destroy(connection);

	return ret;
}

static void gb_spi_remove(struct gbphy_device *gbphy_dev)
{
	struct gb_connection *connection = gb_gbphy_get_data(gbphy_dev);
	int ret;

	ret = gbphy_runtime_get_sync(gbphy_dev);
	if (ret)
		gbphy_runtime_get_noresume(gbphy_dev);

	gb_spilib_master_exit(connection);
	gb_connection_disable(connection);
	gb_connection_destroy(connection);
}

static const struct gbphy_device_id gb_spi_id_table[] = {
	{ GBPHY_PROTOCOL(GREYBUS_PROTOCOL_SPI) },
	{ },
};
MODULE_DEVICE_TABLE(gbphy, gb_spi_id_table);

static struct gbphy_driver spi_driver = {
	.name		= "spi",
	.probe		= gb_spi_probe,
	.remove		= gb_spi_remove,
	.id_table	= gb_spi_id_table,
};

module_gbphy_driver(spi_driver);
MODULE_LICENSE("GPL v2");
