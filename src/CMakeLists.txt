# The core of the program: model, file formats and settings. Built once as a static
# library that the program, the automatic tests and the tools link against.
SET (PINGLYMOTION_CORE_SRC animation.cpp
              undohistory.cpp
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
              scriptlab/rlvreference.cpp
              settings.cpp
              textsize.cpp
              translation.cpp
              usersettings.cpp
              versionnumber.cpp)
ADD_LIBRARY (pinglymotion_core STATIC ${PINGLYMOTION_CORE_SRC})
TARGET_LINK_LIBRARIES (pinglymotion_core PUBLIC ${PINGLYMOTION_QT_LIBS})
# Script Lab Live (issue 292): the folder watch behind lab_watch and the panel
TARGET_SOURCES (pinglymotion_core PRIVATE scriptlab/labwatcher.cpp)
# Script Lab lane 1, job B: the folder is the object (one linkset per folder)
TARGET_SOURCES (pinglymotion_core PRIVATE scriptlab/labfolder.cpp scriptlab/labtint.cpp scriptlab/labprimprops.cpp scriptlab/labrecipe.cpp scriptlab/labweight.cpp scriptlab/labimprove.cpp)
# Script Lab job A4: the channel map (who talks to whom in a run)
TARGET_SOURCES (pinglymotion_core PRIVATE scriptlab/labchannelmap.cpp scriptlab/labchains.cpp)
TARGET_SOURCES (pinglymotion_core PRIVATE scriptlab/labrestrictionchoice.cpp)
# Script Lab lane 1, job F2: the editor's suggestion engine and help texts (QtCore only)
TARGET_SOURCES (pinglymotion_core PRIVATE scriptlab/labsuggest.cpp scriptlab/rlvsuggest.cpp)
# Script Lab step 7 (issue 293): what a run does to the avatar, and the preview the 3D view draws
TARGET_SOURCES (pinglymotion_core PRIVATE scriptlab/labeffects.cpp scriptlab/effectspreview.cpp)
# Script Lab job 7b: the RLV camera limits the 3D view can keep to (uses only the CameraConstraint struct of camera.h)
TARGET_SOURCES (pinglymotion_core PRIVATE scriptlab/cameraoverride.cpp scriptlab/environmentoverride.cpp
                scriptlab/viewmodesoverride.cpp)

# Script Lab: the LSL interpreter (issue 289), QtCore only; the control commands lsl_check and lsl_run
TARGET_SOURCES (pinglymotion_core PRIVATE scriptlab/lsl/ast.cpp scriptlab/lsl/lexer.cpp scriptlab/lsl/parser.cpp scriptlab/lsl/lint.cpp
                scriptlab/lsl/builtins.cpp scriptlab/lsl/interp.cpp scriptlab/lsl/world.cpp scriptlab/lsl/rlvworld.cpp scriptlab/lsl/control.cpp
                scriptlab/lsl/sandbox.cpp scriptlab/lsl/session.cpp scriptlab/lsl/relay.cpp
                scriptlab/lsl/builtins_comm.cpp scriptlab/lsl/builtins_region.cpp scriptlab/lsl/builtins_physics.cpp scriptlab/lsl/builtins_media.cpp scriptlab/lsl/builtins_object.cpp
                scriptlab/lsl/builtins_lists.cpp scriptlab/lsl/builtins_strings.cpp scriptlab/lsl/builtins_math.cpp scriptlab/lsl/builtins_prims.cpp)
# Script Lab lane 1, job G: the LSL-to-SLua converter, level 1 "LSL style" (QtCore only; docs/SLUA_RULES.md)
TARGET_SOURCES (pinglymotion_core PRIVATE scriptlab/lsl/luair.cpp scriptlab/lsl/luaprinter.cpp scriptlab/lsl/sluadata.cpp
                scriptlab/lsl/slualevel1.cpp scriptlab/lsl/lsl2slua.cpp)

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
              figures.cpp
              gltfimport.cpp
              handpose.cpp
              handstab.cpp
              importoptions.cpp
              jointinfo.cpp
              libraryindex.cpp
              librarypanel.cpp
              librarypreview.cpp
              lucideicons.cpp
              mesh.cpp
              motioncleanup.cpp
              motiondescribe.cpp
              motionscript.cpp
              bodydescribe.cpp
              paneltitlebar.cpp
              posedescribe.cpp
              retarget.cpp
              retargetdialog.cpp
              retargetmapping.cpp
              screenshotsheet.cpp
              scriptlab/channelmapview.cpp
              scriptlab/labbeginner.cpp
              scriptlab/labeditor.cpp
              scriptlab/labguideview.cpp
              scriptlab/labhelpview.cpp
              scriptlab/labtree.cpp
              scriptlab/lslhighlighter.cpp
              scriptlab/objectsview.cpp
              scriptlab/labdialogbox.cpp
              scriptlab/labplayer.cpp
              scriptlab/labimproveview.cpp
              scriptlab/labtabs.cpp
              scriptlab/scriptlabpanel.cpp
              scriptlab/scriptlabpanelbeginner.cpp
              scriptlab/scriptlabpanelinput.cpp
              scriptlab/scriptlabpanellive.cpp
              scriptlab/scriptlabpanelview.cpp
              scriptlab/scriptlabpanelwindow.cpp
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
# editing a picked prop as in Second Life (Mockup 2): the handles' arithmetic, the Edit prop card and the Selected prop page
TARGET_SOURCES (pinglymotion_shared PRIVATE propedit.cpp propeditcard.cpp selectedproppanel.cpp)

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
              propsinscene.cpp
              pinglymotion.cpp
              pinglymotionwindow.cpp
              pinglymotionwindowbuild.cpp
              pinglymotionwindowview.cpp
              pinglymotionwindowjoints.cpp
              pinglymotionwindowfile.cpp
              pinglymotionwindowprops.cpp
              pinglymotionwindowplayback.cpp
              pinglymotionwindowundo.cpp
              scriptlabcontroller.cpp
              scriptlabcontrollerview.cpp
              emptystate.cpp
              topbar.cpp
              reasonline.cpp
              setupdialog.cpp
              welcomedialog.cpp
              hintcard.cpp
              helpermode.cpp
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
IF (PINGLYMOTION_WEB)
  # web trial build: Qt's own command also writes the page (PinglyMotion.html) and qtloader.js
  QT_ADD_EXECUTABLE (pinglymotion ${PINGLYMOTION_SRC} ${PINGLYMOTION_UI} ${PINGLYMOTION_RSC})
  TARGET_LINK_LIBRARIES (pinglymotion PRIVATE pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS})
  # the data folder (poses, skeletons, body meshes) goes into PinglyMotion.data, read at /data
  TARGET_LINK_OPTIONS (pinglymotion PRIVATE "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/bin/data@/data")
  # dialogs and message boxes wait with exec(): a browser needs Asyncify for that (doc.qt.io, Qt for WebAssembly)
  TARGET_LINK_OPTIONS (pinglymotion PRIVATE -sASYNCIFY -Os)
ELSE ()
  ADD_EXECUTABLE (pinglymotion ${PINGLYMOTION_SRC} ${PINGLYMOTION_UI} ${PINGLYMOTION_RSC})
  TARGET_LINK_LIBRARIES (pinglymotion pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} OpenGL::GL)
ENDIF ()
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
