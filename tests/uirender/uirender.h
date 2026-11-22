/*
 * PinglyMotion - test helper: draw a panel without a screen in the program's theme
 *
 * A widget drawn with Qt's offscreen platform comes out in Qt's plain light
 * style unless the program's own theme is applied first. This helper applies
 * the theme (src/theme.*, Theme::apply, dark and light) and a text size
 * (100 or 200 percent of the application font, in the helper only), draws the
 * widget, checks token colours at known points and checks that nothing is
 * clipped, and saves the pictures to the folder in the environment variable
 * PINGLYMOTION_UI_OUT (nothing is saved when it is not set).
 *
 * Use, in a test that runs with QT_QPA_PLATFORM=offscreen:
 *
 *   for (const UiRender::Variant& v : UiRender::variants())
 *   {
 *     UiRender::Scope scope(v);          // theme and text size; undone at the end
 *     MyPanel panel;                     // make the widget AFTER the scope
 *     const QImage image = UiRender::render(&panel);
 *     UiRender::save(image, "mypanel", v);
 *     QString why = UiRender::expectColour(image, panel, &button, QPoint(6, 12),
 *                                          Theme::color(Theme::Token::Accent, v.dark), "Start fill");
 *     QVERIFY2(why.isEmpty(), qPrintable(why));
 *     why = UiRender::clipping(&panel).join('\n');
 *     QVERIFY2(why.isEmpty(), qPrintable(why));
 *   }
 *
 * Tests only: nothing in the program uses this file. Link tests/uirender/
 * uirender.cpp and src/theme.cpp into the test program.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#ifndef UIRENDER_H
#define UIRENDER_H

#include <QColor>
#include <QFont>
#include <QImage>
#include <QList>
#include <QPoint>
#include <QString>
#include <QStringList>

class QWidget;

namespace UiRender
{
  // one way to draw: a theme and a text size
  struct Variant
  {
    bool dark;
    int percent; // text size, 100 or 200

    // for file names and messages, for example "dark_200"
    QString name() const;
  };

  // dark and light, each at 100 and 200 percent
  QList<Variant> variants();

  // applies the theme and the text size for as long as it lives, then puts
  // the old theme and the old application font back. Make the widgets after it.
  class Scope
  {
    public:
      explicit Scope(const Variant& variant);
      ~Scope();
      Scope(const Scope&) = delete;
      Scope& operator=(const Scope&) = delete;

    private:
      bool m_wasDark;
      QFont m_font;
  };

  // shows the widget at its own size hint (at least its minimum size hint),
  // away from the mouse, and draws it. The picture is in device pixels.
  QImage render(QWidget* widget);

  // the folder in PINGLYMOTION_UI_OUT, empty when it is not set
  QString outDir();

  // saves <name>_<theme>_<percent>.png in outDir(); answers the path, or an
  // empty string when outDir() is empty or the file cannot be written
  QString save(const QImage& image, const QString& name, const Variant& variant);

  // the colour of the pixel at a logical point of the widget
  QColor colourAt(const QImage& image, QWidget* root, QWidget* widget, const QPoint& point);

  // empty when that pixel has the expected colour (each channel within
  // tolerance), else a sentence with both colours
  QString expectColour(const QImage& image, QWidget* root, QWidget* widget, const QPoint& point,
                       const QColor& expected, const QString& what, int tolerance = 0);

  // what is clipped in the widget and everything inside it, one sentence per
  // problem, empty when nothing is: a visible child smaller than its size
  // hint, or sticking out of its parent. Call after render().
  QStringList clipping(QWidget* root);
}

#endif
