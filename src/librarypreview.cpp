/*
 * PinglyMotion - the small preview figure of the Library panel (see librarypreview.h)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#include "librarypreview.h"

#include <QFileInfo>
#include <QPainter>
#include <QPainterPath>
#include <QSet>

#include <cmath>
#include <memory>

#include "animation.h"
#include "bvh.h"
#include "bvhnode.h"
#include "posedescribe.h"
#include "textsize.h"
#include "theme.h"

namespace
{
  // the mockup draws the figure turned 35 degrees, so arms and legs in front of the body show
  const double Yaw = -35.0 * M_PI / 180.0;
  // the mockup's preview box: 120 pixels wide, 160 by 150
  const int PreviewWidth = 120;
  const int PreviewHeight = 112;

  QPointF seen(const QVector3D& p)
  {
    // turned around the up axis, then seen from the front; y is up in the file, down on screen
    const double x = p.x() * std::cos(Yaw) + p.z() * std::sin(Yaw);
    return QPointF(x, -p.y());
  }

  void bonesBelow(BVHNode* node, BVHNode* classicParent, const QSet<QString>& classic,
                  const QHash<BVHNode*, QMatrix4x4>& world, QList<QPair<QVector3D, QVector3D>>& bones)
  {
    if (!node) return;
    BVHNode* parent = classicParent;
    if (classic.contains(node->name()) && world.contains(node))
    {
      if (classicParent && world.contains(classicParent))
        bones << qMakePair(world.value(classicParent).map(QVector3D()), world.value(node).map(QVector3D()));
      parent = node;
    }
    for (int i = 0; i < node->numChildren(); i++) bonesBelow(node->child(i), parent, classic, world, bones);
  }
}

LibraryPreview::LibraryPreview(QWidget* parent) : QWidget(parent)
{
  setObjectName(QStringLiteral("libraryPreview"));
  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  connect(&m_timer, &QTimer::timeout, this, [this]()
  {
    if (m_frames.isEmpty()) return;
    m_frame = (m_frame + 1) % int(m_frames.size());
    update();
  });
}

QSize LibraryPreview::sizeHint() const
{
  return QSize(TextSize::scaled(PreviewWidth), TextSize::scaled(PreviewHeight));
}

QSize LibraryPreview::minimumSizeHint() const
{
  return sizeHint();
}

bool LibraryPreview::setClip(const QString& path)
{
  setPlaying(false);
  m_frames.clear();
  m_frame = 0;
  m_path = path;
  m_minX = m_maxX = m_minY = m_maxY = 0;
  if (path.isEmpty() || !QFileInfo(path).isFile())
  {
    update();
    return false;
  }

  BVH bvh;
  std::unique_ptr<Animation> animation(new Animation(&bvh, path));
  const int frames = animation->getNumberOfFrames();
  if (!animation->getMotion() || frames < 1)
  {
    update();
    return false;
  }
  m_frameTime = animation->fps() > 0 ? 1.0 / animation->fps() : 1.0 / 30.0;

  const QStringList& names = PoseDescribe::classicJoints();
  const QSet<QString> classic(names.begin(), names.end());
  BVHNode* head = PoseDescribe::findJoint(animation.get(), QStringLiteral("mHead"));
  bool first = true;
  for (int f = 0; f < frames; f++)
  {
    const QHash<BVHNode*, QMatrix4x4> world = PoseDescribe::worldTransforms(animation.get(), f);
    Pose pose;
    bonesBelow(animation->getMotion(), nullptr, classic, world, pose.bones);
    if (head && world.contains(head)) pose.head = world.value(head).map(QVector3D());
    for (const auto& bone : pose.bones)
      for (const QVector3D& p : {bone.first, bone.second})
      {
        const QPointF s = seen(p);
        if (first) { m_minX = m_maxX = s.x(); m_minY = m_maxY = s.y(); first = false; }
        m_minX = std::min(m_minX, s.x());
        m_maxX = std::max(m_maxX, s.x());
        m_minY = std::min(m_minY, s.y());
        m_maxY = std::max(m_maxY, s.y());
      }
    m_frames << pose;
  }
  update();
  return !m_frames.isEmpty();
}

void LibraryPreview::setPlaying(bool on)
{
  if (on == playing()) return;
  if (on && m_frames.size() < 2) return;
  if (on) m_timer.start(std::max(10, int(std::lround(m_frameTime * 1000.0))));
  else
  {
    m_timer.stop();
    m_frame = 0;
    update();
  }
  emit playingChanged(on);
}

void LibraryPreview::paintEvent(QPaintEvent*)
{
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);
  const QRectF box = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
  painter.setPen(QPen(Theme::color(Theme::Token::Border), Theme::Size::borderWidth));
  painter.setBrush(Theme::color(Theme::Token::Background));
  painter.drawRoundedRect(box, Theme::Size::radiusControl, Theme::Size::radiusControl);
  if (m_frames.isEmpty()) return;

  // the head sticks out above the highest joint; leave room for it and a margin
  const double margin = Theme::Size::spacingS;
  const double headRoom = 0.12 * (m_maxY - m_minY);
  const double w = std::max(1e-6, m_maxX - m_minX), h = std::max(1e-6, m_maxY - m_minY + headRoom);
  const double scale = std::min((width() - 2 * margin) / w, (height() - 2 * margin) / h);
  const QPointF centre((m_minX + m_maxX) / 2.0, (m_minY - headRoom + m_maxY) / 2.0);
  painter.translate(width() / 2.0, height() / 2.0);
  painter.scale(scale, scale);
  painter.translate(-centre);

  const Pose& pose = m_frames.at(std::min(m_frame, int(m_frames.size()) - 1));
  QPen bone(Theme::color(Theme::Token::Text), Theme::Size::iconStroke / scale, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
  painter.setPen(bone);
  for (const auto& b : pose.bones) painter.drawLine(seen(b.first), seen(b.second));
  const double r = headRoom * 0.45;
  if (r > 0)
  {
    painter.setBrush(Theme::color(Theme::Token::Background));
    painter.setPen(QPen(Theme::color(Theme::Token::AccentText), Theme::Size::iconStroke / scale));
    painter.drawEllipse(seen(pose.head) - QPointF(0, r * 0.6), r, r);
  }
}
