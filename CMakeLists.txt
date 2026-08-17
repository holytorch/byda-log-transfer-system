CMAKE_MINIMUM_REQUIRED(VERSION 3.15)

IF(POLICY CMP0091)
    CMAKE_POLICY(SET CMP0091 NEW)
ENDIF()

PROJECT(app)

SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_BUILD_TYPE Release)

# Build date (YYYYMMDD)
STRING(TIMESTAMP BUILD_DATE "%Y%m%d")
ADD_COMPILE_DEFINITIONS(APP_VERSION_DATE="${BUILD_DATE}")
MESSAGE(STATUS "Build date: ${BUILD_DATE}")

IF(MSVC)
    ADD_COMPILE_OPTIONS(/utf-8)
    MESSAGE(STATUS "MSVC: UTF-8 encoding enabled")
ENDIF()

MESSAGE(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
MESSAGE(STATUS "Project: ${PROJECT_NAME}")

IF(WIN32 AND NOT DEFINED CMAKE_GENERATOR_PLATFORM)
    SET(CMAKE_GENERATOR_PLATFORM x64)
ENDIF()

# Third-party paths: BUNDLED (default) or EXTERNAL (-DTHIRD_PARTY_ROOT).
# BUNDLED uses the repo's 3rdparty/; EXTERNAL uses <root>/include and <root>/lib.
IF(DEFINED THIRD_PARTY_ROOT AND NOT THIRD_PARTY_ROOT STREQUAL "")
    SET(USE_EXTERNAL_THIRD_PARTY TRUE)
ELSE()
    SET(USE_EXTERNAL_THIRD_PARTY FALSE)
ENDIF()

IF(USE_EXTERNAL_THIRD_PARTY)
    # EXTERNAL mode: all headers and .so files live under one flat root.
    MESSAGE(STATUS "Third-party mode: EXTERNAL (${THIRD_PARTY_ROOT})")

    # EXTERNAL mode is Linux-only; Windows must use bundled mode.
    IF(WIN32)
        MESSAGE(FATAL_ERROR "THIRD_PARTY_ROOT mode is Linux-only. On Windows, build without -DTHIRD_PARTY_ROOT.")
    ENDIF()

    IF(NOT EXISTS ${THIRD_PARTY_ROOT}/include)
        MESSAGE(FATAL_ERROR "THIRD_PARTY_ROOT does not contain include/: ${THIRD_PARTY_ROOT}")
    ENDIF()
    IF(NOT EXISTS ${THIRD_PARTY_ROOT}/lib)
        MESSAGE(FATAL_ERROR "THIRD_PARTY_ROOT does not contain lib/: ${THIRD_PARTY_ROOT}")
    ENDIF()

    # All headers resolve from one include dir.
    SET(LIBUV_INCLUDE_DIR   ${THIRD_PARTY_ROOT}/include)
    SET(SPDLOG_INCLUDE_DIR  ${THIRD_PARTY_ROOT}/include)
    SET(FFMPEG_INCLUDE_DIR  ${THIRD_PARTY_ROOT}/include)
    SET(OPENSSL_INCLUDE_DIR ${THIRD_PARTY_ROOT}/include)

    # All .so files live in one lib dir.
    SET(LIBUV_LIB_DIR   ${THIRD_PARTY_ROOT}/lib)
    SET(FFMPEG_LIB_DIR  ${THIRD_PARTY_ROOT}/lib)
    SET(OPENSSL_LIB_DIR ${THIRD_PARTY_ROOT}/lib)
ELSE()
    # BUNDLED mode: use the 3rdparty/ shipped in the repo (default, local build).
    MESSAGE(STATUS "Third-party mode: BUNDLED (${CMAKE_CURRENT_SOURCE_DIR}/3rdparty)")

    # 3rdparty (libuv)
    SET(LIBUV_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/libuv)
    SET(LIBUV_INCLUDE_DIR ${LIBUV_DIR}/include)
    IF(WIN32)
        SET(LIBUV_LIB_DIR ${LIBUV_DIR}/lib/window/async)
    ELSE()
        SET(LIBUV_LIB_DIR ${LIBUV_DIR}/lib/linux/async)
    ENDIF()

    # 3rdparty (spdlog)
    SET(SPDLOG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/spdlog)
    SET(SPDLOG_INCLUDE_DIR ${SPDLOG_DIR}/include)

    # 3rdparty (FFmpeg)
    SET(FFMPEG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/FFmpeg)
    SET(FFMPEG_INCLUDE_DIR ${FFMPEG_DIR}/include)
    IF(WIN32)
        SET(FFMPEG_LIB_DIR ${FFMPEG_DIR}/lib/window)
    ELSE()
        SET(FFMPEG_LIB_DIR ${FFMPEG_DIR}/lib/linux)
    ENDIF()

    # 3rdparty (OpenSSL)
    SET(OPENSSL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/OpenSSL)
    IF(WIN32)
        SET(OPENSSL_INCLUDE_DIR ${OPENSSL_DIR}/lib/window/include)
        SET(OPENSSL_DLL_DIR ${OPENSSL_DIR}/lib/window/bin)
        SET(OPENSSL_LIB_DIR ${OPENSSL_DIR}/lib/window/lib)
    ELSE()
        SET(OPENSSL_INCLUDE_DIR ${OPENSSL_DIR}/lib/linux/include)
        SET(OPENSSL_LIB_DIR ${OPENSSL_DIR}/lib/linux/lib)
    ENDIF()
ENDIF()

# SOURCE
SET(SOURCES
        src/main.cpp
)

# EXCUTABLE
ADD_EXECUTABLE(${PROJECT_NAME} ${SOURCES})

# INCLUDE
TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PRIVATE
        src
        ${LIBUV_INCLUDE_DIR}
        ${SPDLOG_INCLUDE_DIR}
        ${FFMPEG_INCLUDE_DIR}
        ${OPENSSL_INCLUDE_DIR}
)

