## Socket Test
Sample code for the wireless node integrating RedNodeBus + OpenThread stack with a UDP client socket.

This sample has been developed to be used with the following boards:

### decawave_dwm1001_dev board
```
west build -p -b decawave_dwm1001_dev samples/socket_test -- -DOVERLAY_CONFIG="overlay-ot-rnb.conf"
```
```
nrfjprog -e
west flash
```

### insightsip_isp3010_dev board
```
west build -p -b insightsip_isp3010_dev samples/socket_test -- -DOVERLAY_CONFIG="overlay-ot-rnb.conf"
```
```
nrfjprog -e
west flash
```

### nrf52840dk_nrf52840 board
```
west build -p -b nrf52840dk_nrf52840 samples/socket_test -- -DOVERLAY_CONFIG="overlay-ot-rnb.conf"
```
```
nrfjprog -e
west flash
```

### nrf52840dongle_nrf52840 board
```
west build -p -b nrf52840dongle_nrf52840 samples/socket_test -- -DOVERLAY_CONFIG="overlay-ot-rnb.conf"
```

### Testing the Socket Test
To test the echo client, the `socket_test.py` file located in the `script` folder can be used.

Using the `socket_test.py` script in the same machine running the RNB OTBR docker (or one reachable through the IP network), the echo service can be tested.

The IPv4 address (converted to an IPv6 equivalent) of the machine running the `socket_test.py` can be specified in the `common.h` file:

For example, if the server runs in the following local IP (default Docker IP):
```
172.17.0.1
```
First, we need to take the last 32 bits from the conversion to IPv6 using an [IPv4 to IPv6 converter](https://iplocation.io/ipv4-to-ipv6/):
```
ac11:0001
```
Then, the prefix `2001:db8:1:ffff::` must be added, resulting in the following IP:
```
2001:0db8:0001:ffff:0000:0000:ac11:0001
```

Finally, we specify it in the corresponding define in `common.h`:
```
#define CONFIG_NET_CONFIG_PEER_IPV6_ADDR "2001:0db8:0001:ffff:0000:0000:ac11:0001"
```

### Accelerometer Configuration
It is possible to configure the accelerometer with differents features. To configure, add the necesary in the file `prj.conf`. The accelerometer works if the board acts as a TAG.

#### Resolution
The default resolution is 10 bits, all the possible resolutions are: 

High resolution (12 bit)
```
CONFIG_LIS2DH_OPER_MODE_HIGH_RES=y
```
Normal (10 bit)
```
CONFIG_LIS2DH_OPER_MODE_NORMAL=y
```
Low power operation (8 bit)
```
CONFIG_LIS2DH_OPER_MODE_LOW_POWER=n=y
```

#### Operating modes 
You can choose between two types of operation, default or trigger mode.

##### Default Mode
The default sample works with a sampling frecuency of 0.5 Hz. The following option will have to be disabled:
```
CONFIG_LIS2DH_TRIGGER_GLOBAL_THREAD=n
```

##### Trigger Mode
To set a different output data rate frequency, you have to specify the type of triggering to be used by the driver. The following options will have to be activated:
```
CONFIG_LIS2DH_TRIGGER_GLOBAL_THREAD=y
```
Then you can choose between the different frequencies.
```
CONFIG_LIS2DH_ODR_X=y
```
> X are the different possible frequency values [1 9]. The available sampling frequencies can be viewed at  `zephyr/drivers/sensor/lis2dh/Kconfig`


