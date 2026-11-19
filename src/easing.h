// Easing maths for the six easing choices (issue #53, step 1).
//
// Pure functions: no disk access, no Qt types, nothing that changes the screen.
// It is built into the tests only; the program does not call it yet.
// The maths of the two ease switches as they work today is written down in
// docs/EASING.md; the first four choices are defined to give exactly those
// numbers, so old files can later play identically.
#ifndef EASING_H
#define EASING_H

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
  Preset presetFromSwitches(bool startEaseOut,bool targetEaseIn);

  // Switches for a preset; false for the two choices that need a new stored value.
  bool switchesFromPreset(Preset preset,bool& startEaseOut,bool& targetEaseIn);
}

#endif
