cmake_minimum_required(VERSION 3.24.2)
project(snowbound C)

set(CMAKE_C_STANDARD 17)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)

add_executable(snowbound ${SRC_FILES})

set(ENV{VULKAN_SDK} C:/VulkanSDK/1.3.283.0/)
find_package(Vulkan REQUIRED)
target_include_directories(snowbound PUBLIC C:/VulkanSDK/1.3.283.0/Include/)
target_link_libraries (snowbound ${Vulkan_LIBRARIES})

target_include_directories(snowbound PUBLIC ${CMAKE_SOURCE_DIR}/extern/)
target_include_directories(snowbound PUBLIC ${CMAKE_SOURCE_DIR}/include/)
target_link_directories(snowbound PRIVATE ${CMAKE_SOURCE_DIR}/src/)

set(shaders {CMAKE_CURRENT_SOURCE_DIR}/shaders)
file(COPY shaders DESTINATION ${CMAKE_BINARY_DIR})

set(assets {CMAKE_CURRENT_SOURCE_DIR}/assets)
file(COPY assets DESTINATION ${CMAKE_BINARY_DIR})