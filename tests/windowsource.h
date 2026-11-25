// The main window's source is split into several files by what each part does
// (pinglymotionwindow.cpp and pinglymotionwindow{build,view,joints}.cpp). The tests that
// check the window's code by reading its text read all four through this header, in the
// order below, so what they look for does not depend on which file a function is in.
#pragma once

#include <QFile>
#include <QFileInfo>
#include <QString>

namespace WindowSource
{
  // mainFile: the path of pinglymotionwindow.cpp; the other parts are in the same folder
  inline QString read(const QString& mainFile)
  {
    const QString folder = QFileInfo(mainFile).absolutePath();
    QString text;
    for (const char* name : {"pinglymotionwindow.cpp", "pinglymotionwindowbuild.cpp", "pinglymotionwindowview.cpp",
                             "pinglymotionwindowjoints.cpp"})
    {
      QFile file(folder + QLatin1Char('/') + QLatin1String(name));
      if (!file.open(QIODevice::ReadOnly)) return QString();
      text += QString::fromUtf8(file.readAll());
      text += QLatin1Char('\n');
    }
    return text;
  }
}
