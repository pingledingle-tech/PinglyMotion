/*
 * PinglyMotion - test helper: draw a panel without a screen in the program's theme
 *
 * See uirender.h.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#include "uirender.h"

#include <QAbstractScrollArea>
#include <QApplication>
#include <QCursor>
#include <QDir>
#include <QFont>
#include <QtTest>

#include "textsize.h"
#include "theme.h"

QString UiRender::Variant::name() const
{
  return QStringLiteral("%1_%2").arg(dark ? "dark" : "light").arg(percent);
}

QList<UiRender::Variant> UiRender::variants()
{
  return {{true, 100}, {true, 150}, {true, 200}, {false, 100}, {false, 150}, {false, 200}};
}

UiRender::Scope::Scope(const Variant& variant) : m_wasDark(Theme::isDark()), m_percent(TextSize::percent())
{
  Theme::apply(variant.dark);
  // the program's own text size (src/textsize.h): the application font and
  // the sizes written as numbers, as in the program
  TextSize::apply(variant.percent);
  QCoreApplication::processEvents();
}

UiRender::Scope::~Scope()
{
  TextSize::apply(m_percent);
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
    // the inner parts of Qt's own widgets (the text field of a spin box, the
    // viewport of a scroll area): their widget is checked instead
    if (child->objectName().startsWith(QLatin1String("qt_")))
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
    // the contents of a scroll area may be taller or wider than it: that is
    // what it scrolls; only a side that cannot be scrolled clips them
    const QAbstractScrollArea* scroll =
        parent ? qobject_cast<const QAbstractScrollArea*>(parent->parentWidget()) : nullptr;
    if (scroll && scroll->viewport() == parent)
    {
      const QRect g = child->geometry();
      if (scroll->horizontalScrollBarPolicy() == Qt::ScrollBarAlwaysOff && (g.left() < 0 || g.right() > parent->width() - 1))
        problems << QStringLiteral("%1 is wider than its scroll area, which does not scroll sideways (%2 in %3)")
                        .arg(label).arg(g.width()).arg(parent->width());
      if (scroll->verticalScrollBarPolicy() == Qt::ScrollBarAlwaysOff && (g.top() < 0 || g.bottom() > parent->height() - 1))
        problems << QStringLiteral("%1 is taller than its scroll area, which does not scroll up and down (%2 in %3)")
                        .arg(label).arg(g.height()).arg(parent->height());
    }
    else if (parent && !parent->rect().contains(child->geometry()))
      problems << QStringLiteral("%1 sticks out of its parent").arg(label);
  }
  return problems;
}
