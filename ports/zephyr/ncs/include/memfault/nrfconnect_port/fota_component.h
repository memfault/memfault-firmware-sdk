//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! @brief FOTA for components: software types other than the main application, such as modem
//! firmware or an upgradable bootloader

#pragma once

#include <stddef.h>
#include <zephyr/sys/iterable_sections.h>

#ifdef __cplusplus
extern "C" {
#endif

//! How the FOTA backend installs a component's image
typedef enum {
  //! Not installed by the FOTA backend: the application fetches the URL with
  //! memfault_zephyr_fota_component_get_download_url() and installs the image itself (e.g. on a
  //! downstream MCU). 0, so an unset value never selects an install mechanism.
  kMemfaultFotaComponentInstall_None = 0,
  //! Delta modem firmware update
  kMemfaultFotaComponentInstall_ModemDelta,
  //! Full modem firmware update
  kMemfaultFotaComponentInstall_ModemFull,
  //! Upgradable bootloader (e.g. MCUboot in an s0/s1 slot). Register one component per slot.
  kMemfaultFotaComponentInstall_Bootloader,
} eMemfaultFotaComponentInstall;

//! A registered component, defined with MEMFAULT_FOTA_COMPONENT_DEFINE()
typedef struct MemfaultFotaComponent {
  //! The software type of the component
  const char *software_type;
  //! Writes the current version of the component to @p buf. Returns 0 on success, -ENOTSUP if the
  //! component does not apply to this device (skipped quietly), or another negative errno.
  int (*version_get)(char *buf, size_t buf_len);
  //! How the component's image is installed
  eMemfaultFotaComponentInstall install;
  //! Optional. Returns a dedicated Memfault project key for the component, or NULL to use the
  //! main project.
  const char *(*project_key_get)(void);
} sMemfaultFotaComponent;

//! Registers a component in the device's main Memfault project
#define MEMFAULT_FOTA_COMPONENT_DEFINE(_name, _software_type, _version_get, _install)            \
  MEMFAULT_FOTA_COMPONENT_DEFINE_WITH_PROJECT_KEY(_name, _software_type, _version_get, _install, \
                                                  NULL)

//! Registers a component that may be in a dedicated Memfault project. @p _project_key_get is
//! called for each query and may return NULL to use the main project.
#define MEMFAULT_FOTA_COMPONENT_DEFINE_WITH_PROJECT_KEY(_name, _software_type, _version_get, \
                                                        _install, _project_key_get)          \
  const STRUCT_SECTION_ITERABLE(MemfaultFotaComponent, _name) = {                            \
    .software_type = (_software_type),                                                       \
    .version_get = (_version_get),                                                           \
    .install = (_install),                                                                   \
    .project_key_get = (_project_key_get),                                                   \
  }

//! Checks for an update of the component and starts the download if one is available.
//!
//! @param software_type The software type of the component
//! @return
//!   < 0 Error (-ENOENT if no component with that software type is registered, -ENOTSUP for
//!       kMemfaultFotaComponentInstall_None)
//!     0 No update available
//!     1 Update download started
int memfault_zephyr_fota_component_start(const char *software_type);

//! Fetches the download URL of a pending update for the component, without starting it.
//!
//! @param software_type The software type of the component
//! @param url Set to the download URL if an update is available
//! @return 1 if an update is available, 0 if not, or a negative errno on failure
int memfault_zephyr_fota_component_get_download_url(const char *software_type, char **url);

#ifdef __cplusplus
}
#endif
