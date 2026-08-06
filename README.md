Flightradar based on ESP32-S3 N16R8 and ILI9488IPS 480x320 SPI display, using two touch sensors.
It uses [ADSB.lol](https://adsb.lol/) data, no subscription required.

Based on [micro-radar](https://github.com/AnthonySturdy/micro-radar/tree/main) from Anthony Sturdy.

To set wifi credentials please start the device with any sensor touched.

Please use GPIO04 and GPIO05 for touch sensors. Most likely you will need to experimentally set the TOUCH_THRESHOLD parameter.

Configuration available via ESP web server. 
