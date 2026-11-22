/*
 * Script Lab (the RLV tool) as a whole, through the control commands.
 *
 * The scenarios are in tests/data/scriptlab/scenarios.json: each one is a list of
 * control requests (lsl_check, lsl_lint, lsl_run, lsl_event, rlv_command, rlv_state,
 * rlv_reset, ...) and, for each request, the exact values some fields of the answer
 * must have. This test sends them to ControlApi, the same object the program's local
 * server and the MCP server talk to; tests/smoke_scriptlab.py sends the same file to
 * the real program under Xvfb. Every example script in examples/scriptlab must be used
 * by at least one scenario.
 *
 * To write the expected values again after an intended change:
 *   PINGLYMOTION_UPDATE_SCENARIOS=1 pinglymotion_scriptlabwhole_tests
 * then read the difference in git before committing it.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QtTest>

#include "controlapi.h"

namespace
{
// stand-in for the window: the Script Lab commands need no animation
class NoWindow : public ControlHost
{
  public:
    Animation* animation() override { return nullptr; }
    void goToFrame(int) override {}
    void refresh() override {}
    void selectJoint(const QString&) override {}
    QImage screenshot(int, int) override { return QImage(); }
    bool newAnimation(QString&) override { return false; }
    bool openFile(const QString&, QString&) override { return false; }
    bool openFileData(const QString&, const QByteArray&, QString&) override { return false; }
    bool saveFile(const QString&, QString&) override { return false; }
    void setPlaying(bool) override {}
    bool playing() override { return false; }
    QString fileName() override { return QString(); }
    bool unsavedChanges() override { return false; }
    bool male() override { return false; }
    void setMale(bool) override {}
    void setCamera(float, float, float) override {}
    void resetCamera() override {}
    QVector3D camera() override { return QVector3D(); }
};

QString examplesFolder() { return QString::fromUtf8(PINGLYMOTION_EXAMPLES) + "/scriptlab"; }
QString scenarioFile() { return QString::fromUtf8(PINGLYMOTION_SCENARIOS); }

// "source_file": "hello.lsl" becomes "source": <the example's text>, at any depth
QJsonValue withSources(const QJsonValue& value, QSet<QString>* used)
{
  if (value.isArray())
  {
    QJsonArray out;
    for (const QJsonValue& v : value.toArray()) out.append(withSources(v, used));
    return out;
  }
  if (!value.isObject()) return value;
  QJsonObject out;
  const QJsonObject in = value.toObject();
  for (auto it = in.begin(); it != in.end(); ++it)
  {
    if (it.key() == "source_file")
    {
      QFile file(examplesFolder() + "/" + it.value().toString());
      if (!file.open(QIODevice::ReadOnly)) qFatal("missing example %s", qPrintable(it.value().toString()));
      out["source"] = QString::fromUtf8(file.readAll());
      if (used) used->insert(it.value().toString());
    }
    else out[it.key()] = withSources(it.value(), used);
  }
  return out;
}

// "a.b.0.c": object keys and array indices; a missing part gives null
QJsonValue at(const QJsonValue& root, const QString& path)
{
  QJsonValue value = root;
  for (const QString& part : path.split('.'))
  {
    bool isIndex = false;
    const int index = part.toInt(&isIndex);
    if (value.isArray() && isIndex) value = value.toArray().at(index);
    else if (value.isObject()) value = value.toObject().value(part);
    else return QJsonValue(QJsonValue::Null);
    if (value.isUndefined()) return QJsonValue(QJsonValue::Null);
  }
  return value;
}

QString compact(const QJsonValue& value)
{
  return QString::fromUtf8(QJsonDocument(QJsonArray{value}).toJson(QJsonDocument::Compact)).mid(1).chopped(1);
}

// readable JSON: a value that fits on one line stays on one line
QString pretty(const QJsonValue& value, int indent)
{
  const QString one = compact(value);
  if (one.size() + indent <= 140 || !(value.isArray() || value.isObject())) return one;
  const QString pad(indent + 2, ' ');
  QStringList parts;
  if (value.isArray())
  {
    for (const QJsonValue& v : value.toArray()) parts << pad + pretty(v, indent + 2);
    return "[\n" + parts.join(",\n") + "\n" + QString(indent, ' ') + "]";
  }
  const QJsonObject object = value.toObject();
  for (auto it = object.begin(); it != object.end(); ++it)
  {
    const QString key = compact(it.key());
    parts << pad + key + ": " + pretty(it.value(), indent + 2);
  }
  return "{\n" + parts.join(",\n") + "\n" + QString(indent, ' ') + "}";
}

}

class ScriptLabWholeTest : public QObject
{
  Q_OBJECT

  private slots:
    void initTestCase();
    void scenarios_data();
    void scenarios();
    void everyExampleIsUsed();
    void cleanupTestCase();

  private:
    QJsonObject m_file;
    QJsonArray m_updated;
    bool m_update = false;
    QSet<QString> m_used;
};

void ScriptLabWholeTest::initTestCase()
{
  QFile file(scenarioFile());
  QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(scenarioFile()));
  QJsonParseError error;
  m_file = QJsonDocument::fromJson(file.readAll(), &error).object();
  QVERIFY2(error.error == QJsonParseError::NoError, qPrintable(error.errorString()));
  QVERIFY(m_file["scenarios"].toArray().size() > 0);
  m_update = qEnvironmentVariableIntValue("PINGLYMOTION_UPDATE_SCENARIOS") == 1;
  m_updated = m_file["scenarios"].toArray();
}

void ScriptLabWholeTest::scenarios_data()
{
  QTest::addColumn<int>("index");
  const QJsonArray all = m_file["scenarios"].toArray();
  QSet<QString> names;
  for (int i = 0; i < all.size(); ++i)
  {
    const QString name = all[i].toObject()["name"].toString();
    QVERIFY2(!name.isEmpty() && !names.contains(name), qPrintable("scenario name missing or used twice: " + name));
    names.insert(name);
    QTest::newRow(qPrintable(name)) << i;
  }
}

void ScriptLabWholeTest::scenarios()
{
  QFETCH(int, index);
  QJsonObject scenario = m_updated[index].toObject();
  NoWindow host;
  ControlApi api(&host);

  // every scenario starts from an empty RLV state in the mode it asks for (RLVa unless it says "rlv")
  const QString mode = scenario.value("mode").toString("rlva");
  QVERIFY(api.execute(QJsonObject{{"command", "rlv_reset"}, {"mode", mode}})["ok"].toBool());

  QJsonArray steps = scenario.value("steps").toArray();
  QStringList failures;
  for (int s = 0; s < steps.size(); ++s)
  {
    QJsonObject step = steps[s].toObject();
    const QJsonObject request = withSources(step.value("request"), &m_used).toObject();
    QVERIFY2(!request["command"].toString().isEmpty(), qPrintable(QString("step %1 has no command").arg(s + 1)));
    const QJsonObject answer = api.execute(request);
    QJsonObject expect = step.value("expect").toObject();
    QVERIFY2(!expect.isEmpty(), qPrintable(QString("step %1 expects nothing").arg(s + 1)));
    for (auto it = expect.begin(); it != expect.end(); ++it)
    {
      const QJsonValue actual = at(answer, it.key());
      if (m_update) { it.value() = actual; continue; }
      if (actual != it.value())
        failures << QString("step %1 (%2), %3:\n    expected %4\n    actual   %5")
                        .arg(s + 1).arg(request["command"].toString(), it.key(), compact(it.value()), compact(actual));
    }
    step["expect"] = expect;
    steps[s] = step;
  }
  scenario["steps"] = steps;
  m_updated[index] = scenario;
  QVERIFY2(failures.isEmpty(), qPrintable(failures.join("\n")));
}

void ScriptLabWholeTest::everyExampleIsUsed()
{
  const QStringList examples = QDir(examplesFolder()).entryList({"*.lsl"}, QDir::Files, QDir::Name);
  QCOMPARE(examples.size(), 8);
  for (const QString& name : examples)
    QVERIFY2(m_used.contains(name), qPrintable(name + " is not used by any scenario in tests/data/scriptlab/scenarios.json"));
}

void ScriptLabWholeTest::cleanupTestCase()
{
  if (!m_update) return;
  m_file["scenarios"] = m_updated;
  QFile file(scenarioFile());
  QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
  file.write((pretty(m_file, 0) + "\n").toUtf8());
  qInfo("wrote %s", qPrintable(scenarioFile()));
}

QTEST_GUILESS_MAIN(ScriptLabWholeTest)
#include "tst_scriptlabwhole.moc"
