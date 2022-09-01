# Copyright (C) 2021-2022 Domenico Teodonio
# 
# mtdp is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
# 
# mtdp is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.0)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/TargetHardCompilation.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/DefaultSetting.cmake)

project(mtdp VERSION 0.1.0)

default_setting(MTDP_STATIC_THREADSAFE                      true        )
default_setting(MTDP_STATIC_THREADSAFE_LOCKFREE             true        )
default_setting(MTDP_PIPELINE_STATIC_INSTANCES              0           )
default_setting(MTDP_PIPE_VECTOR_STATIC_SIZE                0           )
default_setting(MTDP_STAGE_VECTOR_STATIC_SIZE               0           )
default_setting(MTDP_STAGE_IMPL_VECTOR_STATIC_SIZE          0           )
default_setting(MTDP_BUFFER_POOL_STATIC_SIZE                0           )
default_setting(MTDP_BUFFER_FIFO_BLOCKS                     0           )
default_setting(MTDP_BUFFER_FIFO_BLOCK_VECTOR_STATIC_SIZE   0           )
default_setting(MTDP_BUFFER_FIFO_BLOCK_STATIC_INSTANCES     0           )
default_setting(MTDP_BUFFER_FIFO_SHIFT_FILLING_RATIO        0.5         )
default_setting(MTDP_BUFFER_FIFO_BLOCK_SIZE                 16          )
default_setting(MTDP_PIPELINE_CONSUMER_TIMEOUT_US           100000      )
default_setting(MTDP_STRICT_ISO_C                           false        )

add_library(mtdp-object OBJECT
    ${CMAKE_CURRENT_SOURCE_DIR}/src/impl/buffer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/impl/errno.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/impl/pipe.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/impl/pipeline.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/impl/sink.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/impl/source.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/impl/stage.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/atomic.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bell.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/buffer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/errno.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/futex.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/memory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/pipe.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/pipeline.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sem.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sink.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/source.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/stage.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/thread.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/worker.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/worker.h
)
if(WIN32)
    # As of 31/08/22 _Thread_local is not implemented in MSVC
    # compiling as C so we compile errno as C++ (with proper
    # name mangling set in the source code)
    set_property(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/errno.c PROPERTY LANGUAGE CXX)
endif()
if(${CMAKE_VERSION} GREATER_EQUAL 3.23)
    target_sources(mtdp-object PUBLIC
        FILE_SET HEADERS
        BASE_DIRS 
            ${CMAKE_CURRENT_SOURCE_DIR}/include/
        FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/include/mtdp.h
            ${CMAKE_CURRENT_SOURCE_DIR}/include/mtdp/buffer.h
            ${CMAKE_CURRENT_SOURCE_DIR}/include/mtdp/pipe.h
            ${CMAKE_CURRENT_SOURCE_DIR}/include/mtdp/pipeline.h
            ${CMAKE_CURRENT_SOURCE_DIR}/include/mtdp/sink.h
            ${CMAKE_CURRENT_SOURCE_DIR}/include/mtdp/source.h
            ${CMAKE_CURRENT_SOURCE_DIR}/include/mtdp/stage.h
            ${CMAKE_CURRENT_SOURCE_DIR}/include/mtdp/errno.h
    )
endif()
if(${CMAKE_VERSION} GREATER_EQUAL 3.21)
    target_compile_features(mtdp-object PUBLIC c_std_17)
elseif()
    target_compile_features(mtdp-object PUBLIC c_std_11)
