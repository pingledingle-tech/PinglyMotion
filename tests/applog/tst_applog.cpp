/*
 * PinglyMotion - animation editor for Second Life avatars
 *
 * Tests of the local error log (src/applog.*, issue 239): the 5 MB limit drops the
 * oldest lines, the home folder is replaced, the clean-exit marker and the crash
 * detection at the next start, emptying the log, and the two Help menu entries
 * "Open log" and "Clear log" (their exact labels and place in the main window's
 * .ui file). Every test writes into a temporary folder, never into the real
 * application data folder.
 */
#include <QFile>
#include <QLoggingCategory>
#include <QTemporaryDir>
#include <QXmlStreamReader>
#include <QtTest>

#include "applog.h"

Q_LOGGING_CATEGORY(lcTest, "pinglymotion.test")

namespace
{
QByteArray readAll(const QString& path)
{
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly))
    return QByteArray();
  return file.readAll();
}

AppLog::Options optionsFor(const QTemporaryDir& dir, const QString& home = QStringLiteral("/home/someone"))
{
  AppLog::Options options;
  options.directory = dir.path();
  options.homePath = home;
  options.crashHandlers = false;
  options.forwardToConsole = false;
  return options;
}
}

class TestAppLog : public QObject
{
  Q_OBJECT

private slots:
  void cleanup() { AppLog::uninstall(); }

  void limitIsFiveMegabytes() { QCOMPARE(AppLog::defaultMaxBytes, qint64(5 * 1024 * 1024)); }

  void trimKeepsNewestWholeLines()
  {
    const QByteArray content("line one\nline two\nline three\n");
    QCOMPARE(AppLog::trimToLimit(content, 1000), content);
    QCOMPARE(AppLog::trimToLimit(content, 11), QByteArray("line three\n"));
    QCOMPARE(AppLog::trimToLimit(content, 20), QByteArray("line two\nline three\n"));
    QCOMPARE(AppLog::trimToLimit(content, 19), QByteArray("line three\n"));
    QCOMPARE(AppLog::trimToLimit(content, 3), QByteArray());
    QCOMPARE(AppLog::trimToLimit(content, 0), QByteArray());
  }

  void fileStaysUnderFiveMegabytesAndDropsOldestLines()
  {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QVERIFY(AppLog::install(optionsFor(dir)));
    const QString padding(80, QLatin1Char('x'));
    const int count = 80000; // about 8 MB of lines
    for (int i = 0; i < count; ++i)
      qCWarning(lcTest, "entry %06d %s", i, qPrintable(padding));
    const QByteArray content = readAll(AppLog::filePath());
    QVERIFY2(content.size() <= AppLog::defaultMaxBytes, QByteArray::number(content.size()).constData());
    QVERIFY(content.size() > AppLog::defaultMaxBytes / 2);
    QVERIFY(content.endsWith('\n'));
    QVERIFY(content.contains("entry 079999 "));
    QVERIFY(!content.contains("entry 000000 "));
    QVERIFY(!content.contains("start PinglyMotion")); // the oldest line went first
    // the file begins with a whole line: a time stamp, then an entry
    QVERIFY(content.at(0) >= '0' && content.at(0) <= '9');
    QVERIFY(content.left(content.indexOf('\n')).contains(" warning pinglymotion.test: entry "));
  }

  void homeFolderIsReplaced()
  {
    QCOMPARE(AppLog::scrubPersonalData(QStringLiteral("cannot read /home/someone/anims/wave.bvh"),
                                       QStringLiteral("/home/someone")),
             QStringLiteral("cannot read <home>/anims/wave.bvh"));
    QCOMPARE(AppLog::scrubPersonalData(QStringLiteral("cannot read /home/someone/x"), QStringLiteral("/home/someone/")),
             QStringLiteral("cannot read <home>/x"));
    QCOMPARE(AppLog::scrubPersonalData(QStringLiteral("open C:\\Users\\someone\\Desktop\\a.dingle and C:/Users/someone/b"),
                                       QStringLiteral("C:\\Users\\someone")),
             QStringLiteral("open <home>\\Desktop\\a.dingle and <home>/b"));
    QCOMPARE(AppLog::scrubPersonalData(QStringLiteral("/home/other/x"), QStringLiteral("/home/someone")),
             QStringLiteral("/home/other/x"));
    // a root folder as home would hide every path: left as it is
    QCOMPARE(AppLog::scrubPersonalData(QStringLiteral("/tmp/x"), QStringLiteral("/")), QStringLiteral("/tmp/x"));
  }

