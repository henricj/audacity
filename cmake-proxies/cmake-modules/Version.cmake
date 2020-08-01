# Executed during build (NOT configuration) to create/update the
# RevisionIdent.h header.  It will only update it if there was
# a change in git.

set( template "#define REV_LONG \"%H\"%n#define REV_TIME \"%cd\"%n" )
string( APPEND template "#cmakedefine REV_REPO_URL \"@REV_REPO_URL@\"%n#cmakedefine REV_REPO_BRANCH \"@REV_REPO_BRANCH@\"%n" )
string( APPEND template "#cmakedefine REV_DESCRIBE \"@REV_DESCRIBE@\"%n" )
string( APPEND template "#cmakedefine REV_MASTER_MERGE_BASE \"@REV_MASTER_MERGE_BASE@\"%n" )
string( APPEND template "#cmakedefine REV_DESCRIBE_MERGE_BASE \"@REV_DESCRIBE_MERGE_BASE@\"%n" )

if( WIN32 )
    string( APPEND template "#cmakedefine AUDACITY_WINDOWS_SDK_VERSION \"@AUDACITY_WINDOWS_SDK_VERSION@\"%n" )

    set(AUDACITY_WINDOWS_SDK_VERSION $ENV{WindowsSDKVersion})
    string(REPLACE "\\" "" AUDACITY_WINDOWS_SDK_VERSION "${AUDACITY_WINDOWS_SDK_VERSION}" )  # Sometimes it has trailing slash (!)
endif()


execute_process(
   COMMAND
      ${GIT} show -s "--format=${template}"
   OUTPUT_FILE
      ${_PRVDIR}/RevisionIdent.h.in
   OUTPUT_STRIP_TRAILING_WHITESPACE
   ERROR_QUIET
 )

execute_process(
   COMMAND
      ${GIT} config --get remote.origin.url
   OUTPUT_VARIABLE REV_REPO_URL
   OUTPUT_STRIP_TRAILING_WHITESPACE
   ERROR_QUIET
 )

execute_process(
   COMMAND
      ${GIT} symbolic-ref --short HEAD
   OUTPUT_VARIABLE REV_REPO_BRANCH
   OUTPUT_STRIP_TRAILING_WHITESPACE
   ERROR_QUIET
 )

execute_process(
   COMMAND
      ${GIT} describe --tags --always --dirty --exclude "latest-*" --exclude "build-*"
   OUTPUT_VARIABLE REV_DESCRIBE
   OUTPUT_STRIP_TRAILING_WHITESPACE
   ERROR_QUIET
 )

if( NOT "${REV_REPO_BRANCH}" STREQUAL "master" )
    execute_process(
       COMMAND
          ${GIT} merge-base origin/master HEAD
       OUTPUT_VARIABLE REV_MASTER_MERGE_BASE
       OUTPUT_STRIP_TRAILING_WHITESPACE
       ERROR_QUIET
    )
    if( NOT ${REV_MASTER_MERGE_BASE} STREQUAL "" )
        execute_process(
           COMMAND
              ${GIT} describe --tags --always ${REV_MASTER_MERGE_BASE}
           OUTPUT_VARIABLE REV_DESCRIBE_MERGE_BASE
           OUTPUT_STRIP_TRAILING_WHITESPACE
           ERROR_QUIET
        )
    endif( )
endif( )

message( STATUS "REV_REPO_URL: ${REV_REPO_URL}" )
message( STATUS "REV_REPO_BRANCH: ${REV_REPO_BRANCH}" )
message( STATUS "REV_DESCRIBE: ${REV_DESCRIBE}" )
message( STATUS "REV_MASTER_MERGE_BASE: ${REV_MASTER_MERGE_BASE}" )
message( STATUS "REV_DESCRIBE_MERGE_BASE: ${REV_DESCRIBE_MERGE_BASE}" )
message( STATUS "AUDACITY_WINDOWS_SDK_VERSION: ${AUDACITY_WINDOWS_SDK_VERSION}" )

configure_file( ${_PRVDIR}/RevisionIdent.h.in ${_PRVDIR}/RevisionIdent.h )

