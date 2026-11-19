/*
 * PinglyMotion - fuzz target for the glTF import (src/gltfimport.*, issue #12)
 *
 * Feeds any bytes through the whole import: GltfImport::read(), sampling to
 * frames (toSource) with both rest poses, and retargeting onto a small
 * Second Life skeleton. It must never crash or hang, a refused file must give
 * a reason, and every accepted result must keep the rules of a parsed
 * animation (Retarget::validate: joint and frame limits, finite numbers).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "animfile.h"
#include "gltfimport.h"
#include "retarget.h"

static void check(bool ok)
{
  if (!ok) abort();
}

// the old 19 joint Second Life skeleton (the names of bin/data/TPose.bvh), Y up, one frame
static const AnimFile::Animation& target()
{
  static const AnimFile::Animation anim = [] {
    const char* text =
      "HIERARCHY\nROOT hip\n{\nOFFSET 0 0 0\nCHANNELS 6 Xposition Yposition Zposition Xrotation Zrotation Yrotation\n"
      "JOINT abdomen\n{\nOFFSET 0 3.4 0\nCHANNELS 3 Xrotation Zrotation Yrotation\n"
      "JOINT chest\n{\nOFFSET 0 8.5 -0.7\nCHANNELS 3 Xrotation Zrotation Yrotation\n"
      "JOINT neck\n{\nOFFSET 0 10.3 -0.3\nCHANNELS 3 Xrotation Zrotation Yrotation\n"
      "JOINT head\n{\nOFFSET 0 3.1 0\nCHANNELS 3 Xrotation Zrotation Yrotation\nEnd Site\n{\nOFFSET 0 3.1 0\n}\n}\n}\n"
      "JOINT lCollar\n{\nOFFSET 3.4 6.7 -0.8\nCHANNELS 3 Yrotation Zrotation Xrotation\n"
      "JOINT lShldr\n{\nOFFSET 3.4 0 0\nCHANNELS 3 Zrotation Yrotation Xrotation\n"
      "JOINT lForeArm\n{\nOFFSET 10.1 0 0\nCHANNELS 3 Yrotation Zrotation Xrotation\n"
      "JOINT lHand\n{\nOFFSET 8.1 0 0\nCHANNELS 3 Zrotation Yrotation Xrotation\nEnd Site\n{\nOFFSET 4 0 0\n}\n}\n}\n}\n}\n"
      "JOINT rCollar\n{\nOFFSET -3.4 6.7 -0.8\nCHANNELS 3 Yrotation Zrotation Xrotation\n"
      "JOINT rShldr\n{\nOFFSET -3.4 0 0\nCHANNELS 3 Zrotation Yrotation Xrotation\n"
      "JOINT rForeArm\n{\nOFFSET -10.1 0 0\nCHANNELS 3 Yrotation Zrotation Xrotation\n"
      "JOINT rHand\n{\nOFFSET -8.1 0 0\nCHANNELS 3 Zrotation Yrotation Xrotation\nEnd Site\n{\nOFFSET -4 0 0\n}\n}\n}\n}\n}\n"
      "}\n}\n"
      "JOINT lThigh\n{\nOFFSET 4.1 -1.4 1.3\nCHANNELS 3 Xrotation Zrotation Yrotation\n"
      "JOINT lShin\n{\nOFFSET -2 -18.9 0\nCHANNELS 3 Xrotation Zrotation Yrotation\n"
      "JOINT lFoot\n{\nOFFSET 0.3 -17.9 -1\nCHANNELS 3 Xrotation Yrotation Zrotation\nEnd Site\n{\nOFFSET 0 -2 6\n}\n}\n}\n}\n"
      "JOINT rThigh\n{\nOFFSET -4.1 -1.4 1.3\nCHANNELS 3 Xrotation Zrotation Yrotation\n"
      "JOINT rShin\n{\nOFFSET 2 -18.9 0\nCHANNELS 3 Xrotation Zrotation Yrotation\n"
      "JOINT rFoot\n{\nOFFSET -0.3 -17.9 -1\nCHANNELS 3 Xrotation Yrotation Zrotation\nEnd Site\n{\nOFFSET 0 -2 6\n}\n}\n}\n}\n"
      "}\nMOTION\nFrames: 1\nFrame Time: 0.04\n";
    QByteArray bvh(text);
    for (int i = 0; i < 6 + 18 * 3; i++) bvh += "0 ";
    bvh += "\n";
    const AnimFile::Result r = AnimFile::parseBvh(bvh);
    if (!r.ok()) abort();
    return *r.animation;
  }();
  return anim;
}

static void checkSource(const GltfImport::SourceResult& s)
{
  if (!s.ok())
  {
    check(!s.error.isEmpty());
    return;
  }
  check(s.error.isEmpty());
  const AnimFile::Animation& a = *s.animation;
  check(Retarget::validate(a, true).isEmpty());
  check(a.joints.size() >= 1 && a.joints.size() <= GltfImport::MAX_JOINTS);
  check(a.frames >= 1 && a.frames <= GltfImport::MAX_FRAMES);
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
  const QByteArray bytes(reinterpret_cast<const char*>(data), static_cast<qsizetype>(size));
  const GltfImport::ReadResult r = GltfImport::read(bytes);
  if (!r.ok())
  {
    check(!r.error.isEmpty());
    return 0;
  }
  check(r.error.isEmpty());
  const GltfImport::Document& doc = *r.document;
  check(doc.nodes.size() <= GltfImport::MAX_NODES);
  check(doc.skins.size() <= GltfImport::MAX_SKINS);
  check(doc.animations.size() <= GltfImport::MAX_ANIMATIONS);

  for (int a = 0; a < doc.animations.size() && a < 2; a++)
  {
    const GltfImport::Clip& clip = doc.animations[a];
    check(clip.start >= 0.0 && clip.end >= clip.start && clip.end <= GltfImport::MAX_KEY_TIME);
    GltfImport::Options options;
    options.animation = a;
    // keep each input fast: at most about 300 frames
    const double length = clip.end - clip.start;
    if (length * options.framesPerSecond > 300.0)
      options.framesPerSecond = std::max(GltfImport::MIN_FRAMES_PER_SECOND, 300.0 / length);
    checkSource(GltfImport::toSource(doc, options));
    options.restPose = GltfImport::RestPose::BindMatrices;
    checkSource(GltfImport::toSource(doc, options));

    options.restPose = GltfImport::RestPose::Nodes;
    const GltfImport::Result imported = GltfImport::importAnimation(bytes, target(), options);
    if (imported.ok())
    {
      check(imported.error.isEmpty());
      check(imported.animation->joints.size() == target().joints.size());
      check(Retarget::validate(*imported.animation, true).isEmpty());
    }
    else check(!imported.error.isEmpty());
  }
  return 0;
}
