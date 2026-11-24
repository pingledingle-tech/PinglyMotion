/*
 * PinglyMotion - tests for dialogs that fit the screen (src/dialogfit.*), the text size steps
 * (src/textsize.*), with the AI Assistant's "Connect an AI" dialog (src/aiconnectdialog.*) on each of its pages
 *
 * Every dialog, at 100 and 200 percent text size, dark and light, on a small screen (1366 x 768
 * with a 40 pixel taskbar) and on a tiny one: it lies inside the available area of the screen and
 * its buttons are visible and enabled. Setup keeps inside the screen while the text size changes
 * and shrinks back. Set PINGLYMOTION_UI_OUT to a folder to get a picture of each dialog.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#include <QAbstractButton>
#include <QDialog>
#include <QDockWidget>
#include <QLabel>
#include <QMainWindow>
#include <QDir>
#include <QFile>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

#include <functional>
#include <memory>

#include "aboutdialog.h"
#include "animexportdialog.h"
#include "animfile.h"
#include "camerakeysdialog.h"
#include "aiconnectdialog.h"
#include "dialogfit.h"
#include "easing.h"
#include "easingtiles.h"
#include "gltfimport.h"
#include "gltfimportdialog.h"
#include "keysmousedialog.h"
#include "retargetdialog.h"
#include "setupdialog.h"
#include "textsize.h"
#include "theme.h"
#include "uirender.h"
#include "updatenotificationdialog.h"
#include "usersettingsdialog.h"
#include "whatsnewdialog.h"

namespace
{
  AnimFile::Animation readBvh(const QString& path)
  {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return AnimFile::Animation();
    const AnimFile::Result result = AnimFile::parseBvh(file.readAll());
    return result.ok() ? *result.animation : AnimFile::Animation();
  }

  struct Maker
  {
    const char* name;
    std::function<QDialog*()> make;
  };

  QList<Maker> makers()
  {
    static const AnimFile::Animation source = readBvh(PINGLYMOTION_MOTIONS "/mixamo_style_wave.bvh");
    static const AnimFile::Animation target = readBvh(PINGLYMOTION_DATA "/SLFemale.bvh");
    QString news;
    for (int i = 1; i <= 80; ++i) news += QStringLiteral("Line %1 of what is new in this version.\n").arg(i);
    return {
      {"setup", []() -> QDialog* { return new SetupDialog; }},
      {"camerakeys", []() -> QDialog* { return new CameraKeysDialog; }},
      {"keysmouse", []() -> QDialog* { return new KeysMouseDialog; }},
      {"retarget", []() -> QDialog* { return new RetargetDialog(source, target, "wave.bvh", nullptr); }},
      {"gltfimport", []() -> QDialog*
       {
         GltfImport::Document document;
         GltfImport::Clip clip;
         clip.name = "Wave";
         clip.end = 2.0;
         document.animations = {clip, clip};
         return new GltfImportDialog(document);
       }},
      {"animexport", []() -> QDialog* { return new AnimExportDialog(2.0); }},
      {"usersettings", []() -> QDialog* { return new UserSettingsDialog; }},
      {"about", []() -> QDialog* { return new AboutDialog; }},
      {"whatsnew", [news]() -> QDialog* { return new WhatsNewDialog(news); }},
      {"updatenotification", []() -> QDialog* { return new UpdateNotificationDialog; }},
      {"curveeditor", []() -> QDialog* { return new CurveEditorDialog(Easing::Bezier()); }},
      {"aiconnect", []() -> QDialog* { return new AiConnectDialog(false); }},
      {"aiconnect_claude", []() -> QDialog* { AiConnectDialog* d = new AiConnectDialog(false); d->showPage(AiConnectDialog::Claude); return d; }},
      {"aiconnect_chatgpt", []() -> QDialog* { AiConnectDialog* d = new AiConnectDialog(false); d->showPage(AiConnectDialog::ChatGpt); return d; }},
      {"aiconnect_other", []() -> QDialog* { AiConnectDialog* d = new AiConnectDialog(false); d->showPage(AiConnectDialog::Other); return d; }},
    };
  }

  void settle()
  {
    QTest::qWait(60);
    QCoreApplication::processEvents();
    QTest::qWait(20);
  }

  // the buttons that are not inside a scroll area: Done, OK, Cancel, Apply ...
  QList<QAbstractButton*> fixedButtons(QDialog& dialog)
  {
    QList<QAbstractButton*> out;
    const QList<QScrollArea*> scrolls = dialog.findChildren<QScrollArea*>();
    for (QAbstractButton* button : dialog.findChildren<QAbstractButton*>())
    {
      if (!qobject_cast<QPushButton*>(button)) continue;
      bool inside = false;
      for (QScrollArea* scroll : scrolls)
        if (scroll->isAncestorOf(button)) inside = true;
      if (!inside) out << button;
    }
    return out;
  }

  // empty when the dialog is inside the area and its buttons are all visible and enabled
  QString problem(QDialog& dialog, const QRect& area)
  {
    const QRect frame = dialog.frameGeometry();
    if (!area.contains(frame))
      return QStringLiteral("dialog %1x%2 at %3,%4 is not inside the available area %5x%6")
        .arg(frame.width()).arg(frame.height()).arg(frame.x()).arg(frame.y()).arg(area.width()).arg(area.height());
    const QList<QAbstractButton*> buttons = fixedButtons(dialog);
    if (buttons.isEmpty()) return QStringLiteral("no button outside the scroll area");
    for (QAbstractButton* button : buttons)
    {
      if (!button->isVisible()) return QStringLiteral("button %1 is not visible").arg(button->text());
      if (!button->isEnabled()) return QStringLiteral("button %1 is not enabled").arg(button->text());
      const QRect where(button->mapTo(&dialog, QPoint(0, 0)), button->size());
      if (!dialog.rect().contains(where))
        return QStringLiteral("button %1 sticks out of the dialog").arg(button->text());
      if (button->width() < 10 || button->height() < 10) return QStringLiteral("button %1 is squeezed").arg(button->text());
    }
    return QString();
  }
}

class TestDialogFit : public QObject
{
  Q_OBJECT

  QTemporaryDir m_settingsFolder;

  private slots:
    void initTestCase()
    {
      QCoreApplication::setOrganizationName("PinglyMotionTest");
      QCoreApplication::setApplicationName("dialogfit");
      QSettings::setDefaultFormat(QSettings::IniFormat);
      QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settingsFolder.path());
    }

    void cleanup()
    {
      DialogFit::setAreaForTests(QRect());
      TextSize::resetForTests();
    }

    // every dialog, 100 and 200 percent text, dark and light, on 1366 x 768 minus a 40 pixel taskbar
    void everyDialogFitsASmallScreen()
    {
      const QRect area(0, 0, 1366, 728);
      DialogFit::setAreaForTests(area);
      for (const UiRender::Variant& variant : UiRender::variants())
      {
        if (variant.percent == 150) continue;
        UiRender::Scope scope(variant);
        for (const Maker& maker : makers())
        {
          std::unique_ptr<QDialog> dialog(maker.make());
          dialog->show();
          settle();
          const QString what = QStringLiteral("%1 %2").arg(QLatin1String(maker.name), variant.name());
          const QString why = problem(*dialog, area);
          QVERIFY2(why.isEmpty(), qPrintable(what + ": " + why));
          const QString out = UiRender::outDir();
          if (!out.isEmpty()) dialog->grab().save(QDir(out).filePath(QStringLiteral("dialog_%1_%2.png").arg(QLatin1String(maker.name), variant.name())));
        }
      }
    }

    // a tiny screen: the scroll area has to do its work, the buttons stay
    void everyDialogFitsATinyScreen()
    {
      const QRect area(0, 0, 800, 480);
      DialogFit::setAreaForTests(area);
      for (int percent : {100, 200})
      {
        UiRender::Scope scope(UiRender::Variant{true, percent});
        for (const Maker& maker : makers())
        {
          std::unique_ptr<QDialog> dialog(maker.make());
          dialog->show();
          settle();
          const QString why = problem(*dialog, area);
          QVERIFY2(why.isEmpty(), qPrintable(QStringLiteral("%1 at %2: %3").arg(QLatin1String(maker.name)).arg(percent).arg(why)));
        }
      }
    }

    // a dialog that is already open follows the text size: it grows, shrinks back and is pulled
    // back onto the screen
    void setupFollowsTheTextSize()
    {
      const QRect area(0, 0, 1366, 728);
      DialogFit::setAreaForTests(area);
      SetupDialog dialog;
      dialog.show();
      settle();
      QVERIFY2(problem(dialog, area).isEmpty(), qPrintable(problem(dialog, area)));
      const QSize at100 = dialog.size();

      TextSize::apply(200);
      settle();
      QVERIFY2(problem(dialog, area).isEmpty(), qPrintable(QStringLiteral("200: ") + problem(dialog, area)));
      const QSize at200 = dialog.size();
      QVERIFY(at200.width() > at100.width() || at200.height() > at100.height());

      // pushed half off the screen by the user, then the text size changes
      dialog.move(1300, 700);
      TextSize::apply(175);
      settle();
      QVERIFY2(problem(dialog, area).isEmpty(), qPrintable(QStringLiteral("175 off screen: ") + problem(dialog, area)));

      // going back does shrink it again (it used to stay as big as it had been)
      TextSize::apply(100);
      settle();
      QVERIFY2(problem(dialog, area).isEmpty(), qPrintable(QStringLiteral("100: ") + problem(dialog, area)));
      QVERIFY(dialog.width() < at200.width() || dialog.height() < at200.height());
      QCOMPARE(dialog.size(), at100);
    }

    // Ctrl+0 after 200 percent: every open dialog goes back to exactly the size it had at 100 percent
    // (the project owner's test, 24 Nov 2026: the text went back, the windows stayed a little too big)
    void everyDialogShrinksBackAfterCtrl0()
    {
      const QRect area(0, 0, 1920, 1040);
      DialogFit::setAreaForTests(area);
      for (const Maker& maker : makers())
      {
        TextSize::apply(100);
        std::unique_ptr<QDialog> dialog(maker.make());
        dialog->show();
        settle();
        const QSize at100 = dialog->size();
        for (int percent : {125, 150, 175, 200})
        {
          TextSize::apply(percent);
          settle();
        }
        TextSize::apply(TextSize::Default);   // what Ctrl+0 does
        settle();
        QVERIFY2(dialog->size() == at100,
                 qPrintable(QStringLiteral("%1: %2x%3 after Ctrl+0, %4x%5 at 100 percent").arg(QLatin1String(maker.name))
                              .arg(dialog->width()).arg(dialog->height()).arg(at100.width()).arg(at100.height())));
      }
    }

    // the panels too: a docked panel pushed wider (or taller) at 200 percent is exactly as big as at
    // 100 percent again after Ctrl+0, and the room the user gave it beyond its smallest size stays
    void panelsShrinkBackAfterCtrl0()
    {
      QMainWindow window;
      window.setCentralWidget(new QWidget(&window));
      window.resize(1600, 1000);
      auto panel = [&window](const char* name, Qt::DockWidgetArea area) {
        QDockWidget* dock = new QDockWidget(QString::fromLatin1(name), &window);
        dock->setObjectName(QString::fromLatin1(name));
        QLabel* label = new QLabel(QStringLiteral("A panel whose text decides its smallest size"), dock);
        label->setMinimumHeight(label->sizeHint().height());
        dock->setWidget(label);
        window.addDockWidget(area, dock);
        return dock;
      };
      QDockWidget* left = panel("left", Qt::LeftDockWidgetArea);
      QDockWidget* bottom = panel("bottom", Qt::BottomDockWidgetArea);
      QDockWidget* floating = panel("floating", Qt::RightDockWidgetArea);
      floating->setFloating(true);
      window.show();
      settle();
      // the user gave the left panel 40 pixels more than it needs
      window.resizeDocks({left}, {DialogFit::leastPanelSize(left).width() + 40}, Qt::Horizontal);
      floating->resize(DialogFit::leastPanelSize(floating) + QSize(30, 20));
      settle();
      const QList<QDockWidget*> docks = {left, bottom, floating};
      QHash<QDockWidget*, QSize> at100;
      for (QDockWidget* dock : docks) at100[dock] = dock->size();

      auto change = [&](int percent) {
        const QList<DialogFit::PanelSize> before = DialogFit::measurePanels(&window, docks);
        TextSize::apply(percent);
        settle();
        DialogFit::followTextSize(&window, before);
        settle();
      };
      change(200);
      // bigger text: the left panel is wider, and still has its 40 pixels of room
      QVERIFY(left->width() > at100[left].width());
      QCOMPARE(left->width(), DialogFit::leastPanelSize(left).width() + 40);
      QVERIFY(floating->width() > at100[floating].width());
      change(TextSize::Default);   // what Ctrl+0 does
      QCOMPARE(left->width(), at100[left].width());
      QCOMPARE(bottom->height(), at100[bottom].height());
      QCOMPARE(floating->size(), at100[floating]);
    }

    // with room on the screen the dialog shows all of its content
    void roomyScreenKeepsTheSizeHint()
    {
      DialogFit::setAreaForTests(QRect(0, 0, 3000, 2000));
      SetupDialog dialog;
      dialog.show();
      settle();
      // everything is shown: the scroll area needs no scroll bar
      QScrollArea* scroll = dialog.findChild<QScrollArea*>();
      QVERIFY(scroll);
      QVERIFY(!scroll->verticalScrollBar()->isVisible());
      QVERIFY(!scroll->horizontalScrollBar()->isVisible());
      QVERIFY(dialog.height() >= dialog.minimumSizeHint().height());
    }

    // Ctrl+= goes 175 -> 200 -> 200 (no wrap to 100), Ctrl+- steps down and stops at 100,
    // Ctrl+0 is 100
    void textSizeSteps()
    {
      QCOMPARE(TextSize::stepUp(100), 125);
      QCOMPARE(TextSize::stepUp(175), 200);
      QCOMPARE(TextSize::stepUp(200), 200);
      QCOMPARE(TextSize::stepDown(200), 175);
      QCOMPARE(TextSize::stepDown(125), 100);
      QCOMPARE(TextSize::stepDown(100), 100);
      QCOMPARE(TextSize::Default, 100);
      // a number outside the range is cut first
      QCOMPARE(TextSize::stepUp(500), 200);
      QCOMPARE(TextSize::stepDown(0), 100);
    }
};

QTEST_MAIN(TestDialogFit)
#include "tst_dialogfit.moc"
