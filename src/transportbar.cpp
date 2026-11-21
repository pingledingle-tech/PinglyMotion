/*
 * PinglyMotion - animation editor for Second Life avatars
 *
 * The transport buttons under the timeline: Start, Stop, Replay from start
 * and the Loop switch (see transportbar.h).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#include "transportbar.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QPushButton>

#include "playback.h"
#include "transportlook.h"

TransportBar::TransportBar(QWidget* parent) : QWidget(parent)
{
  QHBoxLayout* layout=new QHBoxLayout(this);
  layout->setContentsMargins(0,0,0,0);

  m_start=new QPushButton(tr("Start"),this);
  m_start->setObjectName("transportStartButton");

  m_stop=new QPushButton(tr("Stop"),this);
  m_stop->setObjectName("transportStopButton");

  m_replay=new QPushButton(tr("Replay from start"),this);
  m_replay->setObjectName("transportReplayButton");
  setTip(m_replay,tr("Go back to the first frame and play"));

  m_loop=new QCheckBox(tr("Loop"),this);
  m_loop->setObjectName("transportLoopCheckBox");
  m_loop->setToolTip(tr("Play the animation again and again between the loop points"));
  m_loop->setChecked(true);
  // the design system's look: Start the main button, Stop the danger
  // button, Lucide icons, theme colours, focus ring (transportlook.h)
  TransportLook::apply(this);

  layout->addWidget(m_start);
  layout->addWidget(m_stop);
  layout->addWidget(m_replay);
  layout->addWidget(m_loop);

  connect(m_start,&QPushButton::clicked,this,&TransportBar::startClicked);
  connect(m_stop,&QPushButton::clicked,this,&TransportBar::stopClicked);
  connect(m_replay,&QPushButton::clicked,this,&TransportBar::replayClicked);
  connect(m_loop,&QCheckBox::toggled,this,&TransportBar::loopToggled);

  setPlaystate(PLAYSTATE_STOPPED);
}

// the tip on hover, the same words in the status line and for screen readers
void TransportBar::setTip(QPushButton* button,const QString& tip)
{
  button->setToolTip(tip);
  button->setStatusTip(tip);
  button->setAccessibleDescription(tip);
}

void TransportBar::setPlaystate(PlayState state)
{
  const Playback::Buttons enabled=Playback::buttons(state);

  // keyboard use: Start and Stop take turns, so the focus goes from the one
  // that can no longer be clicked to the other one, ring and all
  QPushButton* focused=m_start->hasFocus() ? m_start : (m_stop->hasFocus() ? m_stop : nullptr);
  const bool ring=focused && TransportLook::showsFocusRing(focused);

  // the buttons that can be clicked now first, so the focus has somewhere to go
  for(auto [button,on] : {std::pair<QPushButton*,bool>{m_start,enabled.start},{m_stop,enabled.stop},{m_replay,enabled.replay}})
    if(on) button->setEnabled(true);
  for(auto [button,on] : {std::pair<QPushButton*,bool>{m_start,enabled.start},{m_stop,enabled.stop},{m_replay,enabled.replay}})
    if(!on) button->setEnabled(false);

  // a button that cannot be clicked says why and what to do (charter P6)
  setTip(m_start,enabled.start ? tr("Play the animation from where the playhead is")
                               : tr("It is already playing. Click Stop to stop it."));
  setTip(m_stop,enabled.stop ? tr("Stop playing; the playhead stays on this frame")
                             : tr("It is not playing. Click Start to play it."));

  if(focused && !focused->isEnabled())
  {
    QPushButton* other=focused==m_start ? m_stop : m_start;
    if(other->isEnabled())
    {
      other->setFocus(Qt::OtherFocusReason);
      TransportLook::setFocusRing(other,ring);
    }
  }
}

void TransportBar::setLoop(bool on)
{
  const bool blocked=m_loop->blockSignals(true);
  m_loop->setChecked(on);
  m_loop->blockSignals(blocked);
}

bool TransportBar::loop() const
{
  return m_loop->isChecked();
}
