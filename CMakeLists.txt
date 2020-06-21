# If you want built-in precompiled header support
# then make sure you have cmake 3.16 or higher.
#
# Minimum required is 3.15 due to use of multiple values in
# generator expressions.
cmake_minimum_required( VERSION 3.15 )

#set(CMAKE_CXX_CLANG_TIDY clang-tidy -checks=-*,readability-*)

# Don't allow in-source builds...no real reason, just
# keeping those source trees nice and tidy.  :-)
# (This can be removed if it becomes an issue.)
if( "${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}" )
   message( FATAL_ERROR
      "In-source builds not allowed.\n"
      "Create a new directory and run cmake from there, i.e.:\n"
      "  mkdir build\n"
      "  cd build\n"
      "  cmake ..\n"
      "You will need to delete CMakeCache.txt and CMakeFiles from this directory to clean up."
   )
endif()

# Just a couple of convenience variables
set( topdir "${CMAKE_SOURCE_DIR}" )
set( libsrc "${topdir}/lib-src" )

# Ignore COMPILE_DEFINITIONS_<Config> properties
cmake_policy( SET CMP0043 NEW )

# Link libraries by full path even in implicit directories.
cmake_policy( SET CMP0060 NEW )

# ``INTERPROCEDURAL_OPTIMIZATION`` is enforced when enabled.
cmake_policy( SET CMP0069 NEW )
set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)

# ``FindOpenGL`` prefers GLVND by default when available.
cmake_policy( SET CMP0072 NEW )

# Include file check macros honor ``CMAKE_REQUIRED_LIBRARIES``.
cmake_policy( SET CMP0075 NEW )

set( CLANG_TIDY clang-tidy CACHE STRING "The clang-tidy executable")

if( WIN32 )
    set( audacity_target_architecture ${CMAKE_SYSTEM_PROCESSOR} CACHE STRING "Target processor architecture" )
    set_property( CACHE audacity_target_architecture PROPERTY STRINGS x64 x86 arm64 )

    option( audacity_asan "Enable AddressSanitizer" )
endif()

# Definitions that must happen before the project() command
if( APPLE )

   # Generate schema files
   set( CMAKE_XCODE_GENERATE_SCHEME ON )

   # Define the OSX compatibility parameters
   set( CMAKE_OSX_ARCHITECTURES x86_64 CACHE INTERNAL "" )
   set( CMAKE_OSX_DEPLOYMENT_TARGET 10.7 CACHE INTERNAL "" )
   set( CMAKE_OSX_SYSROOT macosx CACHE INTERNAL "" )
   set( CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "" CACHE INTERNAL "" )

   # This prevents a link error when building with the 10.9 or older SDKs
   set( CMAKE_XCODE_ATTRIBUTE_CLANG_LINK_OBJC_RUNTIME OFF )

   # Shouldn't cmake do this???
   string( APPEND CMAKE_CXX_FLAGS " -stdlib=libc++" )
endif()

# Add our module path
set( CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake-proxies/cmake-modules)

# This "is a good thing" but greatly increases link time on Linux
set( CMAKE_INTERPROCEDURAL_OPTIMIZATION ON )
set( CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON )
set( CMAKE_INTERPROCEDURAL_OPTIMIZATION_DEBUG OFF )

# Set the required C++ standard
set( CMAKE_CXX_STANDARD 14 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )

# Use ccache if available
find_program( CCACHE_PROGRAM ccache )
mark_as_advanced( FORCE CCACHE_PROGRAM )

if( CCACHE_PROGRAM )
   set_property( GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}" )
endif()

# Our very own project
project( Audacity VERSION 2.4.2 )

# Load our functions/macros
include( AudacityFunctions )

# Pull all the modules we'll need
include( CheckCXXCompilerFlag )
include( CheckIncludeFile )
include( CheckIncludeFiles )
include( CheckLibraryExists )
include( CheckSymbolExists )
include( CheckTypeSize )
include( CMakeDependentOption )
include( CMakeDetermineASM_NASMCompiler )
include( CMakePushCheckState )
include( GNUInstallDirs )
include( TestBigEndian )

