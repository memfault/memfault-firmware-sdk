//! @file memfault_platform_log_config.h
#pragma once

#define MEMFAULT_LOG_ERROR(...) hpy_printf (false, HPY_MSG_LEVEL_ERROR, __VA_ARGS__)
#define MEMFAULT_LOG_WARN(...) hpy_printf (false, HPY_MSG_LEVEL_WARN, __VA_ARGS__)
#define MEMFAULT_LOG_INFO(...) hpy_printf (false, HPY_MSG_LEVEL_INFO, __VA_ARGS__)
#define MEMFAULT_LOG_DEBUG(...) hpy_printf (false, HPY_MSG_LEVEL_DEBUG, __VA_ARGS__)
