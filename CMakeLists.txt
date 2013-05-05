#################################################
#
#  (C) 2010-2011 Serghei Amelian
#  serghei (DOT) amelian (AT) gmail.com
#
#  Improvements and feedback are welcome
#
#################################################

cmake_minimum_required( VERSION 2.8 )


##### general package setup #####################

project( kdebase )

set( PACKAGE kdebase )
set( VERSION "3.5.13" )


##### include essential cmake modules ###########

include( FindPkgConfig )
include( CheckIncludeFile )
include( CheckIncludeFiles )
include( CheckCSourceRuns )
include( CheckCXXSourceCompiles )
include( CheckLibraryExists )
include( CheckFunctionExists )
include( CheckStructHasMember )
include( CheckTypeSize )


##### include our cmake modules #################

set( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules" )
include( TDEMacros )


##### setup install paths #######################

include( TDESetupPaths )
tde_setup_paths( )


##### add apidox targets ############

add_custom_target(apidox
  COMMAND "./generate_apidox" "${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}" "${HTML_INSTALL_DIR}" "/usr/share/qt3/doc/html"
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/cmake/")

add_custom_target(install-apidox
  COMMAND "./install_apidox" "${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}" "${HTML_INSTALL_DIR}" "${CMAKE_INSTALL_PREFIX}"
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/cmake/")


##### optional stuff ############################

option( WITH_ALL_OPTIONS "Enable all optional support" OFF )
option( WITH_SASL "Enable SASL support" ${WITH_ALL_OPTIONS} )
option( WITH_LDAP "Enable LDAP support" ${WITH_ALL_OPTIONS} )
option( WITH_SAMBA "Enable SAMBA support" ${WITH_ALL_OPTIONS} )
option( WITH_OPENEXR "Enable openexr support" ${WITH_ALL_OPTIONS} )
option( WITH_XCOMPOSITE "Enable xcomposite support" ${WITH_ALL_OPTIONS} )
option( WITH_XCURSOR "Enable xcursor support" ${WITH_ALL_OPTIONS} )
option( WITH_XFIXES "Enable xfixes support" ${WITH_ALL_OPTIONS} )
option( WITH_XRANDR "Enable xrandr support" ${WITH_ALL_OPTIONS} )
option( WITH_XRENDER "Enable xrender support" ${WITH_ALL_OPTIONS} )
option( WITH_XDAMAGE "Enable xdamage support" ${WITH_ALL_OPTIONS} )
option( WITH_XEXT "Enable xext support" ${WITH_ALL_OPTIONS} )
option( WITH_XTEST "Enable xext support" ${WITH_ALL_OPTIONS} )
option( WITH_XSCREENSAVER "Enable xscreensaver support" ${WITH_ALL_OPTIONS} )
option( WITH_LIBART "Enable libart support (for SVG icons and wallpapers)" ${WITH_ALL_OPTIONS} )
option( WITH_LIBUSB "Enable control of some mouse models through libusb" ${WITH_ALL_OPTIONS} )
option( WITH_LIBRAW1394 "Enable visualization of ieee1394 devices through libraw1394" ${WITH_ALL_OPTIONS} )
option( WITH_SUDO_KDESU_BACKEND "Use sudo as backend for kdesu (default is su)" ${WITH_ALL_OPTIONS} )
option( WITH_PAM "kdm should use PAM" ${WITH_ALL_OPTIONS} )
option( WITH_SHADOW "kdm should use shadow passwords" ${WITH_ALL_OPTIONS} )
option( WITH_XDMCP "xdmcp support for kdm" ${WITH_ALL_OPTIONS} )
option( WITH_XINERAMA "Enable xinerama extension support" ${WITH_ALL_OPTIONS} )
option( WITH_ARTS "Enable aRts support" ${WITH_ALL_OPTIONS} )
option( WITH_I8K "Enable Dell laptop support (ksysguard)" ${WITH_ALL_OPTIONS} )
option( WITH_HAL "Enable HAL support" ${WITH_ALL_OPTIONS} )
option( WITH_KDESKTOP_LOCK_BACKTRACE "Enable backtrace in kdesktop_lock exception handler" ${WITH_ALL_OPTIONS} )


##### user requested modules ####################

option( BUILD_ALL "Build all" OFF )
option( BUILD_L10N "Build l10n" ${BUILD_ALL} )
option( BUILD_PICS "Build pics" ${BUILD_ALL} )
option( BUILD_APPLNK "Build applnk" ${BUILD_ALL} )
option( BUILD_DOC "Build doc" ${BUILD_ALL} )
option( BUILD_LIBKONQ "Build libkonq" ${BUILD_ALL} )
option( BUILD_KCMINIT "Build kcminit" ${BUILD_ALL} )
option( BUILD_KCONTROL "Build kcontrol" ${BUILD_ALL} )
option( BUILD_KWIN "Build kwin" ${BUILD_ALL} )
option( BUILD_KATE "Build kate" ${BUILD_ALL} )
option( BUILD_KONQUEROR "Build konqueror" ${BUILD_ALL} )
option( BUILD_KIOSLAVES "Build kioslaves" ${BUILD_ALL} )
option( BUILD_KICKER "Build kicker" ${BUILD_ALL} )
option( BUILD_KDESKTOP "Build kdesktop" ${BUILD_ALL} )
option( BUILD_KONSOLE "Build konsole" ${BUILD_ALL} )
option( BUILD_KLIPPER "Build klipper" ${BUILD_ALL} )
option( BUILD_KMENUEDIT "Build kmenuedit" ${BUILD_ALL} )
option( BUILD_KDEPRINT "Build kdeprint" ${BUILD_ALL} )
option( BUILD_KPERSONALIZER "Build kpersonalizer" ${BUILD_ALL} )
option( BUILD_KDEPASSWD "Build kdepasswd" ${BUILD_ALL} )
option( BUILD_KTIP "Build ktip" ${BUILD_ALL} )
option( BUILD_KDEEJECT "Build kdeeject" ${BUILD_ALL} )
option( BUILD_KDIALOG "Build kdialog" ${BUILD_ALL} )
option( BUILD_KREADCONFIG "Build kreadconfig" ${BUILD_ALL} )
option( BUILD_KDESU "Build kdesu" ${BUILD_ALL} )
option( BUILD_KDM "Build kdm" ${BUILD_ALL} )
option( BUILD_KSMSERVER "Build ksmserver" ${BUILD_ALL} )
option( BUILD_KSPLASHML "Build ksplashml" ${BUILD_ALL} )
option( BUILD_STARTKDE "Build startkde" ${BUILD_ALL} )
option( BUILD_KFIND "Build kfind" ${BUILD_ALL} )
option( BUILD_DRKONQI "Build drkonqi" ${BUILD_ALL} )
option( BUILD_KDCOP "Build kdcop" ${BUILD_ALL} )
option( BUILD_KDEBUGDIALOG "Build kdebugdialog" ${BUILD_ALL} )
option( BUILD_KNETATTACH "Build knetattach" ${BUILD_ALL} )
option( BUILD_KPAGER "Build kpager" ${BUILD_ALL} )
option( BUILD_KAPPFINDER "Build kappfinder" ${BUILD_ALL} )
option( BUILD_KCHECKPASS "Build kcheckpass" ${BUILD_ALL} )
option( BUILD_KHELPCENTER "Build khelpcenter" ${BUILD_ALL} )
option( BUILD_KHOTKEYS "Build khotkeys" ${BUILD_ALL} )
option( BUILD_KSCREENSAVER "Build kscreensaver" ${BUILD_ALL} )
option( BUILD_KSTART "Build kstart" ${BUILD_ALL} )
option( BUILD_KSYSTRAYCMD "Build ksystraycmd" ${BUILD_ALL} )
option( BUILD_NSPLUGINS "Build nsplugins"  ${BUILD_ALL} )
option( BUILD_KSYSGUARD "Build ksysguard"  ${BUILD_ALL} )
option( BUILD_KXKB "Build kxkb"  ${BUILD_ALL} )
option( BUILD_TSAK "Build tsak"  ${BUILD_ALL} )
option( BUILD_KROOTBACKING "Build krootbacking"  ${BUILD_ALL} )
option( BUILD_TQT3INTEGRATION "Build tqt3integration"  ${BUILD_ALL} )
option( BUILD_CRASHTEST "Build crash test program"  ${BUILD_ALL} )
option( BUILD_PROFILE_SHUTDOWN "Build ksmserver shutdown debugging timing markers"  OFF )


##### set PKG_CONFIG_PATH #######################

set( ENV{PKG_CONFIG_PATH} "${PKG_CONFIG_PATH}:$ENV{PKG_CONFIG_PATH}:${LIB_INSTALL_DIR}/pkgconfig" )


##### configure checks ##########################

include( ConfigureChecks.cmake )


###### global compiler settings #################

add_definitions(
  -DHAVE_CONFIG_H
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TQT_CXX_FLAGS}" )
set( CMAKE_SHARED_LINKER_FLAGS "-Wl,--no-undefined" )
set( CMAKE_MODULE_LINKER_FLAGS "-Wl,--no-undefined" )

check_type_size( "long long" LONG_LONG )


##### kdebase directories #######################

add_subdirectory( kdmlib )

tde_conditional_add_subdirectory( BUILD_L10N l10n )
tde_conditional_add_subdirectory( BUILD_PICS pics )
tde_conditional_add_subdirectory( BUILD_APPLNK applnk )
tde_conditional_add_subdirectory( BUILD_DOC doc )
tde_conditional_add_subdirectory( BUILD_LIBKONQ libkonq )
tde_conditional_add_subdirectory( BUILD_KCMINIT kcminit )
add_subdirectory( kcontrol )
tde_conditional_add_subdirectory( BUILD_KWIN kwin )
tde_conditional_add_subdirectory( BUILD_KATE kate )
tde_conditional_add_subdirectory( BUILD_KONQUEROR konqueror )
tde_conditional_add_subdirectory( BUILD_KIOSLAVES kioslave )
tde_conditional_add_subdirectory( BUILD_KICKER kicker )
tde_conditional_add_subdirectory( BUILD_KDESKTOP kdesktop )
tde_conditional_add_subdirectory( BUILD_KONSOLE konsole )
tde_conditional_add_subdirectory( BUILD_KLIPPER klipper )
tde_conditional_add_subdirectory( BUILD_KMENUEDIT kmenuedit )
tde_conditional_add_subdirectory( BUILD_KDEPRINT kdeprint )
tde_conditional_add_subdirectory( BUILD_KPERSONALIZER kpersonalizer )
tde_conditional_add_subdirectory( BUILD_KDEPASSWD kdepasswd )
tde_conditional_add_subdirectory( BUILD_KTIP ktip )
tde_conditional_add_subdirectory( BUILD_KDEEJECT kdeeject )
tde_conditional_add_subdirectory( BUILD_KDIALOG kdialog )
tde_conditional_add_subdirectory( BUILD_KREADCONFIG kreadconfig )
tde_conditional_add_subdirectory( BUILD_KDESU kdesu )
tde_conditional_add_subdirectory( BUILD_KDM kdm )
tde_conditional_add_subdirectory( BUILD_KSMSERVER ksmserver )
tde_conditional_add_subdirectory( BUILD_KSPLASHML ksplashml )
tde_conditional_add_subdirectory( BUILD_KFIND kfind )
tde_conditional_add_subdirectory( BUILD_DRKONQI drkonqi )
tde_conditional_add_subdirectory( BUILD_KDCOP kdcop )
tde_conditional_add_subdirectory( BUILD_KDEBUGDIALOG kdebugdialog )
tde_conditional_add_subdirectory( BUILD_KNETATTACH knetattach )
tde_conditional_add_subdirectory( BUILD_KPAGER kpager )
tde_conditional_add_subdirectory( BUILD_KAPPFINDER kappfinder )
tde_conditional_add_subdirectory( BUILD_KCHECKPASS kcheckpass )
tde_conditional_add_subdirectory( BUILD_KHELPCENTER khelpcenter )
tde_conditional_add_subdirectory( BUILD_KHOTKEYS khotkeys )
tde_conditional_add_subdirectory( BUILD_KSCREENSAVER kscreensaver )
tde_conditional_add_subdirectory( BUILD_KSTART kstart )
tde_conditional_add_subdirectory( BUILD_KSYSTRAYCMD ksystraycmd )
tde_conditional_add_subdirectory( BUILD_NSPLUGINS nsplugins )
tde_conditional_add_subdirectory( BUILD_KSYSGUARD ksysguard )
tde_conditional_add_subdirectory( BUILD_KXKB kxkb )
tde_conditional_add_subdirectory( BUILD_TSAK tsak )
tde_conditional_add_subdirectory( BUILD_KROOTBACKING krootbacking )
tde_conditional_add_subdirectory( BUILD_TQT3INTEGRATION tqt3integration ) 

##### install startkde & related stuff ##########

if( BUILD_STARTKDE )
  install( PROGRAMS startkde DESTINATION ${BIN_INSTALL_DIR} )
  install( PROGRAMS migratekde3 DESTINATION ${BIN_INSTALL_DIR} )
endif()


##### Determine whether older manpage support is used ##########

if( EXISTS "/usr/sbin/makewhatis" )
  set( WITH_MAKEWHATIS "1" )
  message( STATUS "Selected makewhatis for kio-man on your system" )
else()
  message( STATUS "Selected man-db for kio-man on your system" )
endif()


##### write configure files #####################

configure_file( config.h.cmake config.h @ONLY )
