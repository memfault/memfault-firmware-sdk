#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"


#include <string.h>
#include <stddef.h>
#include <stdio.h>

#include "memfault/config.h"
#include "memfault/core/math.h"
#include "memfault/demo/shell.h"

#include "memfault/demo/shell_commands.h"

static size_t s_num_chars_sent = 0;
static char s_chars_sent_buffer[1024] = {0};

static int prv_send_char(char c) {
  CHECK(s_num_chars_sent < sizeof(s_chars_sent_buffer));
  s_chars_sent_buffer[s_num_chars_sent++] = c;
  return 1;
}

static int prv_test_handler(int argc, char **argv) {
  MockActualCall &m = mock().actualCall(__func__);
  for (int i = 0; i < argc; i++) {
    char buffer[11] = {0};
    snprintf(buffer, MEMFAULT_ARRAY_SIZE(buffer), "%d", i);
    m.withStringParameter(buffer, argv[i]);
  }
  return 0;
}

static const sMemfaultShellCommand s_memfault_shell_commands[] = {
    {"test", prv_test_handler, "test command"},
    {"help", memfault_shell_help_handler, "Lists all commands"},
};

const sMemfaultShellCommand *const g_memfault_shell_commands = s_memfault_shell_commands;
const size_t g_memfault_num_shell_commands = MEMFAULT_ARRAY_SIZE(s_memfault_shell_commands);

static void prv_receive_str(const char *str) {
  for (size_t i = 0; i < strlen(str); ++i) {
    memfault_demo_shell_receive_char(str[i]);
  }
}

static void prv_reset_sent_buffer(void) {
  s_num_chars_sent = 0;
  memset(s_chars_sent_buffer, 0, sizeof(s_chars_sent_buffer));
}

TEST_GROUP(MfltDemoShell){
  void setup() {
    const sMemfaultShellImpl impl = {
        .send_char = prv_send_char,
    };
    memfault_demo_shell_boot(&impl);
    STRCMP_EQUAL("\r\nmflt> ", s_chars_sent_buffer);

    prv_reset_sent_buffer();
  }
  void teardown() {
    mock().checkExpectations();
    mock().clear();
    prv_reset_sent_buffer();
    memfault_shell_command_set_extensions(NULL, 0);
  }
};

TEST(MfltDemoShell, Test_MfltDemoShellEcho) {
  memfault_demo_shell_receive_char('h');
  memfault_demo_shell_receive_char('i');
  CHECK_EQUAL(2, s_num_chars_sent);
  STRCMP_EQUAL("hi", s_chars_sent_buffer);
}

TEST(MfltDemoShell, Test_MfltDemoShellEchoBackspace) {
  mock().expectOneCall("prv_test_handler")
        .withParameter("0", "test");
  prv_receive_str("x\x08test\n");
  STRCMP_EQUAL("x\x08\x20\x08test\r\nmflt> ", s_chars_sent_buffer);
}

TEST(MfltDemoShell, Test_MfltDemoShellEnter) {
  memfault_demo_shell_receive_char('\n');
  STRCMP_EQUAL("\r\nmflt> ", s_chars_sent_buffer);
}

TEST(MfltDemoShell, Test_MfltDemoShellEnterCR) {
  memfault_demo_shell_receive_char('\r');
  memfault_demo_shell_receive_char('\r');
  STRCMP_EQUAL("\r\nmflt> \r\nmflt> ", s_chars_sent_buffer);
}

TEST(MfltDemoShell, Test_MfltDemoShellEnterCRLF) {
  memfault_demo_shell_receive_char('\r');
  memfault_demo_shell_receive_char('\n');
  memfault_demo_shell_receive_char('\r');
  memfault_demo_shell_receive_char('\n');
  STRCMP_EQUAL("\r\nmflt> \r\nmflt> ", s_chars_sent_buffer);
}

TEST(MfltDemoShell, Test_MfltDemoShellEnterLF) {
  memfault_demo_shell_receive_char('\n');
  memfault_demo_shell_receive_char('\n');
  STRCMP_EQUAL("\r\nmflt> \r\nmflt> ", s_chars_sent_buffer);
}