  void homeFolderIsReplacedInTheFile()
  {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QVERIFY(AppLog::install(optionsFor(dir)));
    qCWarning(lcTest, "cannot read %s", "/home/someone/anims/wave.bvh");
    const QByteArray content = readAll(AppLog::filePath());
    QVERIFY(content.contains("warning pinglymotion.test: cannot read <home>/anims/wave.bvh"));
    QVERIFY(!content.contains("someone"));
  }

  void cleanExitAndCrashDetection()
  {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString marker = dir.filePath(QStringLiteral("running"));

    // first start: no marker before, so no crash; the marker exists while running
    QVERIFY(AppLog::install(optionsFor(dir)));
    QVERIFY(!AppLog::lastSessionCrashed());
    QVERIFY(QFile::exists(marker));
    AppLog::markCleanExit();
    QVERIFY(!QFile::exists(marker));
    AppLog::uninstall();

    // clean last session: no crash reported
    QVERIFY(AppLog::install(optionsFor(dir)));
    QVERIFY(!AppLog::lastSessionCrashed());
    AppLog::uninstall(); // ends without markCleanExit: like a crash

    // the next start knows and says so in the log
    QVERIFY(AppLog::install(optionsFor(dir)));
    QVERIFY(AppLog::lastSessionCrashed());
    const QByteArray content = readAll(AppLog::filePath());
    QCOMPARE(content.count("the last session did not close cleanly"), 1);
    QCOMPARE(content.count("clean exit"), 1);
    QCOMPARE(content.count("start PinglyMotion"), 3);
  }

  void clearEmptiesTheFile()
  {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QVERIFY(!AppLog::clear()); // not installed
    QVERIFY(AppLog::install(optionsFor(dir)));
    qCWarning(lcTest, "something");
    QVERIFY(QFileInfo(AppLog::filePath()).size() > 0);
    QVERIFY(AppLog::clear());
    QCOMPARE(QFileInfo(AppLog::filePath()).size(), qint64(0));
    qCWarning(lcTest, "after clearing");
    QVERIFY(readAll(AppLog::filePath()).contains("after clearing"));
  }

  // Help menu of src/pinglymotionwindow.ui: What's New, Open log, Clear log, About,
  // in that order, with exactly these labels
  void helpMenuHasLogEntries()
  {
    QFile ui(QStringLiteral(PINGLYMOTION_WINDOW_UI));
    QVERIFY2(ui.open(QIODevice::ReadOnly), PINGLYMOTION_WINDOW_UI);
    QXmlStreamReader xml(&ui);
    QStringList helpActions;
    QMap<QString, QString> texts;
    bool inHelp = false;
    QString action;
    while (!xml.atEnd())
    {
      xml.readNext();
      if (xml.isStartElement())
      {
        const QString name = xml.name().toString();
        const QString objectName = xml.attributes().value(QStringLiteral("name")).toString();
        if (name == QStringLiteral("widget") && objectName == QStringLiteral("menuHelp"))
          inHelp = true;
        else if (inHelp && name == QStringLiteral("addaction"))
          helpActions << objectName;
        else if (name == QStringLiteral("action"))
          action = objectName;
        else if (!action.isEmpty() && name == QStringLiteral("property") && objectName == QStringLiteral("text"))
        {
          xml.readNextStartElement();
          texts[action] = xml.readElementText();
        }
      }
      else if (xml.isEndElement())
      {
        if (inHelp && xml.name() == QStringLiteral("widget"))
          inHelp = false;
        else if (xml.name() == QStringLiteral("action"))
          action.clear();
      }
    }
    QVERIFY(!xml.hasError());
    QCOMPARE(helpActions, QStringList({QStringLiteral("helpWhatsNewAction"), QStringLiteral("helpOpenLogAction"),
                                       QStringLiteral("helpClearLogAction"), QStringLiteral("helpAboutAction")}));
    QCOMPARE(texts.value(QStringLiteral("helpOpenLogAction")), QStringLiteral("Open log"));
    QCOMPARE(texts.value(QStringLiteral("helpClearLogAction")), QStringLiteral("Clear log"));
  }

  void defaultFolderIsInApplicationData()
  {
    QVERIFY(AppLog::defaultDirectory().endsWith(QStringLiteral("/PinglyMotion/logs")));
  }
};

QTEST_GUILESS_MAIN(TestAppLog)
#include "tst_applog.moc"
