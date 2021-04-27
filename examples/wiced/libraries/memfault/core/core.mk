NAME := MemfaultCore

$(NAME)_SOURCES := \
  src/arch_arm_cortex_m.c \
  src/memfault_build_id.c \
  src/memfault_core_utils.c \
  src/memfault_data_packetizer.c \
  src/memfault_event_storage.c \
  src/memfault_log.c \
  src/memfault_log_data_source.c \
  src/memfault_ram_reboot_info_tracking.c \
  src/memfault_sdk_assert.c \
  src/memfault_serializer_helper.c \

$(NAME)_COMPONENTS :=

$(NAME)_INCLUDES += include

GLOBAL_INCLUDES += include

VALID_OSNS_COMBOS := ThreadX-NetX_Duo FreeRTOS-LwIP

VALID_PLATFORMS := \
  BCM943362WCD4 \
  BCM943362WCD6 \
  BCM943362WCD8 \
  BCM943364WCD1 \
  CYW94343WWCD1_EVB \
  BCM943438WCD1 \
  BCM94343WWCD2 \
  CY8CKIT_062 \
  NEB1DX* \
  CYW9MCU7X9N364 \
  CYW943907AEVAL1F \
  CYW954907AEVAL1F \
  CYW9WCD2REFAD2* \
  CYW9WCD760PINSDAD2 \
  CYW943455EVB* \
  CYW943012EVB*
