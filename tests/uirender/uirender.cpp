/*
 * PinglyMotion - test helper: draw a panel without a screen in the program's theme
 *
 * See uirender.h.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#include "uirender.h"

#include <QApplication>
#include <QCursor>
#include <QDir>
#include <QFont>
#include <QtTest>

#include "theme.h"

QString UiRender::Variant::name() const
{
  return QStringLiteral("%1_%2").arg(dark ? "dark" : "light").arg(percent);
}

QList<UiRender::Variant> UiRender::variants()
{
  return {{true, 100}, {true, 200}, {false, 100}, {false, 200}};
}

UiRender::Scope::Scope(const Variant& variant) : m_wasDark(Theme::isDark()), m_font(QApplication::font())
{
  Theme::apply(variant.dark);
  QFont font = m_font;
  if (m_font.pointSizeF() > 0)
    font.setPointSizeF(m_font.pointSizeF() * variant.percent / 100.0);
  else if (m_font.pixelSize() > 0)
    font.setPixelSize(m_font.pixelSize() * variant.percent / 100);
  QApplication::setFont(font);
  QCoreApplication::processEvents();
}

UiRender::Scope::~Scope()
{
  QApplication::setFont(m_font);
  Theme::apply(m_wasDark);
  QCoreApplication::processEvents();
}

QImage UiRender::render(QWidget* widget)
{
  // away from the mouse, so nothing shows its hover colour
  QCursor::setPos(0, 0);
  widget->ensurePolished();
  widget->resize(widget->sizeHint().expandedTo(widget->minimumSizeHint()));
  widget->move(400, 400);
  widget->show();
  if (!QTest::qWaitForWindowExposed(widget))
    qWarning("UiRender: the widget was not exposed");
  QCoreApplication::processEvents();
  return widget->grab().toImage();
}

QString UiRender::outDir()
{
  return qEnvironmentVariable("PINGLYMOTION_UI_OUT");
}

QString UiRender::save(const QImage& image, const QString& name, const Variant& variant)
{
  const QString dir = outDir();
  if (dir.isEmpty() || !QDir().mkpath(dir))
    return QString();
  const QString path = QDir(dir).filePath(QStringLiteral("%1_%2.png").arg(name, variant.name()));
  return image.save(path) ? path : QString();
}

QColor UiRender::colourAt(const QImage& image, QWidget* root, QWidget* widget, const QPoint& point)
{
  const QPoint p = widget->mapTo(root, point);
  const qreal ratio = image.devicePixelRatio();
  return image.pixelColor(int(p.x() * ratio), int(p.y() * ratio));
}

QString UiRender::expectColour(const QImage& image, QWidget* root, QWidget* widget, const QPoint& point,
                               const QColor& expected, const QString& what, int tolerance)
{
  const QColor got = colourAt(image, root, widget, point);
  if (qAbs(got.red() - expected.red()) <= tolerance && qAbs(got.green() - expected.green()) <= tolerance &&
      qAbs(got.blue() - expected.blue()) <= tolerance)
    return QString();
  return QStringLiteral("%1: pixel at (%2,%3) is %4, expected %5 (%6)")
      .arg(what).arg(point.x()).arg(point.y()).arg(got.name(), expected.name(), Theme::isDark() ? "dark" : "light");
}

QStringList UiRender::clipping(QWidget* root)
{
  QStringList problems;
  const QString rootName = QString::fromLatin1(root->metaObject()->className());
  const QSize rootMin = root->minimumSizeHint();
  if (root->width() < rootMin.width() || root->height() < rootMin.height())
    problems << QStringLiteral("%1 is %2x%3, smaller than its minimum size hint %4x%5")
                    .arg(rootName).arg(root->width()).arg(root->height()).arg(rootMin.width()).arg(rootMin.height());

  const QList<QWidget*> children = root->findChildren<QWidget*>();
  for (QWidget* child : children)
  {
    if (!child->isVisibleTo(root))
      continue;
    const QString label = QStringLiteral("%1 '%2'").arg(QString::fromLatin1(child->metaObject()->className()),
                                                      child->objectName());
    // only widgets that hold no widgets of their own: a layout may squeeze
    // a container, but a button or a label must keep the room its text needs
    if (child->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly).isEmpty())
    {
      const QSize hint = child->sizeHint();
      if (hint.isValid() && (child->width() < hint.width() || child->height() < hint.height()))
        problems << QStringLiteral("%1 is %2x%3, smaller than its size hint %4x%5")
                        .arg(label).arg(child->width()).arg(child->height()).arg(hint.width()).arg(hint.height());
    }
    QWidget* parent = child->parentWidget();
    if (parent && !parent->rect().contains(child->geometry()))
      problems << QStringLiteral("%1 sticks out of its parent").arg(label);
  }
  return problems;
}
