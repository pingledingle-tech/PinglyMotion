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
#include <QAction>
#include <QLabel>
#include <QMainWindow>
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
    void textSizeKeyNames();
    void textSizeKeysWorkOnEveryLayout();
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

// test AW of the project owner (23 November 2026): Ctrl+= did not work on a
// Norwegian layout, where "=" is Shift+0. Bigger is Ctrl and the plus key
// (next to 0 on Nordic and German layouts), Ctrl+= (US) or the number pad's
// plus; the menu shows the first key
void TestTextSize::textSizeKeyNames()
{
  auto names=[](const QList<QKeySequence>& keys)
  {
    QStringList out;
    for(const QKeySequence& key : keys) out << key.toString(QKeySequence::PortableText);
    return out;
  };
  QCOMPARE(names(TextSize::biggerKeys()),(QStringList{"Ctrl++","Ctrl+=","Ctrl+Num++"}));
  QCOMPARE(names(TextSize::smallerKeys()),(QStringList{"Ctrl+-","Ctrl+Num+-"}));
  QCOMPARE(names(TextSize::resetKeys()),(QStringList{"Ctrl+0","Ctrl+Num+0"}));
  // the exact key presses: Qt::Key_Plus, Key_Equal, Key_Minus and Key_0, with Ctrl,
  // and with the number pad flag
  const QList<QKeySequence> bigger=TextSize::biggerKeys();
  QCOMPARE(bigger.at(0)[0],QKeyCombination(Qt::ControlModifier,Qt::Key_Plus));
  QCOMPARE(bigger.at(1)[0],QKeyCombination(Qt::ControlModifier,Qt::Key_Equal));
  QCOMPARE(bigger.at(2)[0],QKeyCombination(Qt::ControlModifier|Qt::KeypadModifier,Qt::Key_Plus));
  QCOMPARE(TextSize::smallerKeys().at(0)[0],QKeyCombination(Qt::ControlModifier,Qt::Key_Minus));
  QCOMPARE(TextSize::smallerKeys().at(1)[0],QKeyCombination(Qt::ControlModifier|Qt::KeypadModifier,Qt::Key_Minus));
  QCOMPARE(TextSize::resetKeys().at(0)[0],QKeyCombination(Qt::ControlModifier,Qt::Key_0));
  // the keys parse back from the names the menu and the Keys and mouse page show
  QCOMPARE(QKeySequence(QStringLiteral("Ctrl++")),bigger.at(0));
  QCOMPARE(QKeySequence(QStringLiteral("Ctrl+=")),bigger.at(1));
  QCOMPARE(QKeySequence(QStringLiteral("Ctrl+-")),TextSize::smallerKeys().at(0));
  // no key is in two lists, so no key press is ambiguous
  QList<QKeySequence> all=bigger+TextSize::smallerKeys()+TextSize::resetKeys();
  for(int i=0;i<all.size();++i)
    for(int j=i+1;j<all.size();++j)
      QVERIFY2(all.at(i)!=all.at(j),qPrintable(all.at(i).toString()));
}

// every key press of every list triggers its action once, and only its own
void TestTextSize::textSizeKeysWorkOnEveryLayout()
{
  QMainWindow window;
  QAction* bigger=new QAction(QStringLiteral("Bigger"),&window);
  QAction* smaller=new QAction(QStringLiteral("Smaller"),&window);
  QAction* reset=new QAction(QStringLiteral("Reset"),&window);
  bigger->setShortcuts(TextSize::biggerKeys());
  smaller->setShortcuts(TextSize::smallerKeys());
  reset->setShortcuts(TextSize::resetKeys());
  window.addAction(bigger);
  window.addAction(smaller);
  window.addAction(reset);
  window.show();
  window.activateWindow();
  QVERIFY(QTest::qWaitForWindowActive(&window));

  int biggerCount=0,smallerCount=0,resetCount=0;
  connect(bigger,&QAction::triggered,this,[&biggerCount]() { ++biggerCount; });
  connect(smaller,&QAction::triggered,this,[&smallerCount]() { ++smallerCount; });
  connect(reset,&QAction::triggered,this,[&resetCount]() { ++resetCount; });

  struct Press { Qt::Key key; Qt::KeyboardModifiers modifiers; int bigger; int smaller; int reset; };
  const Press presses[]={
    {Qt::Key_Plus,Qt::ControlModifier,1,0,0},                         // the plus key next to 0 (Nordic, German)
    {Qt::Key_Equal,Qt::ControlModifier,1,0,0},                        // Ctrl+= (US)
    {Qt::Key_Plus,Qt::ControlModifier|Qt::KeypadModifier,1,0,0},      // the number pad's plus
    {Qt::Key_Minus,Qt::ControlModifier,0,1,0},
    {Qt::Key_Minus,Qt::ControlModifier|Qt::KeypadModifier,0,1,0},
    {Qt::Key_0,Qt::ControlModifier,0,0,1},
    {Qt::Key_0,Qt::ControlModifier|Qt::KeypadModifier,0,0,1},
  };
  for(const Press& press : presses)
  {
    biggerCount=smallerCount=resetCount=0;
    QTest::keyClick(&window,press.key,press.modifiers);
    const QString what=QKeySequence(QKeyCombination(press.modifiers,press.key)).toString();
    QVERIFY2(biggerCount==press.bigger,qPrintable(what));
    QVERIFY2(smallerCount==press.smaller,qPrintable(what));
    QVERIFY2(resetCount==press.reset,qPrintable(what));
  }
}

QTEST_MAIN(TestTextSize)
#include "tst_textsize.moc"