TEST(MfltDemoShell, Test_MfltDemoShellEnterLFCR) {
  memfault_demo_shell_receive_char('\n');
  memfault_demo_shell_receive_char('\r');
  memfault_demo_shell_receive_char('\n');
  memfault_demo_shell_receive_char('\r');
  STRCMP_EQUAL("\r\nmflt> \r\nmflt> ", s_chars_sent_buffer);
}

TEST(MfltDemoShell, Test_MfltDemoShellUnknownCmd) {
  prv_receive_str("foo\n");
  STRCMP_EQUAL("foo\r\nUnknown command: foo\r\nType 'help' to list all commands\r\nmflt> ", s_chars_sent_buffer);
}

TEST(MfltDemoShell, Test_MfltDemoShellTestCmd) {
  mock().expectOneCall("prv_test_handler")
    .withParameter("0", "test")
    .withParameter("1", "123")
    .withParameter("2", "abc")
    .withParameter("3", "def")
    .withParameter("4", "g");
  prv_receive_str("test 123 abc    def g\n");
  STRCMP_EQUAL("test 123 abc    def g\r\nmflt> ", s_chars_sent_buffer);
}

TEST(MfltDemoShell, Test_MfltDemoShellStripLeadingSpaces) {
  mock().expectOneCall("prv_test_handler")
        .withParameter("0", "test");
  prv_receive_str("    test\n");
  STRCMP_EQUAL("    test\r\nmflt> ", s_chars_sent_buffer);
}

TEST(MfltDemoShell, Test_MfltDemoShellHelpCmd) {
  prv_receive_str("help\n");
  STRCMP_EQUAL("help\r\ntest: test command\r\nhelp: Lists all commands\r\nmflt> ", s_chars_sent_buffer);
}

TEST(MfltDemoShell, Test_MfltDemoShellRxBufferFull) {
  for (size_t i = 0; i < MEMFAULT_DEMO_SHELL_RX_BUFFER_SIZE; ++i) {
    memfault_demo_shell_receive_char('X');
  }
  STRCMP_EQUAL(
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\r\n" // MEMFAULT_DEMO_SHELL_RX_BUFFER_SIZE + 1 X's
      "Unknown command: XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\r\n" // MEMFAULT_DEMO_SHELL_RX_BUFFER_SIZE X's
      "Type 'help' to list all commands\r\n"
      "mflt> ",
      s_chars_sent_buffer);
}

TEST(MfltDemoShell, Test_MfltDemoShellBackspaces) {
  mock().expectOneCall("prv_test_handler")
      .withParameter("0", "test")
      .withParameter("1", "1");
  prv_receive_str("\b\bnop\b\b\btest 1\n");
  mock().checkExpectations();

  // use a memcmp so we can "see" the backspaces
  MEMCMP_EQUAL("nop\b \b\b \b\b \btest 1\r\nmflt> ", s_chars_sent_buffer, s_num_chars_sent);
}

TEST(MfltDemoShell, Test_MfltDemoShellNotBooted) {
   const sMemfaultShellImpl impl = {
     .send_char = NULL,
   };
   memfault_demo_shell_boot(&impl);

  prv_receive_str("test 1\n");
  LONGS_EQUAL(0, s_num_chars_sent);
}

TEST(MfltDemoShell, Test_Extension_Commands) {
  // baseline, test help output
  prv_receive_str("help\n");

  STRCMP_EQUAL("help\r\ntest: test command\r\nhelp: Lists all commands\r\nmflt> ",
               s_chars_sent_buffer);
  prv_reset_sent_buffer();

  // attach an extension command table
  static const sMemfaultShellCommand s_extension_commands[] = {
    {"test2", prv_test_handler, "test2 command"},
  };
  memfault_shell_command_set_extensions(s_extension_commands,
                                        MEMFAULT_ARRAY_SIZE(s_extension_commands));
  prv_receive_str("help\n");
  STRCMP_EQUAL(
    "help\r\ntest: test command\r\nhelp: Lists all commands\r\ntest2: test2 command\r\nmflt> ",
    s_chars_sent_buffer);
}
