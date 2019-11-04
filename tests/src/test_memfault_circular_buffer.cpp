#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"

extern "C" {
  #include <string.h>
  #include <stddef.h>

  #include "memfault/util/circular_buffer.h"
}

TEST_GROUP(MfltCircularBufferTestGroup) {
  void setup() {
  }

   void teardown() {
   }
};

TEST(MfltCircularBufferTestGroup, Test_MfltCircularBufferInit) {
  bool success = memfault_circular_buffer_init(NULL, NULL, 0);
  CHECK(!success);

  sMfltCircularBuffer buffer;
  success = memfault_circular_buffer_init(&buffer, NULL, 0);
  CHECK(!success);

  uint8_t storage_buf[10];
  success = memfault_circular_buffer_init(&buffer, storage_buf, 0);
  CHECK(!success);

  success = memfault_circular_buffer_init(&buffer, storage_buf, sizeof(storage_buf));
  CHECK(success);

  size_t bytes_to_read = memfault_circular_buffer_get_read_size(&buffer);
  LONGS_EQUAL(0, bytes_to_read);

  bytes_to_read = memfault_circular_buffer_get_read_size(NULL);
  LONGS_EQUAL(0, bytes_to_read);

  size_t space_available = memfault_circular_buffer_get_write_size(&buffer);
  LONGS_EQUAL(sizeof(storage_buf), space_available);

  space_available = memfault_circular_buffer_get_write_size(NULL);
  LONGS_EQUAL(0, space_available);
}

TEST(MfltCircularBufferTestGroup, Test_MfltCircularWriteAtOffsetBasic) {
  const uint8_t buffer_size = 4;
  uint8_t storage_buf[buffer_size];
  sMfltCircularBuffer buffer;
  bool success = memfault_circular_buffer_init(&buffer, storage_buf, sizeof(storage_buf));
  CHECK(success);

  const uint8_t seq1[] = { 0x1, 0x2 };
  success = memfault_circular_buffer_write_at_offset(&buffer, 0, seq1, sizeof(seq1));
  CHECK(success);

  // overwrite first two bytes and fill rest of buffer
  const uint8_t seq2[] = { 0x3, 0x4, 0x5, 0x6 };
  memfault_circular_buffer_write_at_offset(&buffer, 2, seq2, sizeof(seq2));
  CHECK(success);

  // buffer should be full
  uint8_t result[buffer_size];
  memset(result, 0x0, sizeof(result));
  success = memfault_circular_buffer_read(&buffer, 0, &result, sizeof(result));
  CHECK(success);
  MEMCMP_EQUAL(seq2, result, sizeof(result));
}