endif()
target_include_directories(mtdp-object
    PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/>
    PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
    PUBLIC  $<INSTALL_INTERFACE:include/>
)
target_compile_definitions(mtdp-object PUBLIC
    -DMTDP_STATIC_THREADSAFE=${MTDP_STATIC_THREADSAFE}
    -DMTDP_STATIC_THREADSAFE_LOCKFREE=${MTDP_STATIC_THREADSAFE_LOCKFREE}
    -DMTDP_PIPELINE_STATIC_INSTANCES=${MTDP_PIPELINE_STATIC_INSTANCES}
    -DMTDP_PIPE_VECTOR_STATIC_SIZE=${MTDP_PIPE_VECTOR_STATIC_SIZE}
    -DMTDP_STAGE_VECTOR_STATIC_SIZE=${MTDP_STAGE_VECTOR_STATIC_SIZE}
    -DMTDP_STAGE_IMPL_VECTOR_STATIC_SIZE=${MTDP_STAGE_IMPL_VECTOR_STATIC_SIZE}
    -DMTDP_BUFFER_POOL_STATIC_SIZE=${MTDP_BUFFER_POOL_STATIC_SIZE}
    -DMTDP_BUFFER_FIFO_BLOCKS=${MTDP_BUFFER_FIFO_BLOCKS}
    -DMTDP_BUFFER_FIFO_BLOCK_VECTOR_STATIC_SIZE=${MTDP_BUFFER_FIFO_BLOCK_VECTOR_STATIC_SIZE}
    -DMTDP_BUFFER_FIFO_BLOCK_STATIC_INSTANCES=${MTDP_BUFFER_FIFO_BLOCK_STATIC_INSTANCES}
    -DMTDP_BUFFER_FIFO_SHIFT_FILLING_RATIO=${MTDP_BUFFER_FIFO_SHIFT_FILLING_RATIO}
    -DMTDP_BUFFER_FIFO_BLOCK_SIZE=${MTDP_BUFFER_FIFO_BLOCK_SIZE}
    -DMTDP_PIPELINE_CONSUMER_TIMEOUT_US=${MTDP_PIPELINE_CONSUMER_TIMEOUT_US}
    -DMTDP_STRICT_ISO_C=${MTDP_STRICT_ISO_C}
)
if(UNIX)
    target_link_libraries(mtdp-object PUBLIC m atomic pthread)
elseif(WIN32)
    target_link_libraries(mtdp-object PUBLIC synchronization.lib)
endif()
target_hard_compilation(mtdp-object PUBLIC)

add_library(mtdp)
target_link_libraries(mtdp PRIVATE mtdp-object)

find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
    set(DOXYGEN_GENERATE_MAN YES)
    set(DOXYGEN_STRIP_FROM_INC_PATH include)
    set(DOXYGEN_EXTRACT_ALL YES)
    doxygen_add_docs(docs ${CMAKE_CURRENT_SOURCE_DIR}/include)
    message(STATUS "Doxygen found and configured in the docs target")
else()
    message(STATUS "Doxygen not found, documentation build not configured")
endif()

find_package(unity QUIET)
if(unity_FOUND)
    add_executable(mtdp_fifo_test ${CMAKE_CURRENT_SOURCE_DIR}/test/fifo.c)
    target_include_directories(mtdp_fifo_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/)
    target_link_libraries(mtdp_fifo_test PRIVATE mtdp-object unity::framework)
endif()

add_executable(mtdp_infinite_datastream_example ${CMAKE_CURRENT_SOURCE_DIR}/examples/infinite_datastream.c)
target_link_libraries(mtdp_infinite_datastream_example PRIVATE mtdp-object)

add_executable(mtdp_finite_datastream_example ${CMAKE_CURRENT_SOURCE_DIR}/examples/finite_datastream.c)
target_link_libraries(mtdp_finite_datastream_example PRIVATE mtdp-object)

set(CPACK_GENERATOR "TZST")
include(CPack)

include(CMakePackageConfigHelpers)
configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/mtdpConfig.cmake
    INSTALL_DESTINATION share/cmake/mtdp
    NO_SET_AND_CHECK_MACRO
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/mtdpConfigVersion.cmake
    VERSION ${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR}.${CMAKE_PROJECT_VERSION_PATCH}
    COMPATIBILITY SameMajorVersion
)
if(${CMAKE_VERSION} GREATER_EQUAL 3.23)
    install(TARGETS mtdp mtdp-object
        EXPORT mtdpTargets
        FILE_SET HEADERS
    )
else()
    install(TARGETS mtdp mtdp-object
        EXPORT mtdpTargets
    )
endif()
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/mtdpConfig.cmake 
    ${CMAKE_CURRENT_BINARY_DIR}/mtdpConfigVersion.cmake 
    DESTINATION share/cmake/mtdp
)
install(EXPORT mtdpTargets
    DESTINATION share/cmake/mtdp
    NAMESPACE mtdp::
)
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE
    DESTINATION share
)
