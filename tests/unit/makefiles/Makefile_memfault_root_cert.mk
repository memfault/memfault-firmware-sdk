SRC_FILES = \
  $(MFLT_COMPONENTS_DIR)/http/src/memfault_root_certs_der.c

TEST_SRC_FILES = \
  $(MFLT_TEST_SRC_DIR)/test_memfault_root_cert.cpp \
  $(MFLT_COMPONENTS_DIR)/util/src/memfault_base64.c

include $(CPPUTEST_MAKFILE_INFRA)
