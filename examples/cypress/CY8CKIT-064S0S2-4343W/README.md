# Memfault for Cypress CY8CKIT-064S0S2-4343W kit with Amazon-FreeRTOS

This folder contains an integration example for the Cypress
[CY8CKIT-064S0S2-4343W](https://www.cypress.com/documentation/development-kitsboards/psoc-64-standard-secure-aws-wi-fi-bt-pioneer-kit-cy8ckit)
kit with Amazon-FreeRTOS & AWS IoT/Lambda.

It is an app with a simple console interface that allows you to generate a
fault, heartbeat or trace and once data is available a secure connection is
estabilished with the AWS IoT MQTT broker, all available chunks are published,
then passed from AWS IoT to Lambda using an IoT rule and eventually posted to
Memfault.

The demo app is tested on a
[CY8CKIT-064S0S2-4343W](https://www.cypress.com/documentation/development-kitsboards/psoc-64-standard-secure-aws-wi-fi-bt-pioneer-kit-cy8ckit)
evaluation board with ModusToolbox 2.3.

## Getting Started

- [awscli](https://docs.aws.amazon.com/cli/latest/userguide/install-cliv2.html)
  is required to perform some of the steps in this document.
- ModusToolbox setup instructions can be found
  [here](https://docs.aws.amazon.com/freertos/latest/userguide/getting_started_cypress_psoc64.html).
- You'll need an AWS account with an IAM user. How to setup an IAM user can be
  found
  [here](https://docs.aws.amazon.com/IAM/latest/UserGuide/id_users_create.html).
- The board, IAM user and AWS IoT should be configured according to this
  [document](https://docs.aws.amazon.com/freertos/latest/userguide/freertos-prereqs.html).

## Provisioning, certificates, keys & policies

### Provision the board

In order to be able to do anything with your device it should be provisioned for
secure boot as in the [document](https://community.cypress.com/docs/DOC-20043),
with one difference - use `policy_multi_CM0_CM4_jitp.json`, as this is the one
actually used by the project to generate a firmware image. If you'll specify the
default tfm policy, the board won't be able to boot.

Here's an example of the provisioning steps:

```bash
# install the cysecuretools python tools
❯ pip install --update cysecuretools
# change to the directory in the amazon-freertos tree where we run the provisioning
❯ cd vendors/cypress/MTB/psoc6/psoc64tfm/security
# for our provisioning, we need to copy the device + root certs to specific locations
❯ mkdir -p certificates
❯ cp <path to where you downloaded the certs above>/<device id>-certificate.pem.crt certificates/device_cert.pem
❯ cp <path to where you downloaded the certs above>/AmazonRootCA1.pem certificates/rootCA.pem
# init and provision the device (use 're-provision-device' on a previously provisioned board)
❯ cysecuretools --target CY8CKIT-064S0S2-4343W init
❯ cysecuretools --policy ./policy/policy_multi_CM0_CM4_jitp.json --target CY8CKIT-064S0S2-4343W provision-device
```

### Configure the application

In `demos/include/aws_clientcredential.h`, you'll have to type in:

- `clientcredentialMQTT_BROKER_ENDPOINT` - MQTT broker endpoint retrieved from
  `awscli` in the
  [first steps in FreeRTOS document](https://docs.aws.amazon.com/freertos/latest/userguide/freertos-prereqs.html).
- `clientcredentialIOT_THING_NAME` - AWS IoT Thing name.
- `clientcredentialMEMFAULT_PROJECT_KEY` - Memfault project key, find it in the
  [Memfault dashboard](https://app.memfault.com) under "Settings"
- `clientcredentialWIFI_SSID` - Wifi SSID string.
- `clientcredentialWIFI_PASSWORD` - Wifi password.
- `clientcredentialWIFI_SECURITY` - Wifi security.

You'll also have to generate the header file with your AWS IoT Thing certificate
& private key. To do that, you'll have to pass the files generated during Thing
register process to
`tools/certificate_configuration/CertificateConfigurator.html` and replace
`demos/include/aws_clientcredential_keys.h` with the resulting file.

## Adding the Sources

Clone the amazon-freertos repository, and add the sources + patch included in
this example:

```bash
# clone the amazon-freertos repo at a known-working tag
 git clone  git@github.com:aws/amazon-freertos.git -b 202107.00
❯ cd amazon-freertos

# add the memfault sdk- you can clone it directly, as below;
# or add it as a submodule to the freertos repository;
# or symlink from another location, per your preference
❯ git clone https://github.com/memfault/memfault-firmware-sdk.git \
  libraries/3rdparty/memfault-firmware-sdk/

# apply the patch to integrate this example with amazon-freertos
❯ git apply \
  libraries/3rdparty/memfault-firmware-sdk/examples/cypress/CY8CKIT-064S0S2-4343W/amazon-freertos.patch
```

Once the below steps are done, to build, run `make build` in the example
directory:

```bash
❯ make -C \
  libraries/3rdparty/memfault-firmware-sdk/examples/cypress/CY8CKIT-064S0S2-4343W/ build
```

To program the board, use `make program` (it might be necessary to press the
"MODE SELECT" (SW3) button to switch the board to programming mode).

## Setting up AWS IoT & Lambda

In order to be able to use Lambda with IoT, the user has to
[have permission to create roles and attach role policies](https://docs.aws.amazon.com/IAM/latest/UserGuide/id_roles_create_for-service.html).
On top of that, we want two more actions to be available for the IAM user:
`lambda:AddPermission` and `iam:CreatePolicy`. You can do it either using
`awscli` or the IAM Management Console GUI - either way you'll have to attach
the appropriate permissions policy to the IAM entity you're using. Next, we need
to setup an
[IoT trust role and policy](https://docs.aws.amazon.com/iot/latest/developerguide/iot-create-role.html).

Once that's done, you can use the same configuration here (this example
publishes to the same MQTT topic as the Pinnacle 100 example):

> https://mflt.io/aws-iot-mqtt-cloudformation-proxy

## Memfault port

Port stores coredumps, which include stack, data & bss sections, in flash memory
between `__MfltCoredumpsStart` and `__MfltCoredumpsEnd` symbols. Demo
implementation provides a 512-byte aligned 128kB region at the end of CM4 flash
memory region, just behind the CM4 application signature.

To uniquely identify your firmware type, version & device in your Memfault
dashboard, please also make sure to edit `memfault_platform_port.c` and fill all
the related fields in `memfault_platform_get_device_info`.

## Building, flashing & debugging

Build the application using `Build aws_demos Application`, or do it directly
from shell in the project directory by using `make build`.

Analogously to flash the device either use `aws_demos Program (KitProg3)` from
ModusToolbox, or `make program` from shell. In order for it to work, please make
sure that the KitProg connection is in CMSIS-DAP mode.

You can use the gdbserver/client integrated into ModusToolbox to debug the
application by using `aws_demos Debug (KitProg3)` or `make debug` from shell.

## Application details

Main function is in
`vendors/cypress/boards/CY8CKIT_064S0S2_4343W/aws_demos/application_code/main.c`.
MQTT communication for the application is implemented in
`mqtt_memfault_project/demo/memfault/mqtt_demo_memfault.c`.

Demo overview:

- First the user is asked whether a fault, trace or heartbeat event should be
  generated or the application should continue.
- TCP connection to AWS IoT is estabilished.
- `CONNECT` to MQTT broker is sent.
- All data chunks if available are retrieved and published over MQTT on the
  `prod/+/+/memfault/chunks` topic.
- AWS IoT retrieves the chunks and passes them to the Lambda function.
- Lambda function forwards the chunks to Memfault.

## Serial console via UART

There should be two `/dev/ttyACM` devices available when the board is connected
using the KitProg USB - one of them is the console. Connect to it using e.g.
`screen`. Serial parameters are:

```
Baud rate: 115200
Data: 8 bit
Parity: None
Stop bits: 1
Flow control: None
```

## Uploading symbol file

The demo app's symbol file is located here:

```
build/cy/CY8CKIT-064S0S2-4343W/CY8CKIT_064S0S2_4343W/Debug/aws_demos.elf
```

Upload the symbol file either with the web UI or
[using the cli](https://mflt.io/symbol-file-build-ids/).

## Troubleshooting

### TLS error

If you get a TLS error when trying to estabilish a connection to the IoT MQTT
Broker saying that your certificate/key is incorrect, you might need to use a
different IoT endpoint - try replacing the default address with:
`aws iot describe-endpoint --endpoint-type iot:Data-ATS`.

## Related documents

- [PSoC® 64 Secure MCU Secure Boot SDK User Guide](https://www.cypress.com/documentation/software-and-drivers/psoc-64-secure-mcu-secure-boot-sdk-user-guide)
- [Provisioning Guide for the Cypress CY8CKIT-064S0S2-4343W Kit](https://community.cypress.com/t5/Resource-Library/Provisioning-Guide-for-the-Cypress-CY8CKIT-064S0S2-4343W-Kit/ta-p/252469)
- [PSoC 64 Standard Secure – AWS Wi-Fi BT Pioneer Kit Guide](https://www.cypress.com/file/509676/download)
