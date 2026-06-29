cmake_minimum_required(VERSION 3.20)
project(motif-wasm C)

# Enforce Emscripten toolchain.
if(NOT EMSCRIPTEN)
    message(FATAL_ERROR
        "motif-wasm must be configured through emcmake. "
        "From inside WSL: emcmake cmake -S . -B build")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/artifacts")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/artifacts")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/artifacts")

# --- Locate em-x11 ---

if(DEFINED ENV{EM_X11_SRC})
    set(EM_X11_SRC "$ENV{EM_X11_SRC}" CACHE PATH "Path to em-x11 source root")
elseif(NOT DEFINED EM_X11_SRC)
    set(EM_X11_SRC "${CMAKE_SOURCE_DIR}/../em-x11" CACHE PATH "Path to em-x11 source root")
endif()

if(NOT EXISTS "${EM_X11_SRC}/cmake/em_x11_demo.cmake")
    message(FATAL_ERROR
        "em-x11 not found at ${EM_X11_SRC}. "
        "Set EM_X11_SRC=/path/to/em-x11 or place motif-wasm as a sibling to em-x11.")
endif()

# --- Locate Motif source (thentenaar/motif v2.5.2, external — never forked or modified) ---

if(DEFINED ENV{MOTIF_SRC})
    set(MOTIF_SRC "$ENV{MOTIF_SRC}" CACHE PATH "Path to thentenaar/motif source root")
elseif(NOT DEFINED MOTIF_SRC)
    set(MOTIF_SRC "${CMAKE_SOURCE_DIR}/ignored-area/third-party/motif" CACHE PATH "Path to thentenaar/motif source root")
endif()

if(NOT EXISTS "${MOTIF_SRC}/lib/Xm/Xm.h.in")
    message(FATAL_ERROR
        "thentenaar/motif not found at ${MOTIF_SRC}. "
        "Set MOTIF_SRC=/path/ignored-area/third-party/motif or run: scripts/fetch-motif.sh")
endif()

# --- em-x11 cmake modules ---

# Resolve EM_X11_SRC to an absolute path.  em_x11_finalize_demo references
# all em-x11 resources (port script, js-library, host bundle) directly from
# ${EM_X11_SRC}, so no symlinks or file copies are needed — motif-wasm and
# em-x11 just need to be in the same parent directory.
get_filename_component(EM_X11_SRC_ABSOLUTE "${EM_X11_SRC}" ABSOLUTE)
set(ENV{EM_X11_SRC} "${EM_X11_SRC_ABSOLUTE}")

list(APPEND CMAKE_MODULE_PATH "${EM_X11_SRC}/cmake")
include(em_x11_demo)

set(EM_X11_INCLUDE_DIR "${EM_X11_SRC}/native/include")

set(EM_X11_THIRD_PARTY_INCLUDES
    "${EM_X11_SRC}/ignored-area/third-party/libXt/include"
    "${EM_X11_SRC}/ignored-area/third-party/libXmu/include"
    "${EM_X11_SRC}/ignored-area/third-party/libXpm/include"
    "${EM_X11_SRC}/ignored-area/third-party/libSM/include"
    "${EM_X11_SRC}/ignored-area/third-party/libICE/include"
    "${EM_X11_SRC}/ignored-area/third-party/xorgproto/include"
)

# --- Generate Xm.h from Xm.h.in ---
# thentenaar/motif ships Xm.h.in with @VAR@ substitutions.
# We generate the real header in our build directory.

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/include/Xm")

set(XM_WITH_JPEG "#define XM_WITH_JPEG 1")
set(XM_WITH_PNG "#define XM_WITH_PNG 1")
set(XM_WITH_XRANDR "#define XM_WITH_XRANDR 0")
set(XM_WITH_XINERAMA "#define XM_WITH_XINERAMA 0")
set(XM_PRINTING "#define XM_PRINTING 0")
set(XM_UTF8 "#define XM_UTF8 1")

configure_file(
    "${MOTIF_SRC}/lib/Xm/Xm.h.in"
    "${CMAKE_BINARY_DIR}/include/Xm/Xm.h"
    @ONLY
)

# --- Generate version.h from version.h.in ---

set(XM_MAJOR 2)
set(XM_MINOR 5)
set(XM_PATCH 2)

