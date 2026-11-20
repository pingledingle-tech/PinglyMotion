// The easing tile row (issue #53 step 5). See easingtiles.h.
#include "easingtiles.h"

#include <QButtonGroup>
#include <QDialogButtonBox>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>

#include "animation.h"
#include "bvhnode.h"

namespace EasingChoice
{
  bool segmentEndingAt(const BVHNode* joint,int endFrame,int& startFrame)
  {
    if(!joint || endFrame<=0 || !joint->isKeyframe(endFrame)) return false;
    startFrame=joint->getKeyframeBefore(endFrame).frameNumber();
    return startFrame>=0 && startFrame<endFrame && joint->isKeyframe(startFrame);
  }

  Easing::Preset choiceOf(const BVHNode* joint,int startFrame,int endFrame)
  {
    if(!joint) return Easing::defaultPreset();
    const FrameData start=joint->frameData(startFrame);
    const FrameData end=joint->frameData(endFrame);
    return Easing::segmentPreset(start.curve(),start.easeOut(),end.easeIn());
  }

  void setChoice(Animation* anim,BVHNode* joint,int startFrame,int endFrame,Easing::Preset preset)
  {
    if(!anim || !joint) return;
    bool startEaseOut=true;
    bool endEaseIn=true;
    std::optional<Easing::Preset> curve;
    if(Easing::switchesFromPreset(preset,startEaseOut,endEaseIn))
    {
      // one of the four old-style choices: the switches say it all
      curve.reset();
    }
    else
    {
      // Overshoot then settle or Hold then snap: stored as the start key's
      // curve; the switches hold the default for a reader that does not know
      // the curve
      curve=preset;
      Easing::switchesFromPreset(Easing::defaultPreset(),startEaseOut,endEaseIn);
    }
    // a named choice replaces a custom curve (issue #11)
    anim->setCustomCurve(joint,startFrame,std::nullopt);
    anim->setCurve(joint,startFrame,curve);
    anim->setEaseOut(joint,startFrame,startEaseOut);
    anim->setEaseIn(joint,endFrame,endEaseIn);
  }

  std::optional<Easing::Bezier> customCurveOf(const BVHNode* joint,int startFrame)
  {
    if(!joint) return std::nullopt;
    return joint->customCurve(startFrame);
  }

  void setCustomCurve(Animation* anim,BVHNode* joint,int startFrame,int endFrame,const Easing::Bezier& curve)
  {
    if(!anim || !joint) return;
    bool startEaseOut=true;
    bool endEaseIn=true;
    Easing::switchesFromPreset(Easing::defaultPreset(),startEaseOut,endEaseIn);
    anim->setCurve(joint,startFrame,std::nullopt);
    anim->setCustomCurve(joint,startFrame,curve);
    anim->setEaseOut(joint,startFrame,startEaseOut);
    anim->setEaseIn(joint,endFrame,endEaseIn);
  }

  Easing::Bezier editorStart(const BVHNode* joint,int startFrame,int endFrame)
  {
    if(const std::optional<Easing::Bezier> custom=customCurveOf(joint,startFrame)) return *custom;
    return Easing::bezierFromPreset(choiceOf(joint,startFrame,endFrame));
  }

  QString tileName(Easing::Preset preset)
  {
    switch(preset)
    {
      case Easing::Preset::Even: return EasingTiles::tr("Even");
      case Easing::Preset::SlowStart: return EasingTiles::tr("Slow start");
      case Easing::Preset::SlowFinish: return EasingTiles::tr("Slow finish");
      case Easing::Preset::SlowBothEnds: return EasingTiles::tr("Slow both ends");
      case Easing::Preset::OvershootThenSettle: return EasingTiles::tr("Overshoot, then settle");
      case Easing::Preset::HoldThenSnap: return EasingTiles::tr("Hold, then snap");
    }
    return QString();
  }

  // docs/TEACHING.md, EASE-01 to EASE-06 (approved texts; change them there first)
  QString hint(Easing::Preset preset)
  {
    switch(preset)
    {
      case Easing::Preset::Even:
        return EasingTiles::tr("Same speed all the way. Use it for machines or a steady spin.");
      case Easing::Preset::SlowStart:
        return EasingTiles::tr("Starts gently, then speeds up. Pick it for lifting an arm.");
      case Easing::Preset::SlowFinish:
        return EasingTiles::tr("Moves at speed, then settles into the new pose. Good for putting a hand down.");
      case Easing::Preset::SlowBothEnds:
        return EasingTiles::tr("Starts and stops softly. Good for most arm and head moves.");
      case Easing::Preset::OvershootThenSettle:
        return EasingTiles::tr("Goes a little past the pose, then comes back. Good for a lively wave.");
      case Easing::Preset::HoldThenSnap:
        return EasingTiles::tr("Stays in the old pose, then jumps to the new one. Good for a blink.");
    }
    return QString();
  }
}

