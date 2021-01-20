MEMFAULT_SDK_ROOT := ../../..

COMPONENT_SRCDIRS += \
  $(MEMFAULT_SDK_ROOT)/ports/freertos/src

COMPONENT_ADD_INCLUDEDIRS += \
  $(MEMFAULT_SDK_ROOT)/ports/freertos/include

# By default, the ESP8266 SDK will compile all sources in a directory so
# we explicitly list the .o here because we only want to compile one file
COMPONENT_OBJS := \
  $(MEMFAULT_SDK_ROOT)/ports/freertos/src/memfault_freertos_ram_regions.o
