/*
 * PinglyMotion - the Selected prop page of the Selected joint panel (Mockup 2)
 *
 * While Props is chosen in the Joints panel, the Selected joint panel shows
 * this page instead of the joint's: with no prop picked an empty card
 * ("Selected prop", "Pick a prop in the 3D view or in the list.",
 * PROPED-13 and PROPED-19); with a prop picked its name, the Shape choice
 * (Box, Ring, Cone, Ball, PROPED-23 and PROPED-11), and number fields for
 * Place (metres), Turn (degrees) and Size (Width, Height, Depth in metres;
 * PROPED-14 to PROPED-16), each row with its axis badge (X red, Y green,
 * Z blue, the letter always shown).
 *
 * The fields follow every drag in the 3D view (setState) and a change in a
 * field is sent on as stateEdited, so the window can put it on the prop and
 * make it one Undo step. The state the page holds keeps the exact inches: a
 * field the user did not touch never rounds its value.
 *
 * Shape fields like Second Life (cut, hollow, twist) are not built: a later job.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#ifndef SELECTEDPROPPANEL_H
#define SELECTEDPROPPANEL_H

#include <QWidget>

#include "propedit.h"

class QButtonGroup;
class QDoubleSpinBox;
class QGridLayout;
class QLabel;
class QPushButton;
class QStackedWidget;

namespace SelectedPropSpec
{
  inline QString tr(const char* text) { return QCoreApplication::translate("SelectedPropPanel",text); }

  constexpr const char* heading=QT_TRANSLATE_NOOP("SelectedPropPanel","Selected prop");                      // PROPED-13
  constexpr const char* pickLine=QT_TRANSLATE_NOOP("SelectedPropPanel","Pick a prop in the 3D view or in the list.");  // PROPED-19
  constexpr const char* shapeHeading=QT_TRANSLATE_NOOP("SelectedPropPanel","Shape");                         // PROPED-23
  constexpr const char* placeHeading=QT_TRANSLATE_NOOP("SelectedPropPanel","Place");                         // PROPED-14
  constexpr const char* turnHeading=QT_TRANSLATE_NOOP("SelectedPropPanel","Turn");
  constexpr const char* sizeHeading=QT_TRANSLATE_NOOP("SelectedPropPanel","Size");                           // PROPED-15
  // the four shapes in the mockup's order (PROPED-11) and the program's prop type of each
  // (Prop::PropType: Box 0, Sphere 1, Cone 2, Torus 3)
  constexpr const char* shapes[4]={QT_TRANSLATE_NOOP("SelectedPropPanel","Box"),QT_TRANSLATE_NOOP("SelectedPropPanel","Ring"),
                                   QT_TRANSLATE_NOOP("SelectedPropPanel","Cone"),QT_TRANSLATE_NOOP("SelectedPropPanel","Ball")};
  constexpr int shapeTypes[4]={0,3,2,1};
  constexpr const char* placeRows[3]={QT_TRANSLATE_NOOP("SelectedPropPanel","Left or right"),
                                      QT_TRANSLATE_NOOP("SelectedPropPanel","Up or down"),
                                      QT_TRANSLATE_NOOP("SelectedPropPanel","Forward or back")};
  constexpr const char* turnRows[3]={QT_TRANSLATE_NOOP("SelectedPropPanel","Tilt forward or back"),
                                     QT_TRANSLATE_NOOP("SelectedPropPanel","Turn left or right"),
                                     QT_TRANSLATE_NOOP("SelectedPropPanel","Lean to the side")};
  constexpr const char* sizeRows[3]={QT_TRANSLATE_NOOP("SelectedPropPanel","Width"),           // PROPED-16
                                     QT_TRANSLATE_NOOP("SelectedPropPanel","Height"),
                                     QT_TRANSLATE_NOOP("SelectedPropPanel","Depth")};
  constexpr const char* metres=QT_TRANSLATE_NOOP("SelectedPropPanel"," m");
  constexpr const char* degrees=QT_TRANSLATE_NOOP("SelectedPropPanel","°");

  // the fields: metres with two decimals and steps of 0.01 m, whole degrees in steps of 5
  // (the mockup's number fields), sizes from 0.02 m (PropEdit::Units::minSizeMetres)
  constexpr int metreDecimals=2;
  constexpr double metreStep=0.01;
  constexpr double placeLimit=1000.0;
  constexpr double sizeLimit=100.0;
  constexpr double degreeStep=5.0;
}

class SelectedPropPanel : public QWidget
{
  Q_OBJECT

  public:
    explicit SelectedPropPanel(QWidget* parent=nullptr);

    // the empty card: no prop picked
    void showNothing();
    // a picked prop: its name, prop type (Prop::PropType) and state
    void showProp(const QString& name,int propType,const PropEdit::State& state);
    bool showingProp() const;
    QString propName() const { return m_name; }

    // the fields follow the prop (no stateEdited)
    void setState(const PropEdit::State& state);
    const PropEdit::State& state() const { return m_state; }
    void setPropType(int propType);
    int propType() const { return m_type; }

    QDoubleSpinBox* placeField(int axis) const { return m_place[axis]; }
    QDoubleSpinBox* turnField(int axis) const { return m_turn[axis]; }
    QDoubleSpinBox* sizeField(int axis) const { return m_size[axis]; }
    // the shape button of a prop type
    QPushButton* shapeButton(int propType) const;

    void applyLook();

  signals:
    // the user changed a field: the state before and after (inches, degrees)
    void stateEdited(const PropEdit::State& before,const PropEdit::State& after);
    // the user chose another shape (a prop type)
    void shapeChosen(int propType);

  protected:
    void changeEvent(QEvent* event) override;

  private:
    QWidget* buildNothingPage();
    QWidget* buildPropPage();
    QDoubleSpinBox* addRow(QWidget* parent,QGridLayout* grid,int row,int axis,const char* label);

    QStackedWidget* m_pages=nullptr;
    QLabel* m_nothingIcon=nullptr;
    QLabel* m_nothingHeading=nullptr;
    QLabel* m_nothingLine=nullptr;
    QLabel* m_nameLabel=nullptr;
    QLabel* m_chip=nullptr;
    QList<QLabel*> m_headings;
    QList<QLabel*> m_badges;
    QList<QLabel*> m_rowLabels;
    QPushButton* m_shapes[4]={nullptr,nullptr,nullptr,nullptr};
    QButtonGroup* m_shapeGroup=nullptr;
    QDoubleSpinBox* m_place[3]={nullptr,nullptr,nullptr};
    QDoubleSpinBox* m_turn[3]={nullptr,nullptr,nullptr};
    QDoubleSpinBox* m_size[3]={nullptr,nullptr,nullptr};
    QString m_name;
    int m_type=0;
    PropEdit::State m_state;
    bool m_styling=false;
    bool m_typeShown=false;
};

#endif
