/*
 * PinglyMotion - animation editor for Second Life avatars
 *
 * The transport buttons under the timeline: Start, Stop, Replay from start,
 * Frame back, Frame forward and the Loop switch (see transportbar.h).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#include "transportbar.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QPushButton>

#include "playback.h"

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

  // the window steps the frame; the left / right arrow keys do the same
  m_back=new QPushButton(tr("Frame back"),this);
  m_back->setObjectName("transportBackButton");
  m_back->setToolTip(tr("Go one frame back (Left arrow key)"));

  m_forward=new QPushButton(tr("Frame forward"),this);
  m_forward->setObjectName("transportForwardButton");
  m_forward->setToolTip(tr("Go one frame forward (Right arrow key)"));

  // a click does not take the keyboard focus, so the left / right arrow keys
  // still step the frame afterwards (a focused button would move the focus)
  for(QPushButton* button : {m_start,m_stop,m_replay,m_back,m_forward})
    button->setFocusPolicy(Qt::TabFocus);

  m_loop=new QCheckBox(tr("Loop"),this);
  m_loop->setObjectName("transportLoopCheckBox");
  m_loop->setToolTip(tr("Play the animation again and again between the loop points"));
  m_loop->setChecked(true);

  layout->addWidget(m_start);
  layout->addWidget(m_stop);
  layout->addWidget(m_replay);
  layout->addWidget(m_back);
  layout->addWidget(m_forward);
  layout->addWidget(m_loop);

  connect(m_start,&QPushButton::clicked,this,&TransportBar::startClicked);
  connect(m_stop,&QPushButton::clicked,this,&TransportBar::stopClicked);
  connect(m_replay,&QPushButton::clicked,this,&TransportBar::replayClicked);
  connect(m_back,&QPushButton::clicked,this,&TransportBar::backClicked);
  connect(m_forward,&QPushButton::clicked,this,&TransportBar::forwardClicked);
  connect(m_loop,&QCheckBox::toggled,this,&TransportBar::loopToggled);

  setPlaystate(PLAYSTATE_STOPPED);
  setFrame(0,1);
}

void TransportBar::setPlaystate(PlayState state)
{
  const Playback::Buttons enabled=Playback::buttons(state);
  m_start->setEnabled(enabled.start);
  m_stop->setEnabled(enabled.stop);
  m_replay->setEnabled(enabled.replay);
}

void TransportBar::setFrame(int frame,int totalFrames)
{
  const Playback::StepButtons enabled=Playback::stepButtons(frame,totalFrames);
  m_back->setEnabled(enabled.back);
  m_forward->setEnabled(enabled.forward);
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