message( STATUS "Build Info:" )
message( STATUS "  Host System: ${CMAKE_HOST_SYSTEM}" )
message( STATUS "  Host System Name: ${CMAKE_HOST_SYSTEM_NAME}" )
message( STATUS "  Host System Processor: ${CMAKE_HOST_SYSTEM_PROCESSOR}" )
message( STATUS "  Host System Version: ${CMAKE_HOST_SYSTEM_VERSION}" )
message( STATUS )
message( STATUS "  Compiler: ${CMAKE_CXX_COMPILER}" )
message( STATUS "  Compiler Version: ${CMAKE_CXX_COMPILER_VERSION}" )
message( STATUS "  Compiler Standard: ${CMAKE_CXX_STANDARD}" )
message( STATUS "  Compiler Standard Required: ${CMAKE_CXX_STANDARD_REQUIRED}" )
message( STATUS "  Compiler Extensions: ${CMAKE_CXX_EXTENSIONS}" )
message( STATUS )

if ( audacity_target_architecture )
   message( STATUS "  Target architecture: ${audacity_target_architecture}" )
   message( STATUS )
endif()

if( CMAKE_GENERATOR MATCHES "Visual Studio" )
   message( STATUS "  MSVC Version: ${MSVC_VERSION}" )
   message( STATUS "  MSVC Toolset: ${MSVC_TOOLSET_VERSION}" )
   message( STATUS ) 
elseif( CMAKE_SYSTEM_NAME MATCHES "Darwin" )
   if( CMAKE_GENERATOR MATCHES "Xcode" )
      message( STATUS "  Xcode Version: ${XCODE_VERSION}" )
   endif()
   message( STATUS "  MacOS SDK: ${CMAKE_OSX_SYSROOT}" )
   message( STATUS )
endif()

# Try to get the current commit information
set( GIT_COMMIT_SHORT "unknown" )
set( GIT_COMMIT_LONG "unknown" )
set( GIT_COMMIT_TIME "unknown" )
find_package( Git QUIET )
if( GIT_FOUND )
   execute_process(
      COMMAND
         ${GIT_EXECUTABLE} show -s "--format=%h;%H;%cd"
      WORKING_DIRECTORY
         ${topdir}
      OUTPUT_VARIABLE
         git_output
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_QUIET
   )

   if( git_output )
      list( GET git_output 0 GIT_COMMIT_SHORT )
      list( GET git_output 1 GIT_COMMIT_LONG )
      list( GET git_output 2 GIT_COMMIT_TIME )
   endif()
endif()
message( STATUS "  Current Commit: ${GIT_COMMIT_SHORT}" )
message( STATUS )

# Define option() prefix
set( _OPT "audacity_" )

# Organize subdirectories/targets into folders for the IDEs
set_property( GLOBAL PROPERTY USE_FOLDERS ON )

if( CMAKE_GENERATOR MATCHES "Visual Studio" )
   # Make sure Audacity is the startup project
   set_directory_properties(
      PROPERTIES
         VS_STARTUP_PROJECT "${CMAKE_PROJECT_NAME}"
   )
endif()

# Determine 32-bit or 64-bit target
if( MSVC AND CMAKE_VS_PLATFORM_NAME MATCHES "Win64|x64" )
   set( IS_64BIT ON )
elseif( audacity_target_architecture MATCHES "^(x64|arm64)$")
   set( IS_64BIT ON )
elseif( audacity_target_architecture MATCHES "^(x86|arm)$")
   # 32-bit
elseif( NOT CMAKE_SIZEOF_VOID_P STREQUAL "4" )
   set( IS_64BIT ON )
endif()

if( IS_64BIT )
   message( STATUS "Building for 64-bit target" )
else()
   message( STATUS "Building for 32-bit target" )
endif()

