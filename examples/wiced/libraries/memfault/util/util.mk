NAME := MemfaultUtil

$(NAME)_SOURCES := \
  src/memfault_chunk_transport.c \
  src/memfault_crc16_ccitt.c \
  src/memfault_circular_buffer.c \
  src/memfault_minimal_cbor.c \
  src/memfault_varint.c \

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
