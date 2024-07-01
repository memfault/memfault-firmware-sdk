# Use these environment variables to customize the build
MEMFAULT_PROJECT_KEY ?=
MEMFAULT_DEVICE_SERIAL ?= TESTSERIAL
MEMFAULT_HARDWARE_VERSION ?= mp
MEMFAULT_SOFTWARE_TYPE ?= test-software
MEMFAULT_SOFTWARE_VERSION ?= 0.9.0

CFLAGS += \
  -DMEMFAULT_PROJECT_KEY=\"$(MEMFAULT_PROJECT_KEY)\"

# Enable sanitizers
CFLAGS += \
  -fsanitize=leak
  -fsanitize=address \
  -fsanitize=undefined \
  -fno-sanitize-recover=all

# Common warnings
CFLAGS += \
  -Wall \
  -Werror \
  -Wformat-signedness \
  -Wshadow \

# Static analysis
CFLAGS += \
  -fanalyzer

# Need curl and  openssl libs
CFLAGS += \
  -lcurl -lcrypto -lssl

.PHONY: all
all: build/post-chunks build/get-latest

build:
	mkdir -p $@

build/post-chunks: post_chunks.c | build
	$(CC) $(CFLAGS) $^ -o $@

build/get-latest: get_latest.c | build
	$(CC) $(CFLAGS) $^ -o $@

.PHONY: test
test: build/post-chunks build/get-latest
	./build/post-chunks
	./build/get-latest $(MEMFAULT_DEVICE_SERIAL) $(MEMFAULT_HARDWARE_VERSION) $(MEMFAULT_SOFTWARE_TYPE) $(MEMFAULT_SOFTWARE_VERSION)

.PHONY: clean
clean:
	rm -rf build
