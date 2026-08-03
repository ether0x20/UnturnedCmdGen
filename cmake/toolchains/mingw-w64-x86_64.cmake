# Cross-compile toolchain: Linux host -> Windows x86-64 (MinGW-w64).
#
# Usage (with the mingw cross compiler available, e.g. extracted from Ubuntu
# packages or installed via apt):
#   cmake -S . -B build-win -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/mingw-w64-x86_64.cmake \
#         -DWIN_MINGW_ROOT=/path/to/mingw-root \
#         -DWIN_QT_ROOT=/path/to/Qt/6.4.2/mingw_64 \
#         -DQT_HOST_PATH=/usr   # host Qt for running moc/uic/rcc on Linux
#
# WIN_MINGW_ROOT expects an extracted toolchain with:
#   <root>/usr/bin/x86_64-w64-mingw32-g++          (or -g++-posix symlinked)
#   <root>/usr/x86_64-w64-mingw32/{include,lib}    (mingw-w64 headers/libs)
#   <root>/usr/lib/gcc/x86_64-w64-mingw32/<ver>/{libgcc.a,libstdc++.a,crtbegin.o}

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Paths are FORCE-set here so they are identical for the top-level configure and
# for CMake's internal try_compile projects (which do not carry over cache vars).
set(_mingw_root "$ENV{WIN_MINGW_ROOT}")
if(NOT _mingw_root)
    set(_mingw_root "${CMAKE_CURRENT_LIST_DIR}/../../../mingw-root")
endif()
set(WIN_MINGW_ROOT "${_mingw_root}" CACHE PATH "Extracted MinGW-w64 toolchain root" FORCE)

set(_qt_root "$ENV{WIN_QT_ROOT}")
if(NOT _qt_root)
    set(_qt_root "${CMAKE_CURRENT_LIST_DIR}/../../../Qt")
endif()
if(NOT EXISTS "${_qt_root}/lib/cmake/Qt6")
    # Locate a Qt/6.x.x/mingw_64 subdirectory under the root.
    file(GLOB _qt_subdirs "${_qt_root}/6.*/mingw_64")
    list(GET _qt_subdirs 0 _qt_root)
endif()
set(WIN_QT_ROOT "${_qt_root}" CACHE PATH "Qt for Windows (mingw) prefix" FORCE)

set(MINGW_BIN_DIR  "${WIN_MINGW_ROOT}/usr/bin")
set(MINGW_LIB_DIR  "${WIN_MINGW_ROOT}/usr/x86_64-w64-mingw32/lib")
set(MINGW_INC_DIR  "${WIN_MINGW_ROOT}/usr/x86_64-w64-mingw32/include")

# Locate the GCC internal directory (contains libgcc.a, crtbegin.o, ...).
file(GLOB _gcc_dirs
     "${WIN_MINGW_ROOT}/usr/lib/gcc/x86_64-w64-mingw32/*-posix")
list(GET _gcc_dirs 0 GCC_INTERNAL_DIR)

# The extracted compiler has /usr baked in; add -B for the relocation.
set(_mingw_B
    "-B ${MINGW_BIN_DIR} -B ${MINGW_LIB_DIR}")
if(GCC_INTERNAL_DIR)
    string(APPEND _mingw_B " -B ${GCC_INTERNAL_DIR}")
endif()

set(CMAKE_C_COMPILER   "${MINGW_BIN_DIR}/x86_64-w64-mingw32-gcc")
set(CMAKE_CXX_COMPILER "${MINGW_BIN_DIR}/x86_64-w64-mingw32-g++")
set(CMAKE_RC_COMPILER  "${MINGW_BIN_DIR}/x86_64-w64-mingw32-windres")

set(CMAKE_C_FLAGS_INIT   "${_mingw_B}")
set(CMAKE_CXX_FLAGS_INIT "${_mingw_B}")

# The target's system headers/libs live under the extracted root.
set(CMAKE_FIND_ROOT_PATH
    "${WIN_QT_ROOT}"
    "${WIN_MINGW_ROOT}/usr/x86_64-w64-mingw32")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Compiler-internal include path is /usr/x86_64-w64-mingw32/include; expose the
# extracted copy via CPATH for the C/C++ preprocessor.
set(ENV{CPATH} "${MINGW_INC_DIR}")
set(ENV{LIBRARY_PATH} "${MINGW_LIB_DIR}")

set(CMAKE_SYSTEM_PREFIX_PATH "${WIN_MINGW_ROOT}/usr ${WIN_QT_ROOT}")

# Qt host tools (moc/uic/rcc) run on Linux; pass the host Qt prefix.
if(DEFINED QT_HOST_PATH)
    set(QT_HOST_PATH "${QT_HOST_PATH}" CACHE PATH "Host Qt prefix for build tools")
endif()