configure_file(
    "${MOTIF_SRC}/lib/Xm/version.h.in"
    "${CMAKE_BINARY_DIR}/include/Xm/version.h"
    @ONLY
)

# --- Generate XmStrDefs*.h from .ht templates (fallback if makestrs wasn't run) ---
# The primary source is native/generated/Xm/ (produced by fetch-motif.sh via makestrs).
# This sed fallback only runs when those files are missing.

set(_gen_dir "${CMAKE_BINARY_DIR}/include/Xm")

function(_gen_strdefs stem extern_sym)
    set(_ht "${MOTIF_SRC}/lib/Xm/${stem}.ht")
    set(_h  "${_gen_dir}/${stem}.h")
    if(EXISTS "${_ht}" AND NOT EXISTS "${CMAKE_SOURCE_DIR}/native/generated/Xm/${stem}.h")
        file(READ "${_ht}" _content)
        string(REPLACE "<<<STRING_TABLE_GOES_HERE>>>"
               "extern char *${extern_sym}[];" _content "${_content}")
        file(WRITE "${_h}" "${_content}")
    endif()
endfunction()

_gen_strdefs("XmStrDefs"   "XmStrings")
_gen_strdefs("XmStrDefs22" "XmStrings22")
_gen_strdefs("XmStrDefs23" "XmStrings23")
_gen_strdefs("XmStrDefs25" "XmStrings25")
_gen_strdefs("XmStrDefsI"  "XmStringsI")

# --- libXm (Motif widget library) ---

file(GLOB LIBXM_SOURCES "${MOTIF_SRC}/lib/Xm/*.c")
file(GLOB LIBMRM_SOURCES "${MOTIF_SRC}/lib/Mrm/*.c")

# Exclude files that need host libraries we'll provide via emscripten ports
# or that aren't available in wasm.
list(REMOVE_ITEM LIBXM_SOURCES
    "${MOTIF_SRC}/lib/Xm/sharedlib.c"
)

add_library(Xm STATIC ${LIBXM_SOURCES} ${LIBMRM_SOURCES})

# Add makestrs-generated XmStrDefs.c (if available)
if(EXISTS "${CMAKE_SOURCE_DIR}/native/generated/Xm/XmStrDefs.c")
    target_sources(Xm PRIVATE "${CMAKE_SOURCE_DIR}/native/generated/Xm/XmStrDefs.c")
endif()

target_include_directories(Xm
    # PUBLIC: paths needed by consumers of libXm (periodic)
    PUBLIC
        "${CMAKE_BINARY_DIR}/include"           # generated Xm.h
        "${CMAKE_SOURCE_DIR}/native"            # our config.h
        "${CMAKE_SOURCE_DIR}/native/include"    # shadow Xft.h (before em-x11)
        "${CMAKE_SOURCE_DIR}/native/generated"  # makestrs-generated XmStrDefs*.h
        "${MOTIF_SRC}/lib"                      # <Xm/Xm.h> → lib/Xm/Xm.h
        "${EM_X11_INCLUDE_DIR}"
        ${EM_X11_THIRD_PARTY_INCLUDES}
        "${MOTIF_SRC}/include"                  # Dt/ headers
        "${CMAKE_SOURCE_DIR}/ignored-area/include"  # X11/bitmaps/ symlinks
    # PRIVATE: paths needed only when compiling libXm itself
    PRIVATE
        "${CMAKE_BINARY_DIR}/include/Xm"             # "Xm.h" (ToolTipI.h uses bare include)
        "${CMAKE_SOURCE_DIR}/native/generated/Xm"    # "XmStrDefsI.h" etc. (bare includes)
        "${MOTIF_SRC}/lib/Xm"                        # "XmI.h" etc.
        "${MOTIF_SRC}/lib/Mrm"                       # "MrmI.h" etc.
)

target_compile_definitions(Xm
    PRIVATE
        HAVE_CONFIG_H=1
        XMSTRINGDEFINES
        XOS_USE_NO_LOCKING
        XM_DEFAULT_LOG_LEVEL=XM_LOG_LEVEL_WARN
        USE_XFT=1
        XM_WITH_JPEG=1
        XM_WITH_PNG=1
        XM_UTF8=1
        XTHREADS
        NO_XM_1_2_BC
        XMBINDDIR_FALLBACK=\"/usr/lib/Xm/bindings\"
        INCDIR=\"/usr/include\"
        LIBDIR=\"/usr/lib\"
)

