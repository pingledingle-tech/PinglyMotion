/*
 * PinglyMotion - two figures in one scene, Expert mode only (docs/UX_DECISIONS.md 6m)
 *
 * The texts (FIG-01 to FIG-18 and LIB-26 of docs/TEACHING.md) and the small
 * parts the window puts together, as drawn in the approved mockup
 * (docs/mockup/index.html, "two figures"):
 *
 *  - AddFigureDialog: File > Add a second figure... asks From the library or
 *    From a file...;
 *  - FiguresBox: the box at the top right of the 3D view with one radio button
 *    per figure (number and file name); a click, the arrow keys or Alt+1 and
 *    Alt+2 in the window make a figure active;
 *  - FigureHeader: the timeline's row over each figure's joints, with its
 *    number, its file name, "Active" or Make active, and Remove;
 *  - FigureRows: the other figure's changed joints in the timeline, greyed,
 *    with small grey keys; a click asks to make that figure active and pick
 *    the joint;
 *  - the question before Remove when the figure has unsaved changes.
 *
 * Which figure is active, adding and removing are the window's (it holds the
 * animations); these parts only show and ask. Colours and sizes come from the
 * theme tokens (docs/DESIGN_SYSTEM.md). tests/twofigures checks every text and
 * behaviour exactly.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#ifndef FIGURES_H
#define FIGURES_H

#include <QCoreApplication>
#include <QDialog>
#include <QFrame>
#include <QLabel>
#include <QList>
#include <QString>
#include <QStringList>
#include <QWidget>

#include <functional>

#include "timelinerows.h"

class Animation;
class QButtonGroup;
class QMessageBox;
class QPushButton;
class QRadioButton;
class QVBoxLayout;

namespace FigureSpec
{
  inline constexpr const char* context = "FigureSpec";
  // the program can hold more; two is what is drawn and built (6m)
  inline constexpr int maxFigures = 2;
  // the two figures stand side by side, 0.9 metres apart (scene units are inches)
  inline constexpr double spacingMetres = 0.9;
  inline constexpr float spacingUnits = float(0.9 / 0.0254);

  // FIG-01: File menu, Expert mode only, under Export
  inline constexpr const char* addItem = QT_TRANSLATE_NOOP("FigureSpec", "Add a second figure...");
  // FIG-02: why Add a second figure and Add as a second figure are greyed
  inline constexpr const char* sceneFull = QT_TRANSLATE_NOOP("FigureSpec", "The scene has two figures. Remove one first.");
  // FIG-03 to FIG-05: the Add a second figure window
  inline constexpr const char* dialogTitle = QT_TRANSLATE_NOOP("FigureSpec", "Add a second figure");
  inline constexpr const char* dialogLine = QT_TRANSLATE_NOOP("FigureSpec", "The second figure plays next to the first, in the same scene.");
  inline constexpr const char* fromLibrary = QT_TRANSLATE_NOOP("FigureSpec", "From the library");
  inline constexpr const char* fromLibraryLine = QT_TRANSLATE_NOOP("FigureSpec", "Pick a ready-made move, then press Add as a second figure.");
  inline constexpr const char* fromFile = QT_TRANSLATE_NOOP("FigureSpec", "From a file...");
  inline constexpr const char* fromFileLine = QT_TRANSLATE_NOOP("FigureSpec", "A .dingle, BVH or .avm file.");
  inline constexpr const char* cancel = QT_TRANSLATE_NOOP("FigureSpec", "Cancel");
  // FIG-06 and FIG-07: the Figures box and its tips
  inline constexpr const char* boxHeading = QT_TRANSLATE_NOOP("FigureSpec", "Figures");
  inline constexpr const char* tipActive = QT_TRANSLATE_NOOP("FigureSpec", "This figure is active. Its joints and keys show.");
  inline constexpr const char* tipOther = QT_TRANSLATE_NOOP("FigureSpec", "Click to make this figure active.");
  inline constexpr const char* figureName = QT_TRANSLATE_NOOP("FigureSpec", "Figure %1: %2");
  // FIG-08 to FIG-10: the status line
  inline constexpr const char* added = QT_TRANSLATE_NOOP("FigureSpec", "Added figure %1: %2. Click a figure to make it active.");
  inline constexpr const char* madeActive = QT_TRANSLATE_NOOP("FigureSpec", "Figure %1 is active: %2. Its joints and keys show now.");
  inline constexpr const char* beginnerSwitch = QT_TRANSLATE_NOOP("FigureSpec", "Switching figures is in Expert mode. View > Expert mode turns it on.");
  // FIG-11 and FIG-12: the timeline's row over each figure
  inline constexpr const char* activeChip = QT_TRANSLATE_NOOP("FigureSpec", "Active");
  inline constexpr const char* makeActive = QT_TRANSLATE_NOOP("FigureSpec", "Make active");
  inline constexpr const char* remove = QT_TRANSLATE_NOOP("FigureSpec", "Remove");
  inline constexpr const char* removeTip = QT_TRANSLATE_NOOP("FigureSpec", "Its file is not changed.");
  inline constexpr const char* makeActiveName = QT_TRANSLATE_NOOP("FigureSpec", "Make figure %1 active");
  inline constexpr const char* removeName = QT_TRANSLATE_NOOP("FigureSpec", "Remove figure %1 from the scene");
  // FIG-13: Remove with unsaved changes
  inline constexpr const char* removeTitle = QT_TRANSLATE_NOOP("FigureSpec", "Remove figure %1?");
  inline constexpr const char* removeLine = QT_TRANSLATE_NOOP("FigureSpec", "%1 has changes that are not saved.");
  inline constexpr const char* removeWithout = QT_TRANSLATE_NOOP("FigureSpec", "Remove without saving");
  inline constexpr const char* saveThenRemove = QT_TRANSLATE_NOOP("FigureSpec", "Save, then remove");
  // FIG-14: after Remove (the second line only when figure 1 went)
  inline constexpr const char* removed = QT_TRANSLATE_NOOP("FigureSpec", "Figure %1 removed from the scene. Its file is not changed.");
  inline constexpr const char* renumbered = QT_TRANSLATE_NOOP("FigureSpec", "%1 is figure 1 now.");
  // FIG-15 and FIG-16: the Selected joint panel's chip and the window title
  inline constexpr const char* chip = QT_TRANSLATE_NOOP("FigureSpec", "Figure %1");
  inline constexpr const char* titleSuffix = QT_TRANSLATE_NOOP("FigureSpec", "figure %1 of %2");
  // FIG-17: From the library was chosen in the Add window (the same words as FIG-04's line)
  inline constexpr const char* pickFromLibrary = QT_TRANSLATE_NOOP("FigureSpec", "Pick a move, then press Add as a second figure.");
  // FIG-18: tip of a greyed row of the other figure in the timeline
  inline constexpr const char* otherRowTip = QT_TRANSLATE_NOOP("FigureSpec", "Click to make figure %1 active and pick this joint.");
  // LIB-26: the Library panel's second button, Expert mode only
  inline constexpr const char* libraryAdd = QT_TRANSLATE_NOOP("FigureSpec", "Add as a second figure");
  inline constexpr const char* libraryAddTip = QT_TRANSLATE_NOOP("FigureSpec", "Puts this move on a second figure in the same scene.");

  QString tr(const char* source);

  // the name a figure shows: the file name with its ending ("m2m_walk-relaxed.bvh")
  QString shownName(const QString& path);
  // why a second figure cannot be added now, empty when it can (FIG-02, or MenuSpec::openFirst)
  QString addBlocked(bool animationOpen, int figures);
  // the status lines; numbers count from 1 as on screen
  QString addedMessage(int number, const QString& path);
  QString activeMessage(int number, const QString& path);
  // otherPath: the figure that is figure 1 now when figure 1 was removed, empty otherwise
  QString removedMessage(int number, const QString& otherPath);
  // "figure 2 of 2" after the file name; empty with one figure
  QString titleText(int number, int figures);
  // "Figure 2" beside the key count; empty with one figure
  QString chipText(int number, int figures);
  // the name tag over a figure's head: "2 · m2m_walk-relaxed.bvh"
  QString tagText(int number, const QString& path);
  // where the next figure stands: the first free place (0, 1, ...) next to the ones in use,
  // so adding or removing a figure never moves another one
  int freePlace(const QList<int>& used);
  // the sideways distance of a place from the first figure, in scene units
  float placeOffset(int place);
}

// File > Add a second figure...: From the library or From a file...
class AddFigureDialog : public QDialog
{
  Q_OBJECT

  public:
    enum class Choice { Cancel, Library, File };
    explicit AddFigureDialog(QWidget* parent = nullptr);
    Choice choice() const { return m_choice; }

    QLabel* titleLabel() const { return m_title; }
    QLabel* lineLabel() const { return m_line; }
    QPushButton* libraryButton() const { return m_library; }
    QPushButton* fileButton() const { return m_file; }
    QPushButton* cancelButton() const { return m_cancel; }

  private:
    Choice m_choice = Choice::Cancel;
    QLabel* m_title = nullptr;
    QLabel* m_line = nullptr;
    QPushButton* m_library = nullptr;
    QPushButton* m_file = nullptr;
    QPushButton* m_cancel = nullptr;
};

// the Figures box at the top right of the 3D view (two figures, Expert mode)
class FiguresBox : public QFrame
{
  Q_OBJECT

  public:
    explicit FiguresBox(QWidget* parent = nullptr);
    // the figures' files in figure order and the active one (0-based)
    void setFigures(const QStringList& paths, int active);
    int active() const { return m_active; }
    int count() const { return m_buttons.size(); }
    QLabel* heading() const { return m_heading; }
    QRadioButton* button(int index) const { return m_buttons.value(index); }
    // keeps itself at the top right of its parent
    void place();
    void applyLook();

  signals:
    // a figure was clicked or reached with the arrow keys (0-based)
    void activateRequested(int index);

  protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

  private:
    QLabel* m_heading = nullptr;
    QVBoxLayout* m_column = nullptr;
    QButtonGroup* m_group = nullptr;
    QList<QRadioButton*> m_buttons;
    int m_active = -1;
};

// the name tag over a figure's head in the 3D view: number and file name; the active
// one is filled with `selection`, bold and has a tick (charter P10: never colour alone)
class FigureTag : public QLabel
{
  Q_OBJECT

  public:
    explicit FigureTag(QWidget* parent = nullptr);
    void setTag(int number, const QString& path, bool active);
    bool isActive() const { return m_active; }
    int number() const { return m_number; }

  signals:
    void clicked();

  protected:
    void mousePressEvent(QMouseEvent* event) override;

  private:
    int m_number = 0;
    bool m_active = false;
};

// the timeline's row over one figure's joints
class FigureHeader : public QFrame
{
  Q_OBJECT

  public:
    explicit FigureHeader(QWidget* parent = nullptr);
    // number counts from 1; active: "Active" instead of Make active
    void setFigure(int number, const QString& path, bool active);
    // Beginner mode hides Make active and Remove without moving anything (they keep their room)
    void setExpert(bool expert);
    int number() const { return m_number; }
    bool isActive() const { return m_active; }

    QLabel* numberLabel() const { return m_numberLabel; }
    QLabel* nameLabel() const { return m_name; }
    QLabel* activeChip() const { return m_chip; }
    QPushButton* makeActiveButton() const { return m_makeActive; }
    QPushButton* removeButton() const { return m_remove; }
    void applyLook();

  signals:
    void makeActiveClicked();
    void removeClicked();

  private:
    int m_number = 0;
    bool m_active = false;
    bool m_expert = true;
    QLabel* m_numberLabel = nullptr;
    QLabel* m_name = nullptr;
    QLabel* m_chip = nullptr;
    QPushButton* m_makeActive = nullptr;
    QPushButton* m_remove = nullptr;
    void refresh();
};

// the other figure's changed joints in the timeline: greyed names and small grey keys
class FigureRows : public QWidget
{
  Q_OBJECT

  public:
    explicit FigureRows(QWidget* parent = nullptr);
    void setAnimation(Animation* anim, int number);
    Animation* animation() const { return m_animation; }
    // the joints' names as the timeline shows them
    void setNameOf(const std::function<QString(const QString&)>& nameOf) { m_nameOf = nameOf; update(); }
    // as wide as the timeline's name column; the keys follow the timeline's sideways scroll
    void setNamesWidth(int width);
    void setScrollX(int x);
    const TimelineRows& rows() const { return m_rows; }
    // the joint (track) of a row, -1 when none
    int trackAt(const QPoint& pos) const;
    QString shownName(int track) const;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override { return sizeHint(); }

  signals:
    // a row was clicked: make that figure active and pick this joint
    void rowClicked(int track);

  protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    bool event(QEvent* event) override;

  private:
    Animation* m_animation = nullptr;
    int m_number = 0;
    TimelineRows m_rows;
    std::function<QString(const QString&)> m_nameOf;
    int m_namesWidth = 0;
    int m_scrollX = 0;
};

// the question before Remove when the figure has unsaved changes (FIG-13)
namespace FigureRemove
{
  enum class Choice { Cancel, RemoveWithoutSaving, SaveThenRemove };
  QMessageBox* box(QWidget* parent, int number, const QString& path);
  Choice choiceOf(QMessageBox* box);
}

#endif
