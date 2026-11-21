/*
 * PinglyMotion - animation editor for Second Life avatars
 *
 * The transport buttons under the timeline (docs/UX_DECISIONS.md section 1,
 * build order steps 1 and 2): Start, Stop, Replay from start, Frame back,
 * Frame forward, and the Loop switch.
 * Three plain buttons instead of the old single button that went through
 * play, loop and stop, because that one was confusing. The bar only shows
 * the state and reports clicks; the window does the playing.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#ifndef TRANSPORTBAR_H
#define TRANSPORTBAR_H

#include <QWidget>

#include "playstate.h"

class QCheckBox;
class QPushButton;

class TransportBar : public QWidget
{
  Q_OBJECT

  public:
    explicit TransportBar(QWidget* parent=nullptr);

    // shows which buttons can be clicked (Playback::buttons)
    void setPlaystate(PlayState state);
    // shows which frame step buttons can be clicked (Playback::stepButtons):
    // Frame back not on the first frame, Frame forward not on the last
    void setFrame(int frame,int totalFrames);
    // sets the Loop switch without sending loopToggled
    void setLoop(bool on);
    bool loop() const;

    QPushButton* startButton() const { return m_start; }
    QPushButton* stopButton() const { return m_stop; }
    QPushButton* replayButton() const { return m_replay; }
    QPushButton* backButton() const { return m_back; }
    QPushButton* forwardButton() const { return m_forward; }
    QCheckBox* loopCheckBox() const { return m_loop; }

  signals:
    void startClicked();
    void stopClicked();
    void replayClicked();
    void backClicked();
    void forwardClicked();
    void loopToggled(bool on);

  private:
    QPushButton* m_start;
    QPushButton* m_stop;
    QPushButton* m_replay;
    QPushButton* m_back;
    QPushButton* m_forward;
    QCheckBox* m_loop;
};

#endif
