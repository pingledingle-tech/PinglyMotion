/*
 * PinglyMotion - animation editor for Second Life avatars
 *
 * The program's local error log (issue 239). See applog.h.
 *
 * The log file is opened with the C runtime (open/_wopen) and written with
 * write/_write, so the fatal signal handler can write to the same file with
 * async-signal-safe calls only.
 */
#include "applog.h"

#include "constants.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QStandardPaths>
#include <QSysInfo>

#include <atomic>
#include <csignal>
#include <cstring>
#include <exception>

#ifdef Q_OS_WIN
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace
{
const char* const logFileName = "pinglymotion.log";
const char* const markerFileName = "running";

struct State
{
  QMutex mutex;
  bool installed = false;
  QString directory;
  QString filePath;
  QString markerPath;
  QString homePath;
  qint64 maxBytes = AppLog::defaultMaxBytes;
  qint64 size = 0;
  bool crashed = false;
  bool crashHandlers = false;
  bool forwardToConsole = true;
  QtMessageHandler previousHandler = nullptr;
  std::terminate_handler previousTerminate = nullptr;
};

State& state()
{
  static State s;
  return s;
}

// the file descriptor the signal handler writes to; -1 while closed or being trimmed
std::atomic<int> logFd{-1};

thread_local bool inHandler = false;

const int fatalSignals[] = {
  SIGSEGV, SIGABRT, SIGFPE, SIGILL,
#ifndef Q_OS_WIN
  SIGBUS,
#endif
};

int openAppend(const QString& path)
{
#ifdef Q_OS_WIN
  return _wopen(reinterpret_cast<const wchar_t*>(QDir::toNativeSeparators(path).utf16()),
                _O_WRONLY | _O_APPEND | _O_CREAT | _O_BINARY, _S_IREAD | _S_IWRITE);
#else
  return ::open(QFile::encodeName(path).constData(), O_WRONLY | O_APPEND | O_CREAT | O_CLOEXEC, 0600);
#endif
}

void closeFd(int fd)
{
  if (fd < 0)
    return;
#ifdef Q_OS_WIN
  _close(fd);
#else
  ::close(fd);
#endif
}

// async-signal-safe: plain write, no allocation
void writeRaw(int fd, const char* data, size_t length)
{
  while (fd >= 0 && length > 0)
  {
#ifdef Q_OS_WIN
    const int written = _write(fd, data, static_cast<unsigned int>(length));
#else
    const ssize_t written = ::write(fd, data, length);
#endif
    if (written <= 0)
      return;
    data += written;
    length -= static_cast<size_t>(written);
  }
}

// rewrites the file with its newest lines so that "incoming" more bytes still fit;
// called with the mutex held
void trimFile(State& s, qint64 incoming)
{
  const int fd = logFd.exchange(-1);
  closeFd(fd);
  QFile file(s.filePath);
  QByteArray content;
  if (file.open(QIODevice::ReadOnly))
    content = file.readAll();
  file.close();
  // keep room for more lines, so the file is not rewritten on every line
  const qint64 target = qMax<qint64>(0, s.maxBytes * 3 / 4 - incoming);
  const QByteArray kept = AppLog::trimToLimit(content, target);
  if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
  {
    file.write(kept);
    file.close();
    s.size = kept.size();
  }
  logFd.store(openAppend(s.filePath));
}

// writes one finished line (with '\n'); called with the mutex held
void writeLine(State& s, const QString& line)
{
  const QByteArray bytes = AppLog::scrubPersonalData(line, s.homePath).toUtf8() + '\n';
  if (s.size + bytes.size() > s.maxBytes)
    trimFile(s, bytes.size());
  // a single line longer than the whole log is cut
  const qint64 length = qMin<qint64>(bytes.size(), s.maxBytes);
  writeRaw(logFd.load(), bytes.constData(), static_cast<size_t>(length));
  s.size += length;
}

QString timestamp()
{
  return QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
}

const char* levelName(QtMsgType type)
{
  switch (type)
  {
    case QtDebugMsg: return "debug";
    case QtInfoMsg: return "info";
    case QtWarningMsg: return "warning";
    case QtCriticalMsg: return "error";
    case QtFatalMsg: return "fatal";
  }
  return "?";
}

void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message)
{
  State& s = state();
  if (!inHandler)
  {
    inHandler = true;
    QString line = timestamp() + QLatin1Char(' ') + QLatin1String(levelName(type)) + QLatin1Char(' ');
    if (context.category && std::strcmp(context.category, "default") != 0)
      line += QLatin1String(context.category) + QLatin1String(": ");
    line += message;
    // only the source file name, never the folder it was built in
    if (context.file)
      line += QLatin1String(" (") + QFileInfo(QString::fromUtf8(context.file)).fileName() + QLatin1Char(':')
              + QString::number(context.line) + QLatin1Char(')');
    {
      QMutexLocker lock(&s.mutex);
      if (s.installed)
        writeLine(s, line);
    }
    inHandler = false;
  }
  if (s.forwardToConsole && s.previousHandler)
    s.previousHandler(type, context, message);
}

