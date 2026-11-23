/*
 * PinglyMotion - UX phase 4a: camera habits and view shortcuts
 * (docs/UX_DECISIONS.md, 6c; docs/mockup/index.html, SCHEMES and gestureFor())
 *
 * Exact answers: each of the five rows the Camera and keys dialog shows for a habit is
 * what the camera really does (Camera::gestureFor() and the focus rules), the keys every
 * habit shares are the mockup's words, the status line after a choice, the choice read
 * back after a restart, and that the 3D view and the window follow the habit (their
 * source, since the 3D view needs OpenGL and the window the whole program).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QFile>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

#include "camera.h"
#include "camerahabitspec.h"
#include "usersettings.h"

Q_DECLARE_METATYPE(CameraDrag)

class TestCameraHabits : public QObject
{
  Q_OBJECT

  private slots:
    void initTestCase();
    void rowsSayWhatTheCameraDoes();
    void wheelZoomsInEveryHabit();
    void sharedRowsMatchTheMockup();
    void chosenMessage();
    void habitSurvivesRestart();
    void viewFollowsTheHabit();
    void windowPassesTheChoiceOn();

  private:
    static QString read(const QString& path);
    static QString viewSource() { return read(QStringLiteral(PINGLYMOTION_SOURCE_DIR "/animationview.cpp")); }
    static QString windowSource() { return read(QStringLiteral(PINGLYMOTION_SOURCE_DIR "/pinglymotionwindow.cpp")); }
    QTemporaryDir m_dir;
};

QString TestCameraHabits::read(const QString& path)
{
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) return QString();
  return QString::fromUtf8(file.readAll());
}

void TestCameraHabits::initTestCase()
{
  QVERIFY(m_dir.isValid());
  QStandardPaths::setTestModeEnabled(true);
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_dir.path());
}

// The words in src/camerahabitspec.h (checked against the mockup in tests/setupdialog) and
// the camera: for each habit, the gesture text and the button + keys that do it.
void TestCameraHabits::rowsSayWhatTheCameraDoes()
{
  const Qt::KeyboardModifiers none = Qt::NoModifier, alt = Qt::AltModifier, ctrl = Qt::ControlModifier,
                              shift = Qt::ShiftModifier;
  const Qt::MouseButton left = Qt::LeftButton, right = Qt::RightButton, middle = Qt::MiddleButton;
  struct Case { const CameraHabitSpec::Habit* spec; CameraHabit habit; };
  for (const Case& c : {Case{&CameraHabitSpec::pinglymotion, CameraHabit::PinglyMotion},
                        Case{&CameraHabitSpec::maya, CameraHabit::Maya},
                        Case{&CameraHabitSpec::blender, CameraHabit::Blender},
                        Case{&CameraHabitSpec::sl, CameraHabit::SecondLife}})
    QCOMPARE(Camera::habitFor(QLatin1String(c.spec->id)), c.habit);

  auto drag = [](CameraHabit h, Qt::MouseButton b, Qt::KeyboardModifiers k) { return Camera::gestureFor(h, b, k).drag; };
  auto picks = [](CameraHabit h, Qt::MouseButton b, Qt::KeyboardModifiers k) { return Camera::gestureFor(h, b, k).picks; };
  auto gesture = [](const CameraHabitSpec::Habit& h, int row) { return QString::fromLatin1(h.rows[row].gesture); };

  // PinglyMotion: "Drag empty space", "Right-drag", "Mouse wheel", "Click it, or its row", "F, or double-click it"
  QCOMPARE(gesture(CameraHabitSpec::pinglymotion, 0), QStringLiteral("Drag empty space"));
  QCOMPARE(drag(CameraHabit::PinglyMotion, left, none), CameraDrag::Orbit);
  QVERIFY(picks(CameraHabit::PinglyMotion, left, none));
  QCOMPARE(gesture(CameraHabitSpec::pinglymotion, 1), QStringLiteral("Right-drag"));
  QCOMPARE(drag(CameraHabit::PinglyMotion, right, none), CameraDrag::Pan);
  QCOMPARE(gesture(CameraHabitSpec::pinglymotion, 4), QStringLiteral("F, or double-click it"));
  QVERIFY(Camera::isFocusKey(CameraHabit::PinglyMotion, Qt::Key_F, none));
  QVERIFY(Camera::focusesOnDoubleClick(CameraHabit::PinglyMotion));

  // Maya: "Alt + left-drag", "Alt + middle-drag", "Alt + right-drag, or wheel", "Click", "F"
  QCOMPARE(gesture(CameraHabitSpec::maya, 0), QStringLiteral("Alt + left-drag"));
  QCOMPARE(drag(CameraHabit::Maya, left, alt), CameraDrag::Orbit);
  QCOMPARE(gesture(CameraHabitSpec::maya, 1), QStringLiteral("Alt + middle-drag"));
  QCOMPARE(drag(CameraHabit::Maya, middle, alt), CameraDrag::Pan);
  QCOMPARE(gesture(CameraHabitSpec::maya, 2), QStringLiteral("Alt + right-drag, or wheel"));
  QCOMPARE(drag(CameraHabit::Maya, right, alt), CameraDrag::Zoom);
  QCOMPARE(gesture(CameraHabitSpec::maya, 3), QStringLiteral("Click"));
  QVERIFY(picks(CameraHabit::Maya, left, none));
  QCOMPARE(drag(CameraHabit::Maya, left, none), CameraDrag::None);
  QCOMPARE(gesture(CameraHabitSpec::maya, 4), QStringLiteral("F"));
  QVERIFY(Camera::isFocusKey(CameraHabit::Maya, Qt::Key_F, none));
  QVERIFY(!Camera::isFocusKey(CameraHabit::Maya, Qt::Key_Period, Qt::KeypadModifier));
  QVERIFY(!Camera::focusesOnDoubleClick(CameraHabit::Maya));

  // Blender: "Middle-drag", "Shift + middle-drag", "Ctrl + middle-drag, or wheel", "Click", "Numpad ."
  QCOMPARE(gesture(CameraHabitSpec::blender, 0), QStringLiteral("Middle-drag"));
  QCOMPARE(drag(CameraHabit::Blender, middle, none), CameraDrag::Orbit);
  QCOMPARE(gesture(CameraHabitSpec::blender, 1), QStringLiteral("Shift + middle-drag"));
  QCOMPARE(drag(CameraHabit::Blender, middle, shift), CameraDrag::Pan);
  QCOMPARE(gesture(CameraHabitSpec::blender, 2), QStringLiteral("Ctrl + middle-drag, or wheel"));
  QCOMPARE(drag(CameraHabit::Blender, middle, ctrl), CameraDrag::Zoom);
  QCOMPARE(gesture(CameraHabitSpec::blender, 3), QStringLiteral("Click"));
  QVERIFY(picks(CameraHabit::Blender, left, none));
  QCOMPARE(drag(CameraHabit::Blender, left, none), CameraDrag::None);
  QCOMPARE(gesture(CameraHabitSpec::blender, 4), QStringLiteral("Numpad ."));
  QVERIFY(Camera::isFocusKey(CameraHabit::Blender, Qt::Key_Period, Qt::KeypadModifier));

  // Second Life: "Alt + drag (sideways), Ctrl + Alt + drag", "Ctrl + Alt + Shift + drag",
  // "Alt + drag up or down, or wheel", "Click", "Alt + click"
  QCOMPARE(gesture(CameraHabitSpec::sl, 0), QStringLiteral("Alt + drag (sideways), Ctrl + Alt + drag"));
  QCOMPARE(drag(CameraHabit::SecondLife, left, alt), CameraDrag::OrbitAndZoom);   // sideways turns, up or down zooms
  QCOMPARE(drag(CameraHabit::SecondLife, left, ctrl | alt), CameraDrag::Orbit);
  QCOMPARE(gesture(CameraHabitSpec::sl, 1), QStringLiteral("Ctrl + Alt + Shift + drag"));
  QCOMPARE(drag(CameraHabit::SecondLife, left, ctrl | alt | shift), CameraDrag::Pan);
  QCOMPARE(gesture(CameraHabitSpec::sl, 2), QStringLiteral("Alt + drag up or down, or wheel"));
  QCOMPARE(gesture(CameraHabitSpec::sl, 3), QStringLiteral("Click"));
  QVERIFY(picks(CameraHabit::SecondLife, left, none));
  QCOMPARE(drag(CameraHabit::SecondLife, left, none), CameraDrag::None);
  QCOMPARE(gesture(CameraHabitSpec::sl, 4), QStringLiteral("Alt + click"));
  QVERIFY(Camera::focusesOnAltClick(CameraHabit::SecondLife));
  QVERIFY(!Camera::focusesOnDoubleClick(CameraHabit::SecondLife));
}

void TestCameraHabits::wheelZoomsInEveryHabit()
{
  // the view's wheelEvent zooms without asking the habit; every habit's zoom row names the wheel
  for (const CameraHabitSpec::Habit* habit : CameraHabitSpec::all)
    QVERIFY2(QString::fromLatin1(habit->rows[2].gesture).contains(QStringLiteral("wheel")), habit->id);
  const QString source = viewSource();
  const int begin = source.indexOf(QStringLiteral("void AnimationView::wheelEvent("));
  QVERIFY(begin > 0);
  const QString body = source.mid(begin, source.indexOf(QStringLiteral("\n}\n"), begin) - begin);
  QVERIFY(body.contains(QStringLiteral("camera()->zoom(")));
  QVERIFY(!body.contains(QStringLiteral("m_cameraHabit")));
}

// The mockup's KEYROWS_ALL rows, word for word and in its order: since UX phase 4b part 2 all of
// them (dragging a joint and the keyboard pull came with part 2)
void TestCameraHabits::sharedRowsMatchTheMockup()
{
  const QString mockup = read(QStringLiteral(PINGLYMOTION_MOCKUP));
  QVERIFY(!mockup.isEmpty());
  QCOMPARE(CameraHabitSpec::sharedRowCount, 9);
  QString all;
  for (int row = 0; row < CameraHabitSpec::sharedRowCount; ++row)
  {
    const QString action = QString::fromLatin1(CameraHabitSpec::sharedRows[row].action);
    const QString gesture = QString::fromLatin1(CameraHabitSpec::sharedRows[row].gesture);
    const QString pair = QStringLiteral("[\"%1\", \"%2\"]").arg(action, gesture);
    QVERIFY2(mockup.contains(pair), qPrintable(pair));
    all += (row ? QStringLiteral(", ") : QString()) + pair;
  }
  QCOMPARE(QString::fromLatin1(CameraHabitSpec::sharedRows[0].action), QStringLiteral("Pull a limb"));
  QCOMPARE(QString::fromLatin1(CameraHabitSpec::sharedRows[2].action), QStringLiteral("Turn on one axis"));
  QCOMPARE(QString::fromLatin1(CameraHabitSpec::sharedRows[4].gesture), QStringLiteral("V, O, M, Z"));
  // the whole list, in the mockup's order (its line breaks taken out)
  QString rows = mockup.mid(mockup.indexOf(QStringLiteral("const KEYROWS_ALL = [")) + 21);
  rows = rows.left(rows.indexOf(QStringLiteral("]];")) + 1).replace(QStringLiteral("\n  "), QStringLiteral(" "));
  QCOMPARE(rows, all);
}

void TestCameraHabits::chosenMessage()
{
  QCOMPARE(CameraHabitSpec::chosenMessage(CameraHabitSpec::maya), QStringLiteral("Camera and keys like Maya."));
  QCOMPARE(CameraHabitSpec::chosenMessage(CameraHabitSpec::sl), QStringLiteral("Camera and keys like Second Life."));
  QCOMPARE(CameraHabitSpec::chosenMessage(CameraHabitSpec::pinglymotion), QStringLiteral("Camera and keys like PinglyMotion."));
  // the mockup's feedback in setScheme()
  QVERIFY(read(QStringLiteral(PINGLYMOTION_MOCKUP)).contains(QStringLiteral("feedback(\"Camera and keys like \" + SCHEMES[id].name + \".\")")));
}

void TestCameraHabits::habitSurvivesRestart()
{
  for (const CameraHabitSpec::Habit* habit : CameraHabitSpec::all)
  {
    UserSettings::setCameraHabit(QLatin1String(habit->id));
    UserSettings::writeSettings();
    UserSettings::setCameraHabit(QStringLiteral("pinglymotion"));
    UserSettings::readSettings();
    QCOMPARE(UserSettings::cameraHabit(), QLatin1String(habit->id));
    QCOMPARE(Camera::habitFor(UserSettings::cameraHabit()), Camera::habitFor(QLatin1String(habit->id)));
  }
  // an unknown stored value gives the default: tests/usersettings; the camera then uses PinglyMotion
  QCOMPARE(Camera::habitFor(QStringLiteral("houdini")), CameraHabit::PinglyMotion);
}

void TestCameraHabits::viewFollowsTheHabit()
{
  const QString source = viewSource();
  QVERIFY(!source.isEmpty());
  // the press asks the habit (and, since UX phase 4b, the view tool), a left press that only
  // picks never turns into a camera drag
  QVERIFY(source.contains(QStringLiteral("Camera::gestureFor(m_cameraHabit,m_viewTool,event->button(),keys)")));
  QVERIFY(source.contains(QStringLiteral("if(m_cameraDrag==CameraDrag::None) return;\n    if((event->pos()-m_pressPos).manhattanLength()<4) return;")));
  // double click and Alt + click focus only in their habit
  QVERIFY(source.contains(QStringLiteral("Camera::gestureFor(m_cameraHabit,m_viewTool,event->button(),event->modifiers()).picks")));
  QVERIFY(source.contains(QStringLiteral("else if(Camera::focusesOnDoubleClick(m_cameraHabit))")));
  QVERIFY(source.contains(QStringLiteral("Camera::focusesOnAltClick(m_cameraHabit)")));
  // the keys: Alt + arrows, view keys, number pad 0, Shift+F, F / number pad "."
  QVERIFY(source.contains(QStringLiteral("Camera::turnForKey(event->key(),event->modifiers(),&up,&around)")));
  QVERIFY(source.contains(QStringLiteral("camera()->rotate(up,around);")));
  QVERIFY(source.contains(QStringLiteral("Camera::viewForKey(event->key(),event->modifiers(),&height,&angle)")));
  QVERIFY(source.contains(QStringLiteral("Camera::isHomeKey(event->key(),event->modifiers())")));
  QVERIFY(source.contains(QStringLiteral("Camera::isWholeFigureKey(event->key(),event->modifiers())")));
  QVERIFY(source.contains(QStringLiteral("Camera::isFocusKey(m_cameraHabit,event->key(),event->modifiers())")));
  QVERIFY(source.contains(QStringLiteral("camera()->frameBox(low,high,")));
  // the old mixed handling (every habit at once) is gone
  QVERIFY(!source.contains(QStringLiteral("Camera::dragFor(")));
}

void TestCameraHabits::windowPassesTheChoiceOn()
{
  const QString source = windowSource();
  QVERIFY(!source.isEmpty());
  // at start, and from both dialogs, through one method that also says so in the status line
  QVERIFY(source.contains(QStringLiteral("ui->animationView->setCameraHabit(Camera::habitFor(UserSettings::cameraHabit()));")));
  QVERIFY(source.contains(QStringLiteral("connect(&dialog,&SetupDialog::cameraHabitRequested,this,&PinglyMotionWindow::chooseCameraHabit);")));
  QVERIFY(source.contains(QStringLiteral("connect(&dialog,&CameraKeysDialog::cameraHabitRequested,this,&PinglyMotionWindow::chooseCameraHabit);")));
  const int begin = source.indexOf(QStringLiteral("void PinglyMotionWindow::chooseCameraHabit(const QString& habit)"));
  QVERIFY(begin > 0);
  const QString body = source.mid(begin, source.indexOf(QStringLiteral("\n}\n"), begin) - begin);
  QVERIFY(body.contains(QStringLiteral("UserSettings::setCameraHabit(habit);")));
  QVERIFY(body.contains(QStringLiteral("ui->animationView->setCameraHabit(")));
  QVERIFY(body.contains(QStringLiteral("CameraHabitSpec::chosenMessage(")));
}

QTEST_MAIN(TestCameraHabits)
#include "tst_camerahabits.moc"