# JPEG and PNG support via emscripten ports.
# -s USE_LIBJPEG / USE_LIBPNG provide both headers and link libraries.
target_compile_options(Xm PRIVATE
    -w -g
    -Wno-error=implicit-function-declaration
    -fvisibility=hidden
    -include emscripten.h
    "SHELL:-s USE_LIBJPEG=1"
    "SHELL:-s USE_LIBPNG=1"
)

target_link_options(Xm PRIVATE
    "SHELL:-s USE_LIBJPEG=1"
    "SHELL:-s USE_LIBPNG=1"
)

set_target_properties(Xm PROPERTIES OUTPUT_NAME "Xm")

# --- Rebuild em-x11 when its sources change (tcldide pattern) ---
# Without this, cmake --build won't relink when em-x11 .a files are newer
# because the external archive timestamps aren't tracked as dependencies.

file(GLOB_RECURSE em_x11_src_files CONFIGURE_DEPENDS
    "${EM_X11_SRC}/native/dix/*.c"
    "${EM_X11_SRC}/native/em_x11/*.c"
    "${EM_X11_SRC}/native/em_x11/*.h"
    "${EM_X11_SRC}/native/Xext/*.c"
    "${EM_X11_SRC}/native/render/*.c"
    "${EM_X11_SRC}/native/xkb/*.c"
    "${EM_X11_SRC}/native/glx/*.c"
    "${EM_X11_SRC}/native/libX11/*.c"
    "${EM_X11_SRC}/native/include/*.h"
)

add_custom_command(
    OUTPUT "${EM_X11_SRC_ABSOLUTE}/build/artifacts/.stamp"
    COMMAND ${CMAKE_COMMAND} --build "${EM_X11_SRC_ABSOLUTE}/build"
    COMMAND ${CMAKE_COMMAND} -E touch "${EM_X11_SRC_ABSOLUTE}/build/artifacts/.stamp"
    DEPENDS ${em_x11_src_files}
    COMMENT "Rebuilding em-x11 (source files changed)"
    VERBATIM
)

add_custom_target(em_x11_static_build DEPENDS "${EM_X11_SRC_ABSOLUTE}/build/artifacts/.stamp")

# --- UIL compiler (wasm, runs via Node.js) ---
# Building uil as wasm ensures sizeof(long)=4, matching the wasm MRM reader.
# The wasm uil produces UID files that are byte-identical to what the reader expects,
# eliminating all 64-bit/32-bit binary format mismatches.
file(GLOB UIL_SOURCES "${CMAKE_SOURCE_DIR}/ignored-area/motif-full/clients/uil/*.c")

# UilParser.c is a symlink to UilMain.c — exclude to avoid duplicate symbols
list(REMOVE_ITEM UIL_SOURCES
    "${CMAKE_SOURCE_DIR}/ignored-area/motif-full/clients/uil/UilParser.c"
)

add_executable(uil ${UIL_SOURCES}
    "${CMAKE_SOURCE_DIR}/native/uil_x11_stubs.c"
)

target_include_directories(uil PRIVATE
    "${CMAKE_SOURCE_DIR}/ignored-area/motif-full/clients/uil"  # UilDefI.h etc.
    "${CMAKE_SOURCE_DIR}/ignored-area/motif-full/tools/wml"    # UilDBDef.h
    "${MOTIF_SRC}/lib"              # <Xm/...h>, <Mrm/...h>
    "${CMAKE_SOURCE_DIR}/ignored-area/motif-full/include"  # Dt/ headers
    "${CMAKE_BINARY_DIR}/include"   # generated Xm.h
    "${CMAKE_SOURCE_DIR}/native"    # config.h
    "${EM_X11_INCLUDE_DIR}"         # em-x11 X11 headers
    ${EM_X11_THIRD_PARTY_INCLUDES}  # Xt/Xmu/Xpm etc.
)

target_compile_definitions(uil PRIVATE HAVE_CONFIG_H=1 linux=1)
target_compile_options(uil PRIVATE -w -g)

