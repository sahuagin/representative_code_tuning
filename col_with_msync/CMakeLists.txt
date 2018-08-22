cmake_minimum_required(VERSION 3.2)

include(CheckCXXCompilerFlag)

# Set a default build type for single-configuration
# CMake generators if no build type is set.
IF(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
   SET(CMAKE_BUILD_TYPE RelWithDebInfo)
ENDIF(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)

# initialize CFLAGS
# Initialize CXXFLAGS.
#set(CMAKE_CXX_FLAGS                "-Wall -Werror -march=native -fprofile-instr-generate")
set(CMAKE_CXX_FLAGS                "-Wall -Werror -march=native ")
set(CMAKE_CXX_FLAGS_DEBUG          "-O0 -g")
set(CMAKE_CXX_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE        "-O3 -DNDEBUG -march=native")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")

# for YouCompleteMe
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(DEFINED CMAKE_CXX_COMPILER)
  set(CMAKE_CXX_COMPILER ${CMAKE_CXX_COMPILER})
  message("CMAKE VAR defined Setting CMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}")
elseif(NOT DEFINED ENV{CXX})
  set(CMAKE_CXX_COMPILER "/usr/local/bin/clang++-devel")
  set(CMAKE_LINKER "/usr/local/bin/ld.lld")
  message("CXX not defined Setting CMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}")
else(DEFINED ENV{CXX})
  set(CMAKE_CXX_COMPILER $ENV{CXX})
  message("CXX Defined Setting CMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}")
endif()

if(DEFINED CMAKE_C_COMPILER)
  set(CMAKE_C_COMPILER ${CMAKE_C_COMPILER})
  message("Setting CMAKE_C_COMPILER=${CMAKE_C_COMPILER}")
elseif(NOT DEFINED ENV{CC})
  set(CMAKE_C_COMPILER "/usr/local/bin/clang-devel")
  #set(CMAKE_LINKER "/usr/local/bin/lld")
  set(CMAKE_LINKER "/usr/local/bin/ld.lld")
  message("Setting CMAKE_C_COMPILER=${CMAKE_C_COMPILER}")
else(DEFINED ENV{CC})
  set(CMAKE_C_COMPILER $ENV{CC})
  message("Setting CMAKE_C_COMPILER=${CMAKE_C_COMPILER}")
endif()


if(DEFINED CMAKE_LINKER)
  set(CMAKE_LINKER ${CMAKE_LINKER})
elseif(NOT DEFINED ENV{LD})
  #set(CMAKE_LINKER "/usr/local/bin/ld.gold")
  set(CMAKE_LINKER "/usr/local/bin/ld.lld-devel")
  #set(CMAKE_LINKER "/usr/local/bin/ld")
else(DEFINED ENV{LD})
  set(CMAKE_LINKER $ENV{LD})
endif()
message("CMAKE_LINKER='${CMAKE_LINKER}'")


project(test CXX)

#message("Your C++ compiler supports these C++ features:")
#foreach(i ${CMAKE_CXX_COMPILE_FEATURES})
#message("${i}")
#endforeach()

#message("Your CMake version supports these C++ features:")
#get_property(known_features GLOBAL PROPERTY CMAKE_CXX_KNOWN_FEATURES)
#foreach(i ${known_features})
#message("${i}")
#endforeach()

# find if our compiler supports more advanced flags, we can set explicitly as well
CHECK_CXX_COMPILER_FLAG("--std=c++17" SUPPORTS_CXX17)
CHECK_CXX_COMPILER_FLAG("--std=c++14" SUPPORTS_CXX14)
CHECK_CXX_COMPILER_FLAG("--std=c++11" SUPPORTS_CXX11)
if( USE_FLTO OR NOT DEFINED USE_FLTO)
  #CHECK_CXX_COMPILER_FLAG("-v -flto -fuse-ld=gold"  SUPPORTS_FLTO)
CHECK_CXX_COMPILER_FLAG("-v -flto "  SUPPORTS_FLTO)
endif()
CHECK_CXX_COMPILER_FLAG("-gdwarf-4" SUPPORTS_DWARF4)
CHECK_CXX_COMPILER_FLAG("-gdwarf-2" SUPPORTS_DWARF2)
if( USE_LIBCXX OR NOT DEFINED USE_LIBCXX)
CHECK_CXX_COMPILER_FLAG("-stdlib=libc++" SUPPORTS_LIBCXX)
message("USE_LIBCXX='${USE_LIBCXX}'")
endif()
CHECK_CXX_COMPILER_FLAG("-Qunused-arguments" SUPPORTS_UNUSEDARGS)

if(DEFINED USE_CXX17 OR SUPPORTS_CXX17)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++17")
  message("SUPPORTS_CXX17='${SUPPORTS_CXX17}'")
elseif(USE_CXX14 OR SUPPORTS_CXX14)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++14")
  message("SUPPORTS_CXX14='${SUPPORTS_CXX14}'")
elseif(USE_CXX11 OR SUPPORTS_CXX11)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++11")
  message("SUPPORTS_CXX11='${SUPPORTS_CXX11}'")
else()
  message("Compiler doesn't support --std=c++11 or --std=c++14")
endif()

if( SUPPORTS_UNUSEDARGS)
  set(UNUSED_ARGS "-Qunused-arguments")
endif()

if( SUPPORTS_FLTO OR USE_FLTO)
  set(LTO_FLAGS "-flto -fuse-ld=gold ${UNUSED_ARGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${LTO_FLAGS}")
  message("SUPPORTS_FLTO='${SUPPORTS_FLTO}'")
else()
  set(LTO_FLAGS "")
endif()

if( USE_MPFR )
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lmpfr -lgmp")
  add_definitions( -DUSING_MPFR=1 )
  message("USE_MPFR='${USE_MPFR}'")
endif()

if( USE_THREADS )
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread")
  add_definitions( -DUSE_THREADS=1 )
  message("USE_THREADS='${USE_THREADS}'")
endif()

set(DWARF_FLAGS "")
if(SUPPORTS_DWARF4)
  set(DWARF_FLAGS "-gdwarf-4")
elseif(SUPPORTS_DWARF2)
  set(DWARF_FLAGS "-gdwarf-2")
else()
  set(DWARF_FLAGS "-g")
endif()
message("DWARF_FLAGS='${DWARF_FLAGS}'")

set(LIBCXX "")
if(SUPPORTS_LIBCXX OR USE_LIBCXX)
  if(USE_LIBCXX)
    set(LIBCXX "-stdlib=libc++")
  endif(USE_LIBCXX)

  message("SUPPORTS_LIBCXX='${SUPPORTS_LIBCXX}' USE_LIBCXX='${USE_LIBCXX}'")
else(SUPPORTS_LIBCXX OR USE_LIBCXX)
  set(LIBCXX "")
endif()

  message("SUPPORTS_LIBCXX='${SUPPORTS_LIBCXX}' USE_LIBCXX='${USE_LIBCXX}'")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${LIBCXX}")


message("CMAKE_CXX_FLAGS='${CMAKE_CXX_FLAGS}'")


set(USR_LOCAL_INCLUDE "/usr/local/include/")
set(USR_LOCAL_LIB "/usr/local/lib/")
#set(GMOCK_INCLUDE "/usr/local/include/gmock/")
#set(GTEST_INCLUDE "/usr/local/include/gtest/")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${DWARF_FLAGS}" )
message("CMAKE_CXX_FLAGS_DEBUG='${CMAKE_CXX_FLAGS_DEBUG}'")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${DWARF_FLAGS}" CACHE STRING "")
include_directories(${USR_LOCAL_INCLUDE} ${GMOCK_INCLUDE}  ${GTEST_INCLUDE} ${GCC_INCLUDES} ${GCC_INCLUDES} ${GCC_INCLUDES} ${CXX_INCLUDES})
link_directories("/usr/lib/" ${LIBDIR} ${USR_LOCAL_LIB})
#if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
#  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Werror -g --std=c++14 -stdlib=libc++ -ggdb -gdwarf-4 -g3 -flto")
#elseif("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
#  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Werror -O3 --std=c++14 -stdlib=libc++ -flto")
#else(NOT DEFINED "${CMAKE_BUILD_TYPE}")
#  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Werror -O3 -g --std=c++14 -stdlib=libc++ -ggdb -gdwarf-4 -g3 -flto")
#endif()
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror --std=c++14 -stdlib=libc++ -flto")
set(CMAKE_LD_FLAGS "${CMAKE_CXX_FLAGS}")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" AND CMAKE_GENERATOR STREQUAL "Ninja")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fcolor-diagnostics")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics")
endif ()

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${LTO_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${LTO_FLAGS}")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src )

set(xxxsource 
  xxx.cpp
   )
set(yyysource
  yyy.cpp
   )




add_executable(yyy ${yyysource})
add_executable(xxx ${xxxsource})
target_link_libraries(yyy pthread)
target_link_libraries(xxx pthread)



# add a target to generate API documentation with Doxygen
#find_package(Doxygen)
#if(DOXYGEN_FOUND)
#  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
#  add_custom_target(doc
#    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
#    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
#    COMMENT "Generating API documentation with Doxygen" VERBATIM
#    )
#endif(DOXYGEN_FOUND)

