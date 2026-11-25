/***************************************************************************
 *   Copyright (C) 2006 by Zi Ree                                          *
 *   Zi Ree @ SecondLife                                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <QScopedValueRollback>
#include <QActionGroup>
#include <QMap>
#include <QMenu>
#include <QRegularExpression>
#include <QDockWidget>
#include <QScrollArea>
#include <QStackedWidget>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QCloseEvent>
#include <QDir>
#include <QFileDialog>
#include "startfolder.h"
#include "logging.h"
#include <QListWidget>
#include <QMessageBox>
#include <QSaveFile>
#include <QTemporaryDir>
#include <QScopedPointer>
#include <QSignalBlocker>
#include <QSharedPointer>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QCheckBox>
#include <QHeaderView>
#include <QLineEdit>
#include <QTabBar>
#include <QTableWidget>

#include "aboutdialog.h"
#include "animexportdialog.h"
#include "animimportconvert.h"
#include "dinglesave.h"
#include "gltfimport.h"
#include "gltfimportdialog.h"
#include "retargetdialog.h"
#include "animationview.h"
#include "bvh.h"
#include "dingle.h"
#include "easingtiles.h"
#include "propfile.h"
#include "propsinscene.h"
#include "constants.h"
#include "controlapi.h"
#include "controlserver.h"
#include "controlindicator.h"
#include "auroralink.h"
#include "sliderscale.h"
#include <QStatusBar>
#include "prop.h"
#include "jointgroups.h"
#include "jointinfo.h"
#include "jointlist.h"
#include "panellayout.h"
#include "panelwindow.h"
#include "paneltitlebar.h"
#include "pinglymotion.h"
#include "scene.h"
#include "timeline.h"
#include "timelineview.h"
#include "updatersettings.h"
#include "usersettings.h"
#include "usersettingsdialog.h"

#include "panelvisibility.h"
#include "pinglymotionwindow.h"
#include "undohistory.h"
#include "playback.h"
#include "theme.h"
#include "ui_pinglymotionwindow.h"
#include "whatsnewdialog.h"
#include "figures.h"
#include "librarypanel.h"
#include "libraryindex.h"
#include "wheelguard.h"

// What the remote control commands do in the window: the same steps as clicking.

// Part of PinglyMotionWindow: the file menu: new, open, add, save, import, export and the props files.
// (Split out of pinglymotionwindow.cpp; nothing here changes what the window does.)

// The set-up that File > New and File > Open/Add share. A new animation and one
// without loop points start on the first frame that can be edited (the first
// frame is protected when that setting is on) with the loop in point there.
void PinglyMotionWindow::setDefaultLoopIn(Animation* anim)
{
  if(UserSettings::protectFirstFrame())
  {
    // skip first frame, since it's protected anyway
    anim->setFrame(1);
    setCurrentFrame(1);
    setLoopInPoint(2);
  }
  else
  {
    anim->setFrame(0);
    setCurrentFrame(0);
    setLoopInPoint(1);
  }
}

// errors of the model are shown here, not by the model (no box while nobody can click it)
void PinglyMotionWindow::showLoadProblems(Animation* anim)
{
  if(m_quietDialogs) return;
  for(const Animation::LoadProblem& problem : anim->loadProblems())
    QMessageBox::critical(this,problem.title,problem.text);
}

// the window follows the frame of the animation
void PinglyMotionWindow::connectAnimation(Animation* anim)
{
  connect(anim, &Animation::currentFrame, this, &PinglyMotionWindow::setCurrentFrame);
  // its Undo history starts once it is open (src/undohistory.h)
  if(m_undo) m_undo->watch(anim);
}

// Menu action: File / New
bool PinglyMotionWindow::fileNew()
{
  if(!clearOpenFiles()) return false;
  setEmptyMode(false);
  emit clearProps();

  Animation* anim=new Animation(scene->bvh());
  showLoadProblems(anim);

  // set timeline animation first, because ...
  timeline->setAnimation(anim);
  // ... setting animation here will delete all old animations
  scene->setAnimation(anim);
  selectAnimation(anim);

  // add new animation to internal list
  animationIds.append(anim);
  calculateLongestRunningTime();
  // add new animation to combo box
  addToOpenFiles(m_untitledName);

  setDefaultLoopIn(anim);
  setLoopOutPoint(anim->getNumberOfFrames());

  // show frame as unprotected
  emit protectFrame(false);
  protect=false;

  connectAnimation(anim);

  selectTrackInTable(1);

  setPlaystate(PLAYSTATE_STOPPED);

  updateInputs();
  updateFps();

  enableAvatarRotation(false);
  enableAvatarPosition(true);
  enableProps(false);

  anim->setDirty(false);
  return true;
}

// File > Close animation: asks about unsaved changes like New animation does, then shows
// the empty state. A clean untitled animation stays behind it (see m_empty).
void PinglyMotionWindow::fileClose()
{
  // two figures (docs/UX_DECISIONS.md 6m): Close takes the active figure out, as Remove does
  if(animationIds.count()>1)
  {
    removeFigure(activeFigure());
    return;
  }
  if(fileNew()) setEmptyMode(true);
}

QString PinglyMotionWindow::selectFileToOpen(const QString& caption)
{
   //// For some unknown reason passing "this" locks up the OSX pinglymotion window. Possibly a QT4 bug, needs investigation
#ifdef __APPLE__
   QString file = QFileDialog::getOpenFileName(NULL, caption, StartFolder::forDialogs(UserSettings::lastPath()), m_animFilter);
#else
   QString file = QFileDialog::getOpenFileName(this, caption, StartFolder::forDialogs(UserSettings::lastPath()), m_animFilter);
#endif
  if(!file.isEmpty())
  {
    QFileInfo fileInfo(file);
    if(!fileInfo.exists())
    {
      QMessageBox::warning(this,QObject::tr("Load Animation File"),QObject::tr("<qt>Animation file not found:<br />%1</qt>").arg(file));
      file=QString();
    }
    else
      UserSettings::setLastPath(fileInfo.path());
  }

  return file;
}

// Menu action: File / Open ...
void PinglyMotionWindow::fileOpen()
{
  fileOpen(QString());
}

void PinglyMotionWindow::fileOpen(const QString& name)
{
#ifdef Q_OS_WASM
  // web build: the browser's own file picker uploads the file; the bytes open like a dropped file
  // (docs/DEV.md, "Web-ready code", rule 2)
  if(name.isEmpty())
  {
    QFileDialog::getOpenFileContent(m_animFilter,[this](const QString& fileName,const QByteArray& content)
    {
      if(!fileName.isEmpty()) fileOpenData(fileName,content);
    });
    return;
  }
#endif
  QString file=name;

  if(file.isEmpty())
    file=selectFileToOpen(tr("Select Animation File to Load"));

  // an older prop file (.prp) opens its props into the open animation; the animation stays
  if(!file.isEmpty() && PropsInScene::isPropFileName(file))
  {
    loadPropFile(file);
    return;
  }

  if(!file.isEmpty())
  {
    // check the file before closing the open ones
    if(!checkAnimationFile(file)) return;
    if(!clearOpenFiles()) return;
    emit clearProps();
    addAnimationFile(file);
    showPropsOfOpenedFile();
  }

  // update timeline and animation view with the currently selected body part
  // this helps to sync the visual selections
  partChoice();
}

void PinglyMotionWindow::fileOpenData(const QString& file,const QByteArray& data)
{
  if(file.isEmpty() || !checkAnimationFile(file,&data)) return;
  if(!clearOpenFiles()) return;
  emit clearProps();
  addAnimationFile(file,&data);
  showPropsOfOpenedFile();
  partChoice();
}

// Menu action: File / Add New Animation ...
void PinglyMotionWindow::fileAdd()
{
  fileAdd(QString());
}

void PinglyMotionWindow::fileAdd(const QString& name)
{
  QString file=name;

  if(file.isEmpty())
    file=selectFileToOpen(tr("Select Animation File to Add"));

  if(!file.isEmpty() && checkAnimationFile(file))
    addAnimationFile(file);
}

// shows why when the file cannot be opened; the reason is kept for remote control
bool PinglyMotionWindow::checkAnimationFile(const QString& file,const QByteArray* data)
{
  m_lastOpenError.clear();
  if(!data && !QFile::exists(file))
    m_lastOpenError=tr("Animation file not found.");
  else
    m_lastOpenError=BVH::checkFile(file,data).error;

  if(m_lastOpenError.isEmpty()) return true;
  if(m_quietDialogs) return false;
  QMessageBox::warning(this,tr("Load Animation File"),
                       tr("<qt>%1<br><br>%2</qt>").arg(file.toHtmlEscaped(),m_lastOpenError.toHtmlEscaped()));
  return false;
}

// adds a file that checkAnimationFile() has accepted
void PinglyMotionWindow::addAnimationFile(const QString& file,const QByteArray* data)
{
  setEmptyMode(false);
  {
    // an old .avm file shows up under its new name, <same name>.dingle, and counts as changed
    const bool oldAvm=QFileInfo(file).suffix().toLower()==QLatin1String("avm");
    const QString shownName=oldAvm ? DingleSave::suggestedName(file,m_untitledName) : file;
    if(oldAvm) m_avmOrigin.insert(shownName,file);
    addToOpenFiles(shownName);
    Animation* anim=new Animation(scene->bvh(),file,data);
    showLoadProblems(anim);
    animationIds.append(anim);
    if(!m_quietDialogs && (scene->bvh()->lastLoadedIgnoredKeys>0 || scene->bvh()->lastLoadedNewerVersion))
      QMessageBox::information(this,tr("Open Animation File"),
                               tr("This file comes from a newer version of PinglyMotion.\nSome parts of it are not shown here.\nSaving it will remove those parts. Save under another name to keep the original."));
    calculateLongestRunningTime();

    setCurrentFile(shownName);

    scene->addAnimation(anim);
    timeline->setAnimation(anim);
    selectAnimation(anim);

    // no loop in point? must be a BVH or an older avm. set a sane default
    if(anim->getLoopInPoint()==-1)
    {
      // first set loop out point to avoid clamping of loop in point
      setLoopOutPoint(anim->getNumberOfFrames());

      setDefaultLoopIn(anim);
    }
    else
    {
      setLoopInPoint(anim->getLoopInPoint()+1);
      setLoopOutPoint(anim->getLoopOutPoint()+1);
    }

    connectAnimation(anim);

    ui->animationView->selectPart(m_selectedTrack);
    updateInputs();
    updateFps();
    anim->setDirty(oldAvm);


  }
}

// Menu Action: File / Save
// Save writes .dingle only. A new animation, an opened .avm and an opened .bvh
// ask for a .dingle name first. Returns true when the animation was saved.
bool PinglyMotionWindow::fileSave()
{
#ifdef Q_OS_WASM
  // web build: every save is a download (fileSaveAs)
  return fileSaveAs();
#endif
  if(DingleSave::needsName(currentFile,m_untitledName))
    return fileSaveAs();

  // an opened .avm is saved as .dingle with the same name; the old file goes away after that worked
  const QString avmFile=m_avmOrigin.value(currentFile);
  if(!avmFile.isEmpty() && !checkFileOverwrite(QFileInfo(currentFile))) return false;

  QString error;
  storePropsInAnimation();
  if(!scene->getAnimation()->saveBVH(currentFile,&error))
  {
    QMessageBox::warning(this,tr("Save Animation File"),error);
    return false;
  }
  if(!avmFile.isEmpty()) replaceOldAvm(avmFile,currentFile);
  return true;
}

// Menu Action: File / Save As...
// returns true when the animation was saved
bool PinglyMotionWindow::fileSaveAs()
{
  QString suggested=DingleSave::suggestedName(currentFile,m_untitledName);
  if(QFileInfo(suggested).isRelative())
    suggested=QDir(UserSettings::lastPath()).filePath(suggested);
  QString selectedFilter=m_dingleFilter;

#ifdef Q_OS_WASM
  // web build: written to the browser's memory file system, then the browser downloads it
  // (end of this function; docs/DEV.md, "Web-ready code", rule 2)
  QString file=QDir(QDir::tempPath()).filePath(QFileInfo(suggested).fileName());
#else
   //// For some unknown reason passing "this" locks up the OSX pinglymotion window. Possibly a QT4 bug, needs investigation
#ifdef __APPLE__
   QString file = QFileDialog::getSaveFileName(NULL, tr("Save Animation File"), suggested, m_dingleFilter, &selectedFilter, QFileDialog:: DontConfirmOverwrite);
#else
   QString file = QFileDialog::getSaveFileName(this, tr("Save Animation File"), suggested, m_dingleFilter, &selectedFilter, QFileDialog:: DontConfirmOverwrite);
#endif
#endif

  if(file.isEmpty()) return false;
  file=DingleSave::withDingleEnding(file);

  QFileInfo fileInfo(file);

#ifndef Q_OS_WASM
  // if the file didn't exist yet or the user accepted to overwrite it, save it.
  if(!checkFileOverwrite(fileInfo)) return false;
#endif

  const QString oldFile=m_avmOrigin.value(currentFile,currentFile);
  QString error;
  storePropsInAnimation();
  if(!scene->getAnimation()->saveBVH(file,&error))
  {
    QMessageBox::warning(this,tr("Save Animation File"),error);
    return false;
  }
  m_avmOrigin.remove(currentFile);
  setCurrentFile(file);
  UserSettings::setLastPath(fileInfo.path());
  // update animation selector combo box
  ui->selectAnimationCombo->setItemText(ui->selectAnimationCombo->currentIndex(),fileInfo.baseName());
  openFiles[ui->selectAnimationCombo->currentIndex()]=file;
  ui->fileExportForSecondLifeAction->setEnabled(true);

#ifdef Q_OS_WASM
  QFile saved(file);
  if(!saved.open(QIODevice::ReadOnly))
  {
    QMessageBox::warning(this,tr("Save Animation File"),saved.errorString());
    return false;
  }
  QFileDialog::saveFileContent(saved.readAll(),fileInfo.fileName());
#else
  // an old .avm file is replaced: it goes to the recycle bin, now that the new file is saved
  if(DingleSave::replacesOldFile(oldFile,file)) replaceOldAvm(oldFile,file);
#endif
  return true;
}

void PinglyMotionWindow::replaceOldAvm(const QString& avmFile,const QString& newFile)
{
  const QString title=tr("Save Animation File");
  m_avmOrigin.remove(newFile);
  if(!QFileInfo::exists(avmFile)) return;

  // read the new file back and compare it with the animation before the old file is touched
  QFile saved(newFile);
  bool same=false;
  if(saved.open(QIODevice::ReadOnly) && saved.size()<=Dingle::MAX_FILE_BYTES)
  {
    const Dingle::Result read=Dingle::parse(saved.readAll());
    same=read.ok() && DingleSave::sameAnimation(*read.animation,scene->bvh()->snapshot(scene->getAnimation(),AnimFile::Format::Avm));
  }
  if(!same)
  {
    QMessageBox::warning(this,title,tr("Your animation was saved, but the new file did not pass the check.\nThe old file %1 was left where it is.").arg(QFileInfo(avmFile).fileName()));
    return;
  }
  if(!QFile::moveToTrash(avmFile))
    QMessageBox::information(this,title,tr("Your animation was saved.\nThe old file %1 could not be moved to the recycle bin.\nIt is still there. You can delete it yourself.").arg(QFileInfo(avmFile).fileName()));
}

// Menu Action: File / Export For Second Life
void PinglyMotionWindow::fileExportForSecondLife()
{
  // FIXME: think of a sensible thing to do when the animation has not been saved
  //        as .avm yet
  if (currentFile != m_untitledName)
  {
    QFileInfo fileInfo(currentFile);
    QString exportName=fileInfo.path()+"/"+fileInfo.completeBaseName()+".bvh";

    qCDebug(lcFile,"fileExportForSecondLife(): exporting animation as '%s'.",exportName.toLatin1().constData());
    QString error;
    if(!scene->getAnimation()->saveBVH(exportName,&error))
    {
      QMessageBox::warning(this,QObject::tr("Export for Second Life"),error);
      return;
    }
    QMessageBox::information(this,QObject::tr("Export for Second Life"),QObject::tr("Animation was exported for Second Life as:\n%1").arg(exportName));
  }
}

// Menu Action: File / Export as Second Life .anim...
// Save stays for .dingle files; this writes the file that Second Life uploads.
void PinglyMotionWindow::fileExportAnim()
{
  const QString title=tr("Export as Second Life .anim");
  Animation* animation=scene->getAnimation();
  if(!animation)
  {
    QMessageBox::information(this,title,AnimExportUi::problemBeforeExport(AnimFile::Animation()));
    return;
  }

  const AnimFile::Animation model=scene->bvh()->snapshot(animation,AnimFile::Format::Bvh);
  const QString problem=AnimExportUi::problemBeforeExport(model);
  if(!problem.isEmpty())
  {
    QMessageBox::warning(this,title,problem);
    return;
  }

  AnimExportDialog dialog(AnimExportUi::durationSeconds(model),this);
  if(dialog.exec()!=QDialog::Accepted) return;

  QString suggested=AnimExportUi::suggestedFileName(currentFile,m_untitledName);
  if(QFileInfo(suggested).isRelative())
    suggested=QDir(UserSettings::lastPath()).filePath(suggested);
  QString file=QFileDialog::getSaveFileName(this,title,suggested,tr("Second Life animation (*.anim)"),nullptr,QFileDialog::DontConfirmOverwrite);
  if(file.isEmpty()) return;
  file=AnimExportUi::withAnimEnding(file);
  const QFileInfo fileInfo(file);
  if(!checkFileOverwrite(fileInfo)) return;

  const AnimExport::Options options=AnimExportUi::toOptions(dialog.settings(),model.frameTime,
                                                           animation->getLoopInPoint(),animation->getLoopOutPoint());
  const AnimExport::Result result=AnimExport::write(model,options);
  if(!result.ok())
  {
    QMessageBox::warning(this,title,AnimExportUi::explainFailure(result.error));
    return;
  }

  QSaveFile out(file);
  if(!out.open(QIODevice::WriteOnly) || out.write(result.bytes)!=result.bytes.size() || !out.commit())
  {
    QMessageBox::warning(this,title,AnimExportUi::explainFailure(
                         tr("The file could not be written: %1").arg(out.errorString())));
    return;
  }
  UserSettings::setLastPath(fileInfo.path());

  QString message=tr("Saved as:\n%1").arg(fileInfo.fileName());
  if(!result.warnings.isEmpty())
    message+=QStringLiteral("\n\n")+tr("Please note:")+QStringLiteral("\n")+result.warnings.join(QLatin1Char('\n'));
  QMessageBox::information(this,title,message);
}

// reads the Second Life skeleton that imports are put on
bool PinglyMotionWindow::loadSkeleton(AnimFile::Animation& skeleton,const QString& title)
{
  QFile file(Constants::femaleBvhPath());
  AnimFile::Result parsed;
  if(file.open(QIODevice::ReadOnly) && file.size()<=AnimFile::MAX_FILE_BYTES)
    parsed=AnimFile::parseBvh(file.readAll());
  if(!parsed.ok())
  {
    QMessageBox::warning(this,title,tr("The skeleton of the program could not be read.\nReinstall the program, then try again."));
    return false;
  }
  skeleton=*parsed.animation;
  return true;
}

// opens an animation made from another file as a new animation that has no file yet
bool PinglyMotionWindow::openConvertedAnimation(const AnimFile::Animation& anim,const QString& name,int loopIn,int loopOut,const QString& title)
{
  // the program opens animations from files, so the data goes through a short-lived one
  QTemporaryDir folder;
  const QString path=folder.filePath(QStringLiteral("imported.bvh"));
  QSaveFile out(path);
  const QByteArray bytes=AnimFile::write(anim);
  if(!folder.isValid() || !out.open(QIODevice::WriteOnly) || out.write(bytes)!=bytes.size() || !out.commit())
  {
    QMessageBox::warning(this,title,tr("The animation could not be opened.\nFree some disk space, then try again."));
    return false;
  }

  if(!checkAnimationFile(path)) return false;
  if(!clearOpenFiles()) return false;
  emit clearProps();
  addAnimationFile(path);

  // the animation belongs to no file: Save asks for a name
  setCurrentFile(m_untitledName);
  const int index=ui->selectAnimationCombo->currentIndex();
  if(index>=0 && index<openFiles.size())
  {
    openFiles[index]=m_untitledName;
    ui->selectAnimationCombo->setItemText(index,name);
  }
  Animation* animation=scene->getAnimation();
  if(loopIn>0 && loopOut>0)
  {
    setLoopOutPoint(loopOut);
    setLoopInPoint(loopIn);
  }
  animation->setDirty(true);
  ui->fileExportForSecondLifeAction->setEnabled(false);
  partChoice();
  return true;
}

// Menu Action: File / Import Second Life .anim...
void PinglyMotionWindow::fileImportAnim()
{
  const QString title=tr("Import Second Life .anim");
  const QString file=QFileDialog::getOpenFileName(this,title,StartFolder::forDialogs(UserSettings::lastPath()),tr("Second Life animation (*.anim)"));
  if(file.isEmpty()) return;
  UserSettings::setLastPath(QFileInfo(file).path());

  QFile input(file);
  if(!input.open(QIODevice::ReadOnly))
  {
    QMessageBox::warning(this,title,tr("This file could not be opened.\n%1\nCheck that the file is still there, then try again.").arg(input.errorString()));
    return;
  }
  if(input.size()>AnimImport::MAX_FILE_BYTES)
  {
    QMessageBox::warning(this,title,tr("This file is too big for an animation.\nPick a Second Life .anim file, then try again."));
    return;
  }
  const QByteArray bytes=input.read(AnimImport::MAX_FILE_BYTES+1);

  AnimFile::Animation skeleton;
  if(!loadSkeleton(skeleton,title)) return;

  const AnimImportConvert::Result result=AnimImportConvert::fromBytes(bytes,skeleton);
  if(!result.ok())
  {
    QMessageBox::warning(this,title,result.error);
    return;
  }
  if(!openConvertedAnimation(*result.animation,QFileInfo(file).completeBaseName(),result.loopIn,result.loopOut,title)) return;

  QString message=tr("Opened %1.").arg(QFileInfo(file).fileName());
  if(!result.notes.isEmpty())
    message+=QStringLiteral("\n\n")+tr("What could not be imported:")+QStringLiteral("\n- ")+result.notes.join(QStringLiteral("\n- "));
  QMessageBox::information(this,title,message);
}

// Menu Action: File / Import BVH from another skeleton...
void PinglyMotionWindow::fileImportBvh()
{
  const QString title=tr("Import a BVH from another skeleton");
  const QString file=QFileDialog::getOpenFileName(this,title,StartFolder::forDialogs(UserSettings::lastPath()),tr("BVH files (*.bvh)"));
  if(file.isEmpty()) return;
  UserSettings::setLastPath(QFileInfo(file).path());

  QFile input(file);
  if(!input.open(QIODevice::ReadOnly))
  {
    QMessageBox::warning(this,title,tr("This file could not be opened.\n%1\nCheck that the file is still there, then try again.").arg(input.errorString()));
    return;
  }
  if(input.size()>AnimFile::MAX_FILE_BYTES)
  {
    QMessageBox::warning(this,title,tr("This file is too big for an animation.\nPick a smaller BVH file, then try again."));
    return;
  }
  const AnimFile::Result parsed=AnimFile::parseBvh(input.read(AnimFile::MAX_FILE_BYTES+1));
  if(!parsed.ok())
  {
    const QString where=parsed.line>0 ? tr("Line %1: %2").arg(parsed.line).arg(parsed.error) : parsed.error;
    QMessageBox::warning(this,title,tr("This file could not be read.\n%1\nCheck that it is a BVH file, then try again.").arg(where));
    return;
  }

  AnimFile::Animation skeleton;
  if(!loadSkeleton(skeleton,title)) return;

  RetargetDialog dialog(*parsed.animation,skeleton,QFileInfo(file).fileName(),
                        [this](const QString& name){ return m_jointInfo ? m_jointInfo->friendlyName(name) : QString(); },this);
  if(dialog.exec()!=QDialog::Accepted || !dialog.result().ok()) return;

  openConvertedAnimation(*dialog.result().animation,QFileInfo(file).completeBaseName(),-1,-1,title);
}

// Menu Action: File / Import glTF animation...
void PinglyMotionWindow::fileImportGltf()
{
  const QString title=tr("Import glTF animation");
  const QString file=QFileDialog::getOpenFileName(this,title,StartFolder::forDialogs(UserSettings::lastPath()),tr("glTF files (*.glb *.gltf)"));
  if(file.isEmpty()) return;
  UserSettings::setLastPath(QFileInfo(file).path());

  QFile input(file);
  if(!input.open(QIODevice::ReadOnly))
  {
    QMessageBox::warning(this,title,tr("This file could not be opened.\n%1\nCheck that the file is still there, then try again.").arg(input.errorString()));
    return;
  }
  if(input.size()>GltfImport::MAX_FILE_BYTES)
  {
    QMessageBox::warning(this,title,tr("This file is too big for an animation.\nPick a smaller file, then try again."));
    return;
  }
  const GltfImport::ReadResult read=GltfImport::read(input.read(GltfImport::MAX_FILE_BYTES+1));
  if(!read.ok())
  {
    QMessageBox::warning(this,title,GltfUi::readProblem(read.error));
    return;
  }

  GltfImport::Options options;
  if(GltfUi::needsChoice(*read.document))
  {
    GltfImportDialog choose(*read.document,this);
    if(choose.exec()!=QDialog::Accepted) return;
    options.animation=choose.animation();
    options.skin=choose.skeleton();
  }

  const GltfImport::SourceResult source=GltfImport::toSource(*read.document,options);
  if(!source.ok())
  {
    QMessageBox::warning(this,title,GltfUi::animationProblem(source.error));
    return;
  }

  AnimFile::Animation skeleton;
  if(!loadSkeleton(skeleton,title)) return;

  RetargetDialog dialog(*source.animation,skeleton,QFileInfo(file).fileName(),
                        [this](const QString& name){ return m_jointInfo ? m_jointInfo->friendlyName(name) : QString(); },this);
  dialog.setNotes(source.notes);
  if(dialog.exec()!=QDialog::Accepted || !dialog.result().ok()) return;

  QString name=QFileInfo(file).completeBaseName();
  if(read.document->animations.size()>1 && options.animation<read.document->animations.size() && !read.document->animations[options.animation].name.isEmpty())
    name+=QStringLiteral(" - ")+read.document->animations[options.animation].name;
  openConvertedAnimation(*dialog.result().animation,name,-1,-1,title);
}

// Menu Action: File / Load Props...
void PinglyMotionWindow::fileLoadProps()
{
   //// For some unknown reason passing "this" locks up the OSX pinglymotion window. Possibly a QT4 bug, needs investigation
#ifdef __APPLE__
   QString fileName = QFileDialog::getOpenFileName(nullptr, QString(), UserSettings::lastPath(), m_propFilter);
#else
   QString fileName = QFileDialog::getOpenFileName(this, QString(), UserSettings::lastPath(), m_propFilter);
#endif

  if(fileName.isEmpty()) return;
  loadPropFile(fileName);
}

bool PinglyMotionWindow::loadPropFile(const QString& fileName)
{
  // read and check the whole file first, so a bad file leaves the props as they are
  const PropsInScene::OpenResult read=PropsInScene::readPropFile(fileName);
  if(!read.ok())
  {
    m_lastOpenError=read.error;
    if(!m_quietDialogs)
      QMessageBox::warning(this,tr("Load Props"),tr("<qt>%1<br><br>%2</qt>").arg(fileName.toHtmlEscaped(),read.error.toHtmlEscaped()));
    return false;
  }
  showProps(read.props);
  return true;
}

void PinglyMotionWindow::showProps(const QList<PropRecord>& records)
{
  emit clearProps();
  for(const PropRecord& record : records)
  {
    QSharedPointer<Mesh> mesh;
    switch(static_cast<Prop::PropType>(record.type))
    {
      case Prop::PropType::Box:    mesh=cubeMesh;   break;
      case Prop::PropType::Sphere: mesh=sphereMesh; break;
      case Prop::PropType::Cone:   mesh=coneMesh;   break;
      case Prop::PropType::Torus:  mesh=torusMesh;  break;
    }

    const Prop* prop=PropsInScene::addRecord(*scene->propManager(),record,mesh);
    emit scene->repaint();
    if(prop)
    {
      ui->propNameCombo->addItem(prop->name());
      ui->propNameCombo->setCurrentIndex(ui->propNameCombo->count()-1);
      selectProp();
    }
  }
}

void PinglyMotionWindow::showPropsOfOpenedFile()
{
  const Animation* anim=scene->getAnimation();
  if(anim && !anim->props().isEmpty()) showProps(anim->props());
}

void PinglyMotionWindow::storePropsInAnimation()
{
  if(Animation* anim=scene->getAnimation()) anim->setProps(PropsInScene::recordsOf(*scene->propManager()));
}

// Menu Action: File / Save Props...
void PinglyMotionWindow::fileSaveProps()
{
   //// For some unknown reason passing "this" locks up the OSX pinglymotion window. Possibly a QT4 bug, needs investigation
#ifdef __APPLE__
   QString fileName = QFileDialog::getSaveFileName(NULL, tr("Save Props"), currentFile, m_propFilter);
#else
   QString fileName = QFileDialog::getSaveFileName(this, tr("Save Props"), currentFile, m_propFilter);
#endif

  if(!fileName.isEmpty())
  {
    QFileInfo fileInfo(fileName);
    // make sure file has proper extension (.prp)
    if(fileInfo.suffix().toLower()!="prp")
      fileName+=".prp";

    // check if file exists
    // Obsolete for Qt 4.x
    if(!checkFileOverwrite(fileInfo)) return;

    // the whole file in one go (PropFile::write; docs/DEV.md "Web-ready code"), the dialog only names it
    const QList<PropRecord> records=PropsInScene::recordsOf(*scene->propManager());
    QFile file(fileName);
    if(file.open(QIODevice::WriteOnly))
      file.write(PropFile::write(records));
  }
}

// Menu Action: File / Exit
void PinglyMotionWindow::fileExit()
{
  if(!clearOpenFiles())
    return;

  saveUserSettings();

  // remove all widgets and close the main form
  qApp->exit(0);
}

// checks if a file already exists at the given path and displays a warning message
// returns true if it's ok to save/overwrite, else returns false
bool PinglyMotionWindow::checkFileOverwrite(const QFileInfo& fileInfo)
{
  // get file info
  if(fileInfo.exists())
  {
    int answer=QMessageBox::question(this,tr("File Exists"),tr("A file with the name \"%1\" does already exist. Do you want to overwrite it?").arg(fileInfo.fileName()),QMessageBox::Yes|QMessageBox::No);
    if(answer==QMessageBox::No) return false;
  }
  return true;
}

void PinglyMotionWindow::on_fileNewAction_triggered()
{
  fileNew();
}

void PinglyMotionWindow::on_fileCloseAction_triggered()
{
  fileClose();
}

void PinglyMotionWindow::on_fileOpenAction_triggered()
{
  fileOpen();
}

// File > Add a second figure... (Expert mode only, docs/UX_DECISIONS.md 6m)
void PinglyMotionWindow::on_fileAddAction_triggered()
{
  addSecondFigure();
}

void PinglyMotionWindow::on_fileSaveAction_triggered()
{
  fileSave();
}

void PinglyMotionWindow::on_fileSaveAsAction_triggered()
{
  fileSaveAs();
}

void PinglyMotionWindow::on_fileImportGltfAction_triggered()
{
  fileImportGltf();
}

void PinglyMotionWindow::on_fileImportBvhAction_triggered()
{
  fileImportBvh();
}

void PinglyMotionWindow::on_fileImportAnimAction_triggered()
{
  fileImportAnim();
}

void PinglyMotionWindow::on_fileExportAnimAction_triggered()
{
  fileExportAnim();
}

void PinglyMotionWindow::on_fileExportForSecondLifeAction_triggered()
{
  fileExportForSecondLife();
}

void PinglyMotionWindow::on_fileLoadPropsAction_triggered()
{
  fileLoadProps();
}

void PinglyMotionWindow::on_fileSavePropsAction_triggered()
{
  fileSaveProps();
}

void PinglyMotionWindow::on_fileExitAction_triggered()
{
  fileExit();
}

// ---- Two figures in one scene, Expert mode only (docs/UX_DECISIONS.md 6m)

int PinglyMotionWindow::activeFigure() const
{
  return animationIds.indexOf(scene->getAnimation());
}

void PinglyMotionWindow::addSecondFigure()
{
  const QString why=FigureSpec::addBlocked(!m_empty,animationIds.count());
  if(!UserSettings::expertMode() || !why.isEmpty())
  {
    if(!why.isEmpty()) statusBar()->showMessage(why,5000);
    return;
  }
  AddFigureDialog dialog(this);
  dialog.exec();
  switch(dialog.choice())
  {
    case AddFigureDialog::Choice::Library:
      // the library panel opens at its list; Add as a second figure does the rest
      if(m_libraryDock)
      {
        m_libraryDock->show();
        m_libraryDock->raise();
      }
      if(m_libraryPanel && m_libraryPanel->list()) m_libraryPanel->list()->setFocus();
      statusBar()->showMessage(FigureSpec::tr(FigureSpec::pickFromLibrary),10000);
      break;
    case AddFigureDialog::Choice::File:
    {
      const QString file=selectFileToOpen(FigureSpec::tr(FigureSpec::dialogTitle));
      if(!file.isEmpty() && checkAnimationFile(file)) addFigureFile(file);
      break;
    }
    case AddFigureDialog::Choice::Cancel:
      break;
  }
}

bool PinglyMotionWindow::addFigureFile(const QString& fileName,const QByteArray* data)
{
  if(!FigureSpec::addBlocked(!m_empty,animationIds.count()).isEmpty()) return false;
  // the new figure stands at the first free place, so the figure already there stays put
  QList<int> used;
  for(Animation* anim : animationIds) used << m_figurePlace.value(anim,0);
  const int place=FigureSpec::freePlace(used);
  const int before=animationIds.count();
  addAnimationFile(fileName,data);
  if(animationIds.count()!=before+1) return false;
  Animation* added=animationIds.last();
  m_figurePlace.insert(added,place);
  ui->animationView->setFigureOffset(added,FigureSpec::placeOffset(place));
  refreshFigures();
  m_announcedFigure=activeFigure();
  statusBar()->showMessage(FigureSpec::addedMessage(animationIds.count(),openFiles.last()),10000);
  // the view zooms out so both fit, once the new figure has been drawn
  QTimer::singleShot(0,this,[this]()
  {
    ui->animationView->repaint();
    ui->animationView->frameWholeFigure();
    placeFigureTags();
  });
  return true;
}

void PinglyMotionWindow::addLibraryClipAsFigure(const QString& clipId)
{
  if(!m_libraryPanel || !UserSettings::expertMode()) return;
  const LibraryClip* clip=m_libraryPanel->index().find(clipId);
  if(!clip) return;
  const QString why=FigureSpec::addBlocked(!m_empty,animationIds.count());
  if(!why.isEmpty())
  {
    statusBar()->showMessage(why,5000);
    return;
  }
  const QString path=m_libraryPanel->index().absolutePath(*clip);
  if(!checkAnimationFile(path))
  {
    statusBar()->showMessage(m_lastOpenError,10000);
    return;
  }
  addFigureFile(path);
}

void PinglyMotionWindow::makeFigureActive(int index)
{
  if(index<0 || index>=animationIds.count() || index==activeFigure()) return;
  animationChanged(index);
  refreshFigures();
}

bool PinglyMotionWindow::removeFigure(int index)
{
  if(index<0 || index>=animationIds.count() || animationIds.count()<2) return false;
  Animation* anim=animationIds.at(index);
  if(anim->dirty() && !m_quietDialogs)
  {
    QScopedPointer<QMessageBox> box(FigureRemove::box(this,index+1,openFiles.at(index)));
    box->exec();
    const FigureRemove::Choice choice=FigureRemove::choiceOf(box.data());
    if(choice==FigureRemove::Choice::Cancel) return false;
    if(choice==FigureRemove::Choice::SaveThenRemove)
    {
      makeFigureActive(index);
      if(!fileSave() || anim->dirty()) return false;
    }
  }
  const int count=animationIds.count();
  const QString otherPath=(index==0 && count==2) ? openFiles.at(1) : QString();

  // the panels let go of it first, then the scene; the other figure is not moved
  const bool wasActive=(scene->getAnimation()==anim);
  disconnect(anim,nullptr,this,nullptr);
  if(wasActive) timeline->setAnimation(nullptr);
  scene->removeAnimation(anim);
  animationIds.removeAt(index);
  const QString removedPath=openFiles.takeAt(index);
  m_avmOrigin.remove(removedPath);
  ui->selectAnimationCombo->removeItem(index);
  m_figurePlace.remove(anim);
  ui->animationView->setFigureOffset(anim,0.0f);
  anim->deleteLater();

  Animation* now=scene->getAnimation();
  if(!now && !animationIds.isEmpty()) now=animationIds.first();
  if(now)
  {
    currentPart=nullptr;
    selectAnimation(now);
    ui->animationView->selectPart(0);
  }
  calculateLongestRunningTime();
  m_announcedFigure=activeFigure();
  refreshFigures();
  statusBar()->showMessage(FigureSpec::removedMessage(index+1,otherPath),10000);
  return true;
}

void PinglyMotionWindow::refreshFigures()
{
  const int count=animationIds.count();
  const int active=activeFigure();
  const bool expert=UserSettings::expertMode();
  const bool two=count>=2 && !m_empty;

  // the two ways to add a figure: Expert mode only, greyed with the reason when the scene is full
  const QString why=FigureSpec::addBlocked(!m_empty,count);
  ui->fileAddAction->setVisible(expert);
  if(!m_empty)
  {
    ui->fileAddAction->setEnabled(why.isEmpty());
    ui->fileAddAction->setToolTip(why);
    ui->fileAddAction->setStatusTip(why);
  }
  if(m_libraryPanel) m_libraryPanel->setSecondFigureBlocked(why);

  // switching: Alt+1 and Alt+2 in Expert mode with two figures
  for(int i=0;i<m_figureActions.count();i++) m_figureActions.at(i)->setEnabled(two && expert && i<count);

  // the Figures box at the top right of the 3D view (Expert mode only)
  if(m_figuresBox)
  {
    m_figuresBox->setFigures(two ? openFiles : QStringList(),active);
    m_figuresBox->setVisible(two && expert);
    if(m_figuresBox->isVisible()) m_figuresBox->raise();
  }

  // the name tags over the heads (both modes: they only tell the figures apart)
  while(m_figureTags.count()>(two ? count : 0))
  {
    FigureTag* tag=m_figureTags.takeLast();
    tag->hide();
    tag->deleteLater();
  }
  while(two && m_figureTags.count()<count)
  {
    FigureTag* tag=new FigureTag(ui->animationView);
    const int number=m_figureTags.count();
    connect(tag,&FigureTag::clicked,this,[this,number]()
    {
      if(!UserSettings::expertMode())
      {
        if(number!=activeFigure()) statusBar()->showMessage(FigureSpec::tr(FigureSpec::beginnerSwitch),5000);
        return;
      }
      makeFigureActive(number);
    });
    m_figureTags << tag;
  }
  for(int i=0;i<m_figureTags.count();i++) m_figureTags.at(i)->setTag(i+1,openFiles.at(i),i==active);
  placeFigureTags();

  // the timeline's header rows; the chip and the window title
  ui->timelineView->setFigures(two ? animationIds : QList<Animation*>(),two ? openFiles : QStringList(),active,expert);
  if(m_selectedJoint) m_selectedJoint->setFigureChip(two ? FigureSpec::chipText(active+1,count) : QString());
  updateWindowTitle();

  // the status line says when another figure became active
  if(two && active>=0 && active!=m_announcedFigure)
    statusBar()->showMessage(FigureSpec::activeMessage(active+1,openFiles.at(active)),5000);
  m_announcedFigure=active;
}

void PinglyMotionWindow::placeFigureTags()
{
  QRect box;
  if(m_figuresBox && m_figuresBox->isVisible()) box=m_figuresBox->geometry();
  for(int i=0;i<m_figureTags.count();i++)
  {
    FigureTag* tag=m_figureTags.at(i);
    QPoint point;
    if(!ui->animationView->figureTagPoint(i,&point))
    {
      tag->hide();
      continue;
    }
    QRect place(point.x()-tag->width()/2,point.y()-tag->height(),tag->width(),tag->height());
    // a tag moves down when the Figures box would cover it
    if(box.isValid() && place.intersects(box.adjusted(-Theme::Size::spacingXs,0,0,Theme::Size::spacingXs)))
      place.moveTop(box.bottom()+Theme::Size::spacingS);
    tag->move(place.topLeft());
    tag->setVisible(ui->animationView->rect().intersects(place));
    tag->raise();
  }
}
