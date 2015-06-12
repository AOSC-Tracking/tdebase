#################################################
#
#  (C) 2010-2011 Serghei Amelian
#  serghei (DOT) amelian (AT) gmail.com
#
#  Improvements and feedback are welcome
#
#  This file is released under GPL >= 2
#
#################################################

set( prefix ${CMAKE_INSTALL_PREFIX} )
if( NOT DEFINED HTDIG_SEARCH_BINARY )
  message( STATUS "Looking for htsearch" )
  find_program( HTDIG_SEARCH_BINARY htsearch
    HINTS
      /srv/www/cgi-bin
      /usr/lib/cgi-bin
      /usr/local/www/cgi-bin
  )
  if( HTDIG_SEARCH_BINARY )
    message( STATUS "Looking for htsearch - ${HTDIG_SEARCH_BINARY}" )
  else( HTDIG_SEARCH_BINARY )
    tde_message_fatal( "htsearch not found!\n Please, install htdig or set HTDIG_SEARCH_BINARY." )
  endif( HTDIG_SEARCH_BINARY )
endif( NOT DEFINED HTDIG_SEARCH_BINARY )

configure_file( khc_docbookdig.pl.in khc_docbookdig.pl @ONLY )
configure_file( khc_htdig.pl.in khc_htdig.pl @ONLY )
configure_file( khc_htsearch.pl.in khc_htsearch.pl @ONLY )

install( FILES
    htdig.desktop man.desktop docbook.desktop
  DESTINATION ${DATA_INSTALL_DIR}/khelpcenter/searchhandlers )

install( PROGRAMS
    ${CMAKE_CURRENT_BINARY_DIR}/khc_htdig.pl
    ${CMAKE_CURRENT_BINARY_DIR}/khc_htsearch.pl
    ${CMAKE_CURRENT_BINARY_DIR}/khc_docbookdig.pl
    khc_beagle_search.pl khc_mansearch.pl  khc_beagle_index.pl
  DESTINATION ${BIN_INSTALL_DIR} )

install( FILES
    htdig_long.html
  DESTINATION ${DATA_INSTALL_DIR}/khelpcenter/searchhandlers/htdig )
