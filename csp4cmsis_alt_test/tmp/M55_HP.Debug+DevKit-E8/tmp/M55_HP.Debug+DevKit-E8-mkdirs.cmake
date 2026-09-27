# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/home/oliver/Alif-DK-E8-CSP4CMSIS/csp4cmsis_alt_test/tmp/M55_HP.Debug+DevKit-E8")
  file(MAKE_DIRECTORY "/home/oliver/Alif-DK-E8-CSP4CMSIS/csp4cmsis_alt_test/tmp/M55_HP.Debug+DevKit-E8")
endif()
file(MAKE_DIRECTORY
  "/home/oliver/Alif-DK-E8-CSP4CMSIS/csp4cmsis_alt_test/tmp/1"
  "/home/oliver/Alif-DK-E8-CSP4CMSIS/csp4cmsis_alt_test/tmp/M55_HP.Debug+DevKit-E8"
  "/home/oliver/Alif-DK-E8-CSP4CMSIS/csp4cmsis_alt_test/tmp/M55_HP.Debug+DevKit-E8/tmp"
  "/home/oliver/Alif-DK-E8-CSP4CMSIS/csp4cmsis_alt_test/tmp/M55_HP.Debug+DevKit-E8/src/M55_HP.Debug+DevKit-E8-stamp"
  "/home/oliver/Alif-DK-E8-CSP4CMSIS/csp4cmsis_alt_test/tmp/M55_HP.Debug+DevKit-E8/src"
  "/home/oliver/Alif-DK-E8-CSP4CMSIS/csp4cmsis_alt_test/tmp/M55_HP.Debug+DevKit-E8/src/M55_HP.Debug+DevKit-E8-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/oliver/Alif-DK-E8-CSP4CMSIS/csp4cmsis_alt_test/tmp/M55_HP.Debug+DevKit-E8/src/M55_HP.Debug+DevKit-E8-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/oliver/Alif-DK-E8-CSP4CMSIS/csp4cmsis_alt_test/tmp/M55_HP.Debug+DevKit-E8/src/M55_HP.Debug+DevKit-E8-stamp${cfgdir}") # cfgdir has leading slash
endif()
