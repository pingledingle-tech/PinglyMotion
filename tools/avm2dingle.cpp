/*
 * PinglyMotion - avm2dingle: convert an old .avm file to .dingle (issue #26)
 *
 * Reads the AVM with the program's own parser (src/animfile.cpp) and writes
 * it with the .dingle writer (src/dingle.cpp). Used once, by hand, for the four
 * bundled QAvimator files. It is built only from these two units, with the
 * tests (target avm2dingle), and is not part of the program.
 *
 * Usage: avm2dingle input.avm output.dingle
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QCoreApplication>
#include <QFile>
#include <QTextStream>

#include "animfile.h"
#include "dingle.h"

int main(int argc,char** argv)
{
  QCoreApplication app(argc,argv);
  QTextStream err(stderr);

  if(argc!=3)
  {
    err << "Usage: avm2dingle input.avm output.dingle\n";
    return 2;
  }

  QFile in(QString::fromLocal8Bit(argv[1]));
  if(!in.open(QIODevice::ReadOnly))
  {
    err << "Cannot read " << in.fileName() << "\n";
    return 1;
  }
  const AnimFile::Result parsed=AnimFile::parseAvm(in.readAll());
  if(!parsed.animation)
  {
    err << "Not a valid AVM file: " << parsed.error << "\n";
    return 1;
  }

  const QByteArray text=Dingle::write(*parsed.animation);

  QFile out(QString::fromLocal8Bit(argv[2]));
  if(!out.open(QIODevice::WriteOnly|QIODevice::Truncate) || out.write(text)!=text.size())
  {
    err << "Cannot write " << out.fileName() << "\n";
    return 1;
  }
  return 0;
}
