/*
 * PinglyMotion - glb2sl_bvh: convert a glTF/GLB animation to a Second Life BVH file (issue #27)
 *
 * Reads the file with the program's own glTF reader (src/gltfimport.cpp),
 * retargets it onto the Second Life skeleton with the retargeting core
 * (src/retarget.cpp) and writes BVH at 30 frames per second. Made for the
 * Mesh2Motion humanoid clips (CC0), used by hand and by the tests. It is built
 * with the tests (target glb2sl_bvh) and is not part of the program.
 *
 * Usage: glb2sl_bvh input.glb output.bvh [--target skeleton.bvh] [--animation N] [--fps N]
 *                  [--no-reference-frame]
 *
 * The target skeleton is the T-pose of the program (bin/data/TPose.bvh); the
 * default is the file given at build time. Second Life treats frame 0 of a BVH
 * file as a reference frame that is not played, so the tool puts the rest pose
 * of the target in front of the animation (as tools/cmu_to_sl.py does); the
 * option --no-reference-frame leaves it out. Notes go to stderr.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QCoreApplication>
#include <QFile>
#include <QTextStream>

#include "animfile.h"
#include "gltfimport.h"

#ifndef PINGLYMOTION_DEFAULT_TARGET
#define PINGLYMOTION_DEFAULT_TARGET ""
#endif

int main(int argc,char** argv)
{
  QCoreApplication app(argc,argv);
  QTextStream err(stderr);

  QStringList positional;
  QString targetPath=QStringLiteral(PINGLYMOTION_DEFAULT_TARGET);
  GltfImport::Options options;
  bool referenceFrame=true;
  const QStringList args=app.arguments().mid(1);
  for(int i=0;i<args.size();i++)
  {
    bool ok=true;
    if(args[i]=="--target" && i+1<args.size()) targetPath=args[++i];
    else if(args[i]=="--animation" && i+1<args.size()) options.animation=args[++i].toInt(&ok);
    else if(args[i]=="--fps" && i+1<args.size()) options.framesPerSecond=args[++i].toDouble(&ok);
    else if(args[i]=="--no-reference-frame") referenceFrame=false;
    else if(args[i].startsWith("--")) ok=false;
    else positional.append(args[i]);
    if(!ok)
    {
      err << "Bad option: " << args[i] << "\n";
      return 2;
    }
  }
  if(positional.size()!=2 || targetPath.isEmpty())
  {
    err << "Usage: glb2sl_bvh input.glb output.bvh [--target skeleton.bvh] [--animation N] [--fps N] [--no-reference-frame]\n";
    return 2;
  }

  QFile targetFile(targetPath);
  if(!targetFile.open(QIODevice::ReadOnly))
  {
    err << "Cannot read " << targetPath << "\n";
    return 1;
  }
  const AnimFile::Result target=AnimFile::parseBvh(targetFile.readAll());
  if(!target.ok())
  {
    err << "Not a valid target skeleton: " << target.error << "\n";
    return 1;
  }

  QFile in(positional[0]);
  if(!in.open(QIODevice::ReadOnly))
  {
    err << "Cannot read " << in.fileName() << "\n";
    return 1;
  }
  const GltfImport::Result result=GltfImport::importAnimation(in.readAll(),*target.animation,options);
  for(const QString& note : result.notes) err << "Note: " << note << "\n";
  if(!result.ok())
  {
    err << "Cannot convert: " << result.error << "\n";
    return 1;
  }

  AnimFile::Animation anim=*result.animation;
  if(referenceFrame && target.animation->frames>0 && target.animation->valuesPerFrame==anim.valuesPerFrame)
  {
    const auto first=target.animation->values.begin();
    anim.values.insert(anim.values.begin(),first,first+anim.valuesPerFrame);
    anim.frames++;
  }
  const QString problem=Retarget::validate(anim,true);
  if(!problem.isEmpty() || anim.frames*anim.frameTime>GltfImport::SL_MAX_SECONDS)
  {
    err << "Cannot convert: " << (problem.isEmpty() ? QStringLiteral("the animation is longer than 60 seconds") : problem) << "\n";
    return 1;
  }
  const QByteArray text=AnimFile::write(anim);
  QFile out(positional[1]);
  if(!out.open(QIODevice::WriteOnly|QIODevice::Truncate) || out.write(text)!=text.size())
  {
    err << "Cannot write " << out.fileName() << "\n";
    return 1;
  }
  return 0;
}
