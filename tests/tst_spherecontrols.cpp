/*
 * PinglyMotion - the Script Lab panel's vision sphere sliders (setsphere part 2)
 *
 * Exact answers: the sliders and the colour picker give the exact RLV line;
 * the line, sent as a script would, gives the same sphere back (=force only,
 * RLVa only, unchanged); moving a slider changes the 3D picture (exact pixel
 * values through the same processPicture the 3D view calls); every control is
 * greyed with its reason while the "Show environment in 3D view" box is off and
 * nothing then reaches the picture; Clear shows the script's sphere again; Copy
 * puts the exact line on the clipboard; no text shows a user path; the part
 * draws without clipping, dark and light, at 100 to 200 percent text.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QDir>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>

#include "scriptlab/environmentoverride.h"
#include "scriptlab/rlvstate.h"
#include "scriptlab/scriptlabpanel.h"
#include "scriptlab/scriptlabsection.h"
#include "scriptlab/spherecontrols.h"
#include "theme.h"
#include "usersettings.h"
#include "uirender.h"

class TestSphereControls : public QObject
{
  Q_OBJECT

  private slots:
    void initTestCase();
    void init();
    void exactLine();
    void lineRoundTrip();
    void sliderChangesPicture();
    void greyedWhileBoxOff();
    void clearAndPanelText();
    void copyAndNoUserPath();
    void inThePanel();
    void rendered();

  private:
    QTemporaryDir m_settingsDir;
};

void TestSphereControls::initTestCase()
{
  Theme::apply(true);
  // the settings go to a folder of the test's own, never the user's
  QVERIFY(m_settingsDir.isValid());
  QStandardPaths::setTestModeEnabled(true);
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settingsDir.path());
}

void TestSphereControls::init()
{
  UserSettings::setScriptLabParts(QString());   // every panel starts with the first fold state
}

namespace
{
  // a picker that answers a fixed colour (the real one is Qt's colour dialog), and remembers its parent
  struct FakePicker
  {
    QColor answer;
    QColor asked;
    QWidget* parent = nullptr;
    int calls = 0;
  };

  void usePicker(SphereControls& controls, FakePicker& fake)
  {
    controls.setColourPicker([&fake](const QColor& current, QWidget* parent)
    {
      ++fake.calls;
      fake.asked = current;
      fake.parent = parent;
      return fake.answer;
    });
  }

  // a 3 x 1 white picture: the pixels at 0.5, 3 and 20 metres from the avatar
  QImage white3()
  {
    QImage image(3, 1, QImage::Format_RGBA8888);
    image.fill(QColor(255, 255, 255));
    return image;
  }
  const std::vector<float> Metres{0.5f, 3.0f, 20.0f};
}

void TestSphereControls::exactLine()
{
  SphereControls controls;
  // the start: inner 1 m, outer 4 m, full darkness, black
  QCOMPARE(controls.innerMetres(), 1.0);
  QCOMPARE(controls.outerMetres(), 4.0);
  QCOMPARE(controls.darknessPercent(), 100);
  QCOMPARE(controls.rlvLine(), QString("@setsphere=n,setsphere_mode:0=force,setsphere_distmin:1=force,setsphere_distmax:4=force,"
                                       "setsphere_valuemin:0=force,setsphere_valuemax:1=force,setsphere_param:0/0/0=force"));
  QCOMPARE(controls.lineField()->toPlainText(), controls.rlvLine());
  QVERIFY(controls.lineField()->isReadOnly());
  QCOMPARE(controls.innerValue()->text(), QString("1 m"));
  QCOMPARE(controls.outerValue()->text(), QString("4 m"));
  QCOMPARE(controls.darknessValue()->text(), QString("100%"));
  QCOMPARE(controls.colourButton()->text(), QString("0/0/0..."));

  // half-metre steps up to 20 m; the darkness in percent
  QCOMPARE(controls.innerSlider()->maximum(), 40);
  QCOMPARE(controls.darknessSlider()->maximum(), 100);
  controls.innerSlider()->setValue(5);      // 2.5 m
  controls.outerSlider()->setValue(21);     // 10.5 m
  controls.darknessSlider()->setValue(80);
  FakePicker fake;
  fake.answer = QColor(26, 26, 102);
  usePicker(controls, fake);
  QTest::mouseClick(controls.colourButton(), Qt::LeftButton);   // greyed: the box is off, no dialog
  QCOMPARE(fake.calls, 0);
  controls.setAvailable(true);
  QTest::mouseClick(controls.colourButton(), Qt::LeftButton);
  QCOMPARE(fake.calls, 1);
  QCOMPARE(fake.asked, QColor(0, 0, 0));
  QCOMPARE(controls.colour(), QColor(26, 26, 102));
  const QString line = "@setsphere=n,setsphere_mode:0=force,setsphere_distmin:2.5=force,setsphere_distmax:10.5=force,"
                       "setsphere_valuemin:0=force,setsphere_valuemax:0.8=force,setsphere_param:0.1/0.1/0.4=force";
  QCOMPARE(controls.rlvLine(), line);
  QCOMPARE(controls.lineField()->toPlainText(), line);
  QCOMPARE(controls.innerValue()->text(), QString("2.5 m"));
  QCOMPARE(controls.outerValue()->text(), QString("10.5 m"));
  QCOMPARE(controls.darknessValue()->text(), QString("80%"));
  QCOMPARE(controls.colourButton()->text(), QString("0.1/0.1/0.4..."));

  // the dialog cancelled (an invalid colour): nothing changes
  fake.answer = QColor();
  QTest::mouseClick(controls.colourButton(), Qt::LeftButton);
  QCOMPARE(fake.calls, 2);
  QCOMPARE(controls.rlvLine(), line);

  // the inner distance never passes the outer one: each pushes the other
  controls.innerSlider()->setValue(30);
  QCOMPARE(controls.outerSlider()->value(), 30);
  QVERIFY(controls.rlvLine().contains("setsphere_distmin:15=force,setsphere_distmax:15=force"));
  controls.outerSlider()->setValue(0);
  QCOMPARE(controls.innerSlider()->value(), 0);
  QVERIFY(controls.rlvLine().contains("setsphere_distmin:0=force,setsphere_distmax:0=force"));
  // the ends: white, no darkness, 20 m
  controls.setColour(QColor(255, 255, 255));
  controls.darknessSlider()->setValue(0);
  controls.outerSlider()->setValue(40);
  QCOMPARE(controls.rlvLine(), QString("@setsphere=n,setsphere_mode:0=force,setsphere_distmin:0=force,setsphere_distmax:20=force,"
                                       "setsphere_valuemin:0=force,setsphere_valuemax:0=force,setsphere_param:1/1/1=force"));
  // colour parts in hundredths
  QCOMPARE(SphereControls::colourPart(128), 0.5);
  QCOMPARE(SphereControls::colourPart(64), 0.25);
  QCOMPARE(SphereControls::colourPart(1), 0.0);
}

// the line sent as a script sends it (RLVa) gives back the very sphere the controls show
void TestSphereControls::lineRoundTrip()
{
  SphereControls controls;
  controls.setAvailable(true);
  controls.innerSlider()->setValue(3);
  controls.outerSlider()->setValue(17);
  controls.darknessSlider()->setValue(65);
  controls.setColour(QColor(200, 30, 90));

  RlvState rlv;
  rlv.setMode(Rlv::Mode::Rlva);
  QVERIFY(rlv.apply("visor", controls.rlvLine()).ok());
  const EnvironmentOverride::Look look = EnvironmentOverride::lookFrom(rlv);
  QVERIFY(look.hasSphere);
  QVERIFY(look.ignored.isEmpty());
  QVERIFY(look.notShown.isEmpty());
  QVERIFY(look.sphere == controls.sphere());
  QCOMPARE(SphereControls::lineFor(look.sphere), controls.rlvLine());

  // unchanged rules: the values take =force only, and plain RLV does not know @setsphere
  RlvState refused;
  refused.setMode(Rlv::Mode::Rlva);
  QCOMPARE(refused.apply("visor", "@setsphere_distmin:2=n").outcomes.value(0).reason, QString("@setsphere_distmin does not take =n"));
  RlvState plain;
  plain.setMode(Rlv::Mode::Rlv);
  plain.apply("visor", controls.rlvLine());
  QVERIFY(!EnvironmentOverride::lookFrom(plain).hasSphere);
}

// moving a slider changes the 3D picture at once, through the same call the 3D view makes
void TestSphereControls::sliderChangesPicture()
{
  RlvState rlv;
  rlv.setMode(Rlv::Mode::Rlva);
  EnvironmentOverride environment;
  environment.setState(&rlv);
  ScriptLabPanel panel;
  panel.setState(&rlv);
  panel.setEnvironmentOverride(&environment);
  panel.resize(420, 1000);
  panel.show();
  panel.environmentBox()->setChecked(true);
  SphereControls* controls = panel.sphereControls();
  QVERIFY(controls->available());
  QVERIFY(!environment.sphereActive());   // nothing until a control is moved
  QSignalSpy redraw(&environment, &EnvironmentOverride::changed);

  // darkness 100 to 99 and back: the sliders' sphere (inner 1 m, outer 4 m, black, full) is on
  controls->darknessSlider()->setValue(99);
  controls->darknessSlider()->setValue(100);
  QVERIFY(redraw.count() >= 2);
  QVERIFY(environment.sphereActive());
  QVERIFY(environment.sphere() == controls->sphere());
  QImage image = white3();
  environment.processPicture(image, Metres);
  QCOMPARE(image.pixelColor(0, 0), QColor(255, 255, 255));   // 0.5 m: inside, as it was
  QCOMPARE(image.pixelColor(1, 0), QColor(85, 85, 85));      // 3 m: two thirds of the way to black
  QCOMPARE(image.pixelColor(2, 0), QColor(0, 0, 0));         // 20 m: black

  // the inner distance moved to 5 m: the 3 m pixel is inside now and stays white
  const int before = redraw.count();
  controls->innerSlider()->setValue(10);
  QCOMPARE(redraw.count(), before + 1);
  image = white3();
  environment.processPicture(image, Metres);
  QCOMPARE(image.pixelColor(1, 0), QColor(255, 255, 255));
  QCOMPARE(image.pixelColor(2, 0), QColor(0, 0, 0));

  // darkness 50 percent: the far pixel goes half way
  controls->darknessSlider()->setValue(50);
  image = white3();
  environment.processPicture(image, Metres);
  QCOMPARE(image.pixelColor(2, 0), QColor(128, 128, 128));

  // a colour: the far pixel moves half way to it
  controls->setColour(QColor(0, 0, 255));
  image = white3();
  environment.processPicture(image, Metres);
  QCOMPARE(image.pixelColor(2, 0), QColor(128, 128, 255));
}

// with the box off every control is greyed with the reason, and nothing reaches the picture
void TestSphereControls::greyedWhileBoxOff()
{
  RlvState rlv;
  rlv.setMode(Rlv::Mode::Rlva);
  EnvironmentOverride environment;
  environment.setState(&rlv);
  ScriptLabPanel panel;
  panel.setState(&rlv);
  panel.setEnvironmentOverride(&environment);
  panel.resize(420, 1000);
  panel.show();
  panel.section("sphere")->setExpanded(true);
  SphereControls* controls = panel.sphereControls();
  const QString reason = "Tick Show environment in 3D view to try the sphere.";
  const QList<QWidget*> all{controls->innerSlider(), controls->outerSlider(), controls->darknessSlider(), controls->colourButton(),
                            controls->lineField(), controls->copyButton(), controls->clearButton()};

  QVERIFY(!panel.environmentBox()->isChecked());
  QVERIFY(!controls->available());
  QVERIFY(controls->reason()->isVisibleTo(controls));
  QCOMPARE(controls->reason()->text(), reason);
  for (QWidget* w : all)
  {
    QVERIFY2(!w->isEnabled(), qPrintable(w->objectName()));
    QCOMPARE(w->toolTip(), reason);
  }
  controls->darknessSlider()->setValue(40);   // not by the user (greyed), but even so: nothing reaches the view
  QVERIFY(!environment.hasTrySphere());
  QImage image = white3();
  environment.processPicture(image, Metres);
  QCOMPARE(image, white3());

  // ticked: all but Clear work (Clear waits for a moved slider); the reason goes away
  panel.environmentBox()->setChecked(true);
  QVERIFY(controls->available());
  QVERIFY(!controls->reason()->isVisibleTo(controls));
  for (QWidget* w : all)
    QCOMPARE(w->isEnabled(), w != controls->clearButton());
  QCOMPARE(controls->clearButton()->toolTip(), QString("Move a slider first: then its sphere shows in the 3D view."));
  QCOMPARE(controls->innerSlider()->toolTip(), QString("Inside this distance the view stays clear (Left and Right arrows: half a metre)"));
  // the keys move the slider (P8): Right arrow is half a metre
  controls->innerSlider()->setFocus();
  QTest::keyClick(controls->innerSlider(), Qt::Key_Right);
  QCOMPARE(controls->innerMetres(), 1.5);
  QVERIFY(environment.hasTrySphere());
  QVERIFY(controls->clearButton()->isEnabled());
  QCOMPARE(panel.section("sphere")->count(), QString("sliders on"));
  image = white3();
  environment.processPicture(image, Metres);
  QVERIFY(image != white3());

  // unticked again: greyed, and the picture is left exactly as it was
  panel.environmentBox()->setChecked(false);
  for (QWidget* w : all) QVERIFY(!w->isEnabled());
  image = white3();
  environment.processPicture(image, Metres);
  QCOMPARE(image, white3());
  // ticked with the panel hidden: still nothing in the view
  panel.environmentBox()->setChecked(true);
  panel.hide();
  QVERIFY(!environment.sphereActive());
  image = white3();
  environment.processPicture(image, Metres);
  QCOMPARE(image, white3());
  panel.show();
  QVERIFY(environment.sphereActive());
}

// Clear takes the sliders' sphere off; a sphere the script holds shows again. The text under the box says which
void TestSphereControls::clearAndPanelText()
{
  RlvState rlv;
  rlv.setMode(Rlv::Mode::Rlva);
  EnvironmentOverride environment;
  environment.setState(&rlv);
  ScriptLabPanel panel;
  panel.setState(&rlv);
  panel.setEnvironmentOverride(&environment);
  panel.resize(420, 1000);
  panel.show();
  panel.environmentBox()->setChecked(true);
  SphereControls* controls = panel.sphereControls();
  QSignalSpy trying(controls, &SphereControls::tryingChanged);

  controls->darknessSlider()->setValue(80);
  QCOMPARE(trying.count(), 1);
  QCOMPARE(trying.last().at(0).toBool(), true);
  QCOMPARE(panel.environmentStatus()->text(),
           QString("Vision sphere from the sliders: colour fade to 0/0/0, strength 0 at 1 m to 0.8 at 4 m from the avatar.\n"
                   "The whole picture changes by the distance from the avatar. A rough look only: check it in Second Life.\n"
                   "Untick the box to get the 3D view's own look back."));

  // the script holds a sphere too: the sliders' is shown, and the text says so
  QVERIFY(panel.runLine("@setsphere=n,setsphere_distmin:2=force,setsphere_param:0/0/1=force"));
  QVERIFY(environment.sphere() == controls->sphere());
  QCOMPARE(panel.environmentStatus()->text(),
           QString("Vision sphere from the sliders: colour fade to 0/0/0, strength 0 at 1 m to 0.8 at 4 m from the avatar.\n"
                   "The whole picture changes by the distance from the avatar. A rough look only: check it in Second Life.\n"
                   "The sliders' sphere is shown instead of the script's. Press Clear to show the script's again.\n"
                   "Set by: panel.\n"
                   "Untick the box to get the 3D view's own look back."));

  QTest::mouseClick(controls->clearButton(), Qt::LeftButton);
  QCOMPARE(trying.count(), 2);
  QCOMPARE(trying.last().at(0).toBool(), false);
  QVERIFY(!environment.hasTrySphere());
  QVERIFY(!controls->clearButton()->isEnabled());
  QCOMPARE(panel.section("sphere")->count(), QString());
  QVERIFY(environment.sphere() == environment.look().sphere);
  QCOMPARE(environment.sphere().distMin, 2.0);
  QVERIFY(panel.environmentStatus()->text().startsWith("Vision sphere set by the script: colour fade to 0/0/1, strength 1 at 2 m"));
  // the sliders keep their values
  QCOMPARE(controls->darknessPercent(), 80);

  // nothing held by the script and Clear: no sphere at all
  QVERIFY(panel.runLine("@clear"));
  controls->darknessSlider()->setValue(70);
  QVERIFY(environment.sphereActive());
  QTest::mouseClick(controls->clearButton(), Qt::LeftButton);
  QVERIFY(!environment.sphereActive());
  QImage image = white3();
  environment.processPicture(image, Metres);
  QCOMPARE(image, white3());
}

// Copy puts the exact line on the clipboard and says so; no text shows a user path
void TestSphereControls::copyAndNoUserPath()
{
  EnvironmentOverride environment;
  SphereControls controls;
  controls.setOverride(&environment);
  controls.setAvailable(true);
  controls.innerSlider()->setValue(4);
  QCOMPARE(controls.copyButton()->text(), QString("Copy"));
  QTest::mouseClick(controls.copyButton(), Qt::LeftButton);
  QCOMPARE(QGuiApplication::clipboard()->text(), controls.rlvLine());
  QCOMPARE(controls.copyButton()->text(), QString("Copied"));
  controls.innerSlider()->setValue(5);   // a new value: the button reads Copy again
  QCOMPARE(controls.copyButton()->text(), QString("Copy"));
  QCOMPARE(controls.note()->text(), QString("RLVa only. Paste the line into llOwnerSay in your script."));

  // every text and tip, available or not: no home folder, no user folder
  const QString home = QDir::homePath();
  for (bool available : {true, false})
  {
    controls.setAvailable(available);
    QStringList texts;
    for (QWidget* w : controls.findChildren<QWidget*>())
    {
      texts << w->toolTip() << w->accessibleName() << w->accessibleDescription();
      if (auto* label = qobject_cast<QLabel*>(w)) texts << label->text();
      if (auto* button = qobject_cast<QPushButton*>(w)) texts << button->text();
      if (auto* edit = qobject_cast<QPlainTextEdit*>(w)) texts << edit->toPlainText();
    }
    for (const QString& text : texts)
    {
      QVERIFY2(home.isEmpty() || home == "/" || !text.contains(home), qPrintable(text));
      QVERIFY2(!text.contains("/home/") && !text.contains("C:\\Users") && !text.contains("/Users/"), qPrintable(text));
    }
  }
}

// a folded part of its own under the environment box, in the tab order between the box and the view modes box
void TestSphereControls::inThePanel()
{
  ScriptLabPanel panel;
  ScriptLabSection* part = panel.section("sphere");
  QVERIFY(part);
  QCOMPARE(part->title(), QString("Vision sphere"));
  QVERIFY(!part->isExpanded());
  QCOMPARE(panel.sphereControls()->parentWidget(), part->body());
  QCOMPARE(part->parentWidget(), panel.environmentBox()->parentWidget());
  const auto nextFocus = [](QWidget* from)
  {
    QWidget* next = from->nextInFocusChain();
    while (next && !(next->focusPolicy() & Qt::TabFocus)) next = next->nextInFocusChain();
    return next;
  };
  SphereControls* c = panel.sphereControls();
  QCOMPARE(nextFocus(panel.environmentBox()), static_cast<QWidget*>(part->header()));
  QCOMPARE(nextFocus(part->header()), static_cast<QWidget*>(c->innerSlider()));
  QCOMPARE(nextFocus(c->innerSlider()), static_cast<QWidget*>(c->outerSlider()));
  QCOMPARE(nextFocus(c->outerSlider()), static_cast<QWidget*>(c->darknessSlider()));
  QCOMPARE(nextFocus(c->darknessSlider()), static_cast<QWidget*>(c->colourButton()));
  QCOMPARE(nextFocus(c->colourButton()), static_cast<QWidget*>(c->lineField()));
  QCOMPARE(nextFocus(c->lineField()), static_cast<QWidget*>(c->copyButton()));
  QCOMPARE(nextFocus(c->copyButton()), static_cast<QWidget*>(c->clearButton()));
  QCOMPARE(nextFocus(c->clearButton()), static_cast<QWidget*>(panel.viewModesBox()));
  // Qt's colour dialog gets the panel's part as its parent
  FakePicker fake;
  fake.answer = QColor(1, 2, 3);
  usePicker(*c, fake);
  EnvironmentOverride environment;
  panel.setEnvironmentOverride(&environment);
  panel.environmentBox()->setChecked(true);
  QTest::mouseClick(c->colourButton(), Qt::LeftButton);
  QCOMPARE(fake.parent, part->body());
}

// the part drawn in the program's theme: dark and light, 100 to 200 percent text, nothing clipped, normal buttons
void TestSphereControls::rendered()
{
  for (const UiRender::Variant& v : UiRender::variants())
  {
    UiRender::Scope scope(v);
    EnvironmentOverride environment;
    for (bool available : {true, false})
    {
      // the part's body alone (the folding header is the panel's own, tested in tests/scriptlabpanel)
      QWidget part;
      QVBoxLayout* layout = new QVBoxLayout(&part);
      layout->setContentsMargins(Theme::Size::spacingM, Theme::Size::spacingM, Theme::Size::spacingM, Theme::Size::spacingM);
      SphereControls* controls = new SphereControls(&part);
      layout->addWidget(controls);
      controls->applyStyle(v.dark);
      controls->setOverride(&environment);
      controls->setAvailable(available);
      // a docked panel's width; the height its wrapped texts need at that width
      const int width = 340 * v.percent / 100;
      QVERIFY(part.hasHeightForWidth());
      part.setFixedSize(width, part.heightForWidth(width));
      const QImage image = UiRender::render(&part);
      // a wrapped label is judged by the height its text needs at its width (the helper compares with
      // the one-size hint, which is narrower and taller)
      QStringList clipped;
      for (const QString& problem : UiRender::clipping(&part))
      {
        bool wrappedFits = false;
        for (QLabel* label : part.findChildren<QLabel*>())
          if (label->wordWrap() && !label->objectName().isEmpty() && problem.startsWith("QLabel '" + label->objectName() + "' is ") &&
              label->height() >= label->heightForWidth(label->width()))
            wrappedFits = true;
        if (!wrappedFits) clipped << problem;
      }
      QVERIFY2(clipped.isEmpty(), qPrintable(v.name() + ": " + clipped.join("; ")));
      UiRender::save(image, available ? "spherecontrols" : "spherecontrols_off", v);
      const QString raised = Theme::color(Theme::Token::SurfaceRaised, v.dark).name().toUpper();
      QVERIFY(controls->copyButton()->styleSheet().contains("background: " + raised + ";"));
      QVERIFY(controls->copyButton()->styleSheet().contains(":disabled { color: " +
                                                             Theme::color(Theme::Token::TextDisabled, v.dark).name().toUpper() + "; }"));
      QCOMPARE(controls->note()->palette().color(QPalette::WindowText), Theme::color(Theme::Token::TextMuted, v.dark));
      QCOMPARE(controls->darknessSlider()->palette().color(QPalette::Highlight), Theme::color(Theme::Token::Accent, v.dark));
    }
  }
}

QTEST_MAIN(TestSphereControls)
#include "tst_spherecontrols.moc"
