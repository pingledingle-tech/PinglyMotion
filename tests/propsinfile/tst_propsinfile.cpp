/*
 * PinglyMotion - tests of props saved with the animation (src/propsinscene.*, "props" in src/dingle.*)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

#include "animation.h"
#include "bvh.h"
#include "dingle.h"
#include "propmanager.h"
#include "propsinscene.h"

namespace
{
  QByteArray readFile(const QString& path)
  {
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
  }

  QString example(const char* name)
  {
    return QStringLiteral(PINGLYMOTION_SOURCE_DIR "/examples/") + QLatin1String(name);
  }

  // three props with names, shapes, places, sizes, turns and attachment points
  void fill(PropManager& props)
  {
    props.addProp(Prop::Box, {}, 0, 1.5, 0, -2, 0.5, 0.5, 0.9, 0, 90, 0, QStringLiteral("Chair"));
    props.addProp(Prop::Torus, {}, 12, 0, 1.25, 0, 0.1, 0.2, 0.3, 10, 20, 30, QStringLiteral("Ring on the hand"));
    props.addProp(Prop::Cone, {}, 0, -0.123456, 7, 0.1, 2, 2, 2, 0, 0, 0);   // no name given: "Cone 0"
  }

  void sameRecords(const QList<PropRecord>& a, const QList<PropRecord>& b)
  {
    QCOMPARE(a.size(), b.size());
    for (int k = 0; k < a.size(); k++)
    {
      QCOMPARE(a[k].name, b[k].name);
      QCOMPARE(a[k].type, b[k].type);
      QCOMPARE(a[k].attachment, b[k].attachment);
      for (int i = 0; i < 3; i++)
      {
        QCOMPARE(a[k].position[i], b[k].position[i]);
        QCOMPARE(a[k].scale[i], b[k].scale[i]);
        QCOMPARE(a[k].rotation[i], b[k].rotation[i]);
      }
    }
  }
}

class TestPropsInFile : public QObject
{
  Q_OBJECT

private slots:
  void recordsKeepEverything();
  void saveAndOpenAgain();
  void saveTwiceIsTheSame();
  void noPropsWritesNoPropsList();
  void oldDingleWithoutPropsOpens();
  void openOldPropFiles();
  void propFileHasNoNames();
  void badPropFileChangesNothing();
  void propNameChecks();
  void unknownShapeIsSkippedAndCounted();
  void badPropsListsAreRefused_data();
  void badPropsListsAreRefused();
  void tooManyProps();
  void fileOpenKnowsPropFiles();
};

void TestPropsInFile::recordsKeepEverything()
{
  PropManager props(1);
  fill(props);
  const QList<PropRecord> records = PropsInScene::recordsOf(props);
  QCOMPARE(records.size(), 3);
  QCOMPARE(records[0].name, QStringLiteral("Chair"));
  QCOMPARE(records[0].type, int(Prop::Box));
  QCOMPARE(records[0].position[0], 1.5);
  QCOMPARE(records[0].position[2], -2.0);
  QCOMPARE(records[0].scale[2], double(0.9f));
  QCOMPARE(records[0].rotation[1], 90.0);
  QCOMPARE(records[1].name, QStringLiteral("Ring on the hand"));
  QCOMPARE(records[1].attachment, 12);
  QCOMPARE(records[2].name, QStringLiteral("Cone 0"));
  QCOMPARE(records[2].position[0], double(-0.123456f));
}

void TestPropsInFile::saveAndOpenAgain()
{
  PropManager props(1);
  fill(props);

  BVH bvh;
  Animation anim(&bvh);
  anim.setNumberOfFrames(10);
  anim.setProps(PropsInScene::recordsOf(props));

  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString file = dir.filePath(QStringLiteral("with-props.dingle"));
  QString error;
  QVERIFY2(anim.saveBVH(file, &error), qPrintable(error));
  QVERIFY(readFile(file).contains("\"props\""));

  BVH bvh2;
  Animation opened(&bvh2, file);
  QCOMPARE(opened.getNumberOfFrames(), 10);
  sameRecords(opened.props(), anim.props());

  // and into a scene again: the same props, same names, in the same order
  PropManager scene(1);
  for (const PropRecord& record : opened.props()) QVERIFY(PropsInScene::addRecord(scene, record, {}));
  QCOMPARE(scene.count(), 3);
  QCOMPARE(scene.at(0)->name(), QStringLiteral("Chair"));
  QCOMPARE(scene.at(1)->name(), QStringLiteral("Ring on the hand"));
  QCOMPARE(scene.at(2)->name(), QStringLiteral("Cone 0"));
  QCOMPARE(scene.at(1)->isAttached(), 12u);
  QCOMPARE(scene.at(0)->propType(), Prop::Box);
  QCOMPARE(scene.at(1)->propType(), Prop::Torus);
  QCOMPARE(scene.at(2)->propType(), Prop::Cone);
  QCOMPARE(scene.at(1)->scale(), QVector3D(0.1f, 0.2f, 0.3f));
  QCOMPARE(scene.at(1)->rotation(), QVector3D(10, 20, 30));
  QCOMPARE(scene.at(0)->position(), QVector3D(1.5f, 0, -2));
  sameRecords(PropsInScene::recordsOf(scene), PropsInScene::recordsOf(props));
}

void TestPropsInFile::saveTwiceIsTheSame()
{
  PropManager props(1);
  fill(props);
  BVH bvh;
  Animation anim(&bvh);
  anim.setProps(PropsInScene::recordsOf(props));
  QTemporaryDir dir;
  const QString one = dir.filePath(QStringLiteral("one.dingle"));
  const QString two = dir.filePath(QStringLiteral("two.dingle"));
  QVERIFY(anim.saveBVH(one));
  BVH bvh2;
  Animation opened(&bvh2, one);
  QVERIFY(opened.saveBVH(two));
  QCOMPARE(readFile(two), readFile(one));
}

void TestPropsInFile::noPropsWritesNoPropsList()
{
  BVH bvh;
  Animation anim(&bvh);
  QTemporaryDir dir;
  const QString file = dir.filePath(QStringLiteral("plain.dingle"));
  QVERIFY(anim.saveBVH(file));
  QVERIFY(!readFile(file).contains("props"));
  BVH bvh2;
  Animation opened(&bvh2, file);
  QVERIFY(opened.props().isEmpty());
}

void TestPropsInFile::oldDingleWithoutPropsOpens()
{
  for (const char* name : {"wave.dingle", "jump.dingle", "throne.dingle"})
  {
    const QByteArray data = readFile(example(name));
    QVERIFY2(!data.isEmpty(), name);
    const Dingle::Result result = Dingle::parse(data);
    QVERIFY2(result.ok(), name);
    QVERIFY2(result.props.isEmpty(), name);
    QCOMPARE(result.ignoredKeys, 0);
    BVH bvh;
    Animation opened(&bvh, example(name));
    QVERIFY2(opened.getNumberOfFrames() > 0, name);
    QVERIFY2(opened.props().isEmpty(), name);
    // written back without a props list
    QVERIFY2(!Dingle::write(*result.animation, result.extensions).contains("\"props\""), name);
  }
}

void TestPropsInFile::openOldPropFiles()
{
  // every .prp of the examples opens with the number of props it has lines
  const struct { const char* name; int count; } files[] = {{"ski.prp", 4}, {"throne.prp", 4}, {"boxonhead.prp", 1}};
  for (const auto& f : files)
  {
    const PropsInScene::OpenResult result = PropsInScene::readPropFile(example(f.name));
    QVERIFY2(result.ok(), f.name);
    QCOMPARE(result.props.size(), f.count);
  }
  // the first prop of ski.prp: "0 4.5 -1.4 -12.5 1 1 35 0 0 0 10"
  const PropRecord first = PropsInScene::readPropFile(example("ski.prp")).props[0];
  QCOMPARE(first.type, 0);
  QCOMPARE(first.position[0], 4.5);
  QCOMPARE(first.position[1], -1.4);
  QCOMPARE(first.position[2], -12.5);
  QCOMPARE(first.scale[2], 35.0);
  QCOMPARE(first.attachment, 10);

  // into a scene: the props get the usual names
  PropManager scene(1);
  for (const PropRecord& record : PropsInScene::readPropFile(example("ski.prp")).props)
    QVERIFY(PropsInScene::addRecord(scene, record, {}));
  QCOMPARE(scene.count(), 4);
  for (int i = 0; i < 4; i++) QCOMPARE(scene.at(i)->name(), QStringLiteral("Box %1").arg(i));
  QCOMPARE(scene.at(3)->isAttached(), 23u);
  QCOMPARE(scene.at(3)->position(), QVector3D(0, -3, 3.8f));

  // a file from before attachments (10 values) opens as not attached
  QTemporaryDir dir;
  const QString old = dir.filePath(QStringLiteral("old.prp"));
  QFile file(old);
  QVERIFY(file.open(QIODevice::WriteOnly));
  file.write("1 0 0 1 2 2 2 0 0 0\n");
  file.close();
  const PropsInScene::OpenResult result = PropsInScene::readPropFile(old);
  QVERIFY(result.ok());
  QCOMPARE(result.props.size(), 1);
  QCOMPARE(result.props[0].type, 1);
  QCOMPARE(result.props[0].attachment, 0);
}

void TestPropsInFile::propFileHasNoNames()
{
  // a .prp file stays the same format: no name in the file, and the name of a record is not written
  PropManager props(1);
  fill(props);
  const QByteArray text = PropFile::write(PropsInScene::recordsOf(props));
  QVERIFY(!text.contains("Chair"));
  QCOMPARE(text.count('\n'), 3);
  const PropFile::Result read = PropFile::parse(text);
  QVERIFY(read.ok());
  QCOMPARE(read.props->size(), 3);
  QVERIFY((*read.props)[0].name.isEmpty());
  QCOMPARE((*read.props)[1].attachment, 12);
}

void TestPropsInFile::badPropFileChangesNothing()
{
  QTemporaryDir dir;
  const QString bad = dir.filePath(QStringLiteral("bad.prp"));
  QFile file(bad);
  QVERIFY(file.open(QIODevice::WriteOnly));
  file.write("0 0 0 0 1 1 1 0 0 0 0\n9 0 0 0 1 1 1 0 0 0 0\n");
  file.close();
  const PropsInScene::OpenResult result = PropsInScene::readPropFile(bad);
  QVERIFY(!result.ok());
  QVERIFY(result.props.isEmpty());
  QVERIFY(result.error.contains(QStringLiteral("Line 2")));

  const PropsInScene::OpenResult missing = PropsInScene::readPropFile(dir.filePath(QStringLiteral("missing.prp")));
  QVERIFY(!missing.ok());
}

void TestPropsInFile::propNameChecks()
{
  // a name already taken gets the usual numbered name, so two props never share a name
  PropManager scene(1);
  PropRecord one;
  one.name = QStringLiteral("Hat");
  QCOMPARE(PropsInScene::addRecord(scene, one, {})->name(), QStringLiteral("Hat"));
  QCOMPARE(PropsInScene::addRecord(scene, one, {})->name(), QStringLiteral("Box 0"));

  // a name with quotes and non-ASCII letters is kept exactly
  PropRecord odd;
  odd.name = QStringLiteral("Hatten \"min\" \xC3\xA6\xC3\xB8\xC3\xA5");
  odd.type = 1;
  const QByteArray text = Dingle::write(*Dingle::parse(readFile(example("wave.dingle"))).animation, QJsonObject(), QStringLiteral("PinglyMotion"), {odd});
  const Dingle::Result read = Dingle::parse(text);
  QVERIFY2(read.ok(), qPrintable(read.error));
  QCOMPARE(read.props.size(), 1);
  QCOMPARE(read.props[0].name, odd.name);
  QCOMPARE(read.props[0].type, 1);
}

void TestPropsInFile::unknownShapeIsSkippedAndCounted()
{
  QByteArray text = Dingle::write(*Dingle::parse(readFile(example("wave.dingle"))).animation, QJsonObject(),
                                  QStringLiteral("PinglyMotion"), {PropRecord(), PropRecord()});
  // the second prop becomes a shape a later version might add
  const int at = text.lastIndexOf("\"type\": \"box\"");
  QVERIFY(at > 0);
  text.replace(at, 13, "\"type\": \"cube2\"");
  const Dingle::Result read = Dingle::parse(text);
  QVERIFY2(read.ok(), qPrintable(read.error));
  QCOMPARE(read.props.size(), 1);
  QCOMPARE(read.ignoredKeys, 1);
}

void TestPropsInFile::badPropsListsAreRefused_data()
{
  QTest::addColumn<QByteArray>("props");
  QTest::newRow("not a list") << QByteArray("5");
  QTest::newRow("a prop that is not an object") << QByteArray("[1]");
  QTest::newRow("no type") << QByteArray("[{\"position\":[0,0,0],\"scale\":[1,1,1],\"rotation\":[0,0,0]}]");
  QTest::newRow("type is a number") << QByteArray("[{\"type\":0,\"position\":[0,0,0],\"scale\":[1,1,1],\"rotation\":[0,0,0]}]");
  QTest::newRow("no position") << QByteArray("[{\"type\":\"box\",\"scale\":[1,1,1],\"rotation\":[0,0,0]}]");
  QTest::newRow("two numbers") << QByteArray("[{\"type\":\"box\",\"position\":[0,0],\"scale\":[1,1,1],\"rotation\":[0,0,0]}]");
  QTest::newRow("text in a number") << QByteArray("[{\"type\":\"box\",\"position\":[0,\"x\",0],\"scale\":[1,1,1],\"rotation\":[0,0,0]}]");
  QTest::newRow("too large") << QByteArray("[{\"type\":\"box\",\"position\":[0,2e6,0],\"scale\":[1,1,1],\"rotation\":[0,0,0]}]");
  QTest::newRow("attachment below 0") << QByteArray("[{\"type\":\"box\",\"position\":[0,0,0],\"scale\":[1,1,1],\"rotation\":[0,0,0],\"attachment\":-1}]");
  QTest::newRow("attachment past the limit") << QByteArray("[{\"type\":\"box\",\"position\":[0,0,0],\"scale\":[1,1,1],\"rotation\":[0,0,0],\"attachment\":10001}]");
  QTest::newRow("attachment not whole") << QByteArray("[{\"type\":\"box\",\"position\":[0,0,0],\"scale\":[1,1,1],\"rotation\":[0,0,0],\"attachment\":1.5}]");
  QTest::newRow("name is a number") << QByteArray("[{\"name\":4,\"type\":\"box\",\"position\":[0,0,0],\"scale\":[1,1,1],\"rotation\":[0,0,0]}]");
  QTest::newRow("name too long") << ("[{\"name\":\"" + QByteArray(129, 'a') + "\",\"type\":\"box\",\"position\":[0,0,0],\"scale\":[1,1,1],\"rotation\":[0,0,0]}]");
  QTest::newRow("same key twice") << QByteArray("[{\"type\":\"box\",\"type\":\"box\",\"position\":[0,0,0],\"scale\":[1,1,1],\"rotation\":[0,0,0]}]");
}

void TestPropsInFile::badPropsListsAreRefused()
{
  QFETCH(QByteArray, props);
  QByteArray text = readFile(example("wave.dingle"));
  QVERIFY(text.contains("\"camera\": null,"));
  text.replace("\"camera\": null,", "\"camera\": null, \"props\": " + props + ",");
  const Dingle::Result read = Dingle::parse(text);
  QVERIFY2(!read.ok(), "the file was accepted");
  QVERIFY(!read.error.isEmpty());
  QVERIFY(read.props.isEmpty());
}

void TestPropsInFile::tooManyProps()
{
  const Dingle::Result base = Dingle::parse(readFile(example("wave.dingle")));
  QList<PropRecord> props;
  for (int i = 0; i < PropFile::MAX_PROPS; i++) props << PropRecord();
  const QByteArray full = Dingle::write(*base.animation, QJsonObject(), QStringLiteral("PinglyMotion"), props);
  const Dingle::Result read = Dingle::parse(full);
  QVERIFY2(read.ok(), qPrintable(read.error));
  QCOMPARE(read.props.size(), PropFile::MAX_PROPS);
  // one more is not written, so a saved file always opens
  props << PropRecord();
  QCOMPARE(Dingle::write(*base.animation, QJsonObject(), QStringLiteral("PinglyMotion"), props), full);
}

void TestPropsInFile::fileOpenKnowsPropFiles()
{
  QVERIFY(PropsInScene::isPropFileName(QStringLiteral("ski.prp")));
  QVERIFY(PropsInScene::isPropFileName(QStringLiteral("/a/b/SKI.PRP")));
  QVERIFY(!PropsInScene::isPropFileName(QStringLiteral("ski.dingle")));
  QVERIFY(!PropsInScene::isPropFileName(QStringLiteral("prp")));
  // the Open dialog lists them, and the window routes them and saves the props with the animation
  QFile window(QStringLiteral(PINGLYMOTION_SOURCE_DIR "/src/pinglymotionwindowfile.cpp"));
  QVERIFY(window.open(QIODevice::ReadOnly));
  const QString source = QString::fromUtf8(window.readAll());
  QVERIFY(source.contains(QStringLiteral("if(!file.isEmpty() && PropsInScene::isPropFileName(file))")));
  QCOMPARE(source.count(QStringLiteral("storePropsInAnimation();")), 2);
  QCOMPARE(source.count(QStringLiteral("showPropsOfOpenedFile();")), 2);
  QFile main(QStringLiteral(PINGLYMOTION_SOURCE_DIR "/src/pinglymotionwindow.cpp"));
  QVERIFY(main.open(QIODevice::ReadOnly));
  const QString mainSource = QString::fromUtf8(main.readAll());
  QVERIFY(mainSource.contains(QStringLiteral("(*.dingle *.avm *.bvh *.prp)")));
  QVERIFY(mainSource.contains(QStringLiteral("w->storePropsInAnimation();")));
}

QTEST_MAIN(TestPropsInFile)
#include "tst_propsinfile.moc"
