#################################################
#
#  (C) 2010-2011 Serghei Amelian
#  serghei (DOT) amelian (AT) gmail.com
#
#  Improvements and feedback are welcome
#
#################################################

tde_create_translated_desktop(
  SOURCE
    northafrica.desktop westafrica.desktop southafrica.desktop
    centralafrica.desktop caribbean.desktop oceania.desktop
    centralamerica.desktop northamerica.desktop southamerica.desktop
    easteurope.desktop westeurope.desktop centraleurope.desktop
    northeurope.desktop southeurope.desktop southasia.desktop
    southeastasia.desktop eastasia.desktop centralasia.desktop
    middleeast.desktop
  DESTINATION ${LOCALE_INSTALL_DIR}/l10n
  PO_DIR l10n-desktops
)


file( GLOB _dirs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} * )
foreach( _dir ${_dirs} )
  if( IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${_dir} AND
      NOT (${_dir} STREQUAL ".svn" OR ${_dir} STREQUAL "CMakeFiles") AND
      EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${_dir}/entry.desktop AND
      EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${_dir}/flag.png )
    tde_create_translated_desktop(
      SOURCE ${_dir}/entry.desktop
      DESTINATION ${LOCALE_INSTALL_DIR}/l10n/${_dir}
      PO_DIR l10n-desktops
    )
    install(
      FILES ${_dir}/flag.png
      DESTINATION ${LOCALE_INSTALL_DIR}/l10n/${_dir}
    )
  endif( )
endforeach( )
