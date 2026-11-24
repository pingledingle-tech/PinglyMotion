/*
 * PinglyMotion - tests for props made from the prims and props that move (Script Lab, job A1):
 * the prop each prim of a folder gets (name, shape, joint, place), the documented defaults, and
 * the exact new place of a prop when a script moves, scales or turns its prim
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QTemporaryDir>

#include "scriptlab/labfolder.h"
#include "scriptlab/labprimprops.h"

namespace
{
  bool writeFile(const QString& path, const QByteArray& text)
  {
    QDir().mkpath(QFileInfo(path).path());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    return file.write(text) == text.size();
  }

  QJsonObject runFolder(const QString& folder)
  {
    return LabFolder::run(LabFolder::open(folder));
  }

  const QByteArray Empty = "default { state_entry() { } }\n";

  LabExample::Prop prop(const QString& name, const QString& shape, const QString& joint, QVector3D position, QVector3D scale, QVector3D rotation)
  {
    LabExample::Prop p;
    p.name = name;
    p.shape = shape;
    p.joint = joint;
    p.position = position;
    p.scale = scale;
    p.rotation = rotation;
    return p;
  }

  void compareProp(const LabExample::Prop& got, const LabExample::Prop& want)
  {
    QCOMPARE(got.name, want.name);
    QCOMPARE(got.shape, want.shape);
    QCOMPARE(got.joint, want.joint);
    QCOMPARE(got.position, want.position);
    QCOMPARE(got.scale, want.scale);
    QCOMPARE(got.rotation, want.rotation);
  }
}

class TestLabPrimProps : public QObject
{
  Q_OBJECT

  private slots:
    void propForEveryPrim();
    void exampleWins();
    void defaultsWithoutShapeData();
    void namesToShapes();
    void emptyRootLeftOut();
    void childMovesScalesTurns();
    void rootMoves();
    void primitiveParams();
    void refusedMoveLeftOut();
    void lastOneWins();
    void notRunNoMoves();
    void angles();
};

// a folder of four prims: each gets a prop named like the prim, with the shape and joint of the table
void TestLabPrimProps::propForEveryPrim()
{
  QTemporaryDir dir;
  const QString top = dir.filePath("Set");
  QVERIFY(writeFile(top + "/Root/hud.lsl", Empty));
  QVERIFY(writeFile(top + "/Collar/collar.lsl", Empty));
  QVERIFY(writeFile(top + "/Wrist_right/cuff.lsl", Empty));
  QVERIFY(writeFile(top + "/Lid/lid.lsl", Empty));
  const QJsonObject folder = runFolder(top);
  QVERIFY(folder.value("ok").toBool());
  const QList<LabExample::Prop> props = LabPrimProps::propsFor(folder, {});
  QCOMPARE(props.size(), 4);
  // link order: Root is link 1, then the other folders by name
  compareProp(props[0], prop("Root", "box", "", {-20, 45, 0}, {8, 8, 1}, {0, 0, 0}));
  compareProp(props[1], prop("Collar", "torus", "neck", {0, 1.5f, 0}, {5, 5, 1.6f}, {90, 0, 0}));
  compareProp(props[2], prop("Lid", "box", "", {20, 45, 0}, {4, 4, 4}, {0, 0, 0}));
  compareProp(props[3], prop("Wrist_right", "torus", "rHand", {1, 0, 0}, {2.8f, 2.8f, 1.2f}, {0, 90, 0}));
}

// a prop of the same name in the folder's example.json is used as it is
void TestLabPrimProps::exampleWins()
{
  QTemporaryDir dir;
  const QString top = dir.filePath("Set");
  QVERIFY(writeFile(top + "/Collar/collar.lsl", Empty));
  const QJsonObject folder = runFolder(top);
  const LabExample::Prop own = prop("Collar", "sphere", "chest", {1, 2, 3}, {4, 5, 6}, {7, 8, 9});
  const QList<LabExample::Prop> props = LabPrimProps::propsFor(folder, {own});
  QCOMPARE(props.size(), 1);
  compareProp(props[0], own);
}

// prims whose names say nothing get the documented default: a small box beside the figure, in a row
void TestLabPrimProps::defaultsWithoutShapeData()
{
  QTemporaryDir dir;
  const QString top = dir.filePath("Thing");
  QVERIFY(writeFile(top + "/main.lsl", Empty));        // the root prim, named like the object
  QVERIFY(writeFile(top + "/Door/door.lsl", Empty));
  QVERIFY(writeFile(top + "/Lid/lid.lsl", Empty));
  const QJsonObject folder = runFolder(top);
  QVERIFY(folder.value("ok").toBool());
  const QList<LabExample::Prop> props = LabPrimProps::propsFor(folder, {});
  QCOMPARE(props.size(), 3);
  compareProp(props[0], prop("Thing", "box", "", {20, 45, 0}, {4, 4, 4}, {0, 0, 0}));
  compareProp(props[1], prop("Door", "box", "", {26, 45, 0}, {4, 4, 4}, {0, 0, 0}));
  compareProp(props[2], prop("Lid", "box", "", {32, 45, 0}, {4, 4, 4}, {0, 0, 0}));
}

// the word table, both sides
void TestLabPrimProps::namesToShapes()
{
  struct Case { const char* name; const char* shape; const char* joint; };
  const QList<Case> cases = {
    {"Ankle_left", "torus", "lFoot"}, {"ankle cuff R", "torus", "rFoot"}, {"Cuff_left", "torus", "lHand"},
    {"r_wrist", "torus", "rHand"}, {"Collar", "torus", "neck"}, {"Neck ring", "torus", "neck"},
    {"Blindfold", "torus", "head"}, {"Gag", "sphere", "head"}, {"Hood", "cone", "head"}, {"Hat", "cone", "head"},
    {"HUD", "box", ""}, {"Lamp", "box", ""}};
  for (const Case& c : cases)
  {
    const LabExample::Prop p = LabPrimProps::propFor({c.name, 2}, {});
    QCOMPARE(p.name, QString(c.name));
    QCOMPARE(p.shape, QString(c.shape));
    QCOMPARE(p.joint, QString(c.joint));
  }
  QVERIFY(LabPrimProps::nameMatches("Collar"));
  QVERIFY(!LabPrimProps::nameMatches("Lamp"));
}

// First colour: the root is only the top folder, with nothing in it, so only "Box 0" gets a prop
void TestLabPrimProps::emptyRootLeftOut()
{
  QTemporaryDir dir;
  const QString top = dir.filePath("First_colour");
  QVERIFY(writeFile(top + "/Box 0/colour.lsl", Empty));
  const QJsonObject folder = runFolder(top);
  QVERIFY(folder.value("ok").toBool());
  const QList<LabPrimProps::Prim> prims = LabPrimProps::primsOf(folder);
  QCOMPARE(prims.size(), 1);
  QCOMPARE(prims[0].name, QString("Box 0"));
  QCOMPARE(prims[0].link, 2);
}

// a child prim moved 1 m along x and 0.5 m up, made 1 x 0.5 x 0.25 m, turned 45 degrees about z
void TestLabPrimProps::childMovesScalesTurns()
{
  QTemporaryDir dir;
  const QString top = dir.filePath("Box");
  QVERIFY(writeFile(top + "/Root/main.lsl", Empty));
  QVERIFY(writeFile(top + "/Lid/lid.lsl",
                    "default { state_entry() { llSetPos(<1.0, 0.0, 0.5>); llSetScale(<1.0, 0.5, 0.25>);"
                    " llSetLocalRot(llEuler2Rot(<0.0, 0.0, PI / 4.0>)); } }\n"));
  const QJsonObject folder = runFolder(top);
  QVERIFY(folder.value("ok").toBool());
  const QList<LabPrimProps::Move> moves = LabPrimProps::movesFromFolder(folder);
  QCOMPARE(moves.size(), 1);
  QCOMPARE(moves[0].prim, QString("Lid"));
  QCOMPARE(moves[0].link, 2);
  QVERIFY(moves[0].hasPosition && moves[0].hasSize && moves[0].hasRotation);
  QCOMPARE(moves[0].position, QVector3D(1.0f, 0.0f, 0.5f));
  QCOMPARE(moves[0].size, QVector3D(1.0f, 0.5f, 0.25f));
  const LabExample::Prop base = prop("Lid", "box", "", {20, 45, 0}, {4, 4, 4}, {0, 0, 0});
  const LabExample::Prop p = LabPrimProps::moved(base, moves[0]);
  // Second Life x, y, z are the scene's z, x, y; one metre is 39.37 units
  QCOMPARE(p.position, QVector3D(20.0f, 64.685f, 39.37f));
  // each side grows as the prim's side did: 1 / 0.5, 0.5 / 0.5, 0.25 / 0.5
  QCOMPARE(p.scale, QVector3D(8.0f, 4.0f, 2.0f));
  // about Second Life z is about the scene's y
  QCOMPARE(p.rotation, QVector3D(0.0f, 45.0f, 0.0f));
  // name, shape and joint never change
  QCOMPARE(p.name, base.name);
  QCOMPARE(p.shape, base.shape);
  QCOMPARE(p.joint, base.joint);
}

// the root prim moves in the region: the change from where it was rezzed (128, 128, 20) counts
void TestLabPrimProps::rootMoves()
{
  QTemporaryDir dir;
  const QString top = dir.filePath("Hud");
  QVERIFY(writeFile(top + "/Root/main.lsl", "default { state_entry() { llSetPos(<129.0, 128.0, 21.0>); llSetRot(llEuler2Rot(<PI / 2.0, 0.0, 0.0>)); } }\n"));
  const QJsonObject folder = runFolder(top);
  const QList<LabPrimProps::Move> moves = LabPrimProps::movesFromFolder(folder);
  QCOMPARE(moves.size(), 1);
  QCOMPARE(moves[0].prim, QString("Root"));
  QCOMPARE(moves[0].link, 1);
  const LabExample::Prop base = prop("Root", "box", "", {-20, 45, 0}, {8, 8, 1}, {0, 0, 0});
  const LabExample::Prop p = LabPrimProps::moved(base, moves[0]);
  QCOMPARE(p.position, QVector3D(-20.0f, 84.37f, 39.37f));
  QCOMPARE(p.scale, base.scale);
  // a quarter turn about Second Life x is a quarter turn about the scene's z
  QCOMPARE(p.rotation, QVector3D(0.0f, 0.0f, 90.0f));
}

// llSetLinkPrimitiveParamsFast from the root moves another prim
void TestLabPrimProps::primitiveParams()
{
  QTemporaryDir dir;
  const QString top = dir.filePath("Set");
  QVERIFY(writeFile(top + "/Root/main.lsl",
                    "default { state_entry() { llSetLinkPrimitiveParamsFast(2, [PRIM_POS_LOCAL, <0.0, 0.0, 1.0>, PRIM_SIZE, <0.5, 0.5, 1.0>]); } }\n"));
  QVERIFY(writeFile(top + "/Collar/collar.lsl", Empty));
  const QJsonObject folder = runFolder(top);
  const QList<LabPrimProps::Move> moves = LabPrimProps::movesFromFolder(folder);
  QCOMPARE(moves.size(), 1);
  QCOMPARE(moves[0].prim, QString("Collar"));
  QVERIFY(!moves[0].hasRotation);
  const LabExample::Prop base = LabPrimProps::propFor({"Collar", 2}, {});
  const LabExample::Prop p = LabPrimProps::moved(base, moves[0]);
  QCOMPARE(p.position, QVector3D(0.0f, 40.87f, 0.0f));
  QCOMPARE(p.scale, QVector3D(5.0f, 5.0f, 3.2f));
  QCOMPARE(p.rotation, base.rotation);
}

// a move the sandbox refused (outside the region) does not move the prop
void TestLabPrimProps::refusedMoveLeftOut()
{
  QTemporaryDir dir;
  const QString top = dir.filePath("Hud");
  QVERIFY(writeFile(top + "/Root/main.lsl", "default { state_entry() { llSetPos(<300.0, 128.0, 20.0>); } }\n"));
  QVERIFY(LabPrimProps::movesFromFolder(runFolder(top)).isEmpty());
}

// several moves in one run: the last value of each kind counts
void TestLabPrimProps::lastOneWins()
{
  QTemporaryDir dir;
  const QString top = dir.filePath("Box");
  QVERIFY(writeFile(top + "/Root/main.lsl", Empty));
  QVERIFY(writeFile(top + "/Lid/lid.lsl",
                    "default { state_entry() { llSetPos(<1.0, 0.0, 0.0>); llSetPos(<0.0, 2.0, 0.0>); } }\n"));
  const QList<LabPrimProps::Move> moves = LabPrimProps::movesFromFolder(runFolder(top));
  QCOMPARE(moves.size(), 1);
  QCOMPARE(moves[0].position, QVector3D(0.0f, 2.0f, 0.0f));
  QVERIFY(!moves[0].hasSize);
  QVERIFY(!moves[0].hasRotation);
  const LabExample::Prop p = LabPrimProps::moved(prop("Lid", "box", "", {0, 0, 0}, {1, 1, 1}, {0, 0, 0}), moves[0]);
  QCOMPARE(p.position, QVector3D(78.74f, 0.0f, 0.0f));
}

// a folder that did not run moves nothing, but its prims still get props
void TestLabPrimProps::notRunNoMoves()
{
  QTemporaryDir dir;
  const QString top = dir.filePath("Box");
  QVERIFY(writeFile(top + "/Lid/lid.lsl", "default { state_entry() { llSetPos(<1.0, 0.0, 0.0>) } }\n"));
  const QJsonObject folder = runFolder(top);
  QVERIFY(!folder.value("ok").toBool());
  QVERIFY(LabPrimProps::movesFromFolder(folder).isEmpty());
  QCOMPARE(LabPrimProps::propsFor(folder, {}).size(), 1);
}

// the prop's angles and back, also near a quarter turn about y
void TestLabPrimProps::angles()
{
  for (const QVector3D& a : {QVector3D(0, 0, 0), QVector3D(90, 0, 0), QVector3D(-90, 0, 0), QVector3D(10, 20, 30),
                             QVector3D(0, 90, 0), QVector3D(45, -30, 120)})
  {
    const QVector3D back = LabPrimProps::toPropAngles(LabPrimProps::fromPropAngles(a));
    const QQuaternion q1 = LabPrimProps::fromPropAngles(a), q2 = LabPrimProps::fromPropAngles(back);
    QVERIFY2(std::fabs(QQuaternion::dotProduct(q1, q2)) > 0.99999f, qPrintable(QString("%1 %2 %3").arg(a.x()).arg(a.y()).arg(a.z())));
  }
  QCOMPARE(LabPrimProps::toPropAngles(LabPrimProps::fromPropAngles({10, 20, 30})), QVector3D(10, 20, 30));
  QCOMPARE(LabPrimProps::toScene(QVector3D(1, 2, 3)), QVector3D(2, 3, 1));
}

QTEST_GUILESS_MAIN(TestLabPrimProps)
#include "tst_labprimprops.moc"