// ---------------------------------------------------------------- EasingTile

namespace
{
  const int TileWidth=92;
  const int PictureHeight=34;
  const int NameHeight=30;
  const int TrailSteps=8;
  // the part of the way the figure may go past the end (Overshoot then settle)
  const double OvershootRoom=0.1;
}

EasingTile::EasingTile(Easing::Preset preset,QWidget* parent) :
  QAbstractButton(parent),
  m_preset(preset)
{
  setCheckable(true);
  setText(EasingChoice::tileName(preset));
  setCursor(Qt::PointingHandCursor);
  setAttribute(Qt::WA_Hover);
}

QSize EasingTile::sizeHint() const
{
  return QSize(TileWidth,PictureHeight+NameHeight);
}

void EasingTile::setPhase(double phase)
{
  if(phase<0) phase=0;
  if(phase>1) phase=1;
  if(phase==m_phase) return;
  m_phase=phase;
  update();
}

void EasingTile::enterEvent(QEnterEvent*)
{
  emit hovered(m_preset);
  update();
}

void EasingTile::leaveEvent(QEvent*)
{
  emit left();
  update();
}

// a small stick figure standing on the point (x, y)
static void drawFigure(QPainter& painter,double x,double y,const QColor& color)
{
  painter.setPen(QPen(color,1.4,Qt::SolidLine,Qt::RoundCap));
  painter.setBrush(color);
  painter.drawEllipse(QPointF(x,y-9.5),2.2,2.2);
  painter.setBrush(Qt::NoBrush);
  painter.drawLine(QPointF(x,y-7),QPointF(x,y-2));
  painter.drawLine(QPointF(x-3,y-3.5),QPointF(x+3,y-5.5));
  painter.drawLine(QPointF(x,y-2),QPointF(x-2.5,y+2.5));
  painter.drawLine(QPointF(x,y-2),QPointF(x+2.5,y+2.5));
}

void EasingTile::paintEvent(QPaintEvent*)
{
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  const QPalette pal=palette();
  const bool on=isChecked();
  const bool enabled=isEnabled();
  const QColor text=enabled ? pal.color(QPalette::Text) : pal.color(QPalette::Disabled,QPalette::Text);
  const QColor mark=enabled ? pal.color(QPalette::Highlight) : pal.color(QPalette::Disabled,QPalette::Text);

  // the frame of the tile: filled when it is the current choice
  QRectF frame=QRectF(rect()).adjusted(1,1,-1,-1);
  QColor fill=pal.color(QPalette::Base);
  if(on)
  {
    fill=mark;
    fill.setAlpha(enabled ? 60 : 30);
  }
  else if(underMouse() && enabled)
  {
    fill=mark;
    fill.setAlpha(20);
  }
  painter.setPen(QPen(on ? mark : pal.color(QPalette::Mid),on ? 2 : 1));
  painter.setBrush(fill);
  painter.drawRoundedRect(frame,5,5);

  // the way of the figure, left to right, with room past the end for the overshoot
  const double x0=10;
  const double x1=x0+(width()-2*x0)/(1+OvershootRoom);
  const double y=PictureHeight-6;

  // the trail: one dot per equal time step; their spacing shows the speed
  QColor trail=text;
  trail.setAlpha(enabled ? 110 : 60);
  painter.setPen(Qt::NoPen);
  painter.setBrush(trail);
  for(int i=0;i<=TrailSteps;i++)
  {
    const double s=Easing::s(m_preset,static_cast<double>(i)/TrailSteps);
    painter.drawEllipse(QPointF(x0+s*(x1-x0),y),1.6,1.6);
  }

  // the moving figure
  const double s=Easing::s(m_preset,m_phase);
  drawFigure(painter,x0+s*(x1-x0),y,on ? mark : text);

  // the name under the picture
  QFont font=painter.font();
  if(font.pointSize()>0) font.setPointSize(font.pointSize()-1);
  painter.setFont(font);
  painter.setPen(text);
  painter.setBrush(Qt::NoBrush);
  const QRect nameRect(4,PictureHeight,width()-8,NameHeight-4);
  painter.drawText(nameRect,Qt::AlignHCenter|Qt::AlignTop|Qt::TextWordWrap,this->text());
}

// --------------------------------------------------------------- EasingTiles

