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
#include <QPushButton>

#include "playback.h"
#include "transportlook.h"
#include "wraprow.h"

TransportBar::TransportBar(QWidget* parent) : QWidget(parent)
{
  // two parts: playing (Start, Stop, Replay from start) and stepping (Frame
  // back, Frame forward, Loop); side by side, or one under the other when the
  // panels stand in one column (setWrapped, docs/UX_DECISIONS.md 6c)
  QHBoxLayout* playPart=new QHBoxLayout;
  QHBoxLayout* stepPart=new QHBoxLayout;
  m_rows=WrapRow::make({playPart,stepPart});
  setLayout(m_rows);

  m_start=new QPushButton(tr("Start"),this);
  m_start->setObjectName("transportStartButton");

  m_stop=new QPushButton(tr("Stop"),this);
  m_stop->setObjectName("transportStopButton");

  m_replay=new QPushButton(tr("Replay from start"),this);
  m_replay->setObjectName("transportReplayButton");
  setTip(m_replay,tr("Go back to the first frame and play"));

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
  // the design system's look: Start the main button, Stop the danger
  // button, Lucide icons, theme colours, focus ring (transportlook.h)
  TransportLook::apply(this);

  playPart->addWidget(m_start);
  playPart->addWidget(m_stop);
  playPart->addWidget(m_replay);
  playPart->addStretch(1);
  stepPart->addWidget(m_back);
  stepPart->addWidget(m_forward);
  stepPart->addWidget(m_loop);
  stepPart->addStretch(1);

  connect(m_start,&QPushButton::clicked,this,&TransportBar::startClicked);
  connect(m_stop,&QPushButton::clicked,this,&TransportBar::stopClicked);
  connect(m_replay,&QPushButton::clicked,this,&TransportBar::replayClicked);
  connect(m_back,&QPushButton::clicked,this,&TransportBar::backClicked);
  connect(m_forward,&QPushButton::clicked,this,&TransportBar::forwardClicked);
  connect(m_loop,&QCheckBox::toggled,this,&TransportBar::loopToggled);

  setPlaystate(PLAYSTATE_STOPPED);
  setFrame(0,1);
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

void TransportBar::setWrapped(bool wrapped)
{
  WrapRow::setWrapped(m_rows,wrapped);
}

bool TransportBar::isWrapped() const
{
  return WrapRow::isWrapped(m_rows);
}

int TransportBar::oneLineWidth() const
{
  return WrapRow::oneLineWidth(m_rows);
}

int TransportBar::wrappedWidth() const
{
  return WrapRow::wrappedWidth(m_rows);
}
