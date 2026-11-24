/*
 * PinglyMotion - helper mode (job UX-C, docs/UX_DECISIONS.md, 6y; new UX direction of 24 Nov 2026)
 *
 * One Helper switch at the top of the main window, on or off. Off: a clean tool. On: one helper
 * panel floats beside the tool (over the bottom right of the middle of the window, never inside
 * a function view). It holds the hint card now due (src/hintcard.*, moved into the panel) and a
 * short list of lessons (src/helperspec.h). The control the card or the lesson step talks about
 * gets a highlight: a ring around the real control. A lesson step goes on only when the user
 * does what it says (HelperMode::notify), so every step visibly does something.
 *
 * The switch is the same setting as Help > Show hints, the status bar switch and Setup
 * (UserSettings::tipsOn), so guidance is switched in one place (charter P4) and is remembered
 * between sessions. The panel and the highlight float over the window and never move a control
 * (charter P12).
 *
 * HelperHighlight is the ring, HelperPanel the panel, HelperMode decides what shows. They use no
 * desktop-only code (docs/DEV.md, "Web-ready code").
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#ifndef HELPERMODE_H
#define HELPERMODE_H

#include <QFrame>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QRect>
#include <QString>
#include <QWidget>

#include <functional>

class HintCards;
class QAction;
class QLabel;
class QScrollArea;
class QStackedWidget;
class QToolBar;
class QToolButton;

// a ring around the control a hint or a lesson step talks about; it lets every click through
class HelperHighlight : public QWidget
{
  Q_OBJECT

  public:
    explicit HelperHighlight(QWidget* parent = nullptr);

    // the control to stand around; null hides the ring
    void setTarget(QWidget* target);
    QWidget* target() const { return m_target; }
    // the ring is on screen
    bool shown() const;
    // where the ring is drawn around its control, in the coordinates of the control's window
    QRect ringFor(QWidget* target) const;
    // follow the control again (it moved, changed size, was shown or hidden)
    void follow();

  protected:
    void paintEvent(QPaintEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

  private:
    void watch(bool on);

    QPointer<QWidget> m_target;
    QList<QPointer<QWidget>> m_watched;
};

// the panel: the title row with the close button, then the home page (the hint card and the
// lessons) or the page of the lesson that runs
class HelperPanel : public QFrame
{
  Q_OBJECT

  public:
    explicit HelperPanel(QWidget* parent = nullptr);

    QLabel* titleLabel() const { return m_title; }
    QToolButton* closeButton() const { return m_close; }
    // the place the hint card goes in
    QWidget* cardHolder() const { return m_cardHolder; }
    QStackedWidget* pages() const { return m_pages; }
    QWidget* homePage() const { return m_home; }
    QWidget* lessonPage() const { return m_lesson; }
    QList<QToolButton*> lessonButtons() const { return m_lessonButtons; }
    QLabel* lessonTitle() const { return m_lessonTitle; }
    QLabel* progressLabel() const { return m_progress; }
    QLabel* stepLabel() const { return m_step; }
    QLabel* notOnScreenLabel() const { return m_notOnScreen; }
    QToolButton* stopButton() const { return m_stop; }
    QToolButton* backButton() const { return m_back; }

    // width at the current text size (as wide as the hint card, and its margins)
    int panelWidth() const;
    // the height the content wants at panelWidth()
    int wantedHeight() const;
    QSize sizeHint() const override { return QSize(panelWidth(), wantedHeight()); }
    // colours again after the theme changed
    void applyLook();

  signals:
    void closeRequested();
    void lessonRequested(int index);
    void stopRequested();
    void backRequested();

  protected:
    void keyPressEvent(QKeyEvent* event) override;

  private:
    QLabel* m_icon = nullptr;
    QLabel* m_title = nullptr;
    QToolButton* m_close = nullptr;
    QScrollArea* m_scroll = nullptr;
    QStackedWidget* m_pages = nullptr;
    QWidget* m_home = nullptr;
    QWidget* m_cardHolder = nullptr;
    QLabel* m_lessonsHeading = nullptr;
    QList<QToolButton*> m_lessonButtons;
    QWidget* m_lesson = nullptr;
    QLabel* m_lessonTitle = nullptr;
    QLabel* m_progress = nullptr;
    QLabel* m_step = nullptr;
    QLabel* m_notOnScreen = nullptr;
    QToolButton* m_stop = nullptr;
    QToolButton* m_back = nullptr;
};

class HelperMode : public QObject
{
  Q_OBJECT

  public:
    // the control a step talks about, by its name in HelperSpec ("view", "selected", "setKey",
    // "start", "library", "resetLayout", "scriptSwitch", "animateSwitch"); null when it has none
    using TargetFor = std::function<QWidget*(const QString&)>;
    // true when the thing a step asks for is already so (a joint is picked, the Library is open,
    // the Script side is shown): that step is skipped
    using AlreadyDone = std::function<bool(const QString&)>;
    // the part of the host the panel stands beside (host coordinates); empty means the whole host
    using Area = std::function<QRect()>;

    HelperMode(QWidget* host, HintCards* cards, TargetFor targetFor, AlreadyDone alreadyDone = {}, Area area = {});

    HelperPanel* panel() const { return m_panel; }
    HelperHighlight* highlight() const { return m_highlight; }
    // the checkable switch for the top bar (not in a bar until addToBar)
    QAction* switchAction() const { return m_switch; }
    // puts the switch in the bar before the action given (at the end when null)
    void addToBar(QToolBar* bar, QAction* before);

    bool isOn() const { return m_on; }
    // on: the panel shows; off: the panel, the highlight and a running lesson go
    void setOn(bool on);
    // the panel is on screen
    bool panelShown() const;

    void setAnimationOpen(bool open);
    bool animationOpen() const { return m_open; }

    // the lessons: -1 when none runs
    int lesson() const { return m_lesson; }
    int step() const { return m_step; }
    bool lessonDone() const { return m_done; }
    // the reason a lesson cannot start now, empty when it can
    QString blockedReason(int index) const;
    void startLesson(int index);
    void stopLesson();

    // what the user did (HelperSpec::picked, ...): a step waiting for it goes on
    void notify(const QString& event);

    // decide again what shows, where the panel stands and what is highlighted
    void refresh();
    // colours and icons again after the theme changed
    void applyLook();

  signals:
    // the switch was clicked (on or off)
    void switchClicked(bool on);
    // the close button of the panel was pressed: the window turns the helper off everywhere
    void offRequested();
    // a lesson step went on, a lesson started, stopped or was finished
    void lessonChanged();

  protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

  private:
    void skipDone();
    void syncSwitch();
    void syncCardHolder();
    void syncPanel();
    void place();

    QWidget* m_host;
    HintCards* m_cards;
    TargetFor m_targetFor;
    AlreadyDone m_alreadyDone;
    Area m_area;
    HelperPanel* m_panel;
    HelperHighlight* m_highlight;
    QAction* m_switch;
    bool m_on = false;
    bool m_open = false;
    int m_lesson = -1;
    int m_step = 0;
    bool m_done = false;
    bool m_refreshing = false;
};

#endif