if( MSVC )
    set( AUDACITY_MSVC_FLAGS " /diagnostics:caret /GS /Gy /Gw /utf-8 /volatile:iso /permissive- /Zc:__cplusplus /Zc:inline")

    # 4028: formal parameter 'number' different from declaration
    #       4028 is an error when building with clang or g++
    # 4047: 'operator' : 'identifier1' differs in levels of indirection from 'identifier2'
    # 4172: returning address of local variable or temporary
    #        Note that 4242 and 4244 have the same short description.
    # 4242: 'conversion' conversion from 'type1' to 'type2', possible loss of data
    # 4244: 'conversion' conversion from 'type1' to 'type2', possible loss of data
    # 4305: 'context' : truncation from 'type1' to 'type2'
    # 4311: 'variable' : pointer truncation from 'type' to 'type'
    # 4312: 'operation' : conversion from 'type1' to 'type2' of greater size
    # 4313: 'function' : 'format specifier' in format string conflicts with argument number of type 'type'
    # 4318: passing constant zero as the length to memset
    # 4319: '~' : zero extending 'type1' to 'type2' of greater size
    # 4477: 'function' : format string 'string' requires an argument of type 'type', but variadic argument number has type 'type'
    # 4701: Potentially uninitialized local variable 'name' used
    # 4703: Potentially uninitialized local pointer variable 'name' used
    # 4716: 'function' must return a value

    string( APPEND AUDACITY_MSVC_FLAGS " /we4028 /we4047 /we4172 /w34318 /w34319 /we4477 /w34701 /w34716")
    if( IS_64BIT )
        string( APPEND AUDACITY_MSVC_FLAGS " /we4703 /we4311 /we4312 /we4313")
    else()
        string( APPEND AUDACITY_MSVC_FLAGS " /w34703 /w34311 /w34312 /w34313")
    endif()

    # Build using multiple processors when more than one source file is compiled by the same compiler
    # invocation.
    string( APPEND AUDACITY_MSVC_FLAGS " /MP" )

    string( APPEND AUDACITY_MSVC_ASAN_FLAGS " /Zi /Od /DNDEBUG")

    string( APPEND AUDACITY_MSVC_DEBUG_FLAGS " /Zi /Ob0 /Od /RTC1 /JMC")

    set( AUDACITY_MSVC_RELEASE_FLAGS " /Zi /O2 /Ob3 /fp:fast /DNDEBUG")

    option( audacity_msvc_analyze "Enable MSVC's code analysis" )

    if (audacity_msvc_analyze )
        string( APPEND AUDACITY_MSVC_FLAGS " /analyze /analyze:WX-" )
    endif()

   option( audacity_control_flow_guard "Enable Control Flow Guard (CFG)" )

   if( audacity_control_flow_guard )
      message( STATUS "Enabling Control Flow Guard (CFG)")

      string( APPEND AUDACITY_MSVC_FLAGS " /guard:cf")
      string( APPEND AUDACITY_MSVC_LINKER_FLAGS " /GUARD:CF")
   endif()

    string( APPEND CMAKE_C_FLAGS ${AUDACITY_MSVC_FLAGS} )
    string( APPEND CMAKE_CXX_FLAGS ${AUDACITY_MSVC_FLAGS} )
    string( APPEND CMAKE_EXE_LINKER_FLAGS ${AUDACITY_MSVC_LINKER_FLAGS} )

    if( CMAKE_BUILD_TYPE )
        message( STATUS "CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE}" )
        set( build_list ${CMAKE_BUILD_TYPE} )
    elseif( CMAKE_CONFIGURATION_TYPES )
        message( STATUS "CMAKE_CONFIGURATION_TYPES ${CMAKE_CONFIGURATION_TYPES}" )
        set( build_list ${CMAKE_CONFIGURATION_TYPES} )
    endif()


    foreach( config ${build_list} )
        string( TOUPPER "${config}" config )

        if( audacity_asan )
            if( config MATCHES "DEBUG"  )
                message( ERROR_FAIL "ASAN is not supported with debug builds" )
            endif()

            message( STATUS "Appending ASAN build flags" )
            set( CMAKE_C_FLAGS_${config} ${AUDACITY_MSVC_ASAN_FLAGS} )
            set( CMAKE_CXX_FLAGS_${config} ${AUDACITY_MSVC_ASAN_FLAGS} )
            continue()
        endif()

        if( config MATCHES "DEBUG"  )
            message( STATUS "Appending Debug build flags" )
            set( CMAKE_C_FLAGS_${config} "${AUDACITY_MSVC_DEBUG_FLAGS}" )
            set( CMAKE_CXX_FLAGS_${config} "${AUDACITY_MSVC_DEBUG_FLAGS}" )
            continue()
        endif()

        message( STATUS "Appending Release build flags" )
        set( CMAKE_C_FLAGS_${config} "${AUDACITY_MSVC_RELEASE_FLAGS}" )
        set( CMAKE_CXX_FLAGS_${config} "${AUDACITY_MSVC_RELEASE_FLAGS}" )
    endforeach()

   # ??? This should be set automatically in the TIFF CMakeLists.txt
   add_definitions( -DUSE_WIN32_FILEIO=1 )

   add_compile_definitions(NTDDI_VERSION=NTDDI_VISTASP2 _WIN32_WINNT=_WIN32_WINNT_VISTA)

   # Define system library information, but we'll do the install
   #set( CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP YES )
   #set( CMAKE_INSTALL_UCRT_LIBRARIES NO )
   #set( CMAKE_INSTALL_MFC_LIBRARIES NO )
   #set( CMAKE_INSTALL_OPENMP_LIBRARIES NO )
   #include( InstallRequiredSystemLibraries )