EasingTiles::EasingTiles(QWidget* parent) :
  QWidget(parent)
{
  QVBoxLayout* layout=new QVBoxLayout(this);
  layout->setContentsMargins(4,2,4,2);
  layout->setSpacing(2);

  QHBoxLayout* row=new QHBoxLayout();
  row->setContentsMargins(0,0,0,0);
  row->setSpacing(4);

  QButtonGroup* group=new QButtonGroup(this);
  group->setExclusive(true);
  for(int i=0;i<Easing::PresetCount;i++)
  {
    const Easing::Preset preset=static_cast<Easing::Preset>(i);
    EasingTile* tile=new EasingTile(preset,this);
    tile->setObjectName(QStringLiteral("easingTile%1").arg(i));
    group->addButton(tile,i);
    row->addWidget(tile,0);
    connect(tile,&EasingTile::clicked,this,&EasingTiles::tileClicked);
    connect(tile,&EasingTile::hovered,this,&EasingTiles::tileHovered);
    connect(tile,&EasingTile::left,this,&EasingTiles::tileLeft);
    m_tiles[i]=tile;
  }
  m_group=group;

  // a free curve for this pose change (issue #11), in a dialog
  m_customButton=new QPushButton(tr("Custom curve..."),this);
  m_customButton->setObjectName(QStringLiteral("customCurveButton"));
  m_customButton->setCheckable(true);
  connect(m_customButton,&QPushButton::clicked,this,[this]()
  {
    // the button shows the state; only the dialog changes it
    m_customButton->setChecked(m_custom);
    if(m_hasChoice) emit customCurveRequested();
  });
  row->addWidget(m_customButton,0,Qt::AlignVCenter);

  row->addStretch(1);
  layout->addLayout(row);

  m_hint=new QLabel(this);
  m_hint->setObjectName(QStringLiteral("easingHint"));
  m_hint->setWordWrap(false);
  layout->addWidget(m_hint);

  m_timer=new QTimer(this);
  m_timer->setInterval(40);
  connect(m_timer,&QTimer::timeout,this,&EasingTiles::tick);

  setNoKey();
}

EasingTile* EasingTiles::tile(Easing::Preset preset) const
{
  return m_tiles[static_cast<int>(preset)];
}

void EasingTiles::setChoice(Easing::Preset preset)
{
  m_hasChoice=true;
  m_custom=false;
  m_choice=preset;
  for(int i=0;i<Easing::PresetCount;i++)
  {
    m_tiles[i]->setEnabled(true);
    m_tiles[i]->setChecked(static_cast<Easing::Preset>(i)==preset);
  }
  m_customButton->setEnabled(true);
  m_customButton->setChecked(false);
  refreshHint();
  updateAnimation();
  if(m_customCheck && m_customCheck()) setCustom(true);
}

void EasingTiles::setCustom(bool custom)
{
  if(!m_hasChoice) custom=false;
  m_custom=custom;
  if(custom)
  {
    // an exclusive group keeps one button checked; lift that for a moment
    m_group->setExclusive(false);
    for(int i=0;i<Easing::PresetCount;i++) m_tiles[i]->setChecked(false);
    m_group->setExclusive(true);
  }
  else if(m_hasChoice)
    m_tiles[static_cast<int>(m_choice)]->setChecked(true);
  m_customButton->setChecked(custom);
  refreshHint();
}

void EasingTiles::setNoKey()
{
  m_hasChoice=false;
  m_custom=false;
  m_customButton->setEnabled(false);
  m_customButton->setChecked(false);
  for(int i=0;i<Easing::PresetCount;i++)
  {
    m_tiles[i]->setEnabled(false);
    m_tiles[i]->setChecked(false);
  }
  refreshHint();
  updateAnimation();
}

void EasingTiles::setTipsOn(bool on)
{
  m_tipsOn=on;
  refreshHint();
}

QString EasingTiles::hintText() const
{
  return m_hint->text();
}

bool EasingTiles::isAnimating() const
{
  return m_timer->isActive();
}

void EasingTiles::refreshHint()
{
  for(int i=0;i<Easing::PresetCount;i++)
  {
    const Easing::Preset preset=static_cast<Easing::Preset>(i);
    m_tiles[i]->setToolTip(m_tipsOn ? EasingChoice::hint(preset) : QString());
  }
  m_customButton->setToolTip(m_tipsOn ? tr("Draw your own speed for this pose change.") : QString());

  if(!m_hasChoice)
    m_hint->setText(tr("Select a key frame on the timeline"));
  else if(!m_tipsOn)
    m_hint->setText(QString());
  else if(m_custom && !m_hovered)
    m_hint->setText(tr("This pose change uses your own curve. Click a tile to go back."));
  else
    m_hint->setText(EasingChoice::hint(m_hovered ? *m_hovered : m_choice));
}

