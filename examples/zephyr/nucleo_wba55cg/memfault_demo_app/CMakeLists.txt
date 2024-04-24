cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(memfault_demo_app)

zephyr_include_directories(config)
target_sources(app PRIVATE src/main.c)
