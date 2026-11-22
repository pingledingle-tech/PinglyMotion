/*
 * PinglyMotion - test of the program's text size (src/textsize.*)
 *
 * docs/UX_DECISIONS.md, 6c and section 8: the text size setting, 100 to 200
 * percent, makes the application font that much bigger than the system font
 * the program started with, sizes written as numbers grow with it, and the
 * timeline rows (Constants::lineHeight) follow it. Every answer is exact.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#include <QApplication>
#include <QLabel>
#include <QtTest>

#include "constants.h"
#include "textsize.h"

class TestTextSize : public QObject
{
  Q_OBJECT

  private slots:
    void init();
    void cleanup();
    void percentIsCutTo100To200();
    void pointFontIsScaled();
    void pixelFontIsScaled();
    void applyScalesTheStartFontNotTheLastOne();
    void applyAnswersThePercentItUsed();
    void sizesInPixelsFollowThePercent();
    void timelineRowsFollowTheTextSize();
    void widgetsGetAFontChangeAndGrow();
    void sameSizeAgainSendsNoFontChange();
};

void TestTextSize::init()
{
  TextSize::resetForTests();
  QFont font=QApplication::font();
  font.setPointSizeF(10.0);
  QApplication::setFont(font);
}

void TestTextSize::cleanup()
{
  TextSize::resetForTests();
}

void TestTextSize::percentIsCutTo100To200()
{
  QCOMPARE(TextSize::bounded(50),100);
  QCOMPARE(TextSize::bounded(99),100);
  QCOMPARE(TextSize::bounded(100),100);
  QCOMPARE(TextSize::bounded(150),150);
  QCOMPARE(TextSize::bounded(200),200);
  QCOMPARE(TextSize::bounded(201),200);
  QCOMPARE(TextSize::bounded(-5),100);
}

void TestTextSize::pointFontIsScaled()
{
  QFont base;
  base.setPointSizeF(9.0);
  QCOMPARE(TextSize::scaledFont(base,100).pointSizeF(),9.0);
  QCOMPARE(TextSize::scaledFont(base,150).pointSizeF(),13.5);
  QCOMPARE(TextSize::scaledFont(base,200).pointSizeF(),18.0);
  QCOMPARE(TextSize::scaledFont(base,300).pointSizeF(),18.0);
  // nothing else changes
  base.setBold(true);
  base.setFamily("Sans");
  const QFont big=TextSize::scaledFont(base,200);
  QCOMPARE(big.bold(),true);
  QCOMPARE(big.family(),base.family());
}

void TestTextSize::pixelFontIsScaled()
{
  QFont base;
  base.setPixelSize(13);
  QCOMPARE(TextSize::scaledFont(base,100).pixelSize(),13);
  QCOMPARE(TextSize::scaledFont(base,150).pixelSize(),20);   // 19.5 rounded
  QCOMPARE(TextSize::scaledFont(base,200).pixelSize(),26);
}

void TestTextSize::applyScalesTheStartFontNotTheLastOne()
{
  QCOMPARE(TextSize::percent(),100);
  TextSize::apply(150);
  QCOMPARE(QApplication::font().pointSizeF(),15.0);
  QCOMPARE(TextSize::percent(),150);
  TextSize::apply(200);
  QCOMPARE(QApplication::font().pointSizeF(),20.0);
  TextSize::apply(100);
  QCOMPARE(QApplication::font().pointSizeF(),10.0);
}

void TestTextSize::applyAnswersThePercentItUsed()
{
  QCOMPARE(TextSize::apply(250),200);
  QCOMPARE(TextSize::percent(),200);
  QCOMPARE(QApplication::font().pointSizeF(),20.0);
  QCOMPARE(TextSize::apply(10),100);
  QCOMPARE(QApplication::font().pointSizeF(),10.0);
}

void TestTextSize::sizesInPixelsFollowThePercent()
{
  QCOMPARE(TextSize::scaled(16),16);
  QCOMPARE(TextSize::scaled(16,150),24);
  QCOMPARE(TextSize::scaled(11,150),17);   // 16.5 rounded
  QCOMPARE(TextSize::scaled(16,200),32);
  QCOMPARE(TextSize::scaled(16,400),32);
  TextSize::apply(150);
  QCOMPARE(TextSize::scaled(10),15);
  TextSize::apply(200);
  QCOMPARE(TextSize::scaled(10),20);
}

void TestTextSize::timelineRowsFollowTheTextSize()
{
  QCOMPARE(Constants::lineHeight(),11);
  QCOMPARE(Constants::keyHeight(),10);
  QCOMPARE(Constants::leftStrut(),120);
  // the time axis does not change
  QCOMPARE(Constants::keyWidth(),10);
  TextSize::apply(150);
  QCOMPARE(Constants::lineHeight(),17);
  QCOMPARE(Constants::keyHeight(),15);
  QCOMPARE(Constants::leftStrut(),180);
  QCOMPARE(Constants::keyWidth(),10);
  TextSize::apply(200);
  QCOMPARE(Constants::lineHeight(),22);
  QCOMPARE(Constants::keyHeight(),20);
  QCOMPARE(Constants::leftStrut(),240);
  QCOMPARE(Constants::keyWidth(),10);
}

namespace
{
  class FontWatcher : public QLabel
  {
    public:
      using QLabel::QLabel;
      int changes=0;

    protected:
      void changeEvent(QEvent* event) override
      {
        if(event->type()==QEvent::FontChange) ++changes;
        QLabel::changeEvent(event);
      }
  };
}

void TestTextSize::widgetsGetAFontChangeAndGrow()
{
  FontWatcher label(QStringLiteral("Frames per second:"));
  label.ensurePolished();
  const QSize before=label.sizeHint();
  TextSize::apply(200);
  QCoreApplication::processEvents();
  QCOMPARE(label.changes,1);
  QCOMPARE(label.font().pointSizeF(),20.0);
  const QSize after=label.sizeHint();
  QVERIFY2(after.width()>before.width()*3/2 && after.height()>before.height()*3/2,
           qPrintable(QStringLiteral("%1x%2 then %3x%4").arg(before.width()).arg(before.height())
                      .arg(after.width()).arg(after.height())));
}

void TestTextSize::sameSizeAgainSendsNoFontChange()
{
  FontWatcher label(QStringLiteral("Loop"));
  TextSize::apply(150);
  QCoreApplication::processEvents();
  QCOMPARE(label.changes,1);
  TextSize::apply(150);
  QCoreApplication::processEvents();
  QCOMPARE(label.changes,1);
}

QTEST_MAIN(TestTextSize)
#include "tst_textsize.moc"
