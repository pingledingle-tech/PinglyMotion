// Easing maths for the six easing choices (issue #53, step 1).
//
// Pure functions: no disk access, no Qt types, nothing that changes the screen.
// Playback (BVHNode::interpolate, issue #53 step 2) uses segmentValue below,
// which is header-only so the program and every test that builds bvhnode.cpp
// need no extra source file. The rest is built into the tests only for now.
// The maths of the two ease switches is written down in docs/EASING.md; the
// first four choices give exactly those numbers, so old files play identically.
#ifndef EASING_H
#define EASING_H

#include <cmath>
#include <vector>

namespace Easing
{
  // The six choices, in the order of the tiles (docs/EASING.md).
  enum class Preset { Even, SlowStart, SlowFinish, SlowBothEnds, OvershootThenSettle, HoldThenSnap };

  const int PresetCount=6;

  // The default for a new key.
  inline Preset defaultPreset() { return Preset::SlowBothEnds; }

  // Name on the tile (approved names) and technical name. Never null.
  const char* plainName(Preset preset);
  const char* technicalName(Preset preset);

  // Preset from its number 0..5 (the order above); false if out of range.
  bool presetFromIndex(int index,Preset& preset);

  // Speed curve s(t) for t in [0,1]. t is clamped to [0,1] and NaN counts as 0.
  // s(0)==0 and s(1)==1 exactly, the result is always finite.
  // Monotone for every choice except OvershootThenSettle, which goes at most 10 %
  // past 1 and ends at exactly 1.
  double s(Preset preset,double t);

  // Largest value of s(t) over t in [0,1] for the overshoot choice (1.08).
  double overshootPeak();

  // Blend of two numbers: a at s=0, exactly b at s=1; s may be outside [0,1].
  double blend(double a,double b,double s);

  struct Vec3 { double x=0,y=0,z=0; };
  // Quaternion as in the Second Life viewer: x,y,z,w. Identity is 0,0,0,1.
  struct Quat { double x=0,y=0,z=0,w=1; };

  Vec3 blend(const Vec3& a,const Vec3& b,double s);

  // Normalised copy; a zero or non-finite quaternion gives the identity.
  Quat normalized(const Quat& q);

  // Blend of two rotations with nlerp like the viewer: shortest way (the sign of b
  // is flipped when the dot product is negative), straight blend, normalised.
  // The result is always a unit quaternion.
  Quat nlerp(const Quat& a,const Quat& b,double s);

  // Clamp a joint rotation (three angles in degrees) to its limits. A non-finite
  // angle counts as 0 before clamping. If a minimum is above its maximum the two
  // are swapped.
  Vec3 clampRotation(const Vec3& rotation,const Vec3& minimum,const Vec3& maximum);

  // Baked in-between speeds for a pose change that lasts `frames` frames:
  // frames+1 values s(i/frames), i=0..frames. First is 0, last is 1.
  // frames below 1 count as 1.
  std::vector<double> bake(Preset preset,int frames);

  // Baked in-between numbers from a to b (frames+1 values, first a, last exactly b).
  std::vector<double> bakeValues(Preset preset,double a,double b,int frames);

  // Baked in-between rotations (frames+1 unit quaternions) with nlerp.
  std::vector<Quat> bakeRotations(Preset preset,const Quat& a,const Quat& b,int frames);

  // The four old-style choices are the four combinations of the two switches
  // today: easeOut on the start key and easeIn on the target key.
  inline Preset presetFromSwitches(bool startEaseOut,bool targetEaseIn)
  {
    if(startEaseOut && targetEaseIn) return Preset::SlowBothEnds;
    if(startEaseOut) return Preset::SlowStart;
    if(targetEaseIn) return Preset::SlowFinish;
    return Preset::Even;
  }

  // The value of one channel at frame `pos` of a pose change from `from` to `to`
  // that lasts `steps` frames, as playback computes it (BVHNode::interpolate).
  // For the four old-style choices this is the program's formula unchanged, with
  // the same operations in the same order, so the result is bit for bit what the
  // program has always given (docs/EASING.md): the first half (pos <= steps/2,
  // whole-number division) is eased when the choice slows the start, the second
  // half when it slows the finish; eased means the sine S-curve with pi written
  // as 3.1415 (kept on purpose: the exact pi would move old animations by up to
  // about 0.003 % of each change), otherwise a straight line. from == to gives
  // from unchanged. The two new choices have no stored form yet (step 3), so
  // nothing in the program can pass them; until then they count as the default,
  // Slow both ends. steps must be at least 1 (the caller only calls this between
  // two different key frames).
  inline double segmentValue(Preset preset,double from,double to,int steps,int pos)
  {
    if(preset==Preset::OvershootThenSettle || preset==Preset::HoldThenSnap)
      preset=defaultPreset();

    const bool slowStart=(preset==Preset::SlowStart || preset==Preset::SlowBothEnds);
    const bool slowFinish=(preset==Preset::SlowFinish || preset==Preset::SlowBothEnds);

    // do not start any calculation if there's nothing to do
    if(from==to) return from;

    bool ease=false;
    if(pos<=(steps/2) && slowStart) ease=true;
    if(pos>(steps/2) && slowFinish) ease=true;

    if(ease)
    {
      // sine interpolation for ease in / out
      double distance=to-from;
      double step=3.1415/(steps);
      return from+(0.5-std::cos(step*(double) pos)/2)*distance;
    }
    // classic linear interpolation
    double distance=to-from;
    double increment=distance/(double) steps;
    return from+increment*(double) pos;
  }

  // Switches for a preset; false for the two choices that need a new stored value.
  bool switchesFromPreset(Preset preset,bool& startEaseOut,bool& targetEaseIn);
}

#endif
