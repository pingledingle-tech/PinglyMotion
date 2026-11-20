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
#include <optional>
#include <string_view>
#include <vector>

namespace Easing
{
  // The six choices, in the order of the tiles (docs/EASING.md).
  enum class Preset { Even, SlowStart, SlowFinish, SlowBothEnds, OvershootThenSettle, HoldThenSnap };

  const int PresetCount=6;

  // The default for a new key.
  inline Preset defaultPreset() { return Preset::SlowBothEnds; }

  // Name on the tile (approved names). Never null.
  const char* plainName(Preset preset);

  // Technical name, also the "type" of a key's "curve" in .dingle files
  // (docs/DINGLE_FORMAT.md). Never null. Header-only, so the .dingle reader and
  // writer need no extra source file.
  inline const char* technicalName(Preset preset)
  {
    switch(preset)
    {
      case Preset::Even: return "linear";
      case Preset::SlowStart: return "ease-in";
      case Preset::SlowFinish: return "ease-out";
      case Preset::SlowBothEnds: return "ease-in-out";
      case Preset::OvershootThenSettle: return "ease-out-overshoot";
      case Preset::HoldThenSnap: return "constant";
    }
    return "";
  }

  // Preset from its technical name (exact, case sensitive, the whole text: a
  // zero byte inside does not end it); false if unknown.
  inline bool presetFromTechnicalName(std::string_view name,Preset& preset)
  {
    for(int i=0;i<6;i++)
    {
      const Preset p=static_cast<Preset>(i);
      if(name==std::string_view(technicalName(p))) { preset=p; return true; }
    }
    return false;
  }

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

  // The choice of one segment (pose change) between two keys. A choice stored on
  // the start key (the "curve" of a .dingle key, issue #53 step 3) wins; without
  // one, the start key's easeOut and the target key's easeIn pick one of the four
  // old-style choices, as they always did.
  inline Preset segmentPreset(const std::optional<Preset>& startCurve,bool startEaseOut,bool targetEaseIn)
  {
    if(startCurve) return *startCurve;
    return presetFromSwitches(startEaseOut,targetEaseIn);
  }

  // c1 of the "back" curve of OvershootThenSettle:
  // s = 1 + (c1+1)(t-1)^3 + c1 (t-1)^2, peak 1 + 4 c1^3 / (27 (c1+1)^2)
  const double OvershootC1=1.5;

