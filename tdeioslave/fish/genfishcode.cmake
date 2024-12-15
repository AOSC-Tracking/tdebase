#################################################
#
#  (C) 2024 Slávek Banko
#  slavek (DOT) banko (AT) axis.cz
#
#  Improvements and feedback are welcome
#
#  This file is released under GPL >= 2
#
#################################################

# check and set variables
if( NOT "${MASTER_CURRENT_SOURCE_DIR}" STREQUAL "" )
  set( CMAKE_CURRENT_SOURCE_DIR "${MASTER_CURRENT_SOURCE_DIR}" )
endif()
if( "${FISH_CODE_SOURCE}" STREQUAL "" )
  set( FISH_CODE_SOURCE "fish.pl" )
endif()
if( "${FISH_CODE_OUTPUT}" STREQUAL "" )
  set( FISH_CODE_OUTPUT "fishcode.h" )
endif()
if( NOT IS_ABSOLUTE ${FISH_CODE_SOURCE} )
  set( FISH_CODE_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/${FISH_CODE_SOURCE}" )
endif()
if( NOT IS_ABSOLUTE ${FISH_CODE_OUTPUT} )
  set( FISH_CODE_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${FISH_CODE_OUTPUT}" )
endif()
if( NOT EXISTS ${FISH_CODE_SOURCE} )
  message( FATAL_ERROR "Source file ${FISH_CODE_SOURCE} not exists!" )
endif()

# load fish code source
file( READ ${FISH_CODE_SOURCE} _fish_code )
string( REGEX REPLACE "[^\n]" "" _fish_len ${_fish_code} )
string( LENGTH "+${_fish_len}" _fish_len )
string( MD5 _fish_md5 "${_fish_code}" )

# prepare for C code
set( _fish_pos 0 )
set( _fish_output "\
#define CHECKSUM \"${_fish_md5}\"
static const char *fishCode(
")
string( REGEX REPLACE "\\\\" "\\\\\\\\" _fish_code "${_fish_code}" )
string( REGEX REPLACE "\"" "\\\\\"" _fish_code "${_fish_code}" )
while( _fish_pos LESS ${_fish_len} )
  # pick line
  string( REGEX REPLACE "^([^\n]*)\n(.*)" "\\1" _fish_line "${_fish_code}" )
  string( REGEX REPLACE "^([^\n]*)\n(.*)" "\\2" _fish_code "${_fish_code}" )
  math( EXPR _fish_pos "${_fish_pos}+1" )

  # skip comments and empty lines
  string( REGEX REPLACE "^[ \t]+" "" _fish_line "${_fish_line}" )
  if( "${_fish_line}" STREQUAL "" OR "${_fish_line}" MATCHES "^# " )
    continue()
  endif()

  # add line to output
  set( _fish_output "${_fish_output}\"${_fish_line}\\n\"\n" )
endwhile()
set( _fish_output "${_fish_output});\n" )

# write code to output file
file( WRITE ${FISH_CODE_OUTPUT} "${_fish_output}" )
