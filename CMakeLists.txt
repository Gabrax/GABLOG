cmake_minimum_required(VERSION 3.5)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
project(gabdebug)

cmake_policy(SET CMP0069 NEW)
add_library(${PROJECT_NAME} STATIC)
target_sources(${PROJECT_NAME} PRIVATE "gabdebug.c")
target_include_directories(${PROJECT_NAME} PUBLIC "gabdebug.h")