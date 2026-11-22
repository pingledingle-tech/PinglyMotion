/*
 * PinglyMotion - test of the UI render helper, used on the transport bar
 *
 * Draws the transport bar without a screen in the program's own theme (dark
 * and light, 100 and 200 percent text size), checks the token colours of the
 * buttons and that nothing is clipped. Set PINGLYMOTION_UI_OUT to a folder to
 * get the pictures.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#include <QApplication>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QtTest>

#include "playstate.h"
#include "theme.h"
#include "transportbar.h"
#include "transportlook.h"
#include "uirender.h"

class TestUiRender : public QObject
{
  Q_OBJECT

  private slots:
    void sixVariants();
    void scopeSetsAndRestoresThemeAndFont();
    void textSizeMakesThePanelBigger();
    void transportBarHasTheProgramLook();
    void transportBarTokenColours();
    void transportBarIsNotClipped();
    void clippingIsFound();
    void savesPicturesOnlyWhenAsked();
};

void TestUiRender::sixVariants()
{
  QStringList names;
  for (const UiRender::Variant& v : UiRender::variants())
    names << v.name();
  QCOMPARE(names, QStringList({"dark_100", "dark_150", "dark_200", "light_100", "light_150", "light_200"}));
}

void TestUiRender::scopeSetsAndRestoresThemeAndFont()
{
  const bool wasDark = Theme::isDark();
  const QFont before = QApplication::font();
  const qreal size = before.pointSizeF() > 0 ? before.pointSizeF() : before.pixelSize();
  {
    UiRender::Scope scope({!wasDark, 200});
    QCOMPARE(Theme::isDark(), !wasDark);
    const QFont f = QApplication::font();
    QCOMPARE(f.pointSizeF() > 0 ? f.pointSizeF() : qreal(f.pixelSize()), size * 2);
  }
  QCOMPARE(Theme::isDark(), wasDark);
  QCOMPARE(QApplication::font(), before);
}

void TestUiRender::textSizeMakesThePanelBigger()
{
  int width[2] = {0, 0};
  for (int i = 0; i < 2; ++i)
  {
    UiRender::Scope scope({true, i == 0 ? 100 : 200});
    TransportBar bar;
    width[i] = UiRender::render(&bar).width();
  }
  QVERIFY2(width[1] > width[0] * 3 / 2, qPrintable(QStringLiteral("%1 and %2").arg(width[0]).arg(width[1])));
}

void TestUiRender::transportBarHasTheProgramLook()
{
  // the point of the helper: dark is dark and light is light, not Qt's plain light style
  for (const UiRender::Variant& v : UiRender::variants())
  {
    UiRender::Scope scope(v);
    TransportBar bar;
    const QImage image = UiRender::render(&bar);
    UiRender::save(image, QStringLiteral("transportbar"), v);
    const QColor corner = image.pixelColor(0, 0);
    QCOMPARE(corner.name(), QApplication::palette().color(QPalette::Window).name());
    QVERIFY2(v.dark ? corner.lightness() < 100 : corner.lightness() > 150, qPrintable(v.name()));
  }
}

void TestUiRender::transportBarTokenColours()
{
  using Theme::Token;
  for (const UiRender::Variant& v : UiRender::variants())
  {
    UiRender::Scope scope(v);
    TransportBar bar;
    const QImage image = UiRender::render(&bar);
    // inside the border, in the middle of the left edge of a button
    const int x = TransportLook::ringMargin() + 3;
    auto at = [&](QPushButton* b) { return QPoint(x, b->height() / 2); };
    QString why;
    // stopped: Start is the main button, Replay is normal, Stop cannot be clicked
    why = UiRender::expectColour(image, &bar, bar.startButton(), at(bar.startButton()),
                                 Theme::color(Token::Accent, v.dark), v.name() + " Start");
    QVERIFY2(why.isEmpty(), qPrintable(why));
    why = UiRender::expectColour(image, &bar, bar.replayButton(), at(bar.replayButton()),
                                 Theme::color(Token::SurfaceRaised, v.dark), v.name() + " Replay");
    QVERIFY2(why.isEmpty(), qPrintable(why));
    why = UiRender::expectColour(image, &bar, bar.stopButton(), at(bar.stopButton()),
                                 Theme::color(Token::SurfaceRaised, v.dark), v.name() + " Stop, stopped");
    QVERIFY2(why.isEmpty(), qPrintable(why));
    // playing: Stop is the danger button
    bar.setPlaystate(PLAYSTATE_LOOPING);
    const QImage playing = UiRender::render(&bar);
    why = UiRender::expectColour(playing, &bar, bar.stopButton(), at(bar.stopButton()),
                                 Theme::color(Token::DangerFill, v.dark), v.name() + " Stop, playing");
    QVERIFY2(why.isEmpty(), qPrintable(why));
    // a wrong expectation is reported, not passed
    QVERIFY(!UiRender::expectColour(image, &bar, bar.startButton(), at(bar.startButton()),
                                    Theme::color(Token::Danger, v.dark), "wrong").isEmpty());
  }
}

void TestUiRender::transportBarIsNotClipped()
{
  for (const UiRender::Variant& v : UiRender::variants())
  {
    UiRender::Scope scope(v);
    TransportBar bar;
    UiRender::render(&bar);
    const QStringList problems = UiRender::clipping(&bar);
    QVERIFY2(problems.isEmpty(), qPrintable(v.name() + ": " + problems.join('\n')));
  }
}

void TestUiRender::clippingIsFound()
{
  UiRender::Scope scope({true, 100});
  QWidget panel;
  QVBoxLayout* layout = new QVBoxLayout(&panel);
  QLabel* label = new QLabel(QStringLiteral("A long text that does not fit in a narrow label"), &panel);
  label->setFixedWidth(20);
  layout->addWidget(label);
  UiRender::render(&panel);
  QVERIFY(!UiRender::clipping(&panel).isEmpty());
}

void TestUiRender::savesPicturesOnlyWhenAsked()
{
  const QByteArray old = qgetenv("PINGLYMOTION_UI_OUT");
  QImage image(4, 4, QImage::Format_RGB32);
  image.fill(Qt::red);
  qunsetenv("PINGLYMOTION_UI_OUT");
  QVERIFY(UiRender::save(image, "x", {true, 100}).isEmpty());
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  qputenv("PINGLYMOTION_UI_OUT", (dir.path() + "/sub").toUtf8());
  const QString path = UiRender::save(image, "x", {false, 200});
  QVERIFY(path.endsWith("x_light_200.png"));
  QVERIFY(QFile::exists(path));
  if (old.isEmpty())
    qunsetenv("PINGLYMOTION_UI_OUT");
  else
    qputenv("PINGLYMOTION_UI_OUT", old);
}

QTEST_MAIN(TestUiRender)
#include "tst_uirender.moc"
