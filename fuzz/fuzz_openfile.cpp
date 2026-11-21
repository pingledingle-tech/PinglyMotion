/*
 * PinglyMotion - fuzz target for opening a file end to end (issue 250)
 *
 * Writes any bytes to a file named .bvh, .avm and .dingle in turn, and opens
 * it the way File > Open and the remote control do: BVH::checkFile() first,
 * then a whole Animation from it (BVH::animRead builds the joint tree, the
 * limits are applied, the animation is put on the Bento reference skeleton).
 * It must never crash or hang, a refused file must give a reason, and an
 * opened animation must have at least one frame and a skeleton.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <cstdio>
#include <cstdlib>

#include <QFile>
#include <QGuiApplication>
#include <QTemporaryDir>

#include "animation.h"
#include "animfile.h"
#include "bvh.h"
#include "usersettings.h"

namespace
{

void check(bool ok)
{
  if (!ok) abort();
}

// the parser and the animation write a lot of debug text; keep the fuzzer output readable
void quiet(QtMsgType type, const QMessageLogContext&, const QString& message)
{
  if (type == QtFatalMsg)
  {
    fprintf(stderr, "%s\n", qPrintable(message));
    abort();
  }
}

BVH* g_bvh = nullptr;
QTemporaryDir* g_folder = nullptr;

void openAs(const QByteArray& bytes, const char* suffix)
{
  const QString file = g_folder->filePath(QString("input.") + suffix);
  {
    QFile out(file);
    check(out.open(QIODevice::WriteOnly | QIODevice::Truncate));
    check(out.write(bytes) == bytes.size());
  }
  QString error;
  if (!BVH::checkFile(file, &error))
  {
    check(!error.isEmpty());
    return;
  }
  Animation animation(g_bvh, file);
  check(animation.getMotion() != nullptr);
  check(animation.getNumberOfFrames() >= 1);
  // walking every frame of the opened animation must stay inside its data
  for (int frame = 0; frame < animation.getNumberOfFrames() && frame < 64; frame++)
    animation.setFrame(frame);
}

} // namespace

extern "C" int LLVMFuzzerInitialize(int* argc, char*** argv)
{
  qInstallMessageHandler(quiet);
  if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) qputenv("QT_QPA_PLATFORM", "offscreen");
  static QGuiApplication app(*argc, *argv);
  UserSettings::setJointLimits(true);
  g_bvh = new BVH();
  g_folder = new QTemporaryDir();
  check(g_folder->isValid());
  return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const unsigned char* data, size_t size)
{
  if (size > static_cast<size_t>(AnimFile::MAX_FILE_BYTES) + 1) return -1;
  const QByteArray bytes(reinterpret_cast<const char*>(data), static_cast<qsizetype>(size));
  openAs(bytes, "bvh");
  openAs(bytes, "avm");
  openAs(bytes, "dingle");
  return 0;
}
