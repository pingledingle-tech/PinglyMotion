# The core of the program: model, file formats and settings. Built once as a static
# library that the program, the automatic tests and the tools link against.
SET (PINGLYMOTION_CORE_SRC animation.cpp
              applog.cpp
              animfile.cpp
              avatarskin.cpp
              bvh.cpp
              bvhnode.cpp
              constants.cpp
              dingle.cpp
              dinglesave.cpp
              iktree.cpp
              releaseinfo.cpp
              pmmesh.cpp
              jointgroups.cpp
              propfile.cpp
              scriptlab/rlvbehaviours.cpp
              scriptlab/rlvcommand.cpp
              scriptlab/rlvstate.cpp
              settings.cpp
              usersettings.cpp
              versionnumber.cpp)
ADD_LIBRARY (pinglymotion_core STATIC ${PINGLYMOTION_CORE_SRC})
TARGET_LINK_LIBRARIES (pinglymotion_core PUBLIC ${PINGLYMOTION_QT_LIBS})

# Script Lab: the LSL interpreter (issue 289), QtCore only; the control commands lsl_check and lsl_run
TARGET_SOURCES (pinglymotion_core PRIVATE scriptlab/lsl/ast.cpp scriptlab/lsl/lexer.cpp scriptlab/lsl/parser.cpp
                scriptlab/lsl/builtins.cpp scriptlab/lsl/interp.cpp scriptlab/lsl/world.cpp scriptlab/lsl/control.cpp)

# Sources
SET (PINGLYMOTION_SRC aboutcredits.cpp
              aboutdialog.cpp
              animexport.cpp
              animexportdialog.cpp
              animimport.cpp
              animimportconvert.cpp
              animationmanager.cpp
              animationview.cpp
              animcheck.cpp
              auroralink.cpp
              camera.cpp
              controlapi.cpp
              easing.cpp
              easingtiles.cpp
              controlindicator.cpp
              controlserver.cpp
              floor.cpp
              gltfimport.cpp
              gltfimportdialog.cpp
              jointinfo.cpp
              jointlist.cpp
              retarget.cpp
              importoptions.cpp
              motioncleanup.cpp
              retargetdialog.cpp
              retargetmapping.cpp
              main.cpp
              mesh.cpp
              meshmodel.cpp
              paneltitlebar.cpp
              motiondescribe.cpp
              posedescribe.cpp
              prop.cpp
              propmanager.cpp
              pinglymotion.cpp
              pinglymotionwindow.cpp
              pinglymotionwindowfile.cpp
              pinglymotionwindowprops.cpp
              pinglymotionwindowplayback.cpp
              transportbar.cpp
              scene.cpp
              screenshotsheet.cpp
              scriptlab/scriptlabpanel.cpp
              skinrenderer.cpp
              theme.cpp
              timeline.cpp
              timelineview.cpp
              updatechecker.cpp
              updatenotificationdialog.cpp
              updatersettings.cpp
              usersettingsdialog.cpp
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
TARGET_LINK_LIBRARIES (pinglymotion pinglymotion_core ${PINGLYMOTION_QT_LIBS} OpenGL::GL)
# The program file keeps its old name for now (a later rename job changes it)
SET_TARGET_PROPERTIES (pinglymotion PROPERTIES OUTPUT_NAME PinglyMotion)

# Install pinglymotion
INSTALL (TARGETS pinglymotion DESTINATION ${INSTALL_BIN})