TEST(MfltCircularBufferTestGroup, Test_MfltCircularWriteAndReadBasic) {
  const uint8_t buffer_size = 16;
  uint8_t storage_buf[buffer_size];
  sMfltCircularBuffer buffer;
  bool success = memfault_circular_buffer_init(&buffer, storage_buf, sizeof(storage_buf));
  CHECK(success);

  size_t bytes_to_read = memfault_circular_buffer_get_read_size(&buffer);
  LONGS_EQUAL(0, bytes_to_read);
  size_t space_available = memfault_circular_buffer_get_write_size(&buffer);
  LONGS_EQUAL(buffer_size, space_available);

  // should be nothing to read yet
  uint8_t bytes_read[buffer_size];
  success = memfault_circular_buffer_read(&buffer, 0, bytes_read, sizeof(bytes_read));
  CHECK(!success);

  uint8_t bytes_to_write[buffer_size - 3];
  for (uint8_t i = 0; i < sizeof(bytes_to_write); i++) {
    bytes_to_write[i] = i;
  }

  // let's write some bytes
  for (int i = 0; i < 2; i++) {
    // can't write to a NULL buffer
    success = memfault_circular_buffer_write(NULL, bytes_to_write, sizeof(bytes_to_write));
    CHECK(!success);
    success = memfault_circular_buffer_write(&buffer, NULL, 0);
    CHECK(!success);

    success = memfault_circular_buffer_write(&buffer, bytes_to_write, sizeof(bytes_to_write));
    // 2nd iteration should fail because we are out of space
    LONGS_EQUAL(i == 0, success);
    bytes_to_read = memfault_circular_buffer_get_read_size(&buffer);
    LONGS_EQUAL(sizeof(bytes_to_write), bytes_to_read);

    space_available = memfault_circular_buffer_get_write_size(&buffer);
    LONGS_EQUAL(buffer_size - sizeof(bytes_to_write), space_available);
  }

  // trying to read too many bytes should fail
  success = memfault_circular_buffer_read(&buffer, 0, bytes_read, sizeof(bytes_read));
  CHECK(!success);

  // an attempt to overwrite past the beginning of the buffer should fail
  success = memfault_circular_buffer_write_at_offset(&buffer, sizeof(bytes_to_write) + 1,
                                                     bytes_to_write, sizeof(bytes_to_write));
  CHECK(!success);


  // trying a read with a NULL buffer should fail
  success = memfault_circular_buffer_read(NULL, 0, bytes_read, sizeof(bytes_read));
  CHECK(!success);
  success = memfault_circular_buffer_read(&buffer, 0, NULL, 0);
  CHECK(!success);

  success = memfault_circular_buffer_read(&buffer, 0, bytes_read, sizeof(bytes_to_write));
  CHECK(success);
  MEMCMP_EQUAL(bytes_to_write, bytes_read, sizeof(bytes_to_write));

  // trying to consume too many bytes should fail
  success = memfault_circular_buffer_consume(&buffer, sizeof(bytes_read));
  CHECK(!success);
  success = memfault_circular_buffer_consume_from_end(&buffer, sizeof(bytes_read));
  CHECK(!success);

  // trying to consume without a buffer should fail
  success = memfault_circular_buffer_consume(NULL, sizeof(bytes_to_write));
  CHECK(!success);
  success = memfault_circular_buffer_consume_from_end(NULL, sizeof(bytes_to_write));
  CHECK(!success);

  success = memfault_circular_buffer_consume(&buffer, sizeof(bytes_to_write));
  CHECK(success);

  // should be nothing left
  bytes_to_read = memfault_circular_buffer_get_read_size(&buffer);
  LONGS_EQUAL(0, bytes_to_read);

  space_available = memfault_circular_buffer_get_write_size(&buffer);
  LONGS_EQUAL(buffer_size, space_available);
}

static void prv_fill_write_buf(uint8_t start_idx, uint8_t *buf, size_t buf_len) {
  for (size_t i = 0; i < buf_len; i++) {
    buf[i] = (uint8_t)(start_idx + i);
  }
}

static void prv_read_buf_and_check(sMfltCircularBuffer *buffer, uint8_t start_idx,
                                   size_t total_len) {
  uint8_t read_buf[total_len];
  bool success;

  // as tiny reads
  memset(read_buf, 0x0, sizeof(read_buf));
  for (uint8_t i = 0; i < sizeof(read_buf); i++) {
    success = memfault_circular_buffer_read(buffer, i, &read_buf[i], 1);
    CHECK(success);
    LONGS_EQUAL((uint8_t)(start_idx + i), read_buf[i]);
  }

  // as one big read
  memset(read_buf, 0x0, sizeof(read_buf));
  success = memfault_circular_buffer_read(buffer, 0, read_buf, sizeof(read_buf));
  CHECK(success);

  for (uint8_t i = 0; i < sizeof(read_buf); i++) {
    LONGS_EQUAL((uint8_t)(start_idx + i), read_buf[i]);
  }

  // we are done, consume the data
  success = memfault_circular_buffer_consume(buffer, total_len);
  CHECK(success);
}

