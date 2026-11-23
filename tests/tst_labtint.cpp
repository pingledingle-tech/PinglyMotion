/*
 * PinglyMotion - tests for props as prims (Script Lab, lane 1 job E): the colour and
 * transparency the scripts of a folder give each prim, read from the folder run
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QTemporaryDir>

#include "scriptlab/labfolder.h"
#include "scriptlab/labtint.h"

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
}

class TestLabTint : public QObject
{
  Q_OBJECT

  private slots:
    void colourAndAlphaPerPrim();
    void lastOneWins();
    void otherFacesLeftOut();
    void onePrimObject();
    void folderNotRun();
    void nothingColoured();
};

// the root colours itself, and makes the lid half clear with a link function
void TestLabTint::colourAndAlphaPerPrim()
{
  QTemporaryDir dir;
  const QString top = dir.filePath("Box");
  QVERIFY(writeFile(top + "/Root/main.lsl",
                    "default { state_entry() { llSetColor(<1.0, 0.0, 0.0>, ALL_SIDES); llSetLinkAlpha(2, 0.5, ALL_SIDES); } }\n"));
  QVERIFY(writeFile(top + "/Lid/lid.lsl", "default { state_entry() { } }\n"));
  const QJsonObject folder = runFolder(top);
  QVERIFY(folder.value("ok").toBool());
  const QList<LabTint::Tint> tints = LabTint::fromFolder(folder);
  QCOMPARE(tints.size(), 2);
  QCOMPARE(tints[0].prim, QString("Root"));
  QCOMPARE(tints[0].link, 1);
  QVERIFY(tints[0].hasColour);
  QVERIFY(!tints[0].hasAlpha);
  QCOMPARE(tints[0].colour[0], 1.0f);
  QCOMPARE(tints[0].colour[1], 0.0f);
  QCOMPARE(tints[0].colour[2], 0.0f);
  QCOMPARE(tints[1].prim, QString("Lid"));
  QCOMPARE(tints[1].link, 2);
  QVERIFY(!tints[1].hasColour);
  QVERIFY(tints[1].hasAlpha);
  QCOMPARE(tints[1].alpha, 0.5f);
}

// a later colour replaces an earlier one; PRIM_COLOR sets both; values are kept from 0 to 1
void TestLabTint::lastOneWins()
{
  QTemporaryDir dir;
  const QString top = dir.filePath("Lamp");
  QVERIFY(writeFile(top + "/lamp.lsl",
                    "default { state_entry() { llSetColor(<1, 1, 0>, ALL_SIDES); "
                    "llSetPrimitiveParams([PRIM_COLOR, ALL_SIDES, <0.0, 0.5, 2.0>, 0.25]); } }\n"));
  const QList<LabTint::Tint> tints = LabTint::fromFolder(runFolder(top));
  QCOMPARE(tints.size(), 1);
  QCOMPARE(tints[0].prim, QString("Lamp"));   // a root without a "Root" folder has the object's name
  QVERIFY(tints[0].hasColour);
  QVERIFY(tints[0].hasAlpha);
  QCOMPARE(tints[0].colour[0], 0.0f);
  QCOMPARE(tints[0].colour[1], 0.5f);
  QCOMPARE(tints[0].colour[2], 1.0f);
  QCOMPARE(tints[0].alpha, 0.25f);
}

// a prop has one colour: only ALL_SIDES and face 0 count
void TestLabTint::otherFacesLeftOut()
{
  QTemporaryDir dir;
  const QString top = dir.filePath("Cube");
  QVERIFY(writeFile(top + "/cube.lsl",
                    "default { state_entry() { llSetColor(<0, 1, 0>, 0); llSetColor(<1, 0, 0>, 3); llSetAlpha(0.0, 2); } }\n"));
  const QList<LabTint::Tint> tints = LabTint::fromFolder(runFolder(top));
  QCOMPARE(tints.size(), 1);
  QCOMPARE(tints[0].colour[0], 0.0f);
  QCOMPARE(tints[0].colour[1], 1.0f);
  QVERIFY(!tints[0].hasAlpha);
}

// an object of one prim: the log says link 0, the tint is the root's
void TestLabTint::onePrimObject()
{
  QTemporaryDir dir;
  const QString top = dir.filePath("Ball");
  QVERIFY(writeFile(top + "/Root/ball.lsl", "default { state_entry() { llSetAlpha(0.0, ALL_SIDES); } }\n"));
  const QList<LabTint::Tint> tints = LabTint::fromFolder(runFolder(top));
  QCOMPARE(tints.size(), 1);
  QCOMPARE(tints[0].link, 1);
  QCOMPARE(tints[0].prim, QString("Root"));
  QCOMPARE(tints[0].alpha, 0.0f);
}

// a folder that did not run gives no tints
void TestLabTint::folderNotRun()
{
  QTemporaryDir dir;
  const QString top = dir.filePath("Broken");
  QVERIFY(writeFile(top + "/a.lsl", "default { state_entry() { llSetColor(<1, 0, 0>, ALL_SIDES); } }\n"));
  QVERIFY(writeFile(top + "/b.lsl", "default { state_entry() { llOwnerSay(1, 2); } }\n"));
  const QJsonObject folder = runFolder(top);
  QVERIFY(!folder.value("ok").toBool());
  QVERIFY(LabTint::fromFolder(folder).isEmpty());
  QVERIFY(LabTint::fromFolder(QJsonObject()).isEmpty());
}

void TestLabTint::nothingColoured()
{
  QTemporaryDir dir;
  const QString top = dir.filePath("Plain");
  QVERIFY(writeFile(top + "/plain.lsl", "default { state_entry() { llOwnerSay(\"hi\"); } }\n"));
  QVERIFY(LabTint::fromFolder(runFolder(top)).isEmpty());
}

QTEST_GUILESS_MAIN(TestLabTint)
#include "tst_labtint.moc"
