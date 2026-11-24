/*
 * PinglyMotion - the Selected prop page of the Selected joint panel (see selectedproppanel.h)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "selectedproppanel.h"

#include <QButtonGroup>
#include <QDoubleSpinBox>
#include <QEvent>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "lucideicons.h"
#include "textsize.h"
#include "theme.h"

using namespace SelectedPropSpec;

namespace
{
  QFont labelFont()
  {
    QFont font;
    font.setWeight(QFont::DemiBold);
    return font;
  }

  float component(const QVector3D& v,int axis)
  {
    return axis==0 ? v.x() : axis==1 ? v.y() : v.z();
  }

  void setComponent(QVector3D& v,int axis,double value)
  {
    if(axis==0) v.setX(float(value));
    else if(axis==1) v.setY(float(value));
    else v.setZ(float(value));
  }

  Theme::Token axisToken(int axis)
  {
    return axis==0 ? Theme::Token::AxisX : (axis==1 ? Theme::Token::AxisY : Theme::Token::AxisZ);
  }
}

SelectedPropPanel::SelectedPropPanel(QWidget* parent) : QWidget(parent)
{
  setObjectName(QStringLiteral("selectedPropPanel"));
  QVBoxLayout* outer=new QVBoxLayout(this);
  outer->setContentsMargins(0,0,0,0);
  m_pages=new QStackedWidget(this);
  m_pages->addWidget(buildNothingPage());
  m_pages->addWidget(buildPropPage());
  outer->addWidget(m_pages);
  outer->addStretch(1);
  showNothing();
  applyLook();
}

QWidget* SelectedPropPanel::buildNothingPage()
{
  // the card at the top of its page, as the joint page's empty card (not stretched to the panel)
  QWidget* page=new QWidget(this);
  QVBoxLayout* outer=new QVBoxLayout(page);
  outer->setContentsMargins(0,0,0,0);
  QFrame* card=new QFrame(page);
  card->setObjectName(QStringLiteral("selectedPropNothing"));
  QVBoxLayout* column=new QVBoxLayout(card);
  column->setSpacing(Theme::Size::spacingS);
  QHBoxLayout* head=new QHBoxLayout;
  m_nothingIcon=new QLabel(card);
  m_nothingIcon->setObjectName(QStringLiteral("selectedPropNothingIcon"));
  m_nothingHeading=new QLabel(tr(heading),card);
  m_nothingHeading->setObjectName(QStringLiteral("selectedPropNothingHeading"));
  m_nothingHeading->setFont(labelFont());
  head->addWidget(m_nothingIcon);
  head->addWidget(m_nothingHeading,1);
  column->addLayout(head);
  m_nothingLine=new QLabel(tr(pickLine),card);
  m_nothingLine->setObjectName(QStringLiteral("selectedPropNothingLine"));
  m_nothingLine->setWordWrap(true);
  column->addWidget(m_nothingLine);
  outer->addWidget(card);
  outer->addStretch(1);
  return page;
}

QDoubleSpinBox* SelectedPropPanel::addRow(QWidget* parent,QGridLayout* grid,int row,int axis,const char* label)
{
  QLabel* badge=new QLabel(PropEdit::axisLetter(axis),parent);
  badge->setObjectName(QStringLiteral("propAxisBadge"));
  badge->setAlignment(Qt::AlignCenter);
  badge->setFont(labelFont());
  m_badges << badge;
  QLabel* name=new QLabel(tr(label),parent);
  m_rowLabels << name;
  QDoubleSpinBox* field=new QDoubleSpinBox(parent);
  field->setKeyboardTracking(false);
  field->setAccessibleName(tr(label));
  field->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  name->setBuddy(field);
  grid->addWidget(badge,row,0);
  grid->addWidget(name,row,1);
  grid->addWidget(field,row,2);
  return field;
}

QWidget* SelectedPropPanel::buildPropPage()
{
  QWidget* page=new QWidget(this);
  page->setObjectName(QStringLiteral("selectedPropPage"));
  QVBoxLayout* column=new QVBoxLayout(page);
  column->setContentsMargins(0,0,0,0);
  column->setSpacing(Theme::Size::spacingM);

  // the prop's name and the chip "Selected prop"
  m_nameLabel=new QLabel(page);
  m_nameLabel->setObjectName(QStringLiteral("selectedPropName"));
  QFont big=labelFont();
  big.setPointSizeF(big.pointSizeF()*1.2);
  m_nameLabel->setFont(big);
  column->addWidget(m_nameLabel);
  QHBoxLayout* chipRow=new QHBoxLayout;
  m_chip=new QLabel(tr(heading),page);
  m_chip->setObjectName(QStringLiteral("selectedPropChip"));
  chipRow->addWidget(m_chip);
  chipRow->addStretch(1);
  column->addLayout(chipRow);

  auto headingLabel=[&](const char* text)
  {
    QLabel* label=new QLabel(tr(text),page);
    label->setFont(labelFont());
    m_headings << label;
    column->addWidget(label);
    return label;
  };

  // Shape: a radio group of the four shapes
  QLabel* shapeLabel=headingLabel(shapeHeading);
  shapeLabel->setObjectName(QStringLiteral("selectedPropShapeHeading"));
  QHBoxLayout* shapeRow=new QHBoxLayout;
  shapeRow->setSpacing(0);
  m_shapeGroup=new QButtonGroup(this);
  m_shapeGroup->setExclusive(true);
  for(int i=0;i<4;i++)
  {
    QPushButton* button=new QPushButton(tr(shapes[i]),page);
    button->setObjectName(QStringLiteral("propShape%1").arg(i));
    button->setCheckable(true);
    button->setAutoDefault(false);
    button->setFocusPolicy(Qt::StrongFocus);
    button->setAccessibleName(tr(shapes[i]));
    button->setAccessibleDescription(tr(shapeHeading));
    button->setProperty("propType",shapeTypes[i]);
    m_shapeGroup->addButton(button,shapeTypes[i]);
    m_shapes[i]=button;
    shapeRow->addWidget(button,1);
    connect(button,&QPushButton::clicked,this,[this,i]()
    {
      const int type=shapeTypes[i];
      if(type==m_type) return;
      m_type=type;
      applyLook();
      emit shapeChosen(type);
    });
  }
  column->addLayout(shapeRow);

  auto section=[&](const char* title,const char* const rows[3],QDoubleSpinBox** fields,int kind)
  {
    headingLabel(title);
    QWidget* box=new QWidget(page);
    box->setObjectName(QStringLiteral("propAxisBox"));
    QGridLayout* grid=new QGridLayout(box);
    grid->setContentsMargins(0,0,0,0);
    grid->setHorizontalSpacing(Theme::Size::spacingS);
    grid->setVerticalSpacing(Theme::Size::spacingXs);
    grid->setColumnStretch(1,1);
    for(int axis=0;axis<3;axis++)
    {
      QDoubleSpinBox* field=addRow(box,grid,axis,axis,rows[axis]);
      fields[axis]=field;
      if(kind==1)
      {
        field->setObjectName(QStringLiteral("propTurn%1").arg(PropEdit::axisLetter(axis)));
        field->setDecimals(0);
        field->setRange(0.0,359.0);
        field->setWrapping(true);
        field->setSingleStep(degreeStep);
        field->setSuffix(tr(degrees));
      }
      else
      {
        field->setObjectName((kind==0 ? QStringLiteral("propPlace%1") : QStringLiteral("propSize%1")).arg(PropEdit::axisLetter(axis)));
        field->setDecimals(metreDecimals);
        field->setSingleStep(metreStep);
        field->setSuffix(tr(metres));
        if(kind==0) field->setRange(-placeLimit,placeLimit);
        else field->setRange(PropEdit::Units::minSizeMetres,sizeLimit);
      }
      connect(field,&QDoubleSpinBox::valueChanged,this,[this,kind,axis](double value)
      {
        const PropEdit::State before=m_state;
        PropEdit::State after=m_state;
        if(kind==0) setComponent(after.position,axis,PropEdit::Units::inches(value));
        else if(kind==1) setComponent(after.rotation,axis,PropEdit::wrapDegrees(value));
        else setComponent(after.scale,axis,qMax(PropEdit::Units::minSize(),PropEdit::Units::inches(value)));
        if(after==before) return;
        m_state=after;
        emit stateEdited(before,after);
      });
    }
    column->addWidget(box);
  };
  section(placeHeading,placeRows,m_place,0);
  section(turnHeading,turnRows,m_turn,1);
  section(sizeHeading,sizeRows,m_size,2);
  return page;
}

void SelectedPropPanel::showNothing()
{
  m_name.clear();
  m_pages->setCurrentIndex(0);
}

void SelectedPropPanel::showProp(const QString& name,int propType,const PropEdit::State& state)
{
  m_name=name;
  m_nameLabel->setText(name);
  m_pages->setCurrentIndex(1);
  setPropType(propType);
  setState(state);
}

bool SelectedPropPanel::showingProp() const
{
  return m_pages->currentIndex()==1;
}

void SelectedPropPanel::setPropType(int propType)
{
  // the window asks often: nothing to do when nothing changed
  if(propType==m_type && m_typeShown) return;
  m_typeShown=true;
  m_type=propType;
  for(QPushButton* button : m_shapes)
  {
    const QSignalBlocker block(button);
    button->setChecked(button->property("propType").toInt()==propType);
  }
  applyLook();
}

QPushButton* SelectedPropPanel::shapeButton(int propType) const
{
  for(QPushButton* button : m_shapes)
    if(button->property("propType").toInt()==propType) return button;
  return nullptr;
}

void SelectedPropPanel::setState(const PropEdit::State& state)
{
  m_state=state;
  for(int axis=0;axis<3;axis++)
  {
    const QSignalBlocker a(m_place[axis]), b(m_turn[axis]), c(m_size[axis]);
    m_place[axis]->setValue(PropEdit::Units::metres(component(state.position,axis)));
    m_turn[axis]->setValue(PropEdit::wrapDegrees(component(state.rotation,axis)));
    m_size[axis]->setValue(PropEdit::Units::metres(component(state.scale,axis)));
  }
}

void SelectedPropPanel::changeEvent(QEvent* event)
{
  QWidget::changeEvent(event);
  if(event->type()==QEvent::ApplicationPaletteChange && !m_styling) applyLook();
}

void SelectedPropPanel::applyLook()
{
  m_styling=true;
  using Theme::Token;
  const QColor text=Theme::color(Token::Text);
  const QColor muted=Theme::color(Token::TextMuted);
  const QColor surface=Theme::color(Token::Surface);
  const QColor raised=Theme::color(Token::SurfaceRaised);
  const QColor border=Theme::color(Token::Border);
  const QColor disabled=Theme::color(Token::TextDisabled);
  const QColor selection=Theme::color(Token::Selection);
  const QColor onAccent=Theme::color(Token::OnAccent);
  const QColor area=Theme::color(Token::AreaOther);
  const int icon=TextSize::scaled(Theme::Size::iconSmall);

  if(QFrame* card=findChild<QFrame*>(QStringLiteral("selectedPropNothing")))
    card->setStyleSheet(QStringLiteral("QFrame#selectedPropNothing { background: %1; border: %2px solid %3; border-radius: %4px; }"
                                       "QLabel { border: none; background: transparent; }")
                          .arg(surface.name()).arg(Theme::Size::borderWidth).arg(border.name()).arg(Theme::Size::radiusPanel));
  m_nothingIcon->setPixmap(LucideIcons::icon(QStringLiteral("shapes"),text,disabled).pixmap(icon,icon));
  m_nothingHeading->setStyleSheet(QStringLiteral("color: %1;").arg(text.name()));
  m_nothingLine->setStyleSheet(QStringLiteral("color: %1;").arg(muted.name()));
  m_nameLabel->setStyleSheet(QStringLiteral("color: %1;").arg(text.name()));
  // the chip: the area colour of props (area_other) as the edge, as the joint card's area chip
  m_chip->setStyleSheet(QStringLiteral("QLabel { border: %1px solid %2; border-radius: %3px; padding: 0px %4px; color: %5; background: %6; }")
                          .arg(Theme::Size::borderWidth).arg(area.name()).arg(Theme::Size::radiusSmall).arg(Theme::Size::spacingS)
                          .arg(text.name(),Theme::mix(surface,area,0.14).name()));
  for(QLabel* heading : m_headings) heading->setStyleSheet(QStringLiteral("color: %1;").arg(text.name()));
  for(QLabel* label : m_rowLabels) label->setStyleSheet(QStringLiteral("color: %1;").arg(text.name()));
  // the axis badges: the axis colour as fill, the letter in on_axis (DESIGN_SYSTEM.md, "Axis colours")
  for(int i=0;i<m_badges.size();i++)
  {
    m_badges[i]->setStyleSheet(QStringLiteral("QLabel { background: %1; color: %2; border-radius: %3px; padding: 0px %4px; }")
                                 .arg(Theme::color(axisToken(i%3)).name(),Theme::color(Token::OnAxis).name())
                                 .arg(Theme::Size::radiusSmall).arg(Theme::Size::spacingXs));
    m_badges[i]->setMinimumWidth(TextSize::scaled(Theme::Size::iconSmall+Theme::Size::spacingXs));
  }
  // the shape choice as one segmented row; the one that is on in selection, bold and with a tick (P10)
  const QString radius=QString::number(Theme::Size::radiusControl);
  for(int i=0;i<4;i++)
  {
    QPushButton* button=m_shapes[i];
    const bool on=button->isChecked();
    QFont font=button->font();
    font.setWeight(on ? QFont::Bold : QFont::Normal);
    button->setFont(font);
    button->setIcon(on ? LucideIcons::icon(QStringLiteral("check"),onAccent,disabled) : QIcon());
    button->setIconSize(QSize(icon,icon));
    QString corners=QStringLiteral("border-radius: 0px;");
    if(i==0) corners=QStringLiteral("border-top-left-radius: %1px; border-bottom-left-radius: %1px;").arg(radius);
    if(i==3) corners=QStringLiteral("border-top-right-radius: %1px; border-bottom-right-radius: %1px;").arg(radius);
    button->setStyleSheet(QStringLiteral(
      "QPushButton { background: %1; color: %2; border: %3px solid %4; %5 padding: %6px %7px; }"
      "QPushButton:hover { background: %8; }"
      "QPushButton:checked { background: %9; color: %10; border-color: %9; }"
      "QPushButton:focus { border: %11px solid %12; }")
      .arg(raised.name(),text.name()).arg(Theme::Size::borderWidth).arg(border.name(),corners)
      .arg(Theme::Size::spacingXs).arg(Theme::Size::spacingS)
      .arg(Theme::mix(raised,text,0.08).name(),selection.name(),onAccent.name())
      .arg(Theme::Size::focusRingWidth).arg(Theme::color(Token::FocusRing).name()));
  }
  // the number fields line up: all as wide as the widest
  int wide=0;
  for(int axis=0;axis<3;axis++)
    for(QDoubleSpinBox* field : {m_place[axis],m_turn[axis],m_size[axis]})
    {
      field->setMinimumWidth(0);
      wide=qMax(wide,field->sizeHint().width());
    }
  for(int axis=0;axis<3;axis++)
    for(QDoubleSpinBox* field : {m_place[axis],m_turn[axis],m_size[axis]}) field->setMinimumWidth(wide);
  m_styling=false;
}
