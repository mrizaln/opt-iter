cmake_minimum_required(VERSION 3.12)

project(
  opt-iter
  VERSION     0.0.0
  LANGUAGES   CXX
  DESCRIPTION "A generator to ranges adapter library")

add_library(opt-iter INTERFACE)
target_include_directories(opt-iter INTERFACE include)
target_compile_features(opt-iter INTERFACE cxx_std_20)
set_target_properties(opt-iter PROPERTIES CXX_EXTENSIONS OFF)