endif()

# Where the final product is stored
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}/audacity )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}/audacity )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin )
if( WIN32 )
    set( CMAKE_INSTALL_LOCALEDIR bin/locale )
endif()

# Set up RPATH handling
set( CMAKE_SKIP_BUILD_RPATH FALSE )
set( CMAKE_BUILD_WITH_INSTALL_RPATH FALSE )
set( CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_FULL_LIBDIR}/audacity" )
set( CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE )
set( CMAKE_MACOSX_RPATH FALSE )

# the RPATH to be used when installing, but only if it's not a system directory
#list( FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_FULL_LIBDIR}" isSysDir )
#if( "${isSysDir}" STREQUAL "-1" )
#   set( CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib" )
#endif()

# Add the math library (if found) to the list of required libraries
check_library_exists( m pow "" HAVE_LIBM )
if( HAVE_LIBM )
   list( APPEND CMAKE_REQUIRED_LIBRARIES -lm )
endif()

check_library_exists( atomic __atomic_fetch_add_4 "" HAVE_LIBATOMIC )
if( HAVE_LIBATOMIC )
  list( APPEND CMAKE_REQUIRED_LIBRARIES -latomic )
endif()

# Add the dynamic linker library (if needed) to the list of required libraries
list( APPEND CMAKE_REQUIRED_LIBRARIES ${CMAKE_DL_LIBS} )

# Make sure they're used during the link steps
set( CMAKE_LINK_INTERFACE_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} )

# Various common checks whose results are used by the different targets
test_big_endian( WORDS_BIGENDIAN )

# Check for compiler flags
if( CMAKE_CXX_COMPILER_ID MATCHES "AppleClang|Clang|GNU" )
   check_cxx_compiler_flag( "-mmmx" HAVE_MMX )
   if( HAVE_MMX )
      set( MMX_FLAG "-mmmx" CACHE INTERNAL "" )
   endif()

   check_cxx_compiler_flag( "-msse" HAVE_SSE )
   if( HAVE_SSE )
      set( SSE_FLAG "-msse" CACHE INTERNAL "" )
   endif()

   check_cxx_compiler_flag( "-msse2" HAVE_SSE2 )
   if( HAVE_SSE2 )
      set( SSE_FLAG "-msse2" CACHE INTERNAL "" )
   endif()