void terminateHandler()
{
  QString what = QStringLiteral("unknown exception");
  if (std::exception_ptr current = std::current_exception())
  {
    try
    {
      std::rethrow_exception(current);
    }
    catch (const std::exception& e)
    {
      what = QString::fromUtf8(e.what());
    }
    catch (...)
    {
      what = QStringLiteral("exception that is not a std::exception");
    }
  }
  else
    what = QStringLiteral("std::terminate called without an exception");
  {
    State& s = state();
    QMutexLocker lock(&s.mutex);
    if (s.installed)
      writeLine(s, timestamp() + QStringLiteral(" fatal uncaught C++ exception: ") + what);
  }
  std::terminate_handler previous = state().previousTerminate;
  if (previous && previous != terminateHandler)
    previous();
  std::abort();
}

// best effort and async-signal-safe: a fixed text and the signal number, then the
// default action (the program ends as it would without this handler)
extern "C" void fatalSignalHandler(int signalNumber)
{
  static const char prefix[] = "fatal signal ";
  char number[16];
  int length = 0;
  int value = signalNumber < 0 ? 0 : signalNumber;
  char reversed[16];
  do
  {
    reversed[length++] = static_cast<char>('0' + value % 10);
    value /= 10;
  } while (value > 0 && length < 15);
  for (int i = 0; i < length; ++i)
    number[i] = reversed[length - 1 - i];
  static const char suffix[] = " (the program crashed)\n";
  const int fd = logFd.load();
  writeRaw(fd, prefix, sizeof(prefix) - 1);
  writeRaw(fd, number, static_cast<size_t>(length));
  writeRaw(fd, suffix, sizeof(suffix) - 1);
  std::signal(signalNumber, SIG_DFL);
  std::raise(signalNumber);
}
}