// the figures move only while the row is on screen and a key is selected
void EasingTiles::updateAnimation()
{
  const bool run=isVisible() && m_hasChoice;
  if(run && !m_timer->isActive())
  {
    m_elapsedMs=0;
    m_timer->start();
  }
  else if(!run && m_timer->isActive())
  {
    m_timer->stop();
    m_elapsedMs=0;
    for(int i=0;i<Easing::PresetCount;i++) m_tiles[i]->setPhase(0);
  }
}

void EasingTiles::showEvent(QShowEvent* event)
{
  QWidget::showEvent(event);
  updateAnimation();
}

void EasingTiles::hideEvent(QHideEvent* event)
{
  QWidget::hideEvent(event);
  updateAnimation();
}

void EasingTiles::changeEvent(QEvent* event)
{
  QWidget::changeEvent(event);
  if(event->type()==QEvent::LanguageChange)
  {
    for(int i=0;i<Easing::PresetCount;i++)
      m_tiles[i]->setText(EasingChoice::tileName(static_cast<Easing::Preset>(i)));
    m_customButton->setText(tr("Custom curve..."));
    refreshHint();
  }
}

void EasingTiles::tick()
{
  m_elapsedMs=(m_elapsedMs+m_timer->interval())%cycleMs();
  // the run takes three quarters of the cycle, then the figure rests at the end
  const double phase=static_cast<double>(m_elapsedMs)/(cycleMs()*3/4);
  for(int i=0;i<Easing::PresetCount;i++) m_tiles[i]->setPhase(phase);
}

void EasingTiles::tileClicked()
{
  EasingTile* tile=qobject_cast<EasingTile*>(sender());
  if(!tile || !m_hasChoice) return;
  m_choice=tile->preset();
  m_custom=false;
  m_customButton->setChecked(false);
  refreshHint();
  emit choiceClicked(tile->preset());
}

void EasingTiles::tileHovered(Easing::Preset preset)
{
  m_hovered=preset;
  refreshHint();
}

void EasingTiles::tileLeft()
{
  m_hovered.reset();
  refreshHint();
}

// --------------------------------------------------------------- CurveEditor

namespace
{
  const double HandleRadius=7;
  const double TrailHeight=22;
}

CurveEditor::CurveEditor(QWidget* parent) :
  QWidget(parent)
{
  setObjectName(QStringLiteral("curveEditor"));
  setMouseTracking(true);
}

void CurveEditor::setCurve(const Easing::Bezier& curve)
{
  const Easing::Bezier clean=Easing::sanitized(curve);
  if(clean==m_curve) return;
  m_curve=clean;
  update();
  emit curveChanged(m_curve);
}

// the unit square sits in the middle: y from BezierMinY (bottom) to
// BezierMaxY (top), with room for the dot trail below
QRectF CurveEditor::graphRect() const
{
  const double margin=HandleRadius+4;
  return QRectF(margin,margin,width()-2*margin,height()-2*margin-TrailHeight);
}

QPointF CurveEditor::toWidget(double x,double y) const
{
  const QRectF r=graphRect();
  const double span=Easing::BezierMaxY-Easing::BezierMinY;
  return QPointF(r.left()+x*r.width(),r.bottom()-(y-Easing::BezierMinY)/span*r.height());
}

QPointF CurveEditor::toCurve(const QPointF& position) const
{
  const QRectF r=graphRect();
  const double span=Easing::BezierMaxY-Easing::BezierMinY;
  if(r.width()<=0 || r.height()<=0) return QPointF(0,0);
  return QPointF((position.x()-r.left())/r.width(),Easing::BezierMinY+(r.bottom()-position.y())/r.height()*span);
}

QPointF CurveEditor::handlePosition(int handle) const
{
  return handle==0 ? toWidget(m_curve.x1,m_curve.y1) : toWidget(m_curve.x2,m_curve.y2);
}

void CurveEditor::moveHandle(int handle,const QPointF& position)
{
  const QPointF p=toCurve(position);
  Easing::Bezier c=m_curve;
  if(handle==0) { c.x1=p.x(); c.y1=p.y(); }
  else { c.x2=p.x(); c.y2=p.y(); }
  setCurve(c);
}

