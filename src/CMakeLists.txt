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
              limbpull.cpp
              releaseinfo.cpp
              pmmesh.cpp
              jointgroups.cpp
              propfile.cpp
              scriptlab/rlvbehaviours.cpp
              scriptlab/rlvcommand.cpp
              scriptlab/rlvstate.cpp
              settings.cpp
              textsize.cpp
              translation.cpp
              usersettings.cpp
              versionnumber.cpp)
ADD_LIBRARY (pinglymotion_core STATIC ${PINGLYMOTION_CORE_SRC})
TARGET_LINK_LIBRARIES (pinglymotion_core PUBLIC ${PINGLYMOTION_QT_LIBS})
# Script Lab Live (issue 292): the folder watch behind lab_watch and the panel
TARGET_SOURCES (pinglymotion_core PRIVATE scriptlab/labwatcher.cpp)
# Script Lab step 7 (issue 293): what a run does to the avatar, and the preview the 3D view draws
TARGET_SOURCES (pinglymotion_core PRIVATE scriptlab/labeffects.cpp scriptlab/effectspreview.cpp)
# Script Lab job 7b: the RLV camera limits the 3D view can keep to (uses only the CameraConstraint struct of camera.h)
TARGET_SOURCES (pinglymotion_core PRIVATE scriptlab/cameraoverride.cpp scriptlab/environmentoverride.cpp
                scriptlab/viewmodesoverride.cpp)

# Script Lab: the LSL interpreter (issue 289), QtCore only; the control commands lsl_check and lsl_run
TARGET_SOURCES (pinglymotion_core PRIVATE scriptlab/lsl/ast.cpp scriptlab/lsl/lexer.cpp scriptlab/lsl/parser.cpp scriptlab/lsl/lint.cpp
                scriptlab/lsl/builtins.cpp scriptlab/lsl/interp.cpp scriptlab/lsl/world.cpp scriptlab/lsl/rlvworld.cpp scriptlab/lsl/control.cpp
                scriptlab/lsl/sandbox.cpp scriptlab/lsl/session.cpp)

# Files that several automatic tests need besides the core: built once as a second static library
# (before this, each test compiled its own copy: about 170 repeated compiles, most of the
# Windows build time). The program links it too. Add a file here when two or more tests list it.
SET (PINGLYMOTION_SHARED_SRC
              aiassistant.cpp
              aiconnectdialog.cpp
              animcheck.cpp
              animexport.cpp
              animexportdialog.cpp
              animimport.cpp
              camera.cpp
              controlapi.cpp
              controlindicator.cpp
              dialogfit.cpp
              controlserver.cpp
              easing.cpp
              easingtiles.cpp
              gltfimport.cpp
              handpose.cpp
              handstab.cpp
              importoptions.cpp
              jointinfo.cpp
              lucideicons.cpp
              mesh.cpp
              motioncleanup.cpp
              motiondescribe.cpp
              paneltitlebar.cpp
              posedescribe.cpp
              retarget.cpp
              retargetdialog.cpp
              retargetmapping.cpp
              screenshotsheet.cpp
              scriptlab/objectsview.cpp
              scriptlab/scriptlabpanel.cpp
              scriptlab/scriptlabsection.cpp
              scriptlab/spherecontrols.cpp
              selectedjointpanel.cpp
              theme.cpp
              timeline.cpp
              timelineclip.cpp
              timelinenotes.cpp
              timelinerows.cpp
              timelineview.cpp
              transportbar.cpp
              transportlook.cpp
              viewgizmo.cpp
              viewtoolbar.cpp
)
ADD_LIBRARY (pinglymotion_shared STATIC ${PINGLYMOTION_SHARED_SRC})
TARGET_LINK_LIBRARIES (pinglymotion_shared PUBLIC pinglymotion_core ${PINGLYMOTION_QT_LIBS})

# Sources
SET (PINGLYMOTION_SRC aboutcredits.cpp
              aboutdialog.cpp
              animimportconvert.cpp
              animationmanager.cpp
              animationview.cpp
              auroralink.cpp
              floor.cpp
              gltfimportdialog.cpp
              jointlist.cpp
              main.cpp
              meshmodel.cpp
              prop.cpp
              propmanager.cpp
              pinglymotion.cpp
              pinglymotionwindow.cpp
              pinglymotionwindowfile.cpp
              pinglymotionwindowprops.cpp
              pinglymotionwindowplayback.cpp
              emptystate.cpp
              topbar.cpp
              reasonline.cpp
              setupdialog.cpp
              welcomedialog.cpp
              hintcard.cpp
              camerakeysdialog.cpp
              keysmousedialog.cpp
              scene.cpp
              skinrenderer.cpp
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
TARGET_LINK_LIBRARIES (pinglymotion pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} OpenGL::GL)
# the control server shuts down a socket's sending side itself (controlserver.cpp, endAfterAnswer)
IF (WIN32)
  TARGET_LINK_LIBRARIES (pinglymotion ws2_32)
ENDIF (WIN32)
# Translations (docs/TRANSLATION.md): the English source file is made by lupdate from the code
# (cmake --build <dir> --target update_translations); the program carries the compiled files
# (:/i18n/pinglymotion_<language>.qm). Add a language by adding its .ts file to the list.
IF (Qt6LinguistTools_FOUND)
  SET (PINGLYMOTION_TS_FILES ${CMAKE_SOURCE_DIR}/translations/pinglymotion_en.ts)
  # lupdate reads every source and .ui file of the program, the core library included
  FILE (GLOB_RECURSE PINGLYMOTION_TR_SOURCES CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/*.h ${CMAKE_CURRENT_SOURCE_DIR}/*.ui)
  QT_ADD_TRANSLATIONS (pinglymotion
                       TS_FILES ${PINGLYMOTION_TS_FILES}
                       SOURCES ${PINGLYMOTION_TR_SOURCES}
                       RESOURCE_PREFIX /i18n
                       LUPDATE_OPTIONS -locations none -no-obsolete)
ENDIF ()
# The program file keeps its old name for now (a later rename job changes it)
SET_TARGET_PROPERTIES (pinglymotion PROPERTIES OUTPUT_NAME PinglyMotion)

# Install pinglymotion
INSTALL (TARGETS pinglymotion DESTINATION ${INSTALL_BIN})
