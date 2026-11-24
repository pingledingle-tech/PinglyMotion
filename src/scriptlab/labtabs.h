/*
 * PinglyMotion Script Lab - the tabs inside the Script Lab window (job UX-B)
 *
 * The new UX direction (the project owner, 24 Nov 2026, 13:26, docs/UX_DECISIONS.md 6x): Script Lab is
 * one window with its own tabs, never tabs on the screen edges. Each tab can be pulled out into a
 * window of its own (for people who script a lot: the editor, the map and the player side by side,
 * maybe on two screens) and put back; a beginner keeps everything in one window.
 *
 *  - LabTabBar: the tab bar; a tab dragged and let go well outside the bar asks to pop out.
 *  - LabTabWidget: a QTabWidget with that bar.
 *  - LabPopWindow: the window of one popped-out tab. A normal window of the system (moved and
 *    resized by its frame), with the tab's name and a "Put back" button on top; closing it puts the
 *    tab back too.
 *  - LabTabState: the remembered state ("tab=map;pop=player;place=player:10,20,600,400"), read and
 *    written without a window, so the tests can check it on its own. Only known tab
 *    keys and sane numbers are taken; anything else is left out.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef LABTABS_H
#define LABTABS_H

#include <QMap>
#include <QRect>
#include <QStringList>
#include <QTabBar>
#include <QTabWidget>
#include <QWidget>

class QLabel;
class QPushButton;
class QVBoxLayout;

namespace LabTabState
{
  struct State
  {
    QString current;                 // the tab in front in the Script Lab window ("" when not known)
    QStringList popped;              // the tabs in windows of their own, in the order they were popped
    QMap<QString, QRect> places;     // the last place and size of each tab's own window
    bool operator==(const State& other) const
    {
      return current == other.current && popped == other.popped && places == other.places;
    }
  };
  // reads the text; keys not in `known` and broken entries are left out
  State parse(const QString& text, const QStringList& known);
  QString format(const State& state);
  // the longest text kept (the settings value is cut there)
  inline constexpr int MaxLength = 1000;
}

class LabTabBar : public QTabBar
{
  Q_OBJECT

  public:
    explicit LabTabBar(QWidget* parent = nullptr);
    // how far outside the bar (in pixels) a tab must be let go to pop out: never by accident
    int pullDistance() const;

  signals:
    // a tab was dragged out of the bar and let go at globalPos
    void popOutRequested(int index, const QPoint& globalPos);

  protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

  private:
    int m_pressed = -1;
};

class LabTabWidget : public QTabWidget
{
  Q_OBJECT

  public:
    explicit LabTabWidget(QWidget* parent = nullptr);
    LabTabBar* labTabBar() const { return m_bar; }

  private:
    LabTabBar* m_bar;
};

class LabPopWindow : public QWidget
{
  Q_OBJECT

  public:
    // parent: the Script Lab panel (the window stays with the program and goes with the panel)
    LabPopWindow(const QString& key, const QString& tabName, QWidget* parent);

    QString key() const { return m_key; }
    // the tab's page: put in under the top row, and taken out again (not deleted)
    void setPage(QWidget* page);
    QWidget* takePage();
    QWidget* page() const { return m_page; }
    QLabel* titleLabel() const { return m_title; }
    QPushButton* putBackButton() const { return m_putBack; }
    void applyStyle(bool dark);

    // the first size of a popped tab: most of the screen, as Script Lab is big by default
    static QSize firstSize(const QRect& screen);
    // a place that is on a screen: pulled onto the nearest one and no bigger than it (a second
    // screen may be gone since last time)
    static QRect fitOnScreen(const QRect& place);

  signals:
    void putBackRequested(const QString& key);
    void placeChanged(const QString& key);

  protected:
    void closeEvent(QCloseEvent* event) override;
    void moveEvent(QMoveEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void changeEvent(QEvent* event) override;

  private:
    QString m_key;
    QVBoxLayout* m_layout = nullptr;
    QLabel* m_title = nullptr;
    QPushButton* m_putBack = nullptr;
    QWidget* m_page = nullptr;
};

#endif