namespace AppLog
{
QString defaultDirectory()
{
  return QDir::cleanPath(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
                         + QStringLiteral("/PinglyMotion/logs"));
}

QString scrubPersonalData(const QString& text, const QString& homePath)
{
  // both separators, on every system (a Windows path can show up in a Linux log too)
  QString home = homePath;
  home.replace(QLatin1Char('\\'), QLatin1Char('/'));
  home = QDir::cleanPath(home);
  while (home.endsWith(QLatin1Char('/')))
    home.chop(1);
  // a root or a very short "home" would hide too much
  if (home.size() < 2)
    return text;
#ifdef Q_OS_WIN
  const Qt::CaseSensitivity cs = Qt::CaseInsensitive;
#else
  const Qt::CaseSensitivity cs = Qt::CaseSensitive;
#endif
  QString result = text;
  const QString placeholder = QLatin1String(homePlaceholder());
  result.replace(home, placeholder, cs);
  QString backslashed = home;
  backslashed.replace(QLatin1Char('/'), QLatin1Char('\\'));
  if (backslashed != home)
    result.replace(backslashed, placeholder, cs);
  return result;
}

QByteArray trimToLimit(const QByteArray& content, qint64 maxBytes)
{
  if (content.size() <= maxBytes)
    return content;
  if (maxBytes <= 0)
    return QByteArray();
  // start at the first whole line inside the last maxBytes
  const qsizetype cut = content.size() - maxBytes;
  if (cut > 0 && content.at(cut - 1) == '\n')
    return content.mid(cut);
  const qsizetype newline = content.indexOf('\n', cut);
  if (newline < 0)
    return QByteArray();
  return content.mid(newline + 1);
}

bool install(const Options& options)
{
  State& s = state();
  {
    QMutexLocker lock(&s.mutex);
    if (s.installed)
      return true;
    s.directory = options.directory.isEmpty() ? defaultDirectory() : options.directory;
    s.homePath = options.homePath.isEmpty() ? QDir::homePath() : options.homePath;
    s.maxBytes = options.maxBytes > 0 ? options.maxBytes : defaultMaxBytes;
    s.forwardToConsole = options.forwardToConsole;
    if (!QDir().mkpath(s.directory))
      return false;
    s.filePath = QDir(s.directory).filePath(QLatin1String(logFileName));
    s.markerPath = QDir(s.directory).filePath(QLatin1String(markerFileName));
    const int fd = openAppend(s.filePath);
    if (fd < 0)
      return false;
    logFd.store(fd);
    s.size = QFileInfo(s.filePath).size();
    s.installed = true;
    if (s.size > s.maxBytes)
      trimFile(s, 0);

    s.crashed = QFile::exists(s.markerPath);
    writeLine(s, timestamp() + QStringLiteral(" info start PinglyMotion ") + Constants::buildLabel()
                     + QStringLiteral(", ") + QSysInfo::prettyProductName() + QStringLiteral(", Qt ")
                     + QLatin1String(qVersion()));
    if (s.crashed)
      writeLine(s, timestamp()
                       + QStringLiteral(" warning the last session did not close cleanly (crash or forced end)"));
    QFile marker(s.markerPath);
    if (marker.open(QIODevice::WriteOnly | QIODevice::Truncate))
      marker.close();
  }

  s.previousHandler = qInstallMessageHandler(messageHandler);
  s.crashHandlers = options.crashHandlers;
  if (s.crashHandlers)
  {
    s.previousTerminate = std::set_terminate(terminateHandler);
    for (int signalNumber : fatalSignals)
      std::signal(signalNumber, fatalSignalHandler);
  }
  return true;
}

void markCleanExit()
{
  State& s = state();
  QMutexLocker lock(&s.mutex);
  if (!s.installed)
    return;
  writeLine(s, timestamp() + QStringLiteral(" info clean exit"));
  QFile::remove(s.markerPath);
}

void uninstall()
{
  State& s = state();
  if (!s.installed)
    return;
  qInstallMessageHandler(s.previousHandler);
  if (s.crashHandlers)
  {
    std::set_terminate(s.previousTerminate);
    for (int signalNumber : fatalSignals)
      std::signal(signalNumber, SIG_DFL);
  }
  QMutexLocker lock(&s.mutex);
  closeFd(logFd.exchange(-1));
  s.installed = false;
  s.crashHandlers = false;
  s.previousHandler = nullptr;
  s.previousTerminate = nullptr;
  s.crashed = false;
  s.filePath.clear();
  s.markerPath.clear();
  s.size = 0;
}

QString filePath()
{
  State& s = state();
  QMutexLocker lock(&s.mutex);
  return s.installed ? s.filePath : QString();
}

bool lastSessionCrashed()
{
  State& s = state();
  QMutexLocker lock(&s.mutex);
  return s.crashed;
}

bool clear()
{
  State& s = state();
  QMutexLocker lock(&s.mutex);
  if (!s.installed)
    return false;
  closeFd(logFd.exchange(-1));
  QFile file(s.filePath);
  const bool ok = file.open(QIODevice::WriteOnly | QIODevice::Truncate);
  file.close();
  s.size = ok ? 0 : QFileInfo(s.filePath).size();
  logFd.store(openAppend(s.filePath));
  return ok && logFd.load() >= 0;
}
}
