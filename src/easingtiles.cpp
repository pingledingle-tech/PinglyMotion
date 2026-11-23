// The easing tile row (issue #53 step 5). See easingtiles.h.
#include "easingtiles.h"
#include "dialogfit.h"
#include "lucideicons.h"
#include "theme.h"
#include "transportlook.h"
#include "usersettings.h"
#include "wraprow.h"

#include <QButtonGroup>
#include <QDialogButtonBox>
#include <QEvent>
#include <QHBoxLayout>
#include <QKeyEvent>
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
  const double TickSize=14;
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
  // Tab reaches only the current tile (the row sets it); the others by the arrow keys
  setFocusPolicy(Qt::ClickFocus);
}

void EasingTile::keyPressEvent(QKeyEvent* event)
{
  switch(event->key())
  {
    case Qt::Key_Right:
    case Qt::Key_Down:
      emit stepRequested(1);
      event->accept();
      return;
    case Qt::Key_Left:
    case Qt::Key_Up:
      emit stepRequested(-1);
      event->accept();
      return;
    default:
      break;
  }
  QAbstractButton::keyPressEvent(event);
}

void EasingTile::focusInEvent(QFocusEvent* event)
{
  QAbstractButton::focusInEvent(event);
  update();
}

void EasingTile::focusOutEvent(QFocusEvent* event)
{
  QAbstractButton::focusOutEvent(event);
  update();
}

// the name is drawn one point smaller than the text around it
QFont EasingTile::nameFont() const
{
  QFont small=font();
  if(small.pointSize()>0) small.setPointSize(small.pointSize()-1);
  return small;
}

// at least the first size; wider and taller when the text size makes the
// name bigger, so its longest word and two lines always fit (charter P9)
int EasingTile::nameHeight() const
{
  return qMax(NameHeight,2*QFontMetrics(nameFont()).height()+4);
}