# Node.js command-line tool: access real filesystem, exit after main()
# Link against Xm + em-x11 prebuilt archives (Xt, X11, etc.)
# Uses the same port script as the browser demos, but with Node.js settings.
target_link_libraries(uil PRIVATE Xm)
target_link_options(uil PRIVATE
    "SHELL:--use-port=${EM_X11_SRC_ABSOLUTE}/tools/ports/em_x11.py"
    "SHELL:-s NODERAWFS"
    "SHELL:-s EXIT_RUNTIME=1"
    "SHELL:-s WASM=1"
    "SHELL:-s ENVIRONMENT=node"
    "SHELL:-s ERROR_ON_UNDEFINED_SYMBOLS=0"
    "SHELL:-s USE_LIBJPEG=1"
    "SHELL:-s USE_LIBPNG=1"
    "SHELL:-Wl,--error-limit=0"
)
add_dependencies(uil em_x11_static_build)

# Rename .js → .cjs so Node treats as CommonJS (project package.json has "type": "module")
add_custom_command(TARGET uil POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E rename
        "${CMAKE_BINARY_DIR}/artifacts/uil.js"
        "${CMAKE_BINARY_DIR}/artifacts/uil.cjs"
    BYPRODUCTS "${CMAKE_BINARY_DIR}/artifacts/uil.cjs"
)

find_program(NODE_EXE node REQUIRED)

# Generate periodic.uid from periodic.uil using the wasm uil compiler.
# This must run after uil is built, so the UID file exists before the
# periodic demo links (the UID is --preload-file'd into MEMFS).
set(PERIODIC_UID "${CMAKE_SOURCE_DIR}/examples/periodic/periodic.uid")
set(PERIODIC_UIL "${CMAKE_SOURCE_DIR}/examples/periodic/periodic.uil")

add_custom_command(
    OUTPUT "${PERIODIC_UID}"
    COMMAND ${NODE_EXE} "${CMAKE_BINARY_DIR}/artifacts/uil.cjs"
        -o "${PERIODIC_UID}"
        "${PERIODIC_UIL}"
        "-I${MOTIF_SRC}/lib/Xm"
        "-I${MOTIF_SRC}/demos/programs/periodic"
    DEPENDS "${CMAKE_BINARY_DIR}/artifacts/uil.cjs"
    COMMENT "Generating periodic.uid via wasm uil"
)

add_custom_target(generate_periodic_uid DEPENDS "${PERIODIC_UID}")

# --- Periodic table demo ---

option(BUILD_EXAMPLES "Build example applications" ON)

# Motif virtual key bindings: on real Linux these come from the Motif
# package's xmbind.alias -> vendor-specific file chain.  We preload the
# equivalent into the wasm filesystem so Motif's _XmVirtKeysLoadFallbackBindings
# finds complete bindings (including osfSelectAll/Copy/Cut/Paste/DeselectAll
# that the hardcoded defaultFallbackBindings in Transltns.c omits).
set(MOTIF_BINDINGS_DIR "${CMAKE_SOURCE_DIR}/examples/bindings")
set(MOTIF_BINDINGS_PRELOAD
    "${MOTIF_BINDINGS_DIR}/xmbind.alias@/usr/lib/Xm/bindings/xmbind.alias"
    "${MOTIF_BINDINGS_DIR}/xmbind@/usr/lib/Xm/bindings/xmbind"
)

# --- twm window manager (build artifact, not a standalone demo) ---
# Source comes from em-x11's vendored X.Org twm, fetched by its
# scripts/fetch-third-party.sh. motif-wasm already depends on em-x11
# being built, so the source is guaranteed to exist.

