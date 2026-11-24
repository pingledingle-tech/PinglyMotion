/*
 * PinglyMotion - test helper: a file write that is seen as a change at once (job T2)
 *
 * File times can be as coarse as one second, so a test that writes a file twice used to
 * wait a little over a second in between ("a new file time"). LaterFile::write writes the
 * file and gives it a modification time at least two seconds later than it had before and
 * than now, so the second write is a change without waiting. QtCore only.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef PINGLYMOTION_TESTS_LATERFILE_H
#define PINGLYMOTION_TESTS_LATERFILE_H

#include <QByteArray>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QString>

namespace LaterFile
{
  // gives an existing file a modification time at least two seconds later than before and than now
  inline bool touch(const QString& path, const QDateTime& before = QDateTime())
  {
    const QDateTime old = before.isValid() ? before : QFileInfo(path).lastModified();
    QDateTime later = QDateTime::currentDateTime().addSecs(2);
    if (old.isValid() && old.addSecs(2) > later) later = old.addSecs(2);
    QFile file(path);
    return file.open(QIODevice::ReadWrite) && file.setFileTime(later, QFileDevice::FileModificationTime);
  }

  // writes the file (replacing what was there) and moves its time on, as touch() does
  inline bool write(const QString& path, const QByteArray& text)
  {
    const QDateTime before = QFileInfo(path).lastModified();
    {
      QFile file(path);
      if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate) || file.write(text) != text.size()) return false;
    }
    return touch(path, before);
  }
}

#endif