void CurveEditor::mousePressEvent(QMouseEvent* event)
{
  if(event->button()!=Qt::LeftButton) return;
  const QPointF at=event->position();
  // the nearer handle, if the click is close enough to one
  int best=-1;
  double bestDistance=(HandleRadius+6)*(HandleRadius+6);
  for(int h=0;h<2;h++)
  {
    const QPointF d=handlePosition(h)-at;
    const double distance=d.x()*d.x()+d.y()*d.y();
    if(distance<=bestDistance) { best=h; bestDistance=distance; }
  }
  m_dragging=best;
}

void CurveEditor::mouseMoveEvent(QMouseEvent* event)
{
  if(m_dragging>=0) moveHandle(m_dragging,event->position());
}

void CurveEditor::mouseReleaseEvent(QMouseEvent* event)
{
  if(m_dragging>=0 && event->button()==Qt::LeftButton)
  {
    moveHandle(m_dragging,event->position());
    m_dragging=-1;
  }
}

void CurveEditor::paintEvent(QPaintEvent*)
{
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);
  const QPalette pal=palette();
  const QColor text=pal.color(QPalette::Text);
  const QColor mark=pal.color(QPalette::Highlight);
  QColor faint=text;
  faint.setAlpha(60);

  // background and the two pose lines (old pose at the bottom, new at the top)
  const QRectF r=graphRect();
  painter.setPen(QPen(pal.color(QPalette::Mid),1));
  painter.setBrush(pal.color(QPalette::Base));
  painter.drawRect(r);
  painter.setPen(QPen(faint,1,Qt::DashLine));
  painter.drawLine(toWidget(0,0),toWidget(1,0));
  painter.drawLine(toWidget(0,1),toWidget(1,1));

  // handle lines
  painter.setPen(QPen(faint,1));
  painter.drawLine(toWidget(0,0),handlePosition(0));
  painter.drawLine(toWidget(1,1),handlePosition(1));

  // the curve itself, sampled like playback samples it
  QPainterPath path(toWidget(0,0));
  const int samples=120;
  for(int i=1;i<=samples;i++)
  {
    const double t=static_cast<double>(i)/samples;
    path.lineTo(toWidget(t,Easing::bezierSpeed(m_curve,t)));
  }
  painter.setPen(QPen(text,2));
  painter.setBrush(Qt::NoBrush);
  painter.drawPath(path);

  // the handles
  painter.setPen(QPen(mark,2));
  QColor fill=mark;
  fill.setAlpha(90);
  painter.setBrush(fill);
  for(int h=0;h<2;h++) painter.drawEllipse(handlePosition(h),HandleRadius,HandleRadius);

  // the dot trail, as on the tiles: one dot per equal time step
  const double y=height()-TrailHeight/2;
  const double left=r.left();
  const double right=r.right();
  QColor trail=text;
  trail.setAlpha(140);
  painter.setPen(Qt::NoPen);
  painter.setBrush(trail);
  const int steps=12;
  for(int i=0;i<=steps;i++)
  {
    double s=Easing::bezierSpeed(m_curve,static_cast<double>(i)/steps);
    // the trail has room for the start and end pose only; past them it stops at the edge
    const double x=left+std::clamp(s,0.0,1.0)*(right-left);
    painter.drawEllipse(QPointF(x,y),2.2,2.2);
  }
}

// --------------------------------------------------------------- CurveEditorDialog

CurveEditorDialog::CurveEditorDialog(const Easing::Bezier& start,QWidget* parent) :
  QDialog(parent),
  m_start(Easing::sanitized(start))
{
  setObjectName(QStringLiteral("curveEditorDialog"));
  setWindowTitle(tr("Custom curve"));

  QVBoxLayout* layout=new QVBoxLayout(this);
  QLabel* hint=new QLabel(tr("Drag the two round handles. Steep parts move fast, flat parts slowly."),this);
  hint->setObjectName(QStringLiteral("curveEditorHint"));
  hint->setWordWrap(true);
  layout->addWidget(hint);

  m_editor=new CurveEditor(this);
  m_editor->setCurve(m_start);
  layout->addWidget(m_editor,1);

  QDialogButtonBox* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,this);
  QPushButton* again=buttons->addButton(tr("Start again"),QDialogButtonBox::ResetRole);
  again->setObjectName(QStringLiteral("curveEditorStartAgain"));
  connect(again,&QPushButton::clicked,this,[this]() { m_editor->setCurve(m_start); });
  connect(buttons,&QDialogButtonBox::accepted,this,&QDialog::accept);
  connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
  layout->addWidget(buttons);
}

Easing::Bezier CurveEditorDialog::curve() const
{
  return m_editor->curve();
}
