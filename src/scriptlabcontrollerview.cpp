/*
 * PinglyMotion - the Script Lab's wiring to the 3D view (job R5)
 *
 * The part of ScriptLabController (scriptlabcontroller.h) that needs the 3D
 * view, in a file of its own so the rest builds and is tested without it.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QCursor>

#include "scriptlabcontroller.h"

#include "animationview.h"
#include "scriptlab/scriptlabpanel.h"

void ScriptLabController::connectView(AnimationView* view)
{
  // what a Live run does, shown in the 3D view (issue 293)
  view->setEffectsPreview(m_effects);
  m_scriptLabPanel->setEffects(m_effects);
  // Script Lab chains and ribbons (job A4, part 2): a line in the 3D view between two props a script links
  connect(m_scriptLabPanel,&ScriptLabPanel::chainsChanged,this,[this,view]()
  {
    view->setChainLines(chainLines());
  });
  // a click on a prop in the 3D view picks its piece in the tree (job U3)
  connect(view,&AnimationView::propClicked,this,[this,view](Prop* prop)
  {
    if(!prop) return;
    m_scriptLabPanel->selectPrim(prop->name());
    // job A1: a click on a prim's prop is a touch; job A2: a menu it shows stands next to the click
    m_scriptLabPanel->touchPrim(prop->name(),view->mapFromGlobal(QCursor::pos()));
  });
  m_scriptLabPanel->setMenuBoxHost(view);
  // the RLV camera limits, the sky and draw distance, and the view modes (jobs 7b and 7c)
  view->setCameraOverride(m_cameraOverride);
  m_scriptLabPanel->setCameraOverride(m_cameraOverride);
  view->setEnvironmentOverride(m_environmentOverride);
  m_scriptLabPanel->setEnvironmentOverride(m_environmentOverride);
  view->setViewModesOverride(m_viewModesOverride);
  m_scriptLabPanel->setViewModesOverride(m_viewModesOverride);
}
