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
#include <QIcon>
#include <QPushButton>

#include "playback.h"
#include "transportlook.h"

TransportBar::TransportBar(QWidget* parent) : QWidget(parent)
{
  QHBoxLayout* layout=new QHBoxLayout(this);
  layout->setContentsMargins(0,0,0,0);

  m_start=new QPushButton(QIcon(":/icons/icons/play.png"),tr("Start"),this);
  m_start->setObjectName("transportStartButton");
  m_start->setToolTip(tr("Play the animation from where the playhead is"));

  m_stop=new QPushButton(QIcon(":/icons/icons/stop.png"),tr("Stop"),this);
  m_stop->setObjectName("transportStopButton");
  m_stop->setToolTip(tr("Stop playing; the playhead stays on this frame"));

  m_replay=new QPushButton(tr("Replay from start"),this);
  m_replay->setObjectName("transportReplayButton");
  m_replay->setToolTip(tr("Go back to the first frame and play"));

  m_loop=new QCheckBox(tr("Loop"),this);
  m_loop->setObjectName("transportLoopCheckBox");
  m_loop->setToolTip(tr("Play the animation again and again between the loop points"));
  m_loop->setChecked(true);
  // green Start, red Stop, a round arrow on Replay (transportlook.h)
  TransportLook::apply(this,m_start,m_stop,m_replay);

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

void TransportBar::setPlaystate(PlayState state)
{
  const Playback::Buttons enabled=Playback::buttons(state);
  m_start->setEnabled(enabled.start);
  m_stop->setEnabled(enabled.stop);
  m_replay->setEnabled(enabled.replay);
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
