cmake_minimum_required(VERSION 3.12.4)

if(ESP_PLATFORM)
  include(${CMAKE_CURRENT_LIST_DIR}/ports/esp_idf/memfault/CMakeLists.txt)
else()
  include(${CMAKE_CURRENT_LIST_DIR}/cmake/Memfault.cmake)
endif()