if(BUILD_EXAMPLES)
    set(TWM_SRC_DIR "${EM_X11_SRC}/ignored-area/third-party/twm/src")

    add_executable(twm
        ${TWM_SRC_DIR}/add_window.c
        ${TWM_SRC_DIR}/cursor.c
        ${TWM_SRC_DIR}/deftwmrc.c
        ${TWM_SRC_DIR}/events.c
        ${TWM_SRC_DIR}/gc.c
        ${TWM_SRC_DIR}/gram.c
        ${TWM_SRC_DIR}/iconmgr.c
        ${TWM_SRC_DIR}/icons.c
        ${TWM_SRC_DIR}/lex.c
        ${TWM_SRC_DIR}/list.c
        ${TWM_SRC_DIR}/menus.c
        ${TWM_SRC_DIR}/parse.c
        ${TWM_SRC_DIR}/resize.c
        ${TWM_SRC_DIR}/session.c
        ${TWM_SRC_DIR}/twm.c
        ${TWM_SRC_DIR}/util.c
    )

    target_compile_definitions(twm PRIVATE HAVE_CONFIG_H=1)

    target_include_directories(twm PRIVATE
        ${EM_X11_SRC}/ignored-area/third-party/twm
        ${TWM_SRC_DIR}
        ${EM_X11_INCLUDE_DIR}
        ${EM_X11_THIRD_PARTY_INCLUDES}
    )

    target_compile_options(twm PRIVATE -w)

    em_x11_finalize_demo(twm
        EXPORT_NAME createTwmModule
        LIBS Xmu Xt Xext X11
        EXTRA_FUNCTIONS _em_x11_push_destroy_notify
    )
    add_dependencies(twm em_x11_static_build)

    set_property(TARGET twm APPEND PROPERTY LINK_DEPENDS
        "${EM_X11_SRC_ABSOLUTE}/build/artifacts/.stamp"
    )

    # --- Periodic wasm artifact (spawned by periodic demo) ---

    add_executable(periodic
        "${CMAKE_SOURCE_DIR}/examples/periodic/periodic.c"
        "${CMAKE_SOURCE_DIR}/native/motif_xlib_stubs.c"
    )

    target_include_directories(periodic PRIVATE
        "${CMAKE_BINARY_DIR}/include"   # generated Xm.h
        "${MOTIF_SRC}/lib"              # <Xm/...h> headers
    )

    target_compile_options(periodic PRIVATE -w -g)

    em_x11_finalize_demo(periodic
        EXPORT_NAME createPeriodicModule
        LIBS Xm Xmu Xt Xpm SM ICE Xext Xft Xrender fontconfig X11
        PRELOAD_FILES
            "${CMAKE_SOURCE_DIR}/examples/periodic/periodic.uid@/periodic.uid"
            "${CMAKE_SOURCE_DIR}/examples/periodic/XmdPeriodic.ad@/usr/lib/X11/app-defaults/XmdPeriodic"
            ${MOTIF_BINDINGS_PRELOAD}
    )
    add_dependencies(periodic em_x11_static_build generate_periodic_uid)

    set_property(TARGET periodic APPEND PROPERTY LINK_DEPENDS
        "${EM_X11_SRC_ABSOLUTE}/build/artifacts/.stamp"
        "${PERIODIC_UID}"
    )

    target_link_options(periodic PRIVATE
        "SHELL:-Wl,--whole-archive ${CMAKE_BINARY_DIR}/artifacts/libXm.a -Wl,--no-whole-archive"
        "SHELL:-s USE_LIBJPEG=1"
        "SHELL:-s USE_LIBPNG=1"
        "SHELL:-s STACK_SIZE=524288"
    )

    # --- Widget gallery demo (self-contained, no MRM) ---

    add_executable(widget-gallery
        "${CMAKE_SOURCE_DIR}/examples/widget-gallery/gallery.c"
        "${CMAKE_SOURCE_DIR}/native/motif_xlib_stubs.c"
    )

    target_include_directories(widget-gallery PRIVATE
        "${CMAKE_BINARY_DIR}/include"   # generated Xm.h
        "${MOTIF_SRC}/lib"              # <Xm/...h> headers
    )

    target_compile_options(widget-gallery PRIVATE -w -g)

    em_x11_finalize_demo(widget-gallery
        EXPORT_NAME createWidgetGalleryModule
        LIBS Xm Xmu Xt Xpm SM ICE Xext Xft Xrender fontconfig X11
        PRELOAD_FILES
            ${MOTIF_BINDINGS_PRELOAD}
    )
    add_dependencies(widget-gallery em_x11_static_build)

    set_property(TARGET widget-gallery APPEND PROPERTY LINK_DEPENDS
        "${EM_X11_SRC_ABSOLUTE}/build/artifacts/.stamp"
    )

    target_link_options(widget-gallery PRIVATE
        "SHELL:-Wl,--whole-archive ${CMAKE_BINARY_DIR}/artifacts/libXm.a -Wl,--no-whole-archive"
        "SHELL:-s USE_LIBJPEG=1"
        "SHELL:-s USE_LIBPNG=1"
        "SHELL:-s STACK_SIZE=524288"
    )

endif()
