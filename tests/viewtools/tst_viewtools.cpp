/*
 * PinglyMotion - UX phase 4b part 1: the view toolbar, the view-direction widget, the view's
 * name and the rotation-ring hint (docs/UX_DECISIONS.md 6b; docs/mockup/index.html: the "nav"
 * with the gizmo, the view name and the "vtools" toolbar, GIZ, setTool(), the Show menu, the
 * hint bar's ring line)
 *
 * Exact answers: every text against the mockup (and where the program says something else on
 * purpose), the tools on the left mouse button in every habit and their keys, the view's name
 * for a camera turn, where the widget's ends are and what a click, a drag and the keys do, the
 * toolbar's buttons, menus, keyboard path and layouts, the choices after a restart, the look in
 * both themes at 100 to 200 percent, and the wiring in the 3D view and the window (their source,
 * since the 3D view needs OpenGL and the window the whole program).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QAction>
#include <QFile>
#include <QLabel>
#include <QMenu>
#include <QRegularExpression>
#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

#include "camera.h"
#include "lucideicons.h"
#include "textsize.h"
#include "theme.h"
#include "uirender.h"
#include "usersettings.h"
#include "viewgizmo.h"
#include "viewtoolbar.h"
#include "viewtoolspec.h"

Q_DECLARE_METATYPE(ViewTool)
Q_DECLARE_METATYPE(ViewToolSpec::End)
Q_DECLARE_METATYPE(ViewToolSpec::Part)

class TestViewTools : public QObject
{
  Q_OBJECT

  private slots:
    void initTestCase();
    void toolTextsMatchTheMockup();
    void framingAndMenusMatchTheMockup();
    void widgetTextsMatchTheMockup();
    void ringHintMatchesTheMockup();
    void toolsOnTheLeftButton();
    void toolKeys();
    void toolIds();
    void viewNames();
    void lookFromBelow();
    void widgetEnds();
    void widgetClickDragAndKeys();
    void toolbarButtons();
    void toolbarSignals();
    void toolbarKeyboardPath();
    void toolbarLayouts();
    void choicesSurviveRestart();
    void lookInBothThemes();
    void viewAndWindowWiring();
    void pullTextsMatchTheMockup();

  private:
    static QString read(const QString& path);
    static QString mockup() { return read(QStringLiteral(PINGLYMOTION_MOCKUP)); }
    QTemporaryDir m_dir;
};

QString TestViewTools::read(const QString& path)
{
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) return QString();
  return QString::fromUtf8(file.readAll());
}

void TestViewTools::initTestCase()
{
  QVERIFY(m_dir.isValid());
  QStandardPaths::setTestModeEnabled(true);
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_dir.path());
  QVERIFY(!mockup().isEmpty());
}

// The four tools: the mockup's data-name, data-key and data-tipline, the setTool() feedback and
// the hint bar line. Since part 2 the first tool is the mockup's "Pick and pull"; its tip leaves out
// "Drag empty space to look around.", which is true only in the PinglyMotion habit.
void TestViewTools::toolTextsMatchTheMockup()
{
  const QString page = mockup();
  QRegularExpression button(QStringLiteral("<button class=\"btn\" data-tool=\"(\\w+)\" data-name=\"([^\"]+)\" data-key=\"([^\"]+)\" data-tipline=\"([^\"]+)\">"));
  QStringList ids;
  for (auto it = button.globalMatch(page); it.hasNext();)
  {
    const QRegularExpressionMatch m = it.next();
    ids << m.captured(1);
    const ViewTool tool = Camera::toolFor(m.captured(1));
    QCOMPARE(QLatin1String(Camera::toolId(tool)), m.captured(1));
    const ViewToolSpec::Button& spec = ViewToolSpec::tool(tool);
    QCOMPARE(QLatin1String(spec.key), m.captured(3));
    if (tool == ViewTool::Pick)
    {
      // since part 2 (dragging a joint) the mockup's name; its tip without the last sentence,
      // which is true only in the PinglyMotion habit
      QCOMPARE(QLatin1String(spec.name), m.captured(2));
      QCOMPARE(QLatin1String(spec.label), m.captured(2));
      QCOMPARE(m.captured(4), QStringLiteral("Click a joint to pick it, drag it to pull the limb. Drag empty space to look around."));
      QCOMPARE(QLatin1String(spec.tipline), QLatin1String("Click a joint to pick it, drag it to pull the limb."));
      continue;
    }
    QCOMPARE(QLatin1String(spec.name), m.captured(2));
    QCOMPARE(QLatin1String(spec.label), m.captured(2));
    QCOMPARE(QLatin1String(spec.tipline), m.captured(4));
  }
  QCOMPARE(ids, QStringList({"pick", "orbit", "pan", "zoom"}));

  // setTool() feedback
  QVERIFY(page.contains(QStringLiteral("pick: \"Pick and pull: click a joint to pick it, drag it to pull the limb.\"")));
  QCOMPARE(ViewToolSpec::toolChosenMessage(ViewTool::Pick), QStringLiteral("Pick and pull: click a joint to pick it, drag it to pull the limb."));
  for (ViewTool tool : {ViewTool::Orbit, ViewTool::Pan, ViewTool::Zoom})
  {
    const QString text = ViewToolSpec::toolChosenMessage(tool);
    QVERIFY2(page.contains(QStringLiteral("%1: \"%2\"").arg(QLatin1String(Camera::toolId(tool)), text)), qPrintable(text));
    // the hint bar while hovering the view
    const QString hint = ViewToolSpec::toolHoverHint(tool);
    QVERIFY2(page.contains(QStringLiteral("%1: \"%2\"").arg(QLatin1String(Camera::toolId(tool)), hint)), qPrintable(hint));
  }
  QCOMPARE(ViewToolSpec::toolChosenMessage(ViewTool::Orbit), QStringLiteral("Look around: drag anywhere to turn the view."));
  QCOMPARE(ViewToolSpec::toolHoverHint(ViewTool::Zoom), QStringLiteral("Zoom: drag up or down. V goes back to picking joints."));
  QVERIFY(ViewToolSpec::toolHoverHint(ViewTool::Pick).isEmpty());

  // the icons are the mockup's and the program has them
  for (const ViewToolSpec::Button* spec : ViewToolSpec::tools)
    QVERIFY2(page.contains(QStringLiteral("data-tool=\"")) && LucideIcons::names().contains(QLatin1String(spec->icon)), spec->icon);
  QVERIFY(page.contains(QStringLiteral("data-tool=\"pick\" data-name=\"Pick and pull\" data-key=\"V\" data-tipline=\"Click a joint to pick it, drag it to pull the limb. Drag empty space to look around.\"><svg class=\"i\" data-i=\"mouse-pointer-2\">")));
  QVERIFY(page.contains(QStringLiteral("<svg class=\"i\" data-i=\"orbit\"></svg><span class=\"lbl\">Look around</span>")));
  QVERIFY(page.contains(QStringLiteral("<svg class=\"i\" data-i=\"hand\"></svg><span class=\"lbl\">Move the view</span>")));
  QVERIFY(page.contains(QStringLiteral("<svg class=\"i\" data-i=\"zoom-in\"></svg><span class=\"lbl\">Zoom</span>")));
}

void TestViewTools::framingAndMenusMatchTheMockup()
{
  const QString page = mockup();
  QRegularExpression framing(QStringLiteral("data-cmd=\"(focus|frameall|home)\" data-name=\"([^\"]+)\" data-key=\"([^\"]+)\"><svg class=\"i\" data-i=\"([^\"]+)\"></svg><span class=\"lbl\">([^<]+)</span>"));
  struct Expect { QString cmd; const ViewToolSpec::Button* spec; };
  QList<Expect> expected = {{QStringLiteral("focus"), &ViewToolSpec::focus}, {QStringLiteral("frameall"), &ViewToolSpec::wholeFigure},
                            {QStringLiteral("home"), &ViewToolSpec::home}};
  int found = 0;
  for (auto it = framing.globalMatch(page); it.hasNext();)
  {
    const QRegularExpressionMatch m = it.next();
    const Expect& e = expected.at(found++);
    QCOMPARE(m.captured(1), e.cmd);
    QCOMPARE(m.captured(2), QLatin1String(e.spec->name));
    QCOMPARE(m.captured(3), QLatin1String(e.spec->key));
    QCOMPARE(m.captured(4), QLatin1String(e.spec->icon));
    QCOMPARE(m.captured(5), QLatin1String(e.spec->label));
    QVERIFY(LucideIcons::names().contains(QLatin1String(e.spec->icon)));
  }
  QCOMPARE(found, 3);
  QVERIFY(page.contains(QStringLiteral("feedback(\"Home view: front, normal size.\")")));
  QCOMPARE(ViewToolSpec::tr(ViewToolSpec::homeDone), QStringLiteral("Home view: front, normal size."));
  QVERIFY(page.contains(QStringLiteral("feedback(\"The whole figure fits the view.\")")));
  QCOMPARE(ViewToolSpec::tr(ViewToolSpec::wholeFigureDone), QStringLiteral("The whole figure fits the view."));
  QVERIFY(page.contains(QStringLiteral("feedback(\"Camera turns around \" + fullName(sel) + \" now.\")")));
  QCOMPARE(ViewToolSpec::tr(ViewToolSpec::focusDone).arg(QStringLiteral("Right Hand")), QStringLiteral("Camera turns around Right Hand now."));
  QVERIFY(page.contains(QStringLiteral("say(\"Focus: Pick a joint first.\", \"reason\")")));
  QCOMPARE(ViewToolSpec::tr(ViewToolSpec::focusNoJoint), QStringLiteral("Focus: Pick a joint first."));

  // Look from
  QVERIFY(page.contains(QStringLiteral("data-name=\"Look from: front, side, top or back\" data-key=\"Numpad 1, 3, 7\"><svg class=\"i\" data-i=\"axis-3d\"></svg><span class=\"lbl\">Look from</span>")));
  QCOMPARE(QLatin1String(ViewToolSpec::lookFrom.name), QLatin1String("Look from: front, side, top or back"));
  QCOMPARE(QLatin1String(ViewToolSpec::lookFrom.key), QLatin1String("Numpad 1, 3, 7"));
  QCOMPARE(QLatin1String(ViewToolSpec::lookFrom.label), QLatin1String("Look from"));
  QRegularExpression look(QStringLiteral("<button role=\"menuitem\" data-cmd=\"(v\\w+)\"><span class=\"grow\">([^<]+)</span><kbd>([^<]+)</kbd></button>"));
  QStringList items;
  for (auto it = look.globalMatch(page); it.hasNext();)
  {
    const QRegularExpressionMatch m = it.next();
    items << m.captured(2) + QLatin1Char('|') + m.captured(3);
  }
  QStringList ours;
  for (const ViewToolSpec::Look& l : ViewToolSpec::looks) ours << QLatin1String(l.text) + QLatin1Char('|') + QLatin1String(l.key);
  QCOMPARE(items, ours);
  QCOMPARE(ours, QStringList({"Front|1", "Side|3", "Top|7", "Back|Ctrl+1"}));
  // each item gives the view its key gives in the 3D view (Camera::viewForKey())
  const int keys[] = {Qt::Key_1, Qt::Key_3, Qt::Key_7, Qt::Key_1};
  const Qt::KeyboardModifiers mods[] = {Qt::NoModifier, Qt::NoModifier, Qt::NoModifier, Qt::ControlModifier};
  for (int i = 0; i < ViewToolSpec::lookCount; i++)
  {
    float h = -1, a = -1;
    QVERIFY(Camera::viewForKey(keys[i], mods[i], &h, &a));
    QCOMPARE(h, ViewToolSpec::looks[i].height);
    QCOMPARE(a, ViewToolSpec::looks[i].angle);
  }

  // Show
  QVERIFY(page.contains(QStringLiteral("data-name=\"Show or hide parts of the 3D view\"><svg class=\"i\" data-i=\"eye\"></svg><span class=\"lbl\">Show</span>")));
  QCOMPARE(QLatin1String(ViewToolSpec::show.name), QLatin1String("Show or hide parts of the 3D view"));
  QRegularExpression show(QStringLiteral("data-show=\"(\\w+)\" aria-checked=\"true\"><span class=\"chk\"></span><span class=\"grow\">([^<]+)</span>"));
  QStringList shows;
  for (auto it = show.globalMatch(page); it.hasNext();) shows << it.next().captured(2);
  QStringList ourShows;
  for (ViewToolSpec::Part part : ViewToolSpec::parts) ourShows << ViewToolSpec::partName(part);
  QCOMPARE(shows, ourShows);
  QVERIFY(page.contains(QStringLiteral("(show[shw.dataset.show] ? \"Showing: \" : \"Hiding: \") + $(\".grow\", shw).textContent.toLowerCase() + \".\"")));
  QCOMPARE(ViewToolSpec::shownMessage(ViewToolSpec::Part::Floor, true), QStringLiteral("Showing: floor grid."));
  QCOMPARE(ViewToolSpec::shownMessage(ViewToolSpec::Part::Rings, false), QStringLiteral("Hiding: rotation rings."));
  QCOMPARE(ViewToolSpec::shownMessage(ViewToolSpec::Part::ToolNames, false), QStringLiteral("Hiding: tool names in this toolbar."));
  // the mockup shows every part at first; so does the program
  QVERIFY(page.contains(QStringLiteral("let show = Object.assign({ floor: true, rings: true, joints: true, names: true }")));
}

void TestViewTools::widgetTextsMatchTheMockup()
{
  const QString page = mockup();
  QVERIFY(page.contains(QStringLiteral("aria-label=\"%1\"").arg(ViewToolSpec::tr(ViewToolSpec::widgetName))));
  QVERIFY(page.contains(QStringLiteral("<title>%1</title>").arg(ViewToolSpec::tr(ViewToolSpec::widgetDrag))));
  QVERIFY(page.contains(QStringLiteral("aria-label=\"View tools\"")));
  QCOMPARE(ViewToolSpec::tr(ViewToolSpec::toolbarName), QStringLiteral("View tools"));

  // the six ends, in the mockup's order, with its names
  QRegularExpression giz(QStringLiteral("\\{ k: \"(\\w+)\", c: \"\\w\", v: \\[[^\\]]+\\], l: \"(\\w*)\", name: \"([^\"]+)\", cmd: \"(\\w+)\" \\}"));
  QStringList names;
  for (auto it = giz.globalMatch(page); it.hasNext();) names << it.next().captured(3);
  QStringList ours;
  for (const ViewToolSpec::EndSpec& e : ViewToolSpec::ends) ours << QString::fromUtf8(ViewToolSpec::tr(e.name).toUtf8());
  QCOMPARE(names, ours);
  QCOMPARE(QString::fromUtf8(ViewToolSpec::endSpec(ViewToolSpec::End::MinusY).letter), QStringLiteral("\u2212Y"));
  QCOMPARE(QString::fromUtf8(ViewToolSpec::endSpec(ViewToolSpec::End::PlusZ).letter), QStringLiteral("Z"));
  QVERIFY(page.contains(QStringLiteral("feedback(\"Looking from: \" + cam.label + \".\")")));
  QCOMPARE(ViewToolSpec::lookingFromMessage(ViewName::LeftSide), QStringLiteral("Looking from: Left side."));

  // the view names: the mockup's labels
  for (ViewName n : {ViewName::Front, ViewName::Back, ViewName::LeftSide, ViewName::RightSide, ViewName::Top, ViewName::Below})
    QVERIFY2(page.contains(QStringLiteral("\"%1\")").arg(ViewToolSpec::viewName(n))), qPrintable(ViewToolSpec::viewName(n)));
  QVERIFY(page.contains(QStringLiteral("cam.label = \"Free\"")));
  QCOMPARE(ViewToolSpec::viewName(ViewName::Free), QStringLiteral("Free"));
}

void TestViewTools::ringHintMatchesTheMockup()
{
  const QString page = mockup();
  QVERIFY(page.contains(QStringLiteral("`Drag the ${AXES[hotRing].ring} ${AXES[hotRing].l} ring to turn the joint: ${AXES[hotRing].what.toLowerCase()}.`")));
  QRegularExpression axes(QStringLiteral("\\{ l: \"(\\w)\", c: \"\\w\", what: \"([^\"]+)\", ring: \"(\\w+)\" \\}"));
  int axis = 0;
  for (auto it = axes.globalMatch(page); it.hasNext(); axis++)
  {
    const QRegularExpressionMatch m = it.next();
    const QString expected = QStringLiteral("Drag the %1 %2 ring to turn the joint: %3.").arg(m.captured(3), m.captured(1), m.captured(2).toLower());
    QCOMPARE(ViewToolSpec::ringHoverHint(axis), expected);
  }
  QCOMPARE(axis, 3);
  QCOMPARE(ViewToolSpec::ringHoverHint(0), QStringLiteral("Drag the red X ring to turn the joint: tilt forward or back."));
  QVERIFY(ViewToolSpec::ringHoverHint(3).isEmpty());
  QVERIFY(ViewToolSpec::ringHoverHint(-1).isEmpty());
}

// The mockup's gestureFor(): `if (b === 0 && tool !== "pick" && !alt && !ctrl) return tool;`
void TestViewTools::toolsOnTheLeftButton()
{
  QVERIFY(mockup().contains(QStringLiteral("if (b === 0 && tool !== \"pick\" && !alt && !ctrl) return tool;")));
  const Qt::KeyboardModifiers none = Qt::NoModifier, alt = Qt::AltModifier, ctrl = Qt::ControlModifier, shift = Qt::ShiftModifier;
  for (CameraHabit habit : {CameraHabit::PinglyMotion, CameraHabit::Maya, CameraHabit::Blender, CameraHabit::SecondLife})
  {
    // Pick: the habit decides, for every button and key
    for (Qt::MouseButton b : {Qt::LeftButton, Qt::MiddleButton, Qt::RightButton})
      for (Qt::KeyboardModifiers k : {none, alt, ctrl, shift, Qt::KeyboardModifiers(alt | ctrl)})
        QCOMPARE(Camera::gestureFor(habit, ViewTool::Pick, b, k), Camera::gestureFor(habit, b, k));
    // the other tools: a plain (or Shift) left drag moves the camera and picks nothing
    struct Case { ViewTool tool; CameraDrag drag; };
    for (const Case& c : {Case{ViewTool::Orbit, CameraDrag::Orbit}, Case{ViewTool::Pan, CameraDrag::Pan}, Case{ViewTool::Zoom, CameraDrag::Zoom}})
    {
      for (Qt::KeyboardModifiers k : {none, shift})
      {
        const CameraGesture g = Camera::gestureFor(habit, c.tool, Qt::LeftButton, k);
        QCOMPARE(g.drag, c.drag);
        QCOMPARE(g.picks, false);
      }
      // with Alt or Ctrl, and on the other buttons, the habit decides as before
      for (Qt::KeyboardModifiers k : {alt, ctrl, Qt::KeyboardModifiers(alt | ctrl)})
        QCOMPARE(Camera::gestureFor(habit, c.tool, Qt::LeftButton, k), Camera::gestureFor(habit, Qt::LeftButton, k));
      for (Qt::MouseButton b : {Qt::MiddleButton, Qt::RightButton})
        QCOMPARE(Camera::gestureFor(habit, c.tool, b, none), Camera::gestureFor(habit, b, none));
    }
  }
  // for example: the Maya habit with Move the view, Alt + left still turns
  QCOMPARE(Camera::gestureFor(CameraHabit::Maya, ViewTool::Pan, Qt::LeftButton, alt).drag, CameraDrag::Orbit);
}

void TestViewTools::toolKeys()
{
  QVERIFY(mockup().contains(QStringLiteral("if (k === \"v\" || k === \"V\") { setTool(\"pick\"); return; }")));
  QVERIFY(mockup().contains(QStringLiteral("if (k === \"z\" || k === \"Z\") { setTool(\"zoom\"); return; }")));
  struct Case { int key; ViewTool tool; };
  for (const Case& c : {Case{Qt::Key_V, ViewTool::Pick}, Case{Qt::Key_O, ViewTool::Orbit}, Case{Qt::Key_M, ViewTool::Pan}, Case{Qt::Key_Z, ViewTool::Zoom}})
  {
    for (Qt::KeyboardModifiers k : {Qt::KeyboardModifiers(Qt::NoModifier), Qt::KeyboardModifiers(Qt::ShiftModifier)})
    {
      ViewTool tool = ViewTool::Pick;
      QVERIFY(Camera::toolForKey(c.key, k, &tool));
      QCOMPARE(tool, c.tool);
    }
    // Ctrl+Z (Undo), Ctrl+V (Paste), Alt + ... are not view tools
    for (Qt::KeyboardModifier k : {Qt::ControlModifier, Qt::AltModifier, Qt::MetaModifier})
      QVERIFY(!Camera::toolForKey(c.key, k, nullptr));
  }
  QVERIFY(!Camera::toolForKey(Qt::Key_F, Qt::NoModifier, nullptr));
  QVERIFY(!Camera::toolForKey(Qt::Key_P, Qt::NoModifier, nullptr));
  // the key on each button is the key that chooses it
  for (ViewTool t : {ViewTool::Pick, ViewTool::Orbit, ViewTool::Pan, ViewTool::Zoom})
  {
    ViewTool tool = ViewTool::Pick;
    QVERIFY(Camera::toolForKey(QKeySequence(QLatin1String(ViewToolSpec::tool(t).key))[0].key(), Qt::NoModifier, &tool));
    QCOMPARE(tool, t);
  }
}

void TestViewTools::toolIds()
{
  QCOMPARE(UserSettings::viewTools(), QStringList({"pick", "orbit", "pan", "zoom"}));
  for (ViewTool t : {ViewTool::Pick, ViewTool::Orbit, ViewTool::Pan, ViewTool::Zoom})
    QCOMPARE(Camera::toolFor(QLatin1String(Camera::toolId(t))), t);
  QCOMPARE(Camera::toolFor(QStringLiteral("rotate")), ViewTool::Pick);
  QCOMPARE(Camera::toolFor(QString()), ViewTool::Pick);
  UserSettings::setViewTool(QStringLiteral("nonsense"));
  QCOMPARE(UserSettings::viewTool(), QStringLiteral("pick"));
  UserSettings::setViewTool(QStringLiteral("zoom"));
  QCOMPARE(UserSettings::viewTool(), QStringLiteral("zoom"));
  UserSettings::setViewTool(QStringLiteral("pick"));
}

void TestViewTools::viewNames()
{
  struct Case { float height; float angle; ViewName name; };
  for (const Case& c : {Case{0, 0, ViewName::Front}, Case{10, 0, ViewName::Front}, Case{-10, 0, ViewName::Front},
                        Case{10.5f, 0, ViewName::Front}, Case{11, 0, ViewName::Free}, Case{0, 0.5f, ViewName::Front},
                        Case{0, 359.6f, ViewName::Front}, Case{0, 360, ViewName::Front}, Case{0, -0.4f, ViewName::Front},
                        Case{0, 1, ViewName::Free}, Case{0, 90, ViewName::RightSide}, Case{0, 180, ViewName::Back},
                        Case{0, 270, ViewName::LeftSide}, Case{0, -90, ViewName::LeftSide}, Case{0, 45, ViewName::Free},
                        Case{90, 0, ViewName::Top}, Case{90, 123, ViewName::Top}, Case{89.5f, 0, ViewName::Top},
                        Case{80, 0, ViewName::Free}, Case{-90, 0, ViewName::Below}, Case{-89.5f, 30, ViewName::Below},
                        Case{-80, 0, ViewName::Free}})
  {
    const QByteArray what = QByteArray::number(c.height) + " " + QByteArray::number(c.angle);
    QVERIFY2(Camera::viewNameFor(c.height, c.angle) == c.name, what.constData());
  }
  // the camera's own views: home is Front, the keys' views their names
  Camera camera;
  camera.reset();
  QCOMPARE(Camera::viewNameFor(camera.xRotation(), camera.yRotation()), ViewName::Front);
  float h, a;
  QVERIFY(Camera::viewForKey(Qt::Key_3, Qt::NoModifier, &h, &a));
  camera.setView(h, a);
  QCOMPARE(Camera::viewNameFor(camera.xRotation(), camera.yRotation()), ViewName::RightSide);
  QVERIFY(Camera::viewForKey(Qt::Key_3, Qt::ControlModifier, &h, &a));
  camera.setView(h, a);
  QCOMPARE(Camera::viewNameFor(camera.xRotation(), camera.yRotation()), ViewName::LeftSide);
  QVERIFY(Camera::viewForKey(Qt::Key_7, Qt::NoModifier, &h, &a));
  camera.setView(h, a);
  QCOMPARE(Camera::viewNameFor(camera.xRotation(), camera.yRotation()), ViewName::Top);
  // turning with the keys leaves the named view
  camera.reset();
  camera.rotate(0, Camera::KeyTurnDegrees);
  QCOMPARE(Camera::viewNameFor(camera.xRotation(), camera.yRotation()), ViewName::Free);
}

void TestViewTools::lookFromBelow()
{
  Camera camera;
  camera.reset();
  const QVector3D pivot = camera.pivot();
  const float distance = camera.distance();
  camera.lookFrom(-90, 0);
  QCOMPARE(camera.xRotation(), -90.0f);
  QCOMPARE(camera.yRotation(), 0.0f);
  QVERIFY((camera.pivot() - pivot).length() < 1e-3f);
  QVERIFY(qAbs(camera.distance() - distance) < 1e-3f);
  QCOMPARE(Camera::viewNameFor(camera.xRotation(), camera.yRotation()), ViewName::Below);
  // never past straight down or up
  camera.lookFrom(-120, 0);
  QCOMPARE(camera.xRotation(), -90.0f);
  camera.lookFrom(120, 0);
  QCOMPARE(camera.xRotation(), 90.0f);
  // turning from below keeps the camera below until it is turned up
  camera.lookFrom(-90, 0);
  camera.rotate(0, 10);
  QCOMPARE(camera.xRotation(), -90.0f);
  // setView itself still never goes below the floor
  camera.setView(-90, 0);
  QCOMPARE(camera.xRotation(), 0.0f);

  // each end gives the view its name says
  struct Case { ViewToolSpec::End end; ViewName name; };
  for (const Case& c : {Case{ViewToolSpec::End::PlusX, ViewName::LeftSide}, Case{ViewToolSpec::End::MinusX, ViewName::RightSide},
                        Case{ViewToolSpec::End::PlusY, ViewName::Top}, Case{ViewToolSpec::End::MinusY, ViewName::Below},
                        Case{ViewToolSpec::End::PlusZ, ViewName::Front}, Case{ViewToolSpec::End::MinusZ, ViewName::Back}})
  {
    const ViewToolSpec::EndSpec& spec = ViewToolSpec::endSpec(c.end);
    camera.lookFrom(spec.height, spec.angle);
    QCOMPARE(Camera::viewNameFor(camera.xRotation(), camera.yRotation()), c.name);
    // and the end it names then points straight at the viewer
    const QVector3D d = ViewGizmo::direction(c.end, camera.xRotation(), camera.yRotation());
    QVERIFY2(d.z() > 0.999f, ViewToolSpec::endSpec(c.end).letter);
  }
}

void TestViewTools::widgetEnds()
{
  auto near = [](const QVector3D& a, const QVector3D& b) { return (a - b).length() < 1e-5f; };
  using E = ViewToolSpec::End;
  // front: X to the right, Y up, Z towards the viewer (the figure's left is on the viewer's right)
  QVERIFY(near(ViewGizmo::direction(E::PlusX, 0, 0), QVector3D(1, 0, 0)));
  QVERIFY(near(ViewGizmo::direction(E::PlusY, 0, 0), QVector3D(0, 1, 0)));
  QVERIFY(near(ViewGizmo::direction(E::PlusZ, 0, 0), QVector3D(0, 0, 1)));
  QVERIFY(near(ViewGizmo::direction(E::MinusZ, 0, 0), QVector3D(0, 0, -1)));
  // the Side view (3, angle 90): -X towards the viewer, +Z to the right
  QVERIFY(near(ViewGizmo::direction(E::MinusX, 0, 90), QVector3D(0, 0, 1)));
  QVERIFY(near(ViewGizmo::direction(E::PlusZ, 0, 90), QVector3D(1, 0, 0)));
  // top: +Y towards the viewer, +Z down the screen
  QVERIFY(near(ViewGizmo::direction(E::PlusY, 90, 0), QVector3D(0, 0, 1)));
  QVERIFY(near(ViewGizmo::direction(E::PlusZ, 90, 0), QVector3D(0, -1, 0)));

  // on screen: the widget is 84 pixels, the ends 34 of 96 from the middle
  ViewGizmo gizmo;
  gizmo.resize(96, 96);
  gizmo.setCameraAngles(0, 0);
  QCOMPARE(gizmo.endPosition(E::PlusX), QPointF(48 + 34, 48));
  QCOMPARE(gizmo.endPosition(E::MinusX), QPointF(48 - 34, 48));
  QCOMPARE(gizmo.endPosition(E::PlusY), QPointF(48, 48 - 34));
  QCOMPARE(gizmo.endRadius(E::PlusX), 9.0);
  QCOMPARE(gizmo.endRadius(E::MinusX), 7.0);
  // +Z and -Z lie on the middle: the one nearer the viewer wins
  E end;
  QVERIFY(gizmo.endAt(QPointF(48, 48), &end));
  QCOMPARE(end, E::PlusZ);
  QVERIFY(gizmo.endAt(QPointF(48 + 34 + 10, 48), &end));   // 9 + 2 around the end
  QCOMPARE(end, E::PlusX);
  QVERIFY(!gizmo.endAt(QPointF(48 + 34 + 12, 48), nullptr));
  QVERIFY(!gizmo.endAt(QPointF(70, 70), nullptr));
  gizmo.setCameraAngles(0, 180);
  QVERIFY(gizmo.endAt(QPointF(48, 48), &end));
  QCOMPARE(end, E::MinusZ);
  // it grows with the text size
  TextSize::resetForTests();
  QCOMPARE(gizmo.sizeHint(), QSize(84, 84));
}

void TestViewTools::widgetClickDragAndKeys()
{
  using E = ViewToolSpec::End;
  ViewGizmo gizmo;
  gizmo.resize(96, 96);
  gizmo.setCameraAngles(10, 0);
  gizmo.show();
  QVERIFY(QTest::qWaitForWindowExposed(&gizmo));
  QSignalSpy chosen(&gizmo, &ViewGizmo::endChosen);
  QSignalSpy dragged(&gizmo, &ViewGizmo::dragged);

  // a click on an end chooses it
  QTest::mouseClick(&gizmo, Qt::LeftButton, Qt::NoModifier, gizmo.endPosition(E::PlusX).toPoint());
  QCOMPARE(chosen.count(), 1);
  QCOMPARE(chosen.takeFirst().at(0).value<E>(), E::PlusX);
  QCOMPARE(gizmo.currentEnd(), E::PlusX);
  QTest::mouseClick(&gizmo, Qt::LeftButton, Qt::NoModifier, gizmo.endPosition(E::MinusY).toPoint());
  QCOMPARE(chosen.takeFirst().at(0).value<E>(), E::MinusY);
  QCOMPARE(dragged.count(), 0);

  // a drag on the background turns, by the pixels moved, and chooses nothing
  const QPoint start(20, 70);
  QVERIFY(!gizmo.endAt(start, nullptr));
  QTest::mousePress(&gizmo, Qt::LeftButton, Qt::NoModifier, start);
  QMouseEvent move(QEvent::MouseMove, QPointF(start + QPoint(6, -4)), gizmo.mapToGlobal(QPointF(start + QPoint(6, -4))),
                   Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
  QCoreApplication::sendEvent(&gizmo, &move);
  QTest::mouseRelease(&gizmo, Qt::LeftButton, Qt::NoModifier, start + QPoint(6, -4));
  QCOMPARE(dragged.count(), 1);
  QCOMPARE(dragged.at(0).at(0).toInt(), 6);
  QCOMPARE(dragged.at(0).at(1).toInt(), -4);
  QCOMPARE(chosen.count(), 0);

  // keys: the arrows go through the six ends in the mockup's order, Enter and Space choose
  gizmo.setCurrentEnd(E::PlusZ);
  gizmo.setFocus();
  QTest::keyClick(&gizmo, Qt::Key_Right);
  QCOMPARE(gizmo.currentEnd(), E::MinusZ);
  QTest::keyClick(&gizmo, Qt::Key_Down);
  QCOMPARE(gizmo.currentEnd(), E::PlusX);
  QTest::keyClick(&gizmo, Qt::Key_Left);
  QCOMPARE(gizmo.currentEnd(), E::MinusZ);
  QTest::keyClick(&gizmo, Qt::Key_Up);
  QCOMPARE(gizmo.currentEnd(), E::PlusZ);
  QCOMPARE(chosen.count(), 0);
  QTest::keyClick(&gizmo, Qt::Key_Return);
  QCOMPARE(chosen.takeFirst().at(0).value<E>(), E::PlusZ);
  QTest::keyClick(&gizmo, Qt::Key_Right);
  QTest::keyClick(&gizmo, Qt::Key_Space);
  QCOMPARE(chosen.takeFirst().at(0).value<E>(), E::MinusZ);
  QTest::keyClick(&gizmo, Qt::Key_Enter);
  QCOMPARE(chosen.takeFirst().at(0).value<E>(), E::MinusZ);
  // the name for screen readers, and the end the keyboard is on
  QCOMPARE(gizmo.accessibleName(), QStringLiteral("View direction: click an axis end to look from there"));
  QCOMPARE(gizmo.accessibleDescription(), QString::fromUtf8("Look from the back (\u2212Z)"));
  QCOMPARE(gizmo.focusPolicy(), Qt::StrongFocus);
}

void TestViewTools::toolbarButtons()
{
  QWidget view;
  view.resize(900, 700);
  ViewToolbar bar(&view);
  view.show();
  QVERIFY(QTest::qWaitForWindowExposed(&view));

  QStringList ids, labels, keys, tips;
  for (ViewToolButton* b : bar.buttons())
  {
    ids << b->objectName();
    labels << b->label();
    keys << b->key();
    tips << b->toolTip();
  }
  QCOMPARE(ids, QStringList({"viewToolPick", "viewToolOrbit", "viewToolPan", "viewToolZoom", "viewFocus", "viewWholeFigure",
                             "viewHome", "viewLookFrom", "viewShow"}));
  QCOMPARE(labels, QStringList({"Pick and pull", "Look around", "Move the view", "Zoom", "Picked joint", "Whole figure", "Home view",
                                "Look from", "Show"}));
  QCOMPARE(keys, QStringList({"V", "O", "M", "Z", "F", "Shift+F", "0", "Numpad 1, 3, 7", ""}));
  QCOMPARE(tips.at(0), QStringLiteral("Pick and pull (V)\nClick a joint to pick it, drag it to pull the limb."));
  QCOMPARE(tips.at(1), QStringLiteral("Look around (O)\nDrag anywhere to turn the view around the figure."));
  QCOMPARE(tips.at(3), QStringLiteral("Zoom (Z)\nDrag up to come closer, down to go back. The mouse wheel zooms too."));
  QCOMPARE(tips.at(5), QStringLiteral("Frame the whole figure (Shift+F)"));
  QCOMPARE(tips.at(6), QStringLiteral("Home view: front, normal size (0)"));
  QCOMPARE(tips.at(7), QStringLiteral("Look from: front, side, top or back (Numpad 1, 3, 7)"));
  QCOMPARE(tips.at(8), QStringLiteral("Show or hide parts of the 3D view"));
  // every button has a name for screen readers (charter P3)
  for (ViewToolButton* b : bar.buttons()) QVERIFY(!b->accessibleName().isEmpty());
  QCOMPARE(bar.button("viewFocus")->accessibleName(), QStringLiteral("Focus on the picked joint"));

  // exactly one tool is on, and it is the toolbar's tool
  for (ViewTool t : {ViewTool::Orbit, ViewTool::Zoom, ViewTool::Pick, ViewTool::Pan})
  {
    bar.setTool(t);
    QCOMPARE(bar.tool(), t);
    int on = 0;
    for (ViewTool u : {ViewTool::Pick, ViewTool::Orbit, ViewTool::Pan, ViewTool::Zoom})
    {
      QVERIFY(bar.toolButton(u)->isCheckable());
      if (bar.toolButton(u)->isChecked()) { on++; QCOMPARE(u, t); }
    }
    QCOMPARE(on, 1);
  }
  QVERIFY(!bar.button("viewHome")->isCheckable());

  // the menus
  QStringList looks;
  for (QAction* a : bar.lookMenu()->actions()) looks << a->text();
  QCOMPARE(looks, QStringList({"Front\t1", "Side\t3", "Top\t7", "Back\tCtrl+1"}));
  QStringList shows;
  for (QAction* a : bar.showMenu()->actions()) { shows << a->text(); QVERIFY(a->isCheckable()); }
  QCOMPARE(shows, QStringList({"Floor grid", "Rotation rings", "Joint dots", "Tool names in this toolbar"}));
  QCOMPARE(bar.button("viewLookFrom")->menu(), bar.lookMenu());
  QCOMPARE(bar.button("viewShow")->menu(), bar.showMenu());

  // the view's name follows the camera
  bar.setCameraAngles(10, 0);
  QCOMPARE(bar.viewNameText(), QStringLiteral("Front"));
  QCOMPARE(bar.viewNameLabel()->accessibleName(), QStringLiteral("Front"));
  bar.setCameraAngles(0, 90);
  QCOMPARE(bar.viewNameText(), QStringLiteral("Right side"));
  QCOMPARE(bar.gizmo()->cameraAngle(), 90.0f);
  bar.setCameraAngles(35, 20);
  QCOMPARE(bar.viewNameText(), QStringLiteral("Free"));
  bar.setCameraAngles(-90, 0);
  QCOMPARE(bar.viewNameText(), QStringLiteral("Below"));

  // it sits at the top left of the view, 12 pixels in (spacing.m)
  QCOMPARE(bar.pos(), QPoint(Theme::Size::spacingM, Theme::Size::spacingM));
  // the widget is above the name, the name above the buttons
  auto top = [&bar](QWidget* w) { return w->mapTo(&bar, QPoint(0, 0)).y(); };
  auto bottom = [&bar](QWidget* w) { return w->mapTo(&bar, QPoint(0, w->height() - 1)).y(); };
  QVERIFY(bottom(bar.gizmo()) < top(bar.viewNameLabel()));
  QVERIFY(bottom(bar.viewNameLabel()) < top(bar.toolButton(ViewTool::Pick)));
}

void TestViewTools::toolbarSignals()
{
  QWidget view;
  view.resize(900, 700);
  ViewToolbar bar(&view);
  view.show();
  QVERIFY(QTest::qWaitForWindowExposed(&view));

  // a tool button asks; the tool changes only when the window sets it
  QSignalSpy tool(&bar, &ViewToolbar::toolChosen);
  bar.toolButton(ViewTool::Zoom)->click();
  QCOMPARE(tool.count(), 1);
  QCOMPARE(tool.takeFirst().at(0).value<ViewTool>(), ViewTool::Zoom);
  QCOMPARE(bar.tool(), ViewTool::Pick);
  QVERIFY(bar.toolButton(ViewTool::Pick)->isChecked());
  QVERIFY(!bar.toolButton(ViewTool::Zoom)->isChecked());

  QSignalSpy focus(&bar, &ViewToolbar::focusRequested);
  QSignalSpy whole(&bar, &ViewToolbar::wholeFigureRequested);
  QSignalSpy home(&bar, &ViewToolbar::homeRequested);
  bar.button("viewFocus")->click();
  bar.button("viewWholeFigure")->click();
  bar.button("viewHome")->click();
  QCOMPARE(focus.count(), 1);
  QCOMPARE(whole.count(), 1);
  QCOMPARE(home.count(), 1);

  QSignalSpy look(&bar, &ViewToolbar::lookRequested);
  for (QAction* a : bar.lookMenu()->actions()) a->trigger();
  QCOMPARE(look.count(), 4);
  for (int i = 0; i < 4; i++) QCOMPARE(look.at(i).at(0).toInt(), i);

  // a Show item asks; it stays as it was until the window sets it
  QSignalSpy part(&bar, &ViewToolbar::partToggled);
  QAction* rings = bar.showAction(ViewToolSpec::Part::Rings);
  QVERIFY(rings->isChecked());
  rings->trigger();
  QCOMPARE(part.count(), 1);
  QCOMPARE(part.at(0).at(0).value<ViewToolSpec::Part>(), ViewToolSpec::Part::Rings);
  QCOMPARE(part.at(0).at(1).toBool(), false);
  QVERIFY(rings->isChecked());
  bar.setPartShown(ViewToolSpec::Part::Rings, false);
  QVERIFY(!bar.partShown(ViewToolSpec::Part::Rings));
  rings->trigger();
  QCOMPARE(part.at(1).at(1).toBool(), true);

  // the widget's signals come through
  QSignalSpy end(&bar, &ViewToolbar::endChosen);
  QSignalSpy drag(&bar, &ViewToolbar::dragged);
  emit bar.gizmo()->endChosen(ViewToolSpec::End::MinusZ);
  emit bar.gizmo()->dragged(3, 4);
  QCOMPARE(end.count(), 1);
  QCOMPARE(end.at(0).at(0).value<ViewToolSpec::End>(), ViewToolSpec::End::MinusZ);
  QCOMPARE(drag.count(), 1);
}

// Tab goes from the widget through the buttons in their order (charter P8; the mockup's toolbar
// order); the widget is one stop
void TestViewTools::toolbarKeyboardPath()
{
  QWidget view;
  view.setFocusPolicy(Qt::StrongFocus);
  view.resize(900, 700);
  ViewToolbar bar(&view);
  view.show();
  QVERIFY(QTest::qWaitForWindowExposed(&view));
  view.activateWindow();
  bar.gizmo()->setFocus(Qt::TabFocusReason);
  QStringList order;
  QWidget* w = bar.gizmo();
  for (int i = 0; i < 10; i++)
  {
    order << w->objectName();
    w = w->nextInFocusChain();
    while (w && !(w->focusPolicy() & Qt::TabFocus)) w = w->nextInFocusChain();
    if (!w || w == bar.gizmo()) break;
  }
  QCOMPARE(order.mid(0, 10), QStringList({"viewGizmo", "viewToolPick", "viewToolOrbit", "viewToolPan", "viewToolZoom", "viewFocus",
                                          "viewWholeFigure", "viewHome", "viewLookFrom", "viewShow"}));
  for (ViewToolButton* b : bar.buttons()) QVERIFY(b->focusPolicy() & Qt::TabFocus);
}

void TestViewTools::toolbarLayouts()
{
  QWidget view;
  view.resize(900, 700);
  ViewToolbar bar(&view);
  view.show();
  QVERIFY(QTest::qWaitForWindowExposed(&view));

  // a tall view: one column with names, all buttons as wide
  QVERIFY(!bar.twoColumns());
  for (ViewToolButton* b : bar.buttons()) QVERIFY(b->showsLabel());
  const int width = bar.toolButton(ViewTool::Pick)->width();
  for (ViewToolButton* b : bar.buttons()) QCOMPARE(b->width(), width);
  QVERIFY(bar.geometry().bottom() <= view.height() - Theme::Size::spacingM);

  // names switched off: icons only, square buttons
  bar.setPartShown(ViewToolSpec::Part::ToolNames, false);
  for (ViewToolButton* b : bar.buttons())
  {
    QVERIFY(!b->showsLabel());
    QCOMPARE(b->sizeHint().width(), b->sizeHint().height());
  }
  QVERIFY(!bar.twoColumns());
  bar.setPartShown(ViewToolSpec::Part::ToolNames, true);
  QVERIFY(bar.toolButton(ViewTool::Pick)->showsLabel());

  // a view too low for one column: two columns of icons
  const int named = bar.sizeHint().height();
  view.resize(900, named + 2 * Theme::Size::spacingM);
  QCoreApplication::processEvents();
  QVERIFY(!bar.twoColumns());
  QVERIFY(bar.toolButton(ViewTool::Pick)->showsLabel());
  view.resize(900, named + 2 * Theme::Size::spacingM - 1);
  QCoreApplication::processEvents();
  QVERIFY(bar.twoColumns());
  QVERIFY(!bar.toolButton(ViewTool::Pick)->showsLabel());
  QVERIFY(bar.sizeHint().height() < named);
  QVERIFY(bar.geometry().bottom() < view.height());
  // two by two: Pick beside Look around, Move the view beside Zoom
  QCOMPARE(bar.toolButton(ViewTool::Pick)->y(), bar.toolButton(ViewTool::Orbit)->y());
  QVERIFY(bar.toolButton(ViewTool::Orbit)->x() > bar.toolButton(ViewTool::Pick)->x());
  QVERIFY(bar.toolButton(ViewTool::Pan)->y() > bar.toolButton(ViewTool::Pick)->y());
  // back to a tall view: names again
  view.resize(900, 800);
  QCoreApplication::processEvents();
  QVERIFY(!bar.twoColumns());
  QVERIFY(bar.toolButton(ViewTool::Pick)->showsLabel());
}

void TestViewTools::choicesSurviveRestart()
{
  UserSettings::setViewTool(QStringLiteral("pan"));
  UserSettings::setShowFloor(false);
  UserSettings::setShowRings(false);
  UserSettings::setViewToolNames(false);
  UserSettings::writeSettings();
  UserSettings::setViewTool(QStringLiteral("pick"));
  UserSettings::setShowFloor(true);
  UserSettings::setShowRings(true);
  UserSettings::setViewToolNames(true);
  UserSettings::readSettings();
  QCOMPARE(UserSettings::viewTool(), QStringLiteral("pan"));
  QCOMPARE(Camera::toolFor(UserSettings::viewTool()), ViewTool::Pan);
  QCOMPARE(UserSettings::showFloor(), false);
  QCOMPARE(UserSettings::showRings(), false);
  QCOMPARE(UserSettings::viewToolNames(), false);

  // and back
  UserSettings::setViewTool(QStringLiteral("orbit"));
  UserSettings::setShowFloor(true);
  UserSettings::setShowRings(true);
  UserSettings::setViewToolNames(true);
  UserSettings::writeSettings();
  UserSettings::readSettings();
  QCOMPARE(UserSettings::viewTool(), QStringLiteral("orbit"));
  QVERIFY(UserSettings::showFloor());
  QVERIFY(UserSettings::showRings());
  QVERIFY(UserSettings::viewToolNames());
  UserSettings::setViewTool(QStringLiteral("pick"));
  UserSettings::writeSettings();
}

void TestViewTools::lookInBothThemes()
{
  for (const UiRender::Variant& variant : UiRender::variants())
  {
    UiRender::Scope scope(variant);
    QWidget view;
    view.setMinimumSize(900, 900);
    ViewToolbar bar(&view);
    bar.setTool(ViewTool::Orbit);
    bar.setCameraAngles(10, 0);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    const QImage image = UiRender::render(&view);
    UiRender::save(image, QStringLiteral("viewtoolbar"), variant);
    const QStringList clipped = UiRender::clipping(&bar);
    QVERIFY2(clipped.isEmpty(), qPrintable(variant.name() + QLatin1String(": ") + clipped.join(QLatin1Char(' '))));
    // the widget grows with the text size
    QCOMPARE(bar.gizmo()->width(), TextSize::scaled(ViewGizmo::BaseSize));
    // the active tool: an accent_text bar on its left, and selection mixed 22 percent into surface
    ViewToolButton* on = bar.toolButton(ViewTool::Orbit);
    const int ring = Theme::Size::focusRingOffset + Theme::Size::focusRingWidth;
    const QString mark = UiRender::expectColour(image, &view, on, QPoint(1, on->height() / 2), Theme::color(Theme::Token::AccentText),
                                                QStringLiteral("active tool bar"), 8);
    QVERIFY2(mark.isEmpty(), qPrintable(variant.name() + QLatin1String(": ") + mark));
    const QColor fill = Theme::mix(Theme::color(Theme::Token::Surface), Theme::color(Theme::Token::Selection), 0.22);
    const QString filled = UiRender::expectColour(image, &view, on, QPoint(ring + 3, on->height() / 2), fill,
                                                  QStringLiteral("active tool fill"), 8);
    QVERIFY2(filled.isEmpty(), qPrintable(variant.name() + QLatin1String(": ") + filled));
    // another tool has no fill of its own: the panel's surface shows
    ViewToolButton* off = bar.toolButton(ViewTool::Pan);
    const QString plain = UiRender::expectColour(image, &view, off, QPoint(ring + 3, off->height() / 2), Theme::color(Theme::Token::Surface),
                                                 QStringLiteral("tool that is off"), 8);
    QVERIFY2(plain.isEmpty(), qPrintable(variant.name() + QLatin1String(": ") + plain));
    // the +Z end at the middle of the widget (front view) is the axis_z colour
    // (beside the letter, inside the disc)
    const double radius = bar.gizmo()->endRadius(ViewToolSpec::End::PlusZ);
    const QPoint z = bar.gizmo()->endPosition(ViewToolSpec::End::PlusZ).toPoint() + QPoint(-int(radius * 0.75), 0);
    const QString zColour = UiRender::expectColour(image, &view, bar.gizmo(), z, Theme::color(Theme::Token::AxisZ),
                                                   QStringLiteral("+Z end"), 40);
    QVERIFY2(zColour.isEmpty(), qPrintable(variant.name() + QLatin1String(": ") + zColour));
  }
}

void TestViewTools::viewAndWindowWiring()
{
  const QString view = read(QStringLiteral(PINGLYMOTION_SOURCE_DIR "/animationview.cpp"));
  const QString window = read(QStringLiteral(PINGLYMOTION_SOURCE_DIR "/pinglymotionwindow.cpp"));
  QVERIFY(!view.isEmpty());
  QVERIFY(!window.isEmpty());
  // the 3D view: the tool decides a press, the keys ask for a tool, Show hides rings and floor,
  // the hint follows the ring under the mouse
  QVERIFY(view.contains(QStringLiteral("Camera::gestureFor(m_cameraHabit,m_viewTool,event->button(),keys)")));
  QVERIFY(view.contains(QStringLiteral("Camera::gestureFor(m_cameraHabit,m_viewTool,event->button(),event->modifiers())")));
  QVERIFY(!view.contains(QStringLiteral("Camera::gestureFor(m_cameraHabit,event->button()")));
  QVERIFY(view.contains(QStringLiteral("Camera::toolForKey(event->key(),event->modifiers(),&tool)")));
  QVERIFY(view.contains(QStringLiteral("emit viewToolRequested(tool);")));
  QVERIFY(view.contains(QStringLiteral("partSelected==selectName && m_showRings")));
  QVERIFY(view.contains(QStringLiteral("if(m_showFloor)")));
  QVERIFY(view.contains(QStringLiteral("ViewToolSpec::ringHoverHint(ring)")));
  QVERIFY(view.contains(QStringLiteral("ViewToolSpec::toolHoverHint(m_viewTool)")));
  QVERIFY(view.contains(QStringLiteral("QMetaObject::invokeMethod(this,&AnimationView::placeRingLabels,Qt::QueuedConnection);\n    emit viewDrawn();")));
  // the window: one path for the toolbar and the keys, the saved choices at start
  QVERIFY(window.contains(QStringLiteral("connect(m_viewToolbar,&ViewToolbar::toolChosen,this,&PinglyMotionWindow::chooseViewTool);")));
  QVERIFY(window.contains(QStringLiteral("connect(view,&AnimationView::viewToolRequested,this,&PinglyMotionWindow::chooseViewTool);")));
  QVERIFY(window.contains(QStringLiteral("const ViewTool tool=Camera::toolFor(UserSettings::viewTool());")));
  QVERIFY(window.contains(QStringLiteral("UserSettings::setViewTool(QLatin1String(Camera::toolId(tool)));")));
  QVERIFY(window.contains(QStringLiteral("statusBar()->showMessage(ViewToolSpec::toolChosenMessage(tool),5000);")));
  QVERIFY(window.contains(QStringLiteral("view->camera()->lookFrom(spec.height,spec.angle);")));
  QVERIFY(window.contains(QStringLiteral("m_viewToolbar->setCameraAngles(view->camera()->xRotation(),view->camera()->yRotation());")));
  QVERIFY(window.contains(QStringLiteral("ui->optionsSkeletonAction->setChecked(shown);")));
  QVERIFY(window.contains(QStringLiteral("statusBar()->showMessage(ViewToolSpec::shownMessage(part,shown),5000);")));
  QVERIFY(window.contains(QStringLiteral("if(m_viewToolbar) m_viewToolbar->setVisible(!empty);")));
  QVERIFY(window.contains(QStringLiteral("if(m_viewToolbar) m_viewToolbar->applyLook();")));
  QVERIFY(window.contains(QStringLiteral("connect(view,&AnimationView::hoverHintChanged,this,&PinglyMotionWindow::showHoverHint);")));
  QVERIFY(window.contains(QStringLiteral("if(hint.isEmpty() || !UserSettings::tipsOn())")));

  // UX phase 4b part 2, dragging a joint: a press on a joint picks it, a drag of 5 pixels pulls it
  // the drag choice's way (Shift: the other one); release sets the key, Esc cancels; Enter or G
  // picks it up for the arrow keys (12 pixels, Shift 3), Enter sets the key
  QVERIFY(view.contains(QStringLiteral("if((event->pos()-m_pull.pressPos).manhattanLength()<5) return;")));
  QVERIFY(view.contains(QStringLiteral("if(selected!=partSelected) clickSelect(selected);")));
  QVERIFY(view.contains(QStringLiteral("const bool turnOnly=UserSettings::pullLimbOnDrag()==m_pull.shiftAtPress;")));
  QVERIFY(view.contains(QStringLiteral("if(m_pull.active && !m_pull.viaKeys) finishPull();")));
  QVERIFY(view.contains(QStringLiteral("if(key==Qt::Key_Escape)\n    {\n      cancelPull();")));
  QVERIFY(view.contains(QStringLiteral("if(key==Qt::Key_Return || key==Qt::Key_Enter || key==Qt::Key_Space)\n      {\n        finishPull();")));
  QVERIFY(view.contains(QStringLiteral("startKeyPull(event->key()!=Qt::Key_G && (event->modifiers() & Qt::ShiftModifier));")));
  QVERIFY(view.contains(QStringLiteral("startPull(partSelected,UserSettings::pullLimbOnDrag()==otherWay,true);")));
  QVERIFY(view.contains(QStringLiteral("LimbPull::solve(m_pull.pull.chain,fromScreen(m_pull.target,m_pull.depth),true);")));
  QVERIFY(view.contains(QStringLiteral("emit pullFinished(pull.pulled,LimbPull::drivenTurns(pull),pull.chain.turnOnly);")));
  QVERIFY(view.contains(QStringLiteral("if(event->type()==QEvent::ShortcutOverride && m_pull.active)")));
  QVERIFY(view.contains(QStringLiteral("setCursor(QCursor(Qt::ClosedHandCursor));")));
  QVERIFY(view.contains(QStringLiteral("ViewToolSpec::tr(ViewToolSpec::jointHover)")));
  // the picture shows the pull's turns before release, and the rings their letters and faint back half
  QVERIFY(view.contains(QStringLiteral("QVector3D rotation = previewed!=m_pullPreview.constEnd() ? previewed.value() : motion->frameData(frame).rotation();")));
  QVERIFY(view.contains(QStringLiteral("back=depth>centreDepth;")));
  QVERIFY(view.contains(QStringLiteral("static const float dirs[3][2]={{1.0f,-0.35f},{-1.0f,-0.35f},{0.25f,1.0f}};")));
  // the window: the same reasons as Set key, the keys set at release, the Selected joint panel's button
  QVERIFY(window.contains(QStringLiteral("return TimelineSpec::setKeyBlocked(!m_empty,true,playstate!=PLAYSTATE_STOPPED,protect);")));
  QVERIFY(window.contains(QStringLiteral("connect(view,&AnimationView::pullFinished,this,&PinglyMotionWindow::pullFinished);")));
  QVERIFY(window.contains(QStringLiteral("anim->setRotation(turn.first,turn.second.x(),turn.second.y(),turn.second.z());")));
  QVERIFY(window.contains(QStringLiteral("ViewToolSpec::pullDoneMessage(jointDisplayName(pulled->name()),driven,anim->getFrame()+1,turnOnly)")));
  QVERIFY(window.contains(QStringLiteral("statusBar()->showMessage(ViewToolSpec::tr(ViewToolSpec::pullCancelled),5000);")));
  QVERIFY(window.contains(QStringLiteral("ui->animationView->startKeyPull(false);")));
}

void TestViewTools::pullTextsMatchTheMockup()
{
  const QString page = mockup();
  QVERIFY(!page.isEmpty());
  // the label next to the pointer (showDragTip())
  QVERIFY(page.contains(QStringLiteral("`Turning only ${name} (it holds the body)` : `Turning only ${name}`) : `Pulling ${name}`")));
  QVERIFY(page.contains(QStringLiteral("\"Arrow keys move it · Enter: set key · Esc: cancel\" : \"Release: set key · Esc: cancel\"")));
  QVERIFY(page.contains(QStringLiteral("Shift when you press: turn only this joint")));
  QCOMPARE(ViewToolSpec::pullLines(QStringLiteral("Right Hand"), false, false, false),
           QStringList({"Pulling Right Hand", "Release: set key · Esc: cancel", "Shift when you press: turn only this joint"}));
  QCOMPARE(ViewToolSpec::pullLines(QStringLiteral("Right Forearm"), true, false, false),
           QStringList({"Turning only Right Forearm", "Release: set key · Esc: cancel"}));
  QCOMPARE(ViewToolSpec::pullLines(QStringLiteral("Abdomen"), true, true, false),
           QStringList({"Turning only Abdomen (it holds the body)", "Release: set key · Esc: cancel"}));
  QCOMPARE(ViewToolSpec::pullLines(QStringLiteral("Right Hand"), false, false, true),
           QStringList({"Pulling Right Hand", "Arrow keys move it · Enter: set key · Esc: cancel"}));
  // the status line at the end (finishJointDrag() without "Ctrl+Z undoes it.": Undo is not built)
  QVERIFY(page.contains(QStringLiteral("(d.turnOnly ? \"Turned \" + fullName(d.id) : \"Pulled \" + fullName(d.id)) + \". Key set at frame \"")));
  QCOMPARE(ViewToolSpec::pullDoneMessage(QStringLiteral("Right Hand"), {"Right Forearm", "Right Upper Arm", "Right Collar"}, 5, false),
           QStringLiteral("Pulled Right Hand. Key set at frame 5 for Right Forearm, Right Upper Arm, Right Collar and Right Hand."));
  QCOMPARE(ViewToolSpec::pullDoneMessage(QStringLiteral("Right Forearm"), {}, 12, true),
           QStringLiteral("Turned Right Forearm. Key set at frame 12 for Right Forearm."));
  // cancel and the reasons
  for (const char* text : {ViewToolSpec::pullCancelled, ViewToolSpec::keysNoJoint, ViewToolSpec::finishFirst, ViewToolSpec::jointHover})
    QVERIFY2(page.contains(QString::fromUtf8(text)), text);
  QVERIFY(page.contains(QStringLiteral("Pull a joint: Stop playing first.")));
  QCOMPARE(ViewToolSpec::tr(ViewToolSpec::pullBlocked).arg(QStringLiteral("Stop playing first.")), QStringLiteral("Pull a joint: Stop playing first."));
  QVERIFY(page.contains(QStringLiteral("Pull with keys: Open an animation first.")));
  QCOMPARE(ViewToolSpec::tr(ViewToolSpec::keysBlocked).arg(QStringLiteral("Open an animation first.")), QStringLiteral("Pull with keys: Open an animation first."));
  // the arrow keys' steps
  QVERIFY(page.contains(QStringLiteral("const st = e.shiftKey ? 3 : 12")));
  QCOMPARE(ViewToolSpec::keyStep, 12);
  QCOMPARE(ViewToolSpec::keyStepSmall, 3);
}

QTEST_MAIN(TestViewTools)
#include "tst_viewtools.moc"
