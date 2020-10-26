cmake_minimum_required(VERSION 3.13.1)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(memfault_demo_app)

zephyr_include_directories(config)
target_sources(app PRIVATE src/main.c)
