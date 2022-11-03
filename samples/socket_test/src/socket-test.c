/* socket-test.c - Networking UDP socket client */

/*
 * Copyright (c) 2022 RedNodeLabs UG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The socket-test application is acting as a client that is run in Zephyr OS,
 * and the server is run in the host acting as a server.
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_socket_test_sample, LOG_LEVEL_DBG);

#include <zephyr.h>
#include <errno.h>
#include <stdio.h>

#include <syscalls/rand32.h>
#include <net/socket.h>
#include <net/net_mgmt.h>
#include <net/net_event.h>
#include <net/net_conn_mgr.h>
#include <net/ieee802154_radio.h>

#include <device.h>
#include <drivers/sensor.h>

#if defined(CONFIG_REDNODEBUS)
#include "rnb_utils.h"
#endif

#include "common.h"

#define INVALID_SOCK (-1)

#define EVENT_MASK (NET_EVENT_L4_CONNECTED | \
		    NET_EVENT_L4_DISCONNECTED)

#define MTU_SIZE 1280

static uint8_t rnb_role;

void rnb_utils_handle_new_state(const struct rednodebus_user_event_state *event_state)
{
	rnb_role = event_state->role;
}

int32_t lorem_ipsum[10];
const int ipsum_len = sizeof(lorem_ipsum);

const struct device *sensor;

struct configs conf = {
	.ipv6 = {
		.proto = "IPv6",
		.udp.sock = INVALID_SOCK
	},
};

static void init_app(void)
{
	conf.ipv6.udp.mtu = MTU_SIZE;

#if defined(CONFIG_REDNODEBUS)
	uint64_t euid = 0;
	rnb_utils_get_euid(&euid);

	lorem_ipsum[0] = (uint32_t)euid;
	lorem_ipsum[1] = (uint32_t)(euid >> 32);

	//sprintf(lorem_ipsum, "%04X%04X", (uint32_t)(euid >> 32), (uint32_t)euid);

	int session_rand = sys_rand32_get();
	lorem_ipsum[2] = session_rand;
#endif
}

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
	// Data accelerometer to lorem_ipsum
	lorem_ipsum[4] = accel[0].val1;
	lorem_ipsum[5] = accel[0].val2;
	lorem_ipsum[6] = accel[1].val1;
	lorem_ipsum[7] = accel[1].val2;
	lorem_ipsum[8] = accel[2].val1;
	lorem_ipsum[9] = accel[2].val2;

	if (rc < 0) {
		LOG_INF("ERROR: Update failed: %d\n", rc);
	} else {
		LOG_INF("#%u EUID 0x%04X%04X @ %u ms: %sx: %06d.%06d  , y: %06d.%06d , z: %06d.%06d\n",
			   count, lorem_ipsum[0], lorem_ipsum[1], k_uptime_get_32(), overrun,
			   lorem_ipsum[4],lorem_ipsum[5],
			   lorem_ipsum[6],lorem_ipsum[7],
			   lorem_ipsum[8],lorem_ipsum[9]);
	}
}

#ifdef CONFIG_LIS2DH_TRIGGER
static void trigger_handler(const struct device *dev,
			    const struct sensor_trigger *trig)
{
	fetch_and_display(dev);
}
#endif

static void accelerometer(){

	sensor = DEVICE_DT_GET_ANY(st_lis2dh);

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
#endif /* CONFIG_LIS2DH_TRIGGER */

}

static int start_client(void)
{
	int iterations = CONFIG_NET_SAMPLE_SEND_ITERATIONS;
	int i = 0;
	int ret;

	while (iterations == 0 || i < iterations) {
		ret = start_udp();

		while (ret == 0) {
			accelerometer();
			fetch_and_display(sensor);
			k_sleep(K_MSEC(2000));
			send_udp_data(&conf.ipv6);

			if (iterations > 0) {
				i++;
				if (i >= iterations) {
					break;

				}
			}
			//if (rnb_role == REDNODEBUS_USER_ROLE_TAG)
			//{
			//	k_sleep(K_MSEC(UDP_TRANSMISSION_PERIOD_TAG_MSEC));
			//}
			//else
			//{
			//	k_sleep(K_MSEC(UDP_TRANSMISSION_PERIOD_MSEC));
			//}
		}
		stop_udp();
	}
	return ret;
}


void main(void)
{
#if defined(CONFIG_REDNODEBUS)
	init_rnb();

	while (!is_rnb_connected())
	{
		k_sleep(K_MSEC(1000));
	}

	k_sleep(K_MSEC(1000));
#endif

	init_app();

	k_thread_priority_set(k_current_get(), THREAD_PRIORITY);

	exit(start_client());
}
