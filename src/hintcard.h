/*
 * PinglyMotion - the hint cards (UX phase 6a, docs/UX_DECISIONS.md, 6k;
 * docs/mockup/index.html, #hintcard and CARDS)
 *
 * One small card at a time stands by the part of the window it talks about
 * ("Pick a joint", "Pull a limb", ...), with Next and "Turn hints off". It
 * shows only while hints are on (Help > Show hints, Setup, the Tips switch),
 * an animation is open and the part it talks about is on screen. A card
 * whose job is already done (a joint picked, a limb pulled, a joint turned)
 * is skipped. Which card comes next is remembered between sessions
 * (UserSettings::hintCard). The card floats over the window, so it never
 * moves another control (charter P12).
 *
 * HintCard is the widget; HintCards decides which card shows where.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#ifndef HINTCARD_H
#define HINTCARD_H

#include <QFrame>
#include <QObject>
#include <QString>

#include <functional>

class QLabel;
class QToolButton;

class HintCard : public QFrame
{
  Q_OBJECT

  public:
    explicit HintCard(QWidget* parent = nullptr);

    // the card: its place in the row (0-based) of count, the title and the text
    void setCard(int index, int count, const QString& title, const QString& text);
    int index() const { return m_index; }
    int count() const { return m_count; }
    // colours again after the theme changed
    void applyLook();

    // the card is as wide as fitWidth() says and as high as its wrapped text needs for that width
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override { return sizeHint(); }

    QLabel* titleLabel() const { return m_title; }
    QLabel* textLabel() const { return m_text; }
    QToolButton* closeButton() const { return m_close; }
    QToolButton* hintsOffButton() const { return m_hintsOff; }
    QToolButton* nextButton() const { return m_next; }
    QWidget* dots() const { return m_dots; }

  signals:
    // Next, the close button and Esc on the card all go to the next card
    void nextRequested();
    void hintsOffRequested();

  protected:
    void keyPressEvent(QKeyEvent* event) override;
    void changeEvent(QEvent* event) override;

  private:
    void fitWidth();

    int m_index = 0;
    int m_count = 0;
    QLabel* m_title = nullptr;
    QLabel* m_text = nullptr;
    QLabel* m_icon = nullptr;
    QToolButton* m_close = nullptr;
    QToolButton* m_hintsOff = nullptr;
    QToolButton* m_next = nullptr;
    QWidget* m_dots = nullptr;
};

class HintCards : public QObject
{
  Q_OBJECT

  public:
    // the part of the window a card talks about, by its name in GuidanceSpec::cards
    // ("view", "selected", "setKey", "start", "tiles", "joints", "setup"); null when it has none
    using TargetFor = std::function<QWidget*(const QString&)>;

    HintCards(QWidget* host, TargetFor targetFor);

    HintCard* card() const { return m_card; }
    // the card now due (0 to GuidanceSpec::cardCount; the count means all are seen)
    int index() const { return m_index; }
    // true while the card is on screen
    bool cardShown() const;

    void setHintsOn(bool on);
    void setAnimationOpen(bool open);
    // what the user did: a joint picked, a limb pulled, a joint turned
    void setPicked(bool picked);
    void markPulled();
    void markTurned();
    // the next card (Next, the close button, Esc on the card)
    void next();
    // start again from the first card (not used by the program yet; tests)
    void restart();
    // decide again which card shows and where; called after anything that may change it
    void refresh();

  signals:
    // "Turn hints off" was pressed: the window turns hints off everywhere
    void hintsOffRequested();

  protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

  private:
    bool isDone(int index) const;
    void refreshNow();
    void place(const QString& target);

    QWidget* m_host;
    TargetFor m_targetFor;
    HintCard* m_card;
    int m_index;
    bool m_hintsOn = true;
    bool m_open = false;
    bool m_picked = false;
    bool m_pulled = false;
    bool m_turned = false;
    bool m_refreshing = false;
};

#endif