QSize EasingTile::sizeHint() const
{
  const QFontMetrics metrics(nameFont());
  int widest=0;
  for(const QString& word : text().split(QLatin1Char(' '),Qt::SkipEmptyParts))
    widest=qMax(widest,metrics.horizontalAdvance(word));
  // wide enough that the name takes at most the two lines nameHeight() has
  // room for (at 200 percent text "Slow both ends" would need three)
  int width=qMax(TileWidth,widest+16);
  const int twoLines=2*metrics.height();
  while(width<TileWidth*4 &&
        metrics.boundingRect(QRect(0,0,width-8,10000),Qt::AlignHCenter|Qt::AlignTop|Qt::TextWordWrap,text()).height()>twoLines)
    width+=4;
  return QSize(width,PictureHeight+nameHeight());
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
  painter.setFont(nameFont());
  painter.setPen(text);
  painter.setBrush(Qt::NoBrush);
  const QRect nameRect(4,PictureHeight,width()-8,nameHeight()-4);
  painter.drawText(nameRect,Qt::AlignHCenter|Qt::AlignTop|Qt::TextWordWrap,this->text());

  // the current choice: a check mark in a round badge in the top right corner (the mockup's tick)
  if(on)
  {
    const QRectF badge(width()-3-TickSize,3,TickSize,TickSize);
    painter.setPen(Qt::NoPen);
    painter.setBrush(mark);
    painter.drawEllipse(badge);
    // Lucide "check", the mockup's icon
    const QPainterPath check=LucideIcons::path(QStringLiteral("check"));
    painter.save();
    painter.translate(badge.topLeft()+QPointF(2,2));
    const double scale=(TickSize-4)/24.0;
    painter.scale(scale,scale);
    painter.setPen(QPen(enabled ? Theme::color(Theme::Token::OnAccent) : pal.color(QPalette::Base),2.0/scale*1.2,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(check);
    painter.restore();
  }

  // the keyboard focus: the ring of the design system (focus_ring, 2 px)
  if(hasFocus())
  {
    painter.setPen(QPen(Theme::color(Theme::Token::FocusRing),Theme::Size::focusRingWidth));
    painter.setBrush(Qt::NoBrush);
    const double inset=Theme::Size::focusRingWidth/2.0;
    painter.drawRoundedRect(QRectF(rect()).adjusted(inset,inset,-inset,-inset),6,6);
  }
}

// --------------------------------------------------------------- EasingTiles

EasingTiles::EasingTiles(QWidget* parent) :
  QWidget(parent)
{
  QVBoxLayout* layout=new QVBoxLayout(this);
  layout->setContentsMargins(4,2,4,2);
  layout->setSpacing(2);

  // the heading of the row, as in the mockup (label size, semibold)
  m_heading=new QLabel(tr("How the move feels"),this);
  m_heading->setObjectName(QStringLiteral("easingHeading"));
  QFont headingFont=m_heading->font();
  headingFont.setWeight(QFont::DemiBold);
  m_heading->setFont(headingFont);
  layout->addWidget(m_heading);

  // the six tiles and Edit curve in two parts: side by side, or one under
  // the other when they do not fit on one line (docs/UX_DECISIONS.md, 6c)
  QHBoxLayout* firstPart=new QHBoxLayout();
  firstPart->setContentsMargins(0,0,0,0);
  firstPart->setSpacing(4);
  QHBoxLayout* secondPart=new QHBoxLayout();
  secondPart->setContentsMargins(0,0,0,0);
  secondPart->setSpacing(4);

  QButtonGroup* group=new QButtonGroup(this);
  group->setExclusive(true);
  for(int i=0;i<Easing::PresetCount;i++)
  {
    const Easing::Preset preset=static_cast<Easing::Preset>(i);
    EasingTile* tile=new EasingTile(preset,this);
    tile->setObjectName(QStringLiteral("easingTile%1").arg(i));
    group->addButton(tile,i);
    (i<Easing::PresetCount/2 ? firstPart : secondPart)->addWidget(tile,0);
    connect(tile,&EasingTile::clicked,this,&EasingTiles::tileClicked);
    connect(tile,&EasingTile::hovered,this,&EasingTiles::tileHovered);
    connect(tile,&EasingTile::left,this,&EasingTiles::tileLeft);
    connect(tile,&EasingTile::stepRequested,this,&EasingTiles::tileStep);
    m_tiles[i]=tile;
  }
  m_group=group;

  // a free curve for this pose change (issue #11), in the Edit curve window;
  // Expert mode only, as in the mockup
  m_editButton=new QPushButton(tr("Edit curve..."),this);
  m_editButton->setObjectName(QStringLiteral("editCurveButton"));
  // the status line names it without the dots (reason line, charter P6)
  m_editButton->setProperty("reasonName",tr("Edit curve"));
  connect(m_editButton,&QPushButton::clicked,this,[this]()
  {
    if(m_hasChoice) emit editCurveRequested();
  });
  // hidden in Beginner mode, but its room stays, so the tiles break into rows in the same way in
  // both modes and none of them moves when the mode changes (charter P12)
  {
    QSizePolicy keepSpace=m_editButton->sizePolicy();
    keepSpace.setRetainSizeWhenHidden(true);
    m_editButton->setSizePolicy(keepSpace);
  }
  secondPart->addWidget(m_editButton,0,Qt::AlignVCenter);

  firstPart->addStretch(1);
  secondPart->addStretch(1);
  m_rows=WrapRow::make({firstPart,secondPart});
  m_rows->setSpacing(4);
  // one run of tiles from the left, as in the mockup: on one line the first part keeps its
  // own width and the second part, with the room left over, starts right after it (UX
  // fixes 1: the two parts shared the room, so three tiles stood far off on the right)
  m_rows->setStretch(0,0);
  m_rows->setStretch(1,1);
  layout->addLayout(m_rows);

  m_hint=new QLabel(this);
  m_hint->setObjectName(QStringLiteral("easingHint"));
  m_hint->setWordWrap(false);
  layout->addWidget(m_hint);

  m_timer=new QTimer(this);
  m_timer->setInterval(40);
  connect(m_timer,&QTimer::timeout,this,&EasingTiles::tick);

  // the Edit curve button in the look of the design system (icon, focus ring)
  TransportLook::apply(this);

  m_expert=UserSettings::expertMode();
  m_editButton->setVisible(m_expert);
  UserSettings::watchExpertMode(this,[this](bool on) { setExpertMode(on); });

  setNoKey();
}

void EasingTiles::setExpertMode(bool expert)
{
  m_expert=expert;
  m_editButton->setVisible(expert);
}

void EasingTiles::setWrapped(bool wrapped)
{
  WrapRow::setWrapped(m_rows,wrapped);
}

bool EasingTiles::isWrapped() const
{
  return WrapRow::isWrapped(m_rows);
}

// the widest of the tile row and the hint line, with the margins around them
int EasingTiles::widthWith(int rowWidth) const
{
  const QMargins m=layout()->contentsMargins();
  return qMax(rowWidth,m_hint->minimumSizeHint().width())+m.left()+m.right();
}

int EasingTiles::oneLineWidth() const
{
  return widthWith(WrapRow::oneLineWidth(m_rows));
}

int EasingTiles::wrappedWidth() const
{
  return widthWith(WrapRow::wrappedWidth(m_rows));
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
    m_tiles[i]->setStatusTip(QString());
  }
  m_editButton->setEnabled(true);
  m_editButton->setStatusTip(QString());
  refreshHint();
  updateAnimation();
  if(m_customCheck && m_customCheck()) setCustom(true);
  refreshFocusOrder();
}

// only one tile is in the Tab order: the current one, or the first when none
// is marked (a custom curve); the arrow keys reach the others
void EasingTiles::refreshFocusOrder()
{
  int inOrder=0;
  for(int i=0;i<Easing::PresetCount;i++)
    if(m_tiles[i]->isChecked()) inOrder=i;
  for(int i=0;i<Easing::PresetCount;i++)
    m_tiles[i]->setFocusPolicy(i==inOrder ? Qt::StrongFocus : Qt::ClickFocus);
}

void EasingTiles::tileStep(int step)
{
  EasingTile* from=qobject_cast<EasingTile*>(sender());
  if(!from || !m_hasChoice) return;
  const int count=Easing::PresetCount;
  const int next=((static_cast<int>(from->preset())+step)%count+count)%count;
  EasingTile* tile=m_tiles[next];
  // the arrow key picks the choice, as a click does, and the focus follows
  tile->setFocus(Qt::TabFocusReason);
  tile->click();
}

QString EasingTiles::tileReason() const
{
  if(m_hasChoice) return QString();
  switch(m_noKey)
  {
    case NoKey::NoAnimation: return tr("Open an animation first.");
    case NoKey::NoJoint: return tr("Pick a joint first.");
    case NoKey::NoMove: break;
  }
  return tr("Select a key on the timeline. The first key has no move before it.");
}

QString EasingTiles::editCurveReason() const
{
  if(m_hasChoice) return QString();
  if(m_noKey==NoKey::NoAnimation) return tr("Open an animation first.");
  return tr("Select a key on the timeline first.");
}

void EasingTiles::refreshNoKey()
{
  if(!m_hasChoice) setNoKey();
}

// greys the row with its reasons: the tip and the status line of each greyed control
void EasingTiles::applyNoKeyState()
{
  m_noKey=m_noKeyCheck ? m_noKeyCheck() : NoKey::NoMove;
  const QString tileWhy=tileReason();
  for(int i=0;i<Easing::PresetCount;i++)
  {
    m_tiles[i]->setStatusTip(tileWhy);
    m_tiles[i]->setProperty("reasonName",tr("Easing choice"));
  }
  m_editButton->setStatusTip(editCurveReason());
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
  refreshHint();
  refreshFocusOrder();
}

void EasingTiles::setNoKey()
{
  m_hasChoice=false;
  m_custom=false;
  m_editButton->setEnabled(false);
  // an exclusive group keeps one button checked; lift that for a moment
  m_group->setExclusive(false);
  for(int i=0;i<Easing::PresetCount;i++)
  {
    m_tiles[i]->setEnabled(false);
    m_tiles[i]->setChecked(false);
  }
  m_group->setExclusive(true);
  applyNoKeyState();
  refreshHint();
  updateAnimation();
  refreshFocusOrder();
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

std::optional<Easing::Preset> EasingTiles::animatingTile() const
{
  if(!m_timer->isActive()) return std::nullopt;
  return m_hovered;
}

void EasingTiles::refreshHint()
{
  // a greyed-out control's tip is its reason (charter P6), with the tips on or off
  for(int i=0;i<Easing::PresetCount;i++)
  {
    const Easing::Preset preset=static_cast<Easing::Preset>(i);
    if(!m_hasChoice) m_tiles[i]->setToolTip(tileReason());
    else m_tiles[i]->setToolTip(m_tipsOn ? EasingChoice::hint(preset) : QString());
  }
  if(!m_hasChoice) m_editButton->setToolTip(editCurveReason());
  else m_editButton->setToolTip(m_tipsOn ? tr("Draw your own speed for this pose change.") : QString());

  // the "select a key" line and the custom curve line are the state, not tips:
  // they show with the tips off too; with no animation open the line is empty
  if(!m_hasChoice)
    m_hint->setText(m_noKey==NoKey::NoAnimation ? QString() : tr("Select a key on the timeline"));
  else if(m_custom && !m_hovered)
    m_hint->setText(tr("This pose change uses your own curve. Click a tile to go back."));
  else if(!m_tipsOn)
    m_hint->setText(QString());
  else
    m_hint->setText(EasingChoice::hint(m_hovered ? *m_hovered : m_choice));
}

// only the figure of the tile under the mouse moves, and only while the row is
// on screen and a key is selected; every other figure stands at the start
void EasingTiles::updateAnimation()
{
  const bool run=isVisible() && m_hasChoice && m_hovered.has_value();
  // a new hovered tile starts its run from the beginning
  m_elapsedMs=0;
  for(int i=0;i<Easing::PresetCount;i++) m_tiles[i]->setPhase(0);
  if(run && !m_timer->isActive()) m_timer->start();
  else if(!run && m_timer->isActive()) m_timer->stop();
}

void EasingTiles::showEvent(QShowEvent* event)
{
  QWidget::showEvent(event);
  updateAnimation();
}

void EasingTiles::hideEvent(QHideEvent* event)
{
  QWidget::hideEvent(event);
  // a hidden row gets no leave event, so it forgets the hover here
  m_hovered.reset();
  refreshHint();
  updateAnimation();
}

void EasingTiles::changeEvent(QEvent* event)
{
  QWidget::changeEvent(event);
  if(event->type()==QEvent::LanguageChange)
  {
    for(int i=0;i<Easing::PresetCount;i++)
      m_tiles[i]->setText(EasingChoice::tileName(static_cast<Easing::Preset>(i)));
    m_heading->setText(tr("How the move feels"));
    m_editButton->setText(tr("Edit curve..."));
    m_editButton->setProperty("reasonName",tr("Edit curve"));
    if(!m_hasChoice) applyNoKeyState();
    refreshHint();
  }
}

void EasingTiles::tick()
{
  m_elapsedMs=(m_elapsedMs+m_timer->interval())%cycleMs();
  // the run takes three quarters of the cycle, then the figure rests at the end
  const double phase=static_cast<double>(m_elapsedMs)/(cycleMs()*3/4);
  if(m_hovered) m_tiles[static_cast<int>(*m_hovered)]->setPhase(phase);
}

void EasingTiles::tileClicked()
{
  EasingTile* tile=qobject_cast<EasingTile*>(sender());
  if(!tile || !m_hasChoice) return;
  m_choice=tile->preset();
  m_custom=false;
  refreshHint();
  refreshFocusOrder();
  emit choiceClicked(tile->preset());
}

void EasingTiles::tileHovered(Easing::Preset preset)
{
  m_hovered=preset;
  refreshHint();
  updateAnimation();
}

void EasingTiles::tileLeft()
{
  // only the tile that is hovered may end the hover (leave and enter can come
  // in either order when the mouse goes from one tile to the next)
  EasingTile* tile=qobject_cast<EasingTile*>(sender());
  if(tile && m_hovered && tile->preset()!=*m_hovered) return;
  m_hovered.reset();
  refreshHint();
  updateAnimation();
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
  setFocusPolicy(Qt::StrongFocus);
  setAccessibleName(tr("Speed curve"));
}

void CurveEditor::keyPressEvent(QKeyEvent* event)
{
  const double step=(event->modifiers() & Qt::ShiftModifier) ? bigStep() : keyStep();
  double dx=0;
  double dy=0;
  switch(event->key())
  {
    case Qt::Key_Left: dx=-step; break;
    case Qt::Key_Right: dx=step; break;
    case Qt::Key_Up: dy=step; break;
    case Qt::Key_Down: dy=-step; break;
    case Qt::Key_Space:
      m_active=1-m_active;
      update();
      event->accept();
      return;
    default:
      QWidget::keyPressEvent(event);
      return;
  }
  Easing::Bezier c=m_curve;
  if(m_active==0) { c.x1+=dx; c.y1+=dy; }
  else { c.x2+=dx; c.y2+=dy; }
  setCurve(c);
  event->accept();
}

void CurveEditor::focusInEvent(QFocusEvent* event)
{
  QWidget::focusInEvent(event);
  update();
}

void CurveEditor::focusOutEvent(QFocusEvent* event)
{
  QWidget::focusOutEvent(event);
  update();
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
  if(best>=0 && best!=m_active)
  {
    m_active=best;
    update();
  }
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

  // with the keyboard focus: the ring of the design system around the
  // editor, and a second ring around the handle the arrow keys move
  if(hasFocus())
  {
    const QColor ring=Theme::color(Theme::Token::FocusRing);
    painter.setPen(QPen(ring,Theme::Size::focusRingWidth));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(handlePosition(m_active),HandleRadius+Theme::Size::focusRingOffset+1,HandleRadius+Theme::Size::focusRingOffset+1);
    const double inset=Theme::Size::focusRingWidth/2.0;
    painter.drawRect(QRectF(rect()).adjusted(inset,inset,-inset,-inset));
  }

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
  setWindowTitle(tr("Edit curve"));
  // a window of its own, with the normal title bar buttons (no "?" help button)
  setWindowFlag(Qt::WindowContextHelpButtonHint,false);
  setSizeGripEnabled(true);

  QVBoxLayout* layout=DialogFit::contentLayout(this);
  QLabel* hint=new QLabel(tr("Drag the two round handles. Steep parts move fast, flat parts slowly."),this);
  hint->setObjectName(QStringLiteral("curveEditorHint"));
  hint->setWordWrap(true);
  layout->addWidget(hint);
  QLabel* keys=new QLabel(tr("Keys: the arrow keys move a handle (Shift: bigger steps), Space picks the other handle."),this);
  keys->setObjectName(QStringLiteral("curveEditorKeys"));
  keys->setWordWrap(true);
  keys->setForegroundRole(QPalette::PlaceholderText);
  layout->addWidget(keys);

  m_editor=new CurveEditor(this);
  m_editor->setCurve(m_start);
  layout->addWidget(m_editor,1);

  QDialogButtonBox* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,this);
  QPushButton* again=buttons->addButton(tr("Start again"),QDialogButtonBox::ResetRole);
  again->setObjectName(QStringLiteral("curveEditorStartAgain"));
  connect(again,&QPushButton::clicked,this,[this]() { m_editor->setCurve(m_start); });
  connect(buttons,&QDialogButtonBox::accepted,this,&QDialog::accept);
  connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
  DialogFit::finish(this,buttons);
  // opens where it was last time (kept between sessions), clamped to the screen
  DialogFit::remember(this,placeName());
  m_editor->setFocus();
}

Easing::Bezier CurveEditorDialog::curve() const
{
  return m_editor->curve();
}
