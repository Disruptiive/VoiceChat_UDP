cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()


set(portaudio_DIR "${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-windows/share/portaudio")
set(opus_DIR "${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-windows/share/opus")
set(Boost_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-windows/include")
set(BOOST_ROOT "${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-windows/share")

project ("UDP Voice Chat")
find_package(portaudio CONFIG REQUIRED)
find_package(opus CONFIG REQUIRED)
find_package(Boost REQUIRED)

add_executable (AudioGawd "main.cpp" "constants.h" "main.cpp" "recorder.cpp" "recorder.h" "helpers.h" "helpers.cpp" "player.h" "player.cpp" "sharedmemory.h" "client.h" "client.cpp" "server.h")
if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET AudioGawd PROPERTY CXX_STANDARD 20)
endif()

target_link_libraries(AudioGawd PRIVATE portaudio)
target_link_libraries(AudioGawd PRIVATE ${CMAKE_SOURCE_DIR}/build/vcpkg_installed/x64-windows/lib/opus.lib)




