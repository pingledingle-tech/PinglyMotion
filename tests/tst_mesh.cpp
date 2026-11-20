/*
 * PinglyMotion - test of the .obj mesh reader (src/mesh.cpp, issue 204)
 *
 * The shipped prop meshes read as before; broken files stop with an exact
 * error text and leave the mesh empty instead of turning bad numbers into 0.
 */
#include <QtTest>
#include <QTemporaryDir>

#include "constants.h"
#include "mesh.h"

class MeshTest : public QObject
{
  Q_OBJECT
private slots:
  void shippedCube();
  void shippedTorus();
  void goodFile();
  void brokenFile_data();
  void brokenFile();
  void missingFile();

private:
  QString write(const QByteArray& text);
  QTemporaryDir m_dir;
  int m_count = 0;
};

QString MeshTest::write(const QByteArray& text)
{
  const QString path = m_dir.filePath(QString("mesh%1.obj").arg(++m_count));
  QFile file(path);
  if (!file.open(QIODevice::WriteOnly)) return QString();
  file.write(text);
  return path;
}

void MeshTest::shippedCube()
{
  Mesh mesh(Constants::cubeMeshFilePath(), Mesh::Shape::CUBE);
  QVERIFY2(mesh.errorString().isEmpty(), qPrintable(mesh.errorString()));
  // 6 sides, 2 triangles each, 3 corners, 3 floats
  QCOMPARE(mesh.drawVertices().size(), 6 * 2 * 3 * 3);
  QCOMPARE(mesh.drawNormals().size(), 6 * 2 * 3 * 3);
  for (float v : mesh.drawVertices())
    QVERIFY(qFuzzyCompare(qAbs(v), 0.5f));
}

void MeshTest::shippedTorus()
{
  Mesh mesh(Constants::torusMeshFilePath(), Mesh::Shape::TORUS);
  QVERIFY2(mesh.errorString().isEmpty(), qPrintable(mesh.errorString()));
  QVERIFY(mesh.drawVertices().size() > 0);
  QCOMPARE(mesh.drawVertices().size() % 9, 0);
  QCOMPARE(mesh.drawNormals().size(), mesh.drawVertices().size());
}

void MeshTest::goodFile()
{
  const QString path = write("# a comment\n"
                             "o Triangle\n"
                             "v 0 0 0\r\n"
                             "v 1.5 0 0\n"
                             "   \n"
                             "v 0 2.25 0\n"
                             "vn 0 0 1\n"
                             "s off\n"
                             "f 1//1  2//1 3//1\n");
  Mesh mesh(path, Mesh::Shape::CUSTOM);
  QVERIFY2(mesh.errorString().isEmpty(), qPrintable(mesh.errorString()));
  const QVector<float> expected = {0, 0, 0, 1.5f, 0, 0, 0, 2.25f, 0};
  QCOMPARE(mesh.drawVertices(), expected);
  const QVector<float> normals = {0, 0, 1, 0, 0, 1, 0, 0, 1};
  QCOMPARE(mesh.drawNormals(), normals);
}

void MeshTest::brokenFile_data()
{
  QTest::addColumn<QByteArray>("text");
  QTest::addColumn<QString>("error");

  const QByteArray tail = "vn 0 0 1\nf 1//1 2//1 3//1\n";
  QTest::newRow("bad vertex number") << QByteArray("v 0 0 0\nv 1 x 0\nv 0 1 0\n") + tail
                                     << "line 2: a vertex needs three numbers";
  QTest::newRow("vertex with two numbers") << QByteArray("v 0 0 0\nv 1 0\nv 0 1 0\n") + tail
                                           << "line 2: a vertex needs three numbers";
  QTest::newRow("bad normal number") << QByteArray("v 0 0 0\nv 1 0 0\nv 0 1 0\nvn 0 0 one\nf 1//1 2//1 3//1\n")
                                     << "line 4: a normal needs three numbers";
  QTest::newRow("infinite number") << QByteArray("v 0 0 inf\nv 1 0 0\nv 0 1 0\n") + tail
                                   << "line 1: a vertex needs three numbers";
  QTest::newRow("face with four corners") << QByteArray("v 0 0 0\nv 1 0 0\nv 0 1 0\nvn 0 0 1\nf 1//1 2//1 3//1 1//1\n")
                                          << "line 5: a face needs three corners";
  QTest::newRow("face corner without normal") << QByteArray("v 0 0 0\nv 1 0 0\nv 0 1 0\nvn 0 0 1\nf 1//1 2 3//1\n")
                                              << "line 5: a face corner must be vertex//normal with numbers from 1";
  QTest::newRow("face corner not a number") << QByteArray("v 0 0 0\nv 1 0 0\nv 0 1 0\nvn 0 0 1\nf 1//1 a//1 3//1\n")
                                            << "line 5: a face corner must be vertex//normal with numbers from 1";
  QTest::newRow("face index zero") << QByteArray("v 0 0 0\nv 1 0 0\nv 0 1 0\nvn 0 0 1\nf 0//1 2//1 3//1\n")
                                   << "line 5: a face corner must be vertex//normal with numbers from 1";
  QTest::newRow("face vertex missing") << QByteArray("v 0 0 0\nv 1 0 0\nv 0 1 0\nvn 0 0 1\nf 1//1 2//1 4//1\n")
                                       << "a face uses vertex 4, but there are only 3";
  QTest::newRow("face normal missing") << QByteArray("v 0 0 0\nv 1 0 0\nv 0 1 0\nvn 0 0 1\nf 1//1 2//2 3//1\n")
                                       << "a face uses normal 2, but there are only 1";
}

void MeshTest::brokenFile()
{
  QFETCH(QByteArray, text);
  QFETCH(QString, error);
  const QString path = write(text);
  QTest::ignoreMessage(QtWarningMsg, qPrintable(QString("Mesh: cannot read %1: %2").arg(path, error)));
  Mesh mesh(path, Mesh::Shape::CUSTOM);
  QCOMPARE(mesh.errorString(), error);
  QVERIFY(mesh.drawVertices().isEmpty());
  QVERIFY(mesh.drawNormals().isEmpty());
}

void MeshTest::missingFile()
{
  const QString path = m_dir.filePath("does-not-exist.obj");
  QTest::ignoreMessage(QtWarningMsg, qPrintable(QString("Mesh: cannot read %1: the file does not exist").arg(path)));
  Mesh mesh(path, Mesh::Shape::CUSTOM);
  QCOMPARE(mesh.errorString(), QString("the file does not exist"));
  QVERIFY(mesh.drawVertices().isEmpty());
}

QTEST_MAIN(MeshTest)
#include "tst_mesh.moc"