TEST(MfltCircularBufferTestGroup, Test_MfltCircularWriteAndReadWrapAround) {
  const uint8_t buffer_size = 14;
  uint8_t storage_buf[buffer_size];
  sMfltCircularBuffer buffer;
  bool success = memfault_circular_buffer_init(&buffer, storage_buf, sizeof(storage_buf));
  CHECK(success);

  // write 3 bytes at a time & read 9 bytes every 3 iterations
  uint8_t write_buf[3];
  uint8_t expected_val = 0;
  uint8_t curr_write_val = 0;

  for (size_t i = 0; i < 99; i++) {
    prv_fill_write_buf(curr_write_val, write_buf, sizeof(write_buf));
    curr_write_val += 3;

    // write an initial pattern that we will overwrite
    uint8_t init_pattern_buf[sizeof(write_buf)];
    memset(init_pattern_buf, 0xfe, sizeof(init_pattern_buf));
    success = memfault_circular_buffer_write(&buffer, init_pattern_buf,
                                             sizeof(init_pattern_buf));
    CHECK(success);

    // re-write first byte
    success = memfault_circular_buffer_write_at_offset(&buffer, sizeof(write_buf), write_buf, 1);
    CHECK(success);

    // consume one byte
    success = memfault_circular_buffer_consume_from_end(&buffer, 1);
    CHECK(success);

    // re-write remaining
    success = memfault_circular_buffer_write_at_offset(&buffer, sizeof(write_buf) - 2, &write_buf[1], sizeof(write_buf) - 1);
    CHECK(success);

    if ((i == 0) || ((i % 3) != 0)) {
      continue;
    }

    const size_t read_size = 9;
    prv_read_buf_and_check(&buffer, expected_val, read_size);
    expected_val += read_size;
  }
}

TEST(MfltCircularBufferTestGroup, Test_MfltCircularWriteAndGetReadPointerBadInput) {
  const uint8_t buffer_size = 20;
  uint8_t storage_buf[buffer_size];
  sMfltCircularBuffer buffer;
  bool success = memfault_circular_buffer_init(&buffer, storage_buf, sizeof(storage_buf));
  CHECK(success);

  uint8_t data = 0xab;
  success = memfault_circular_buffer_write(&buffer, &data, sizeof(data));
  CHECK(success);

  uint8_t *read_ptr;
  size_t read_ptr_len;
  success = memfault_circular_buffer_get_read_pointer(NULL, 0, &read_ptr, &read_ptr_len);
  CHECK(!success);

  success = memfault_circular_buffer_get_read_pointer(&buffer, 0, NULL, &read_ptr_len);
  CHECK(!success);

  success = memfault_circular_buffer_get_read_pointer(&buffer, 0, &read_ptr, NULL);
  CHECK(!success);

  success = memfault_circular_buffer_get_read_pointer(&buffer, 2, &read_ptr, &read_ptr_len);
  CHECK(!success);
}

TEST(MfltCircularBufferTestGroup, Test_MfltCircularWriteAndGetReadPointer) {
  const uint8_t buffer_size = 20;
  uint8_t storage_buf[buffer_size];
  sMfltCircularBuffer buffer;
  bool success = memfault_circular_buffer_init(&buffer, storage_buf, sizeof(storage_buf));
  CHECK(success);

  // fill the buffer up, our read size should keep increasing by 1
  for (uint8_t i = 0; i < buffer_size; i++) {
    uint8_t data = i;
    success = memfault_circular_buffer_write(&buffer, &data, sizeof(data));
    CHECK(success);

    uint8_t *read_ptr;
    size_t read_ptr_len;
    success = memfault_circular_buffer_get_read_pointer(&buffer, 0, &read_ptr, &read_ptr_len);
    CHECK(success);
    LONGS_EQUAL(read_ptr_len, i + 1);
    for (uint8_t j = 0; j < read_ptr_len; j++) {
      LONGS_EQUAL(j, read_ptr[j]);
    }
  }

  // consume & add one byte at a time
  for (uint8_t i = 0; i < buffer_size; i++) {
    success = memfault_circular_buffer_consume(&buffer, 1);
    CHECK(success);

    uint8_t data = i + buffer_size;
    success = memfault_circular_buffer_write(&buffer, &data, sizeof(data));
    CHECK(success);

    uint8_t *read_ptr;
    size_t read_ptr_len;
    success = memfault_circular_buffer_get_read_pointer(&buffer, 0, &read_ptr, &read_ptr_len);
    CHECK(success);
    const uint8_t base = i + 1;
    size_t read_size = buffer_size - base;
    // once we have wrapped around, the read size will be the full buffer again
    LONGS_EQUAL(read_size == 0 ? buffer_size : read_size, read_ptr_len);

    for (uint8_t j = 0; j < read_ptr_len; j++) {
      LONGS_EQUAL(j + base, read_ptr[j]);
    }
  }
}