# LINK
IF (WIN32)
    TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE
        # 3rdparty (libuv)
        ${LIBUV_LIB_DIR}/uv.lib
        # 3rdparty (OpenSSL)
        ${OPENSSL_LIB_DIR}/libcrypto.lib
        ${OPENSSL_LIB_DIR}/libssl.lib
        # 3rdparty (FFmpeg)
        ${FFMPEG_LIB_DIR}/avcodec.lib
        ${FFMPEG_LIB_DIR}/avutil.lib
        ${FFMPEG_LIB_DIR}/swscale.lib
        # window
        ws2_32.lib
        iphlpapi.lib
        userenv.lib
        psapi.lib
    )

    # POST BUILD
    ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${LIBUV_LIB_DIR}/uv.dll"
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMENT "copying uv.dll to output directory"
    )
    ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${OPENSSL_DLL_DIR}/libcrypto-3.dll"
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMENT "copying libcrypto-3.dll to output directory"
    )
    ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${OPENSSL_DLL_DIR}/libssl-3.dll"
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMENT "copying libssl-3.dll to output directory"
    )
    ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${FFMPEG_LIB_DIR}/avcodec-62.dll"
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMENT "copying avcodec-62.dll to output directory"
    )
    ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${FFMPEG_LIB_DIR}/avutil-60.dll"
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMENT "copying avutil-60.dll to output directory"
    )
    ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${FFMPEG_LIB_DIR}/swscale-9.dll"
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMENT "copying swscale-9.dll to output directory"
    )
    # MinGW GCC runtime DLLs required by FFmpeg x86 DLLs
    ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${FFMPEG_LIB_DIR}/libgcc_s_dw2-1.dll"
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMENT "copying libgcc_s_dw2-1.dll to output directory"
    )
    ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${FFMPEG_LIB_DIR}/libwinpthread-1.dll"
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMENT "copying libwinpthread-1.dll to output directory"
    )
    ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${FFMPEG_LIB_DIR}/libiconv-2.dll"
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMENT "copying libiconv-2.dll to output directory"
    )
ELSEIF(UNIX)
    # Link directories for Linux shared libraries
    TARGET_LINK_DIRECTORIES(${PROJECT_NAME} PRIVATE
        ${LIBUV_LIB_DIR}
        ${OPENSSL_LIB_DIR}
        ${FFMPEG_LIB_DIR}
    )

    TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE
        # 3rdparty (libuv)
        uv
        # 3rdparty (openssl)
        crypto
        ssl
        # 3rdparty (FFmpeg)
        avcodec
        avutil
        swscale
        # system
        pthread
        dl
    )

    # POST BUILD
    SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
        BUILD_RPATH "$ORIGIN:${LIBUV_LIB_DIR}:${OPENSSL_LIB_DIR}:${FFMPEG_LIB_DIR}"
        INSTALL_RPATH "$ORIGIN:${LIBUV_LIB_DIR}:${OPENSSL_LIB_DIR}:${FFMPEG_LIB_DIR}"
        BUILD_WITH_INSTALL_RPATH FALSE
        INSTALL_RPATH_USE_LINK_PATH TRUE
    )

    # Copy .so next to the exe only in bundled mode (RPATH $ORIGIN finds them).
    # EXTERNAL mode skips this: the base image already registered lib via ldconfig.
    IF(NOT USE_EXTERNAL_THIRD_PARTY)
        ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${LIBUV_LIB_DIR}/libuv.so.1"
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
            COMMENT "Copying libuv.so.1 to output directory"
        )
        ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${OPENSSL_LIB_DIR}/libcrypto.so.3"
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
            COMMENT "Copying libcrypto.so.3 to output directory"
        )
        ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${OPENSSL_LIB_DIR}/libssl.so.3"
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
            COMMENT "Copying libssl.so.3 to output directory"
        )
        # FFmpeg shared libraries
        ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${FFMPEG_LIB_DIR}/libavcodec.so.62"
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
            COMMENT "Copying libavcodec.so.62 to output directory"
        )
        ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${FFMPEG_LIB_DIR}/libavutil.so.60"
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
            COMMENT "Copying libavutil.so.60 to output directory"
        )
        ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${FFMPEG_LIB_DIR}/libswscale.so.9"
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
            COMMENT "Copying libswscale.so.9 to output directory"
        )
    ENDIF()

    # Optional: Print library paths for debugging
    MESSAGE(STATUS "Linux library directories:")
    MESSAGE(STATUS "  LIBUV:   ${LIBUV_LIB_DIR}")
    MESSAGE(STATUS "  OPENSSL: ${OPENSSL_LIB_DIR}")
    MESSAGE(STATUS "  FFMPEG:  ${FFMPEG_LIB_DIR}")
ENDIF()
