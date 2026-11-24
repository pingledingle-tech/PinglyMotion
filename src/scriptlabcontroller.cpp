/*
 * PinglyMotion - the Script Lab's wiring to the rest of the window (job R5)
 *
 * See scriptlabcontroller.h. The 3D view's part is in scriptlabcontrollerview.cpp.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QAction>
#include <QColor>
#include <QDockWidget>
#include <QMainWindow>

#include "scriptlabcontroller.h"

#include "controlapi.h"
#include "propmanager.h"
#include "scene.h"
#include "scriptlab/scriptlabpanel.h"

ScriptLabController::ScriptLabController(QMainWindow* window) :
  QObject(window),
  m_window(window)
{
  // --- Script Lab panel (issue 288): RLV commands and what the avatar may
  // still do. Hidden until View > Script Lab; place, size and visibility
  // are saved with the layout like the other panels.
  m_scriptLabPanel=new ScriptLabPanel;
  m_scriptLabDock=ScriptLabPanel::makeDock(window,m_scriptLabPanel);
}

void ScriptLabController::connectControl(ControlApi* controlApi)
{
  m_controlApi=controlApi;
  // the Script Lab panel shows and changes the same RLV state as rlv_command (issue 288)
  m_scriptLabPanel->setState(&m_controlApi->rlv());
  m_scriptLabPanel->setSession(&m_controlApi->lslSession());
  // and the same folder watch as lab_watch (issue 292)
  m_scriptLabPanel->setWatcher(&m_controlApi->lab());
  // and shows what a Live run does in the 3D view, off until its box is ticked (issue 293)
  m_effects=new EffectsPreview(m_window);
  // and lets the RLV camera limits limit the 3D view's camera, off until its box is ticked (job 7b)
  m_cameraOverride=new CameraOverride(m_window);
  m_cameraOverride->setState(&m_controlApi->rlv());
  // and shows the sky and draw distance a script asks for as the background and fog, off until its box is ticked (job 7c)
  m_environmentOverride=new EnvironmentOverride(m_window);
  m_environmentOverride->setState(&m_controlApi->rlv());
  // and the view modes a script holds (hidden avatar, plain textures, overlay), off until its box is ticked (job 7c)
  m_viewModesOverride=new ViewModesOverride(m_window);
  m_viewModesOverride->setState(&m_controlApi->rlv());
}

QList<ChainLines::Line> ScriptLabController::chainLines() const
{
  QList<ChainLines::Line> lines;
  for(const LabChains::Chain& chain : m_scriptLabPanel->chains())
    lines.append(ChainLines::Line{chain.from,chain.to,chain.locked,chain.word()});
  return lines;
}

void ScriptLabController::applyPropTints()
{
  if(!m_scene) return;
  const QList<LabTint::Tint> tints=m_scriptLabPanel->propTints();
  PropManager* props=m_scene->propManager();
  for(int i=0;i<props->count();++i)
  {
    Prop* prop=props->at(i);
    prop->clearTint();
    for(const LabTint::Tint& tint : tints)
    {
      if(tint.prim!=prop->name()) continue;
      const QColor colour=tint.hasColour ? QColor::fromRgbF(tint.colour[0],tint.colour[1],tint.colour[2]) : prop->drawColour();
      prop->setTint(colour,tint.hasAlpha ? tint.alpha : 1.0f);
    }
  }
  if(m_hooks.redraw) m_hooks.redraw();
}

void ScriptLabController::connectScene(Scene* scene,const Meshes& meshes,const WindowHooks& windowHooks)
{
  m_scene=scene;
  m_meshes=meshes;
  m_hooks=windowHooks;
  // props as prims (lane 1 job E): a prop named like a prim of the watched object shows the
  // colour and transparency its scripts gave that prim; the other props keep their own colour
  connect(m_scriptLabPanel,&ScriptLabPanel::propTintsChanged,this,&ScriptLabController::applyPropTints);
  // Script Lab for beginners (job U3): the panel puts an example's props in the scene by name, lights
  // up the prop of a piece picked in its tree (the existing prop selection), and a click on a prop in
  // the 3D view picks its piece in the tree (connectView)
  ScriptLabPanel::SceneHooks hooks;
  hooks.hasProp=[this](const QString& name) { return m_scene->getPropByName(name)!=nullptr; };
  hooks.addProp=[this](const LabExample::Prop& p) { return addExampleProp(p); };
  hooks.selectProp=[this](const QString& name)
  {
    Prop* prop=m_scene->getPropByName(name);
    if(!prop) return false;
    if(m_hooks.selectProp) m_hooks.selectProp(prop);
    if(m_hooks.redraw) m_hooks.redraw();
    return true;
  };
  hooks.showPropsTab=[this]() { if(m_hooks.showPropsTab) m_hooks.showPropsTab(); };
  // job A1, props that move: the panel reads where a prop stands and puts it where a script moved its prim
  hooks.propPlace=[this](const QString& name,LabExample::Prop& out)
  {
    const Prop* prop=m_scene->getPropByName(name);
    if(!prop) return false;
    out.name=prop->name();
    out.position=prop->position();
    out.scale=prop->scale();
    out.rotation=prop->rotation();
    return true;
  };
  hooks.placeProp=[this](const LabExample::Prop& p) { return placeProp(p); };
  m_scriptLabPanel->setSceneHooks(hooks);
}

bool ScriptLabController::addExampleProp(const LabExample::Prop& p)
{
  if(m_scene->getPropByName(p.name)) return true;   // there already: nothing is added twice
  Prop::PropType type=Prop::Box;
  QSharedPointer<Mesh> mesh=m_meshes.cube;
  if(p.shape==QLatin1String("sphere")) { type=Prop::Sphere; mesh=m_meshes.sphere; }
  else if(p.shape==QLatin1String("cone")) { type=Prop::Cone; mesh=m_meshes.cone; }
  else if(p.shape==QLatin1String("torus")) { type=Prop::Torus; mesh=m_meshes.torus; }
  // the joint by name: its place in the Attach to list (0 is not attached)
  const QStringList joints=m_hooks.jointNames ? m_hooks.jointNames() : QStringList();
  const int attach=p.joint.isEmpty() ? 0 : qMax(0,int(joints.indexOf(p.joint)));
  const Prop* prop=m_scene->addProp(type,mesh,attach,
                                    p.position.x(),p.position.y(),p.position.z(),
                                    p.scale.x(),p.scale.y(),p.scale.z(),
                                    p.rotation.x(),p.rotation.y(),p.rotation.z(),p.name);
  if(!prop) return false;
  if(m_hooks.propAdded) m_hooks.propAdded(prop);
  if(m_hooks.redraw) m_hooks.redraw();
  return prop->name()==p.name;
}

bool ScriptLabController::placeProp(const LabExample::Prop& p)
{
  Prop* prop=m_scene->getPropByName(p.name);
  if(!prop) return false;
  prop->setPosition(p.position.x(),p.position.y(),p.position.z());
  prop->setScale(p.scale.x(),p.scale.y(),p.scale.z());
  prop->setRotation(p.rotation.x(),p.rotation.y(),p.rotation.z());
  if(m_hooks.propMoved) m_hooks.propMoved(prop);
  if(m_hooks.redraw) m_hooks.redraw();
  return true;
}

QAction* ScriptLabController::makeStepsAction(QObject* parent)
{
  // the steps of the Script Lab example project again (job U3); the panel opens with them
  QAction* scriptLabStepsAction=m_scriptLabPanel->makeStepsAction(parent);
  connect(scriptLabStepsAction,&QAction::triggered,this,[this]()
  {
    m_scriptLabDock->show();
    m_scriptLabDock->raise();
    m_scriptLabPanel->showSteps();   // again, now that it is on screen: the first step gets the keys
  });
  return scriptLabStepsAction;
}

void ScriptLabController::resetLayout()
{
  // the Script Lab goes back to its side and closes, as at the first start
  m_scriptLabDock->setFloating(false);
  m_window->addDockWidget(Qt::RightDockWidgetArea,m_scriptLabDock);
  m_scriptLabDock->hide();
  // its tabs back in it, Live scripts in front, and big again the next time it opens (job UX-B)
  m_scriptLabPanel->resetWindows();
}
