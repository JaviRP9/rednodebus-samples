/* main.c - RedNodeBus node */

/*
 * Copyright (c) 2022 RedNodeLabs.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>

#include <stdio.h>
#include <zephyr.h>
#include <device.h>
#include <drivers/sensor.h>
LOG_MODULE_REGISTER(rednodebus_node, LOG_LEVEL_INF);

#if defined(CONFIG_REDNODEBUS)
#include "rnb_utils.h"
#endif

static void fetch_and_display(const struct device *sensor)
{
	static unsigned int count;
	struct sensor_value accel[3];
	const char *overrun = "";
	int rc = sensor_sample_fetch(sensor);

	++count;
	if (rc == -EBADMSG) {
		/* Sample overrun.  Ignore in polled mode. */
		if (IS_ENABLED(CONFIG_LIS2DH_TRIGGER)) {
			overrun = "[OVERRUN] ";
		}
		rc = 0;
	}
	if (rc == 0) {
		rc = sensor_channel_get(sensor,
					SENSOR_CHAN_ACCEL_XYZ,
					accel);

	}
	if (rc < 0) {
		LOG_INF("ERROR: Update failed: %d\n", rc);
	} else {
		LOG_INF("#%u @ %u ms: %sx: %d.%d  , y: %d.%d , z: %d.%d\n",
			   count, k_uptime_get_32(), overrun,
			   accel[0].val1,accel[0].val2,
		       accel[1].val1,accel[1].val2,
			   accel[2].val1,accel[2].val2);
	}
}

#ifdef CONFIG_LIS2DH_TRIGGER
static void trigger_handler(const struct device *dev,
			    const struct sensor_trigger *trig)
{
	fetch_and_display(dev);
}
#endif

void main(void)
{
    LOG_INF("RedNodeBus node sample");

#if defined(CONFIG_REDNODEBUS)
    init_rnb();
#endif

	const struct device *sensor = DEVICE_DT_GET_ANY(st_lis2dh);

	if (sensor == NULL) {
		LOG_INF("No device found\n");
		return;
	}
	if (!device_is_ready(sensor)) {
		LOG_INF("Device %s is not ready\n", sensor->name);
		return;
	}

#if CONFIG_LIS2DH_TRIGGER
	{
		struct sensor_trigger trig;
		int rc;

		trig.type = SENSOR_TRIG_DATA_READY;
		trig.chan = SENSOR_CHAN_ACCEL_XYZ;

		if (IS_ENABLED(CONFIG_LIS2DH_ODR_RUNTIME)) {
			struct sensor_value odr = {
				.val1 = 1,
			};

			rc = sensor_attr_set(sensor, trig.chan,
					     SENSOR_ATTR_SAMPLING_FREQUENCY,
					     &odr);
			if (rc != 0) {
				LOG_INF("Failed to set odr: %d\n", rc);
				return;
			}
			LOG_INF("Sampling at %u Hz\n", odr.val1);
		}

		rc = sensor_trigger_set(sensor, &trig, trigger_handler);
		if (rc != 0) {
			LOG_INF("Failed to set trigger: %d\n", rc);
			return;
		}

		LOG_INF("Waiting for triggers\n");
		while (true) {
			k_sleep(K_MSEC(2000));
		}
	}
#else /* CONFIG_LIS2DH_TRIGGER */
	LOG_INF("Polling at 0.5 Hz\n");
	while (true) {
		fetch_and_display(sensor);
		k_sleep(K_MSEC(2000));
	}
#endif /* CONFIG_LIS2DH_TRIGGER */
}
