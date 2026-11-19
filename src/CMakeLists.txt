# Sources
SET (PINGLYMOTION_SRC aboutdialog.cpp
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
              controlindicator.cpp
              controlserver.cpp
              floor.cpp
              iktree.cpp
              llmmesh.cpp
              pmmesh.cpp
              jointgroups.cpp
              propfile.cpp
              main.cpp
              mesh.cpp
              meshmodel.cpp
              paneltitlebar.cpp
              prop.cpp
              propmanager.cpp
              releaseinfo.cpp
              pingmator.cpp
              pingmatorwindow.cpp
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
              pingmatorwindow.ui
              updatenotificationdialog.ui
              usersettingsdialog.ui
              whatsnewdialog.ui)

# Resources
SET (PINGLYMOTION_RSC icons.qrc)

# Add the Windows resource file if applicable
IF (WIN32)
  SET (PINGLYMOTION_SRC ${PINGLYMOTION_SRC} pingmator.rc)
ENDIF (WIN32)

# Compile and link (ui files and resources are handled by AUTOUIC/AUTORCC)
ADD_EXECUTABLE (pingmator ${PINGLYMOTION_SRC} ${PINGLYMOTION_UI} ${PINGLYMOTION_RSC})
TARGET_LINK_LIBRARIES (pingmator ${PINGLYMOTION_QT_LIBS} OpenGL::GL)

# Install pingmator
INSTALL (TARGETS pingmator DESTINATION ${INSTALL_BIN})