elseif( MSVC )
   if( ${audacity_target_architecture} MATCHES "^(x86|x64)$" )
      set( audacity_target_architecture_extension "" CACHE STRING "Enable the avx, avx2, or avx512 exensions (avx2 implies avx, avx512 implies avx and avx2" )
      set_property(CACHE audacity_target_architecture_extension PROPERTY STRINGS "" avx avx2 avx512)

      set( HAVE_MMX ON )
      set( HAVE_SSE ON )
      set( HAVE_SSE2 ON )
      if( "${audacity_target_architecture_extension}" STREQUAL "avx")
         set( HAVE_AVX ON )
         message( STATUS "Enabling AVX instructions")
         string( APPEND CMAKE_C_FLAGS " /arch:AVX" )
         string( APPEND CMAKE_CXX_FLAGS " /arch:AVX" )
      elseif( "${audacity_target_architecture_extension}" STREQUAL "avx2")
         set( HAVE_AVX2 ON )
         message( STATUS "Enabling AVX and AVX2 instructions")
         string( APPEND CMAKE_C_FLAGS " /arch:AVX2" )
         string( APPEND CMAKE_CXX_FLAGS " /arch:AVX2" )
      elseif( "${audacity_target_architecture_extension}" STREQUAL "avx512")
         set( HAVE_AVX512 ON )
         message( STATUS "Enabling AVX, AVX2, and AVX512 instructions")
         string( APPEND CMAKE_C_FLAGS " /arch:AVX512" )
         string( APPEND CMAKE_CXX_FLAGS " /arch:AVX512" )
      elseif ( NOT "${audacity_target_architecture_extension}" STREQUAL "" )
         message ( FATAL_ERROR "Unknown architecture extensions: ${audacity_target_architecture_extension}")
      else()
         message( STATUS "Using default instructions (SSE2)")
      endif()
   endif()
endif()

check_include_files( "float.h;stdarg.h;stdlib.h;string.h" STDC_HEADERS )

check_include_file( "assert.h" HAVE_ASSERT_H )
check_include_file( "errno.h" HAVE_ERRNO_H )
check_include_file( "fcntl.h" HAVE_FCNTL_H )
check_include_file( "fenv.h" HAVE_FENV_H )
check_include_file( "inttypes.h" HAVE_INTTYPES_H )
check_include_file( "limits.h" HAVE_LIMITS_H )
if( CMAKE_SYSTEM_NAME MATCHES "FreeBSD" )
  check_include_file( "stdlib.h" HAVE_MALLOC_H )
  check_include_file( "sys/endian.h" HAVE_ENDIAN_H )
else()
  check_include_file( "malloc.h" HAVE_MALLOC_H )
  check_include_file( "byteswap.h" HAVE_BYTESWAP_H )
endif()
check_include_file( "memory.h" HAVE_MEMORY_H )
check_include_file( "stdbool.h" HAVE_STDBOOL_H )
check_include_file( "stdint.h" HAVE_STDINT_H )
check_include_file( "stdlib.h" HAVE_STDLIB_H )
check_include_file( "string.h" HAVE_STRING_H )
check_include_file( "strings.h" HAVE_STRINGS_H )
check_include_file( "unistd.h" HAVE_UNISTD_H )
check_include_file( "xmmintrin.h" HAVE_XMMINTRIN_H )
check_include_file( "sys/param.h" HAVE_SYS_PARAM_H )
check_include_file( "sys/stat.h" HAVE_SYS_STAT_H )
check_include_file( "sys/types.h" HAVE_SYS_TYPES_H )
check_include_file( "sys/time.h" HAVE_SYS_TIME_H )
check_include_file( "sys/wait.h" HAVE_SYS_WAIT_H )

check_symbol_exists( bcopy "strings.h" HAVE_BCOPY )
check_symbol_exists( fileno "stdio.h" HAVE_FILENO )
check_symbol_exists( flock "sys/file.h" HAVE_FLOCK )
check_symbol_exists( fork "unistd.h" HAVE_FORK )
check_symbol_exists( fsync "unistd.h" HAVE_FSYNC )
check_symbol_exists( ftruncate "unistd.h" HAVE_FTRUNCATE )
check_symbol_exists( getpagesize "unistd.h" HAVE_GETPAGESIZE )
check_symbol_exists( gettimeofday "sys/time.h" HAVE_GETTIMEOFDAY )
check_symbol_exists( gmtime "time.h" HAVE_GMTIME )
check_symbol_exists( gmtime_r "time.h" HAVE_GMTIME_R )
check_symbol_exists( lrint "math.h" HAVE_LRINT )
check_symbol_exists( lrintf "math.h" HAVE_LRINTF )
check_symbol_exists( lround "math.h" HAVE_LROUND )
check_symbol_exists( lstat "sys/stat.h" HAVE_LSTAT )
check_symbol_exists( memcpy "string.h" HAVE_MEMCPY )
check_symbol_exists( memmove "string.h" HAVE_MEMMOVE )
check_symbol_exists( mlock "sys/mman.h" HAVE_MLOCK )
check_symbol_exists( pipe "unistd.h" HAVE_PIPE )
check_symbol_exists( posix_fadvise "fcntl.h" HAVE_POSIX_FADVISE )
check_symbol_exists( posix_memalign "stdlib.h" HAVE_POSIX_MEMALIGN )
check_symbol_exists( strchr "string.h" HAVE_STRCHR )
check_symbol_exists( waitpid "sys/wait.h" HAVE_WAITPID )

check_type_size( "int8_t" SIZEOF_INT8 LANGUAGE C )
check_type_size( "int16_t" SIZEOF_INT16 LANGUAGE C )
check_type_size( "uint16_t" SIZEOF_UINT16 LANGUAGE C )
check_type_size( "u_int16_t" SIZEOF_U_INT16 LANGUAGE C )
check_type_size( "int32_t" SIZEOF_INT32 LANGUAGE C )
check_type_size( "uint32_t" SIZEOF_UINT32 LANGUAGE C )
check_type_size( "u_int32_t" SIZEOF_U_INT32 LANGUAGE C )
check_type_size( "int64_t" SIZEOF_INT64 LANGUAGE C )
check_type_size( "short" SIZEOF_SHORT LANGUAGE C )
check_type_size( "unsigned short" SIZEOF_UNSIGNED_SHORT LANGUAGE C )
check_type_size( "int" SIZEOF_INT LANGUAGE C )
check_type_size( "unsigned int" SIZEOF_UNSIGNED_INT LANGUAGE C )
check_type_size( "long" SIZEOF_LONG LANGUAGE C )
check_type_size( "unsigned long" SIZEOF_UNSIGNED_LONG LANGUAGE C )
check_type_size( "long long" SIZEOF_LONG_LONG LANGUAGE C )
check_type_size( "unsigned long long" SIZEOF_UNSIGNED_LONG_LONG LANGUAGE C )
check_type_size( "float" SIZEOF_FLOAT LANGUAGE C )
check_type_size( "double" SIZEOF_DOUBLE LANGUAGE C )
check_type_size( "long double" SIZEOF_LONG_DOUBLE LANGUAGE C )
check_type_size( "loff_t" SIZEOF_LOFF LANGUAGE C )
check_type_size( "off_t" SIZEOF_OFF LANGUAGE C )
check_type_size( "off64_t" SIZEOF_OFF64 LANGUAGE C )
check_type_size( "size_t" SIZEOF_SIZE LANGUAGE C )
check_type_size( "wchar_t" SIZEOF_WCHAR LANGUAGE C )
check_type_size( "void*" SIZEOF_POINTER LANGUAGE C )

# We'll be using it if it's available
find_package( PkgConfig QUIET )

# Mostly just to make the CMP0072 policy happy
find_package( OpenGL QUIET )

