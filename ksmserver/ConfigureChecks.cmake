#################################################
#
#  (C) 2014 Timothy Pearson
#  kb9vqf (AT) pearsoncomputing (DOT) net
#
#  Improvements and feedback are welcome
#
#  This file is released under GPL >= 2
#
#################################################

pkg_search_module( LIBICE ice )
check_library_exists( ICE _IceTransNoListen "${LIBICE_LIBRARY_DIRS}" HAVE__ICETRANSNOLISTEN )