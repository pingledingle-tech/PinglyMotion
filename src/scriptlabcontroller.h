/*
 * PinglyMotion - the Script Lab's wiring to the rest of the window (job R5)
 *
 * Makes the Script Lab panel and its dock, and connects them to what the
 * main window owns: the control API (the same RLV state, LSL session and
 * folder watch as the AI Assistant's commands), the 3D view (effects preview,
 * camera, sky and view mode overrides, chains, a click on a prop) and the
 * scene (example props, prop tints, props a script moves). It also makes the
 * Help > Script Lab steps action and puts the Script Lab back in View >
 * Reset layout.
 *
 * Before job R5 all of this stood in the PinglyMotionWindow constructor; the
 * window still builds its menus and saves and restores the dock layout, with
 * panel() and dock().
 *
 * The parts that need the 3D view (connectView) are in
 * scriptlabcontrollerview.cpp, so a test can build the rest without the 3D
 * view (tests/scriptlabcontroller).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef SCRIPTLABCONTROLLER_H
#define SCRIPTLABCONTROLLER_H

#include <QList>
#include <QObject>
#include <QSharedPointer>
#include <QStringList>

#include <functional>

#include "chainlines.h"
#include "prop.h"
#include "scriptlab/labbeginner.h"

class QAction;
class QDockWidget;
class QMainWindow;
class AnimationView;
class CameraOverride;
class ControlApi;
class EffectsPreview;
class EnvironmentOverride;
class Mesh;
class Scene;
class ScriptLabPanel;
class ViewModesOverride;

class ScriptLabController : public QObject
{
  Q_OBJECT

  public:
    // the shapes a prop from an example is made of (the window's meshes)
    struct Meshes
    {
      QSharedPointer<Mesh> cube;
      QSharedPointer<Mesh> sphere;
      QSharedPointer<Mesh> cone;
      QSharedPointer<Mesh> torus;
    };

    // what only the window knows or does; an empty hook is skipped
    struct WindowHooks
    {
      std::function<QStringList()> jointNames;          // the 3D view's joints, in Attach to order (0 is not attached)
      std::function<void(const Prop*)> propAdded;       // a prop the Script Lab put in the scene: into the prop list
      std::function<void(Prop*)> selectProp;            // the window's own prop selection
      std::function<void()> showPropsTab;               // shows Props in the Joints panel
      std::function<void(const Prop*)> propMoved;       // a prop the Script Lab moved (the selected prop's spins follow)
      std::function<void()> redraw;                     // the 3D view draws again
    };

    // makes the panel and its dock in the window (hidden until View > Script Lab)
    explicit ScriptLabController(QMainWindow* window);

    ScriptLabPanel* panel() const { return m_scriptLabPanel; }
    QDockWidget* dock() const { return m_scriptLabDock; }

    // the panel shows and changes the same RLV state, LSL session and folder
    // watch as the control commands (issues 288 and 292); the effects preview
    // and the three overrides are made here, off until their boxes are ticked
    void connectControl(ControlApi* controlApi);
    EffectsPreview* effects() const { return m_effects; }
    CameraOverride* cameraOverride() const { return m_cameraOverride; }
    EnvironmentOverride* environmentOverride() const { return m_environmentOverride; }
    ViewModesOverride* viewModesOverride() const { return m_viewModesOverride; }

    // the 3D view shows the effects, the overrides and the chains, and a click on a prop is a
    // pick and a touch in the Script Lab (scriptlabcontrollerview.cpp). After connectControl().
    void connectView(AnimationView* view);

    // the scene: example props by name, prop tints, props a script moves (job U3, lane 1 job E, job A1)
    void connectScene(Scene* scene, const Meshes& meshes, const WindowHooks& hooks);

    // the Script Lab's chains as lines for the 3D view (job A4, part 2)
    QList<ChainLines::Line> chainLines() const;
    // every prop's tint from the panel's prop tints: a prop named like a prim of the watched object
    // shows that prim's colour and transparency, the other props keep their own colour (lane 1 job E)
    void applyPropTints();

    // Help > Script Lab steps (job U3): opens the panel with the steps of the example project
    QAction* makeStepsAction(QObject* parent);

    // View > Reset layout: back to its side and closed, as at the first start, its tabs back in
    // it and big again the next time it opens (job UX-B)
    void resetLayout();

  private:
    bool addExampleProp(const LabExample::Prop& p);
    bool placeProp(const LabExample::Prop& p);

    QMainWindow* m_window;
    ScriptLabPanel* m_scriptLabPanel=nullptr;
    QDockWidget* m_scriptLabDock=nullptr;
    ControlApi* m_controlApi=nullptr;
    EffectsPreview* m_effects=nullptr;
    CameraOverride* m_cameraOverride=nullptr;
    EnvironmentOverride* m_environmentOverride=nullptr;
    ViewModesOverride* m_viewModesOverride=nullptr;
    Scene* m_scene=nullptr;
    Meshes m_meshes;
    WindowHooks m_hooks;
};

#endif
