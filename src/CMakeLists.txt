# Sources
SET (PINGLYMOTION_SRC aboutcredits.cpp
              aboutdialog.cpp
              animexport.cpp
              animexportdialog.cpp
              animimport.cpp
              animimportconvert.cpp
              animation.cpp
              animationmanager.cpp
              animationview.cpp
              animfile.cpp
              avatarskin.cpp
              auroralink.cpp
              bvh.cpp
              bvhnode.cpp
              camera.cpp
              constants.cpp
              controlapi.cpp
              dingle.cpp
              dinglesave.cpp
              easing.cpp
              easingtiles.cpp
              controlindicator.cpp
              controlserver.cpp
              floor.cpp
              gltfimport.cpp
              gltfimportdialog.cpp
              iktree.cpp
              pmmesh.cpp
              jointgroups.cpp
              jointinfo.cpp
              jointlist.cpp
              propfile.cpp
              retarget.cpp
              retargetdialog.cpp
              main.cpp
              mesh.cpp
              meshmodel.cpp
              paneltitlebar.cpp
              prop.cpp
              propmanager.cpp
              releaseinfo.cpp
              pinglymotion.cpp
              pinglymotionwindow.cpp
              scene.cpp
              settings.cpp
              skinrenderer.cpp
              theme.cpp
              timeline.cpp
              timelineview.cpp
              updatechecker.cpp
              updatenotificationdialog.cpp
              updatersettings.cpp
              usersettings.cpp
              usersettingsdialog.cpp
              versionnumber.cpp
              whatsnewdialog.cpp)

# Interfaces
SET (PINGLYMOTION_UI  aboutdialog.ui
              pinglymotionwindow.ui
              updatenotificationdialog.ui
              usersettingsdialog.ui
              whatsnewdialog.ui)

# Resources
SET (PINGLYMOTION_RSC icons.qrc)

# Add the Windows resource file if applicable
IF (WIN32)
  SET (PINGLYMOTION_SRC ${PINGLYMOTION_SRC} pinglymotion.rc)
ENDIF (WIN32)

# Compile and link (ui files and resources are handled by AUTOUIC/AUTORCC)
ADD_EXECUTABLE (pinglymotion ${PINGLYMOTION_SRC} ${PINGLYMOTION_UI} ${PINGLYMOTION_RSC})
TARGET_LINK_LIBRARIES (pinglymotion ${PINGLYMOTION_QT_LIBS} OpenGL::GL)
# The program file keeps its old name for now (a later rename job changes it)
SET_TARGET_PROPERTIES (pinglymotion PROPERTIES OUTPUT_NAME PinglyMotion)

# Install pinglymotion
INSTALL (TARGETS pinglymotion DESTINATION ${INSTALL_BIN})
