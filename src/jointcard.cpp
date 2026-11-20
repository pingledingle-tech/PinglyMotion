/*
 * PinglyMotion - the selected-joint card (#88, posing UX step 4)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "jointcard.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QSlider>
#include <QStackedWidget>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>

#include "jointinfo.h"

namespace
{
  // axis colours as in the 3D view's rings: X red, Y green, Z blue
  const QColor AXIS_COLOUR[3]={QColor(0xe0,0x50,0x50),QColor(0x30,0xb0,0x30),QColor(0x3c,0x78,0xff)};
  const char* const AXIS_NAME[3]={"X","Y","Z"};
  // the value field of an axis near its limit
  const char* const NEAR_LIMIT_STYLE="color: #d08000; font-weight: bold;";
  // degrees per pixel of trackball drag; Shift turns five times finer
  const double TRACKBALL_DEGREES_PER_PIXEL=0.5;
  const double TRACKBALL_FINE=0.2;
}

// ---------------------------------------------------------------------------
// the ball

JointTrackball::JointTrackball(QWidget* parent) : QWidget(parent)
{
  setObjectName("jointTrackball");
  setCursor(Qt::OpenHandCursor);
  setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
  setToolTip(tr("Drag the ball to turn the joint freely. Hold Shift for finer control. "
                "\"Turn around\" below chooses the axes it turns around."));
}

QSize JointTrackball::sizeHint() const
{
  return QSize(96,96);
}

QSize JointTrackball::minimumSizeHint() const
{
  return QSize(72,72);
}

void JointTrackball::setOrientation(const QQuaternion& orientation)
{
  m_orientation=orientation.normalized();
  update();
}

void JointTrackball::paintEvent(QPaintEvent*)
{
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing);
  const QPointF centre(width()/2.0,height()/2.0);
  const double radius=std::min(width(),height())/2.0-2.0;

  QColor ball=palette().color(isEnabled() ? QPalette::Active : QPalette::Disabled,QPalette::Base);
  QColor rim=palette().color(isEnabled() ? QPalette::Active : QPalette::Disabled,QPalette::Mid);
  p.setPen(QPen(rim,1.5));
  p.setBrush(ball);
  p.drawEllipse(centre,radius,radius);

  // the three axes of the joint, seen from the front of the screen: the
  // axes pointing away are drawn first and thinner
  struct Line { int axis; QVector3D tip; };
  std::array<Line,3> lines;
  for(int axis=0;axis<3;axis++)
  {
    QVector3D unit;
    unit[axis]=1.0f;
    lines[axis]={axis,m_orientation.rotatedVector(unit)};
  }
  std::sort(lines.begin(),lines.end(),[](const Line& a,const Line& b){ return a.tip.z()<b.tip.z(); });
  for(const Line& line : lines)
  {
    QColor colour=AXIS_COLOUR[line.axis];
    if(!isEnabled()) colour=rim;
    const bool away=line.tip.z()<0;
    if(away) colour.setAlpha(110);
    p.setPen(QPen(colour,away ? 1.5 : 2.5,Qt::SolidLine,Qt::RoundCap));
    const QPointF tip=centre+QPointF(line.tip.x(),-line.tip.y())*radius*0.8;
    p.drawLine(centre,tip);
  }
  p.setPen(Qt::NoPen);
  p.setBrush(palette().color(QPalette::Text));
  p.drawEllipse(centre,2.5,2.5);
}

void JointTrackball::mousePressEvent(QMouseEvent* event)
{
  if(event->button()!=Qt::LeftButton)
  {
    QWidget::mousePressEvent(event);
    return;
  }
  m_dragging=true;
  m_lastPos=event->position();
  setCursor(Qt::ClosedHandCursor);
  event->accept();
}

void JointTrackball::mouseMoveEvent(QMouseEvent* event)
{
  if(!m_dragging)
  {
    QWidget::mouseMoveEvent(event);
    return;
  }
  const QPointF delta=event->position()-m_lastPos;
  m_lastPos=event->position();
  if(!delta.isNull())
    emit dragged(delta.x(),delta.y(),event->modifiers() & Qt::ShiftModifier);
  event->accept();
}

void JointTrackball::mouseReleaseEvent(QMouseEvent* event)
{
  if(event->button()==Qt::LeftButton && m_dragging)
  {
    m_dragging=false;
    setCursor(Qt::OpenHandCursor);
    event->accept();
    return;
  }
  QWidget::mouseReleaseEvent(event);
}

// ---------------------------------------------------------------------------
// the card

JointCard::JointCard(int precision,const std::array<AxisRow,3>& rotation,
                     const std::array<AxisRow,3>& position,QWidget* parent)
  : QWidget(parent),
    m_precision(precision>0 ? precision : 1),
    m_rotation(rotation),
    m_position(position)
{
  setObjectName("jointCard");
  QVBoxLayout* layout=new QVBoxLayout(this);
  layout->setContentsMargins(0,0,0,0);

  // --- header: name and SL name on the left; IK, lock, reset on the right
  QHBoxLayout* header=new QHBoxLayout;
  QVBoxLayout* names=new QVBoxLayout;
  names->setSpacing(0);
  m_title=new QLabel;
  m_title->setObjectName("jointCardTitle");
  QFont titleFont=m_title->font();
  titleFont.setBold(true);
  titleFont.setPointSizeF(titleFont.pointSizeF()*1.25);
  m_title->setFont(titleFont);
  m_title->setTextInteractionFlags(Qt::TextSelectableByMouse);
  m_slName=new QLabel;
  m_slName->setObjectName("jointCardSlName");
  QFont slFont(QStringLiteral("monospace"));
  slFont.setStyleHint(QFont::Monospace);
  slFont.setPointSizeF(m_slName->font().pointSizeF()*0.9);
  m_slName->setFont(slFont);
  m_slFont=slFont;
  m_slName->setTextInteractionFlags(Qt::TextSelectableByMouse);
  m_slName->setToolTip(tr("The joint's name in Second Life"));
  names->addWidget(m_title);
  names->addWidget(m_slName);
  header->addLayout(names,1);

  m_ik=new QCheckBox(tr("IK"));
  m_ik->setObjectName("jointCardIk");
  header->addWidget(m_ik);
  m_lock=new QToolButton;
  m_lock->setObjectName("jointCardLock");
  m_lock->setText(tr("Lock"));
  m_lock->setCheckable(true);
  m_lock->setToolTip(tr("Lock this joint: the sliders, the trackball and the rings in the 3D view leave it alone."));
  header->addWidget(m_lock);
  m_reset=new QToolButton;
  m_reset->setObjectName("jointCardReset");
  m_reset->setText(tr("Reset"));
  header->addWidget(m_reset);
  layout->addLayout(header);

  // --- Rotate / Move
  QHBoxLayout* tabRow=new QHBoxLayout;
  tabRow->setSpacing(0);
  m_rotateTab=new QToolButton;
  m_rotateTab->setObjectName("jointCardRotateTab");
  m_rotateTab->setText(tr("Rotate"));
  m_moveTab=new QToolButton;
  m_moveTab->setObjectName("jointCardMoveTab");
  m_moveTab->setText(tr("Move"));
  m_moveTab->setToolTip(tr("Move the whole avatar"));
  m_tabs=new QButtonGroup(this);
  m_tabs->setExclusive(true);
  for(QToolButton* tab : {m_rotateTab,m_moveTab})
  {
    tab->setCheckable(true);
    tab->setAutoRaise(true);
    tab->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
    tabRow->addWidget(tab);
  }
  m_tabs->addButton(m_rotateTab,RotatePage);
  m_tabs->addButton(m_moveTab,MovePage);
  m_rotateTab->setChecked(true);
  layout->addLayout(tabRow);

  m_pages=new QStackedWidget;
  m_pages->setObjectName("jointCardPages");

  // Rotate page: trackball and sliders side by side, the warning below
  QWidget* rotatePage=new QWidget;
  QVBoxLayout* rotateLayout=new QVBoxLayout(rotatePage);
  rotateLayout->setContentsMargins(0,4,0,0);
  QHBoxLayout* ballAndSliders=new QHBoxLayout;
  m_trackball=new JointTrackball;
  ballAndSliders->addWidget(m_trackball,0,Qt::AlignTop);
  ballAndSliders->addWidget(makeSliderGrid(m_rotation,m_rotationRange,QStringLiteral("jointCardRotation")),1);
  rotateLayout->addLayout(ballAndSliders);
  m_warning=new QLabel;
  m_warning->setObjectName("jointCardWarning");
  m_warning->setWordWrap(true);
  m_warning->setStyleSheet(QStringLiteral("QLabel { background: rgba(230,160,0,0.18); border: 1px solid rgba(230,160,0,0.7);"
                                          " border-radius: 4px; padding: 4px; }"));
  m_warning->hide();
  rotateLayout->addWidget(m_warning);
  rotateLayout->addStretch(1);
  m_pages->addWidget(rotatePage);

  // Move page: the avatar position sliders
  QWidget* movePage=new QWidget;
  QVBoxLayout* moveLayout=new QVBoxLayout(movePage);
  moveLayout->setContentsMargins(0,4,0,0);
  moveLayout->addWidget(makeSliderGrid(m_position,m_positionRange,QStringLiteral("jointCardPosition")));
  moveLayout->addStretch(1);
  m_pages->addWidget(movePage);
  layout->addWidget(m_pages);

  // --- Turn around: Joint / Avatar / Screen
  m_frameRow=new QWidget;
  QHBoxLayout* frameLayout=new QHBoxLayout(m_frameRow);
  frameLayout->setContentsMargins(0,4,0,0);
  frameLayout->setSpacing(2);
  QLabel* frameLabel=new QLabel(tr("Turn around"));
  frameLabel->setToolTip(tr("The axes the trackball turns the joint around"));
  frameLayout->addWidget(frameLabel);
  frameLayout->addSpacing(6);
  m_frames=new QButtonGroup(this);
  m_frames->setExclusive(true);
  const struct { JointTurn::Frame frame; const char* name; const char* objectName; const char* tip; } frames[]={
    {JointTurn::JointFrame,QT_TR_NOOP("Joint"),"jointCardFrameJoint",QT_TR_NOOP("Turn around the joint's own axes")},
    {JointTurn::AvatarFrame,QT_TR_NOOP("Avatar"),"jointCardFrameAvatar",QT_TR_NOOP("Turn around the avatar's axes: up, sideways, forward")},
    {JointTurn::ScreenFrame,QT_TR_NOOP("Screen"),"jointCardFrameScreen",QT_TR_NOOP("Turn the way the mouse moves on the screen")},
  };
  for(const auto& f : frames)
  {
    QToolButton* button=new QToolButton;
    button->setObjectName(QString::fromLatin1(f.objectName));
    button->setText(tr(f.name));
    button->setToolTip(tr(f.tip));
    button->setCheckable(true);
    button->setAutoRaise(true);
    m_frames->addButton(button,f.frame);
    frameLayout->addWidget(button);
  }
  frameLayout->addStretch(1);
  m_frames->button(JointTurn::JointFrame)->setChecked(true);
  layout->addWidget(m_frameRow);
  layout->addStretch(1);

  connect(m_tabs,&QButtonGroup::idClicked,this,[this](int id){ m_pages->setCurrentIndex(id); });
  connect(m_frames,&QButtonGroup::idClicked,this,[this](int id){ emit frameChanged(static_cast<JointTurn::Frame>(id)); });
  connect(m_ik,&QCheckBox::toggled,this,&JointCard::ikToggled);
  connect(m_lock,&QToolButton::toggled,this,[this](bool on){ updateEnabled(); emit lockToggled(on); });
  connect(m_reset,&QToolButton::clicked,this,&JointCard::resetClicked);
  connect(m_trackball,&JointTrackball::dragged,this,&JointCard::trackballDragged);
  for(int axis=0;axis<3;axis++)
  {
    for(QSlider* slider : {m_rotation[axis].slider,m_position[axis].slider})
      connect(slider,&QSlider::rangeChanged,this,&JointCard::updateRanges);
    connect(m_rotation[axis].slider,&QSlider::valueChanged,this,&JointCard::updateWarning);
    connect(m_rotation[axis].slider,&QSlider::rangeChanged,this,&JointCard::updateWarning);
  }

  setIk(false,false);
  setJoint(NoJoint,Header());
  updateRanges();
  updateDimmedText();
}

void JointCard::changeEvent(QEvent* event)
{
  QWidget::changeEvent(event);
  // a new theme: the pale texts follow its text colour
  if(event->type()==QEvent::PaletteChange && !m_dimming) updateDimmedText();
}

void JointCard::updateDimmedText()
{
  m_dimming=true;
  QColor pale=palette().color(QPalette::WindowText);
  pale.setAlpha(165);
  QList<QLabel*> labels{m_slName};
  for(QLabel* label : m_rotationRange) labels << label;
  for(QLabel* label : m_positionRange) labels << label;
  for(QLabel* label : labels)
  {
    QPalette p=label->palette();
    p.setColor(QPalette::WindowText,pale);
    label->setPalette(p);
  }
  m_dimming=false;
}

void JointCard::refresh()
{
  updateRanges();
  updateWarning();
}

// a grid of three rows: axis letter, slider with its range below, value field
QWidget* JointCard::makeSliderGrid(std::array<AxisRow,3>& rows,std::array<QLabel*,6>& rangeLabels,
                                   const QString& objectPrefix)
{
  QWidget* grid=new QWidget;
  QGridLayout* layout=new QGridLayout(grid);
  layout->setContentsMargins(0,0,0,0);
  layout->setVerticalSpacing(2);
  for(int axis=0;axis<3;axis++)
  {
    AxisRow& row=rows[axis];
    if(!row.slider)
    {
      row.slider=new QSlider(Qt::Horizontal);
      row.slider->setObjectName(objectPrefix+QLatin1String(AXIS_NAME[axis])+QLatin1String("Slider"));
    }
    if(!row.edit)
    {
      row.edit=new QLineEdit;
      row.edit->setObjectName(objectPrefix+QLatin1String(AXIS_NAME[axis])+QLatin1String("Edit"));
    }
    row.slider->setOrientation(Qt::Horizontal);
    row.slider->setMinimumWidth(80);
    row.edit->setMaximumWidth(64);
    row.edit->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    QLabel* letter=new QLabel(QLatin1String(AXIS_NAME[axis]));
    letter->setStyleSheet(QStringLiteral("color: %1; font-weight: bold;").arg(AXIS_COLOUR[axis].name()));
    layout->addWidget(letter,axis*2,0);
    layout->addWidget(row.slider,axis*2,1);
    layout->addWidget(row.edit,axis*2,2);

    // the allowed range, small, under the slider: minimum left, maximum right
    QHBoxLayout* range=new QHBoxLayout;
    range->setContentsMargins(0,0,0,0);
    for(int end=0;end<2;end++)
    {
      QLabel* label=new QLabel;
      label->setObjectName(objectPrefix+QLatin1String(AXIS_NAME[axis])+(end==0 ? QLatin1String("Min") : QLatin1String("Max")));
      QFont small=label->font();
      small.setPointSizeF(small.pointSizeF()*0.8);
      label->setFont(small);
      rangeLabels[axis*2+end]=label;
      if(end==1) range->addStretch(1);
      range->addWidget(label);
    }
    layout->addLayout(range,axis*2+1,1);
  }
  layout->setColumnStretch(1,1);
  return grid;
}

JointCard::Header JointCard::headerFor(const JointInfo* info,const QString& fileName)
{
  Header header;
  if(fileName==QLatin1String("position"))
  {
    header.title=tr("Avatar Position");
    header.slName=tr("whole avatar");
    return header;
  }
  const JointInfo::Joint* joint=info ? info->joint(fileName) : nullptr;
  if(!joint)
  {
    header.title=fileName;
    return header;
  }
  header.title=joint->name.isEmpty() ? fileName : joint->name;
  header.slName=joint->sl;
  // "Shoulder · Left": the side from the section ("Left Arm", "Right Hand"),
  // unless the name says it already ("Right Ankle")
  const struct { const char* word; QString text; } sides[]={{"Left",tr("Left")},{"Right",tr("Right")}};
  for(const auto& side : sides)
  {
    const QString word=QLatin1String(side.word);
    if(joint->firestormSection.startsWith(word+QLatin1Char(' ')) &&
       !header.title.contains(word,Qt::CaseInsensitive))
      header.title+=QStringLiteral(" \u00b7 ")+side.text;
  }
  return header;
}

double JointCard::nearLimitMargin(double minimum,double maximum)
{
  return qBound(2.0,(maximum-minimum)/10.0,10.0);
}

QString JointCard::limitWarning(const QString& axis,double value,double minimum,double maximum)
{
  if(maximum-minimum<0.01) return QString();
  const double margin=nearLimitMargin(minimum,maximum);
  // the limit nearer to the value
  const bool upper=(maximum-value)<=(value-minimum);
  const double limit=upper ? maximum : minimum;
  const double distance=std::abs(limit-value);
  if(distance>margin) return QString();
  // the rest pose sits at a limit of 0 for many joints (a straight elbow or
  // knee): nothing to warn about
  if(std::abs(limit)<0.05 && std::abs(value)<0.05) return QString();
  if(distance<0.05)
    return tr("%1 is at the joint limit (%2)").arg(axis,degrees(limit));
  return tr("%1 is close to the joint limit (%2)").arg(axis,degrees(limit));
}

QString JointCard::degrees(double value)
{
  // whole degrees without decimals, others with one
  const double rounded=std::round(value*10.0)/10.0;
  if(rounded==std::round(rounded)) return QStringLiteral("%1°").arg(static_cast<int>(std::round(rounded)));
  return QStringLiteral("%1°").arg(rounded,0,'f',1);
}

void JointCard::setJoint(Kind kind,const Header& header)
{
  m_kind=kind;
  m_title->setText(kind==NoJoint ? tr("No joint selected") : header.title);
  m_slName->setText(kind==NoJoint ? tr("Click a joint in the 3D view or in the list") : header.slName);
  // the SL name in a typewriter font, the hint in the normal one
  m_slName->setFont(kind==NoJoint ? font() : m_slFont);
  m_slName->setVisible(!m_slName->text().isEmpty());

  // Move only for the pelvis and the avatar position; the avatar position
  // has no rotation
  m_moveTab->setVisible(kind==Root || kind==PositionTrack);
  m_rotateTab->setVisible(kind!=PositionTrack);
  if(kind==PositionTrack) setPage(MovePage);
  else if(kind!=Root) setPage(RotatePage);
  m_frameRow->setVisible(kind!=PositionTrack);
  m_reset->setToolTip(kind==PositionTrack
                      ? tr("Put the avatar back where it stands in the first frame")
                      : tr("Turn this joint back to its rest pose (0, 0, 0) in this frame"));
  updateEnabled();
  refresh();
}

void JointCard::setIk(bool available,bool on)
{
  m_ikAvailable=available;
  m_ik->blockSignals(true);
  m_ik->setChecked(available && on);
  m_ik->blockSignals(false);
  m_ik->setToolTip(available
                   ? tr("IK - inverse kinematics: the hand or foot stays where it is when you turn the body, "
                        "and the arm or leg follows. Turn off to turn each joint yourself.")
                   : tr("IK works on the arms (collar to wrist) and the legs (hip to ankle)."));
  updateEnabled();
}

bool JointCard::ikOn() const
{
  return m_ik->isChecked();
}

void JointCard::setLocked(bool locked)
{
  m_lock->blockSignals(true);
  m_lock->setChecked(locked);
  m_lock->blockSignals(false);
  updateEnabled();
}

bool JointCard::isLocked() const
{
  return m_lock->isChecked();
}

void JointCard::setEditable(bool editable)
{
  m_editable=editable;
  updateEnabled();
}

void JointCard::setMoveEditable(bool editable)
{
  m_moveEditable=editable;
  updateEnabled();
}

void JointCard::setLimited(bool limited)
{
  m_limited=limited;
  updateWarning();
}

void JointCard::setTrackballOrientation(const QQuaternion& orientation)
{
  m_trackball->setOrientation(orientation);
}

JointTurn::Frame JointCard::frame() const
{
  return static_cast<JointTurn::Frame>(m_frames->checkedId());
}

void JointCard::setFrame(JointTurn::Frame frame)
{
  if(QAbstractButton* button=m_frames->button(frame)) button->setChecked(true);
}

JointCard::Page JointCard::page() const
{
  return static_cast<Page>(m_pages->currentIndex());
}

void JointCard::setPage(Page page)
{
  m_tabs->button(page)->setChecked(true);
  m_pages->setCurrentIndex(page);
}

QString JointCard::warningText() const
{
  return m_warning->isHidden() ? QString() : m_warning->text();
}

QString JointCard::rotationRangeText(int axis) const
{
  return m_rotationRange[axis*2]->text()+QStringLiteral(" … ")+m_rotationRange[axis*2+1]->text();
}

void JointCard::updateEnabled()
{
  const bool joint=m_kind!=NoJoint;
  const bool locked=m_lock->isChecked();
  const bool rotate=joint && m_kind!=PositionTrack && m_editable && !locked;
  const bool move=(m_kind==Root || m_kind==PositionTrack) && m_moveEditable && !locked;

  m_ik->setEnabled(joint && m_ikAvailable);
  m_lock->setEnabled(joint);
  m_reset->setEnabled(m_kind==PositionTrack ? move : rotate);
  m_trackball->setEnabled(rotate);
  for(int axis=0;axis<3;axis++)
  {
    m_rotation[axis].slider->setEnabled(rotate);
    m_rotation[axis].edit->setEnabled(rotate);
    m_position[axis].slider->setEnabled(move);
    m_position[axis].edit->setEnabled(move);
  }
  m_frameRow->setEnabled(rotate);
}

void JointCard::updateRanges()
{
  for(int axis=0;axis<3;axis++)
  {
    for(int kind=0;kind<2;kind++)
    {
      const QSlider* slider=kind==0 ? m_rotation[axis].slider : m_position[axis].slider;
      std::array<QLabel*,6>& labels=kind==0 ? m_rotationRange : m_positionRange;
      const double minimum=slider->minimum()/static_cast<double>(m_precision);
      const double maximum=slider->maximum()/static_cast<double>(m_precision);
      if(kind==0)
      {
        labels[axis*2]->setText(degrees(minimum));
        labels[axis*2+1]->setText(degrees(maximum));
      }
      else
      {
        labels[axis*2]->setText(QString::number(minimum,'f',0));
        labels[axis*2+1]->setText(QString::number(maximum,'f',0));
      }
      const QString tip=(kind==0 ? tr("Allowed: %1 to %2") : tr("From %1 to %2"))
                        .arg(labels[axis*2]->text(),labels[axis*2+1]->text());
      const_cast<QSlider*>(slider)->setToolTip(tip);
    }
  }
}

void JointCard::updateWarning()
{
  QStringList warnings;
  for(int axis=0;axis<3;axis++)
  {
    const QSlider* slider=m_rotation[axis].slider;
    QString warning;
    if(m_limited && (m_kind==Joint))
      warning=limitWarning(QLatin1String(AXIS_NAME[axis]),slider->value()/static_cast<double>(m_precision),
                           slider->minimum()/static_cast<double>(m_precision),
                           slider->maximum()/static_cast<double>(m_precision));
    m_rotation[axis].edit->setStyleSheet(warning.isEmpty() ? QString() : QString::fromLatin1(NEAR_LIMIT_STYLE));
    if(!warning.isEmpty()) warnings << warning;
  }
  m_warning->setText(QStringLiteral("⚠ ")+warnings.join(QLatin1Char('\n')));
  m_warning->setVisible(!warnings.isEmpty());
  if(warnings.isEmpty()) m_warning->setText(QString());
}

void JointCard::trackballDragged(double dx,double dy,bool fine)
{
  const double scale=TRACKBALL_DEGREES_PER_PIXEL*(fine ? TRACKBALL_FINE : 1.0);
  // dragging right turns the front of the joint to the right (around up),
  // dragging down turns its top towards the viewer (around sideways)
  emit trackballTurned(dx*scale,dy*scale,frame());
}
