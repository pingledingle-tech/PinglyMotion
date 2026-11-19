/*
 * PinglyMotion - animation editor for Second Life avatars
 *
 * Start folder of the file dialogs. Path logic only, no windows.
 *   - the remembered folder wins when it still exists;
 *   - otherwise the folder "examples" next to the program, when it exists
 *     (the Windows zip has it);
 *   - otherwise the remembered folder as it is (a Linux dev build keeps its old behaviour).
 */
#ifndef STARTFOLDER_H
#define STARTFOLDER_H

#include <QCoreApplication>
#include <QDir>
#include <QString>

namespace StartFolder
{
inline QString choose(const QString& remembered,const QString& appDir)
{
  if(!remembered.isEmpty() && QDir(remembered).exists()) return remembered;
  const QString examples=QDir(appDir).filePath("examples");
  if(QDir(examples).exists()) return examples;
  return remembered;
}

inline QString forDialogs(const QString& remembered)
{
  return choose(remembered,QCoreApplication::applicationDirPath());
}
}

#endif
