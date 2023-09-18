# esp32 Demo Application

This Demo App is based on the console example from ESP-IDF, which can be found
here relative to the ESP-IDF SDK root folder:

- `examples/system/console/advanced/`

## Configuring for MQTT

This application includes an option to send Memfault data over MQTT. This option requires a few extra pieces to set up.
You can either follow the steps outlined here or use your own MQTT setup.

### Broker Setup

1. Install a local installtion of Cedalo by following the [installation guide](https://docs.cedalo.com/management-center/installation/)
2. Login to Cedalo at <http://localhost:8088>
3. Create a new client login for the device
   - Ensure device client has the "client" role to allow publishing data
4. Create a new client login for the Python service
   - Ensure Python service client has "client" role to allow subscribing to data

### Service Setup

1. Modify the script found in Docs->Best Practices->MQTT with Memfault with the the following:
   1. The service client login information previously created
   2. Connection info for your local broker
   3. Map of Memfault projects to project keys
2. Start the service by running `python mqtt.py`

### Device Setup

1. Make the following modifications to `main/app_memfault_transport_mqtt.c`:
   1. Update `MEMFAULT_PROJECT` macro with your project's name
   2. Update `s_mqtt_config` with your setup's IP address, and MQTT client username and password
2. Clean your existing build with `idf.py fullclean && rm sdkconfig`
3. Set your target: `idf.py set-target <esp32_platform_name>`
4. Build your image: `idf.py -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.mqtt" build`
5. Flash to your device using `idf.py flash`
