/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <logging/log.h>
#include <net/ieee802154_radio.h>

#ifdef CONFIG_REDNODEBUS
#include "rnb_leds.h"
#endif /* CONFIG_REDNODEBUS */

LOG_MODULE_REGISTER(coprocessor_sample, CONFIG_OT_COPROCESSOR_LOG_LEVEL);

#ifdef CONFIG_REDNODEBUS
/* Convenience defines for RADIO */
#define REDNODEBUS_API(dev) ((const struct ieee802154_radio_api *const)(dev)->api)


static void handle_rnb_user_rxtx_signal(const struct device *dev,
					enum rednodebus_user_rxtx_signal sig,
					bool active);


static void handle_rnb_user_rxtx_signal(const struct device *dev,
					enum rednodebus_user_rxtx_signal sig,
					bool active)
{
	switch (sig)
	{
	case REDNODEBUS_USER_RX_SIGNAL:
		rnb_leds_set_tx(active);
		break;
	case REDNODEBUS_USER_TX_SIGNAL:
		rnb_leds_set_rx(active);
		break;
	default:
		break;
	}
}
#endif /* CONFIG_REDNODEBUS */

void main(void)
{
#ifdef CONFIG_REDNODEBUS
	int ret;

	ret = rnb_leds_init();

	if (ret != 0)
	{
		LOG_WRN("Cannot init RedNodeBus LEDs");
	}

	const struct device *dev = device_get_binding(CONFIG_IEEE802154_NRF5_DRV_NAME);
	struct ieee802154_config ieee802154_config;

	ieee802154_config.rnb_user_rxtx_signal_handler = handle_rnb_user_rxtx_signal;
	REDNODEBUS_API(dev)->configure(dev, REDNODEBUS_CONFIG_USER_RXTX_SIGNAL_HANDLER, &ieee802154_config);
#endif /* CONFIG_REDNODEBUS */
}
