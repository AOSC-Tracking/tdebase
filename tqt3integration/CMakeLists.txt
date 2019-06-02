#################################################
#
#  (C) 2011 Timothy Pearson
#  kb9vqf (AT) pearsoncomputing.net
#
#  Improvements and feedback are welcome
#
#  This file is released under GPL >= 2
#
#################################################


##### configure checks ##########################

include( ConfigureChecks.cmake )


##### subdirectories ############################

add_subdirectory( libqtkde )
add_subdirectory( module )
add_subdirectory( utils )