# Define the non-install and executable destinations
#
# If this is a multi-config build system (VS, Xcode), CMAKE_CFG_INTDIR
# will (eventually) resolve to the build type, i.e., Debug, Release, etc.
# and CMAKE_BUILD_TYPE will be empty.
#
# For single-config build systems, CMAKE_CFG_INTDIR will be "." and
# CMAKE_BUILD_TYPE will be something like Debug.
#
# So, in either case we end up with what we want:
#     .../bin/Debug//
# or:
#     .../bin//Debug
set( _DESTDIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/${CMAKE_BUILD_TYPE}" )
set( _DEST "${_DESTDIR}" )
set( _EXEDIR "${_DEST}" )
string( REGEX REPLACE "/+$" "" _EXEDIR "${_EXEDIR}" )

# Adjust them for the Mac
if( CMAKE_SYSTEM_NAME MATCHES "Darwin" )
   set( _DEST "${_DEST}/Audacity.app/Contents" )
   set( _EXEDIR "${_DEST}/MacOS" )
endif()

set( _PREFIX "${CMAKE_INSTALL_PREFIX}" )
set( _LIBDIR "${CMAKE_INSTALL_LIBDIR}/audacity" )
set( _RPATH "\$ORIGIN/../${_LIBDIR}" )
set( _DATADIR "${CMAKE_INSTALL_DATADIR}" )
set( _PKGDATA "${_DATADIR}/audacity/" )
set( _MANDIR "${CMAKE_INSTALL_MANDIR}" )

# Precreate the lib and lib64 directories so we can make then the same
if( NOT EXISTS "${CMAKE_BINARY_DIR}/lib" )
   file( MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/lib" )
endif()

# Only create on systems that need it, effectively excluding Windows where links
# may not work due to insufficient privileges
if( NOT CMAKE_INSTALL_LIBDIR STREQUAL "lib" AND NOT EXISTS "${CMAKE_BINARY_DIR}/lib64" )
   file( CREATE_LINK "${CMAKE_BINARY_DIR}/lib" "${CMAKE_BINARY_DIR}/lib64" SYMBOLIC )
endif()

# Define Audacity's name
if( CMAKE_SYSTEM_NAME MATCHES "Darwin|Windows" )
   set( AUDACITY_NAME "Audacity" )
else()
   set( AUDACITY_NAME "audacity" )
endif()

# Extract the Audacity version information
file(
   STRINGS
      ${topdir}/src/Audacity.h output
   REGEX
      "^#define +AUDACITY_(VERSION|RELEASE|REVISION|MODLEVEL|BUILD_LEVEL) +"
)

# And store as variables
foreach( line ${output} )
   string( REGEX MATCHALL "[A-Za-z0-9_]+" line "${line}" )
   list( GET line 1 name )
   list( GET line 2 value )
   set( ${name} ${value} )
endforeach()

# Create short and full version strings
set( AUDACITY_DIST_VERSION ${AUDACITY_VERSION}.${AUDACITY_RELEASE}.${AUDACITY_REVISION} )
set( AUDACITY_INFO_VERSION ${AUDACITY_VERSION}.${AUDACITY_RELEASE}.${AUDACITY_REVISION}.${AUDACITY_MODLEVEL} )

# Define the version suffix (duplicates what happens in src/Audacity.h)
string( TIMESTAMP __TDATE__ "%Y%m%d" )
if( AUDACITY_BUILD_LEVEL EQUAL 0 )
   set( AUDACITY_SUFFIX "-alpha-${__TDATE__}" )
elseif(  AUDACITY_BUILD_LEVEL EQUAL 1 )
   set( AUDACITY_SUFFIX "-beta-${__TDATE__}" )
else()
   set( AUDACITY_SUFFIX "" )
endif()

# Python is used for the manual and (possibly) message catalogs
find_package( Python3 )
if( Python3_FOUND )
   set( PYTHON "${Python3_EXECUTABLE}" )
elseif( CMAKE_SYSTEM_NAME MATCHES "Windows" )
   nuget_package( pkgdir "python3" "3.7.7" )
   file( TO_NATIVE_PATH "${pkgdir}/tools/python.exe" PYTHON )
endif()

# Add our children
add_subdirectory( "cmake-proxies" )
add_subdirectory( "help" )
add_subdirectory( "images" )
add_subdirectory( "locale" )
add_subdirectory( "modules" )
add_subdirectory( "nyquist" )
add_subdirectory( "plug-ins" )
add_subdirectory( "src" )
add_subdirectory( "scripts" )

if(WIN32)
    if(audacity_target_architecture STREQUAL x86)
        set(CPACK_WIX_UPGRADE_GUID "1A147375-E83F-4406-90CF-98C880DC80A6")
    elseif(audacity_target_architecture STREQUAL x64)
        set(CPACK_WIX_UPGRADE_GUID "80AD0C78-645A-42E9-810E-E19B848A4240")
    elseif(audacity_target_architecture STREQUAL arm64)
        set(CPACK_WIX_UPGRADE_GUID "1B92ECBF-EEC9-4EB5-979E-FD37A2E2F4C8")
    else()
        message(ERROR "Unknown system processor ${TARGET_PROCESSOR_NAME}")
    endif()

    set(CPACK_PACKAGE_NAME "${CMAKE_PROJECT_NAME} ${CMAKE_PROJECT_VERSION} (${audacity_target_architecture})")
    if(TARGET_MSI_NAME)
        set(CPACK_PACKAGE_FILE_NAME "${TARGET_MSI_NAME}")
    else()
        set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${audacity_target_architecture}")
    endif()

    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/win/Inno_Setup_Wizard/audacity_InnoWizard_InfoBefore.rtf")

    set(CPACK_WIX_PRODUCT_ICON "${CMAKE_CURRENT_SOURCE_DIR}/win/audacity.ico")
    set(CPACK_WIX_UI_BANNER "${CMAKE_CURRENT_SOURCE_DIR}/win/wix/banner.bmp")
    set(CPACK_WIX_UI_DIALOG "${CMAKE_CURRENT_SOURCE_DIR}/win/wix/welcome.bmp")

    set(CPACK_WIX_CANDLE_EXTRA_FLAGS -arch ${audacity_target_architecture})
    set(CPACK_WIX_TEMPLATE "${CMAKE_CURRENT_SOURCE_DIR}/win/wix/wix-arm64.template.in")

    set (CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_PACKAGE_NAME}")

    set_property(INSTALL "bin/$<TARGET_FILE_NAME:Audacity>"
        PROPERTY CPACK_START_MENU_SHORTCUTS "Audacity"
    )

    set(CPACK_WIX_PROPERTY_ALLUSERS 1)
endif()

cpack_add_component(applications
    DISPLAY_NAME "Application"
    DESCRIPTION "The Audacity application"
)

cpack_add_component(sdk
    DISPLAY_NAME "SDK"
    DESCRIPTION "The module development SDK"
    DISABLED
    GROUP "Development"
)

cpack_add_component_group(Runtime)

cpack_add_component_group(Development
    DESCRIPTION "Libraries and headers needed to build Audacity modules"
)

#set(CPACK_COMPONENT_GROUP_DEVELOPMENT_PARENT_GROUP "Runtime")
#unset(CPACK_COMPONENT_APPLICATIONS_REQUIRED)
set(CPACK_COMPONENTS_ALL applications sdk Unspecified)

set(CPACK_ALL_INSTALL_TYPES Full Developer)

set(CPACK_COMPONENT_LIBRARIES_INSTALL_TYPES Developer Full)
set(CPACK_COMPONENT_HEADERS_INSTALL_TYPES Developer Full)
set(CPACK_COMPONENT_APPLICATIONS_INSTALL_TYPES Full)

include( CPack )

message( STATUS "CMAKE_C_FLAGS   ${CMAKE_C_FLAGS}" )
message( STATUS "CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS}" )
message( STATUS "CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS}" )

foreach( config ${build_list} )
    string( TOUPPER "${config}" config )

    if (CMAKE_C_FLAGS_${config})
        message( STATUS "CMAKE_C_FLAGS_${config} ${CMAKE_C_FLAGS_${config}}" )
    endif()
    if (CMAKE_CXX_FLAGS_${config})
        message( STATUS "CMAKE_CXX_FLAGS_${config} ${CMAKE_CXX_FLAGS_${config}}" )
    endif()
    if (CMAKE_EXE_LINKER_FLAGS_${config})
        message( STATUS "CMAKE_EXE_LINKER_FLAGS_${config} ${CMAKE_EXE_LINKER_FLAGS_${config}}" )
    endif()
endforeach()


# Uncomment what follows for symbol values.
#[[
   get_cmake_property( _variableNames VARIABLES )
   foreach( _variableName ${_variableNames} )
      message( STATUS "${_variableName}=${${_variableName}}" )
   endforeach()
#]]
#[[
   include( PrintProperties )
   print_properties( TARGET "wxWidgets" )

function(get_all_targets _result _dir)
    get_property(_subdirs DIRECTORY "${_dir}" PROPERTY SUBDIRECTORIES)
    foreach(_subdir IN LISTS _subdirs)
        get_all_targets(${_result} "${_subdir}")
    endforeach()

    get_directory_property(_sub_targets DIRECTORY "${_dir}" BUILDSYSTEM_TARGETS)
    #set(${_result} ${${_result}} ${_sub_targets} PARENT_SCOPE)
    message( "Found targets: ${_sub_targets} in ${_dir}" )
    foreach ( tgt ${_sub_targets})
        print_properties( TARGET ${tgt} )
    endforeach()
endfunction()

get_all_targets( AUD_TARG ".")
#]]