  // s(t) of OvershootThenSettle for t in [0,1] (s() clamps t first).
  inline double overshootSpeed(double t)
  {
    const double u=t-1;
    return 1+(OvershootC1+1)*u*u*u+OvershootC1*u*u;
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
  // from unchanged. The two new choices (stored as a key's "curve" since issue
  // #53 step 3) follow s(): Overshoot then settle goes up to 8 % past the target
  // and comes back, Hold then snap stays at `from` until the target frame. No
  // joint limits are applied here. steps must be at least 1 (the caller only
  // calls this between two different key frames).
  inline double segmentValue(Preset preset,double from,double to,int steps,int pos)
  {
    // do not start any calculation if there's nothing to do
    if(from==to) return from;

    if(preset==Preset::HoldThenSnap)
      return pos>=steps ? to : from;
    if(preset==Preset::OvershootThenSettle)
    {
      if(pos<=0) return from;
      if(pos>=steps) return to;
      const double s=overshootSpeed(static_cast<double>(pos)/steps);
      return from+(to-from)*s;
    }

    const bool slowStart=(preset==Preset::SlowStart || preset==Preset::SlowBothEnds);
    const bool slowFinish=(preset==Preset::SlowFinish || preset==Preset::SlowBothEnds);

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

  // ------------------------------------------------------------ custom curve (issue #11)

  // A free speed curve for one pose change, drawn in the curve editor: a cubic
  // Bezier from (0,0) to (1,1) with the two handles (x1,y1) and (x2,y2), as in
  // CSS cubic-bezier(). x is time, y is how far the movement has come. Stored
  // on the start key as a .dingle "curve" of type "bezier" (docs/DINGLE_FORMAT.md).
  struct Bezier
  {
    double x1=0.37,y1=0,x2=0.63,y2=1;
    bool operator==(const Bezier& other) const
    { return x1==other.x1 && y1==other.y1 && x2==other.x2 && y2==other.y2; }
    bool operator!=(const Bezier& other) const { return !(*this==other); }
  };

  // How far the handles may go up and down: at most half a movement past
  // either pose. Time (x) stays inside [0,1], so the curve never goes back.
  const double BezierMinY=-0.5;
  const double BezierMaxY=1.5;

  // The handles inside their limits: x clamped to [0,1], y to
  // [BezierMinY,BezierMaxY]; a non-finite number gives the default handle value.
  inline Bezier sanitized(const Bezier& b)
  {
    const Bezier d;
    auto fix=[](double v,double fallback,double low,double high)
    {
      if(!std::isfinite(v)) v=fallback;
      return v<low ? low : (v>high ? high : v);
    };
    Bezier r;
    r.x1=fix(b.x1,d.x1,0,1);
    r.y1=fix(b.y1,d.y1,BezierMinY,BezierMaxY);
    r.x2=fix(b.x2,d.x2,0,1);
    r.y2=fix(b.y2,d.y2,BezierMinY,BezierMaxY);
    return r;
  }

  // True when every number is finite and inside the limits above.
  inline bool isValid(const Bezier& b)
  {
    return std::isfinite(b.x1) && std::isfinite(b.y1) && std::isfinite(b.x2) && std::isfinite(b.y2) &&
           sanitized(b)==b;
  }

  // One coordinate of the cubic at parameter u, for handles a and b.
  inline double bezierCoordinate(double a,double b,double u)
  {
    const double v=1-u;
    return 3*v*v*u*a+3*v*u*u*b+u*u*u;
  }

  // Speed curve of a custom curve: s(t) for t in [0,1] (clamped, NaN counts
  // as 0). s(0)==0 and s(1)==1 exactly. The handles are sanitized first. The
  // time coordinate rises with u because x1 and x2 lie in [0,1], so u is found
  // by bisection (fixed 64 steps: the same result on every machine).
  inline double bezierSpeed(const Bezier& curve,double t)
  {
    if(!(t>0)) return 0;
    if(t>=1) return 1;
    const Bezier b=sanitized(curve);
    double low=0,high=1;
    for(int i=0;i<64;i++)
    {
      const double mid=(low+high)/2;
      if(bezierCoordinate(b.x1,b.x2,mid)<t) low=mid; else high=mid;
    }
    return bezierCoordinate(b.y1,b.y2,(low+high)/2);
  }

  // The value of one channel at frame `pos` of a pose change from `from` to
  // `to` that lasts `steps` frames, with a custom curve: exactly `from` at the
  // start and exactly `to` at the target frame. No joint limits are applied
  // here (BVHNode::frameData does that). steps below 1 count as 1.
  inline double customSegmentValue(const Bezier& curve,double from,double to,int steps,int pos)
  {
    if(from==to) return from;
    if(steps<1) steps=1;
    if(pos<=0) return from;
    if(pos>=steps) return to;
    return from+(to-from)*bezierSpeed(curve,static_cast<double>(pos)/steps);
  }

  // The handles the curve editor opens with for a named choice: the same shape
  // for Even, close matches for the others (the named choices are not all
  // cubic curves, docs/DINGLE_FORMAT.md).
  inline Bezier bezierFromPreset(Preset preset)
  {
    switch(preset)
    {
      case Preset::Even: return {1.0/3,1.0/3,2.0/3,2.0/3};
      case Preset::SlowStart: return {0.42,0,1,1};
      case Preset::SlowFinish: return {0,0,0.58,1};
      case Preset::SlowBothEnds: return {0.37,0,0.63,1};
      case Preset::OvershootThenSettle: return {0.3,1.3,0.6,1};
      case Preset::HoldThenSnap: return {1,0,1,0};
    }
    return {};
  }

  // Baked custom curve: frames+1 speeds s(i/frames), first 0, last 1; frames
  // below 1 count as 1.
  inline std::vector<double> bakeCustom(const Bezier& curve,int frames)
  {
    if(frames<1) frames=1;
    std::vector<double> out(static_cast<size_t>(frames)+1);
    for(int i=0;i<=frames;i++) out[static_cast<size_t>(i)]=(i==frames) ? 1.0 : bezierSpeed(curve,static_cast<double>(i)/frames);
    return out;
  }
}

#endif
