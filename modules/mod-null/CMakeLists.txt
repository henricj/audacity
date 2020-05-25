
set( TARGET mod-null )
set( TARGET_ROOT ${CMAKE_CURRENT_SOURCE_DIR} )

message( STATUS "========== Configuring ${TARGET} ==========" )

def_vars()

option( BUILD_MODULE_NULL "Build the mod-null module" )

if( BUILD_MODULE_NULL )
    add_library( ${TARGET} MODULE )
else()
    add_library( ${TARGET} MODULE EXCLUDE_FROM_ALL )
endif()

list( APPEND SOURCES
   PRIVATE
      ${TARGET_ROOT}/ModNullCallback.cpp
      ${TARGET_ROOT}/ModNullCallback.h
)

get_target_property( INCLUDES wxWidgets INTERFACE_INCLUDE_DIRECTORIES )
list( APPEND INCLUDES
   PUBLIC
      ${TARGET_ROOT}
)

get_target_property( DEFINES wxWidgets INTERFACE_COMPILE_DEFINITIONS )
list( APPEND DEFINES
   PRIVATE
      # This is needed until the transition to cmake is complete and
      # the Windows pragmas are removed from ModNullCallback.cpp.
      # Without it, the wxWidgets "debug.h" will define __WXDEBUG__
      # which then causes this module to emit library pragmas for the
      # debug versions of wxWidgets...even if the build is for Release.
      wxDEBUG_LEVEL=0
)

list( APPEND LOPTS
   PRIVATE
      $<$<PLATFORM_ID:Darwin>:-undefined dynamic_lookup>
)

list( APPEND LIBRARIES
   PRIVATE
      Audacity
      $<$<PLATFORM_ID:Windows>:wxWidgets>
)

#set_target_property_all( ${TARGET} LIBRARY_OUTPUT_DIRECTORY "${_DEST}/modules" )
#set_target_properties( ${TARGET}
#   PROPERTIES
#      PREFIX ""
#      FOLDER "lib-src"
#)

organize_source( "${TARGET_ROOT}" "" "${SOURCES}" )
target_sources( ${TARGET} PRIVATE ${SOURCES} )
target_compile_definitions( ${TARGET} PRIVATE ${DEFINES} )
target_include_directories( ${TARGET} PRIVATE ${INCLUDES} )
target_link_options( ${TARGET} PRIVATE ${LOPTS} )
target_link_libraries( ${TARGET} PRIVATE ${LIBRARIES} )

if( BUILD_MODULE_NULL AND NOT "${CMAKE_GENERATOR}" MATCHES "Xcode|Visual Studio*" )
    install(TARGETS ${TARGET}
        RUNTIME DESTINATION bin/modules COMPONENT applications
        LIBRARY DESTINATION sdk/lib COMPONENT sdk)
endif()
