// Easing maths for the six easing choices (issue #53, step 1). See easing.h.
#include "easing.h"

#include <cmath>

namespace Easing
{
namespace
{
  const double Pi=3.14159265358979323846;
  // c1 of the "back" curve: s = 1 + (c1+1)(t-1)^3 + c1 (t-1)^2, peak 1 + 4 c1^3 / (27 (c1+1)^2)
  const double BackC1=1.5;

  // the sine ease that BVHNode::interpolate uses today, as a function of t=pos/steps
  double sine(double t) { return 0.5-std::cos(Pi*t)/2; }

  double clamp01(double t)
  {
    if(!(t>0)) return 0;   // also NaN
    if(t>1) return 1;
    return t;
  }

  double finiteOr(double v,double fallback) { return std::isfinite(v) ? v : fallback; }
}

const char* plainName(Preset preset)
{
  switch(preset)
  {
    case Preset::Even: return "Even";
    case Preset::SlowStart: return "Slow start";
    case Preset::SlowFinish: return "Slow finish";
    case Preset::SlowBothEnds: return "Slow both ends";
    case Preset::OvershootThenSettle: return "Overshoot then settle";
    case Preset::HoldThenSnap: return "Hold then snap";
  }
  return "";
}

const char* technicalName(Preset preset)
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

bool presetFromIndex(int index,Preset& preset)
{
  if(index<0 || index>=PresetCount) return false;
  preset=static_cast<Preset>(index);
  return true;
}

double s(Preset preset,double t)
{
  t=clamp01(t);
  if(t==0) return 0;
  if(t==1) return 1;

  switch(preset)
  {
    case Preset::Even: return t;
    // today: only the first half is eased, the second half is a straight line
    case Preset::SlowStart: return t<=0.5 ? sine(t) : t;
    // today: the first half is a straight line, only the second half is eased
    case Preset::SlowFinish: return t<=0.5 ? t : sine(t);
    case Preset::SlowBothEnds: return sine(t);
    case Preset::OvershootThenSettle:
    {
      const double u=t-1;
      return 1+(BackC1+1)*u*u*u+BackC1*u*u;
    }
    case Preset::HoldThenSnap: return 0;
  }
  return t;
}

double overshootPeak()
{
  return 1+4*BackC1*BackC1*BackC1/(27*(BackC1+1)*(BackC1+1));
}

double blend(double a,double b,double s)
{
  return a*(1-s)+b*s;
}

Vec3 blend(const Vec3& a,const Vec3& b,double s)
{
  Vec3 r;
  r.x=blend(a.x,b.x,s);
  r.y=blend(a.y,b.y,s);
  r.z=blend(a.z,b.z,s);
  return r;
}

Quat normalized(const Quat& q)
{
  const double len=std::sqrt(q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w);
  if(!std::isfinite(len) || len<1e-12) return Quat();
  Quat r;
  r.x=q.x/len; r.y=q.y/len; r.z=q.z/len; r.w=q.w/len;
  return r;
}

Quat nlerp(const Quat& a,const Quat& b,double s)
{
  const Quat qa=normalized(a);
  Quat qb=normalized(b);
  s=finiteOr(s,0);
  const double dot=qa.x*qb.x+qa.y*qb.y+qa.z*qb.z+qa.w*qb.w;
  if(dot<0) { qb.x=-qb.x; qb.y=-qb.y; qb.z=-qb.z; qb.w=-qb.w; }
  Quat r;
  r.x=blend(qa.x,qb.x,s);
  r.y=blend(qa.y,qb.y,s);
  r.z=blend(qa.z,qb.z,s);
  r.w=blend(qa.w,qb.w,s);
  const double len=std::sqrt(r.x*r.x+r.y*r.y+r.z*r.z+r.w*r.w);
  // opposite rotations blended half way give nothing to normalise: keep the start
  if(!std::isfinite(len) || len<1e-12) return qa;
  return normalized(r);
}

namespace
{
  double clampAngle(double v,double lo,double hi)
  {
    lo=finiteOr(lo,0);
    hi=finiteOr(hi,0);
    if(lo>hi) { const double tmp=lo; lo=hi; hi=tmp; }
    v=finiteOr(v,0);
    if(v<lo) return lo;
    if(v>hi) return hi;
    return v;
  }
}

Vec3 clampRotation(const Vec3& rotation,const Vec3& minimum,const Vec3& maximum)
{
  Vec3 r;
  r.x=clampAngle(rotation.x,minimum.x,maximum.x);
  r.y=clampAngle(rotation.y,minimum.y,maximum.y);
  r.z=clampAngle(rotation.z,minimum.z,maximum.z);
  return r;
}

std::vector<double> bake(Preset preset,int frames)
{
  if(frames<1) frames=1;
  std::vector<double> out;
  out.reserve(static_cast<size_t>(frames)+1);
  for(int i=0;i<=frames;i++)
    out.push_back(s(preset,static_cast<double>(i)/frames));
  return out;
}

std::vector<double> bakeValues(Preset preset,double a,double b,int frames)
{
  std::vector<double> out=bake(preset,frames);
  for(double& v : out) v=blend(a,b,v);
  return out;
}

std::vector<Quat> bakeRotations(Preset preset,const Quat& a,const Quat& b,int frames)
{
  const std::vector<double> speeds=bake(preset,frames);
  std::vector<Quat> out;
  out.reserve(speeds.size());
  for(double v : speeds) out.push_back(nlerp(a,b,v));
  return out;
}

Preset presetFromSwitches(bool startEaseOut,bool targetEaseIn)
{
  if(startEaseOut && targetEaseIn) return Preset::SlowBothEnds;
  if(startEaseOut) return Preset::SlowStart;
  if(targetEaseIn) return Preset::SlowFinish;
  return Preset::Even;
}

bool switchesFromPreset(Preset preset,bool& startEaseOut,bool& targetEaseIn)
{
  switch(preset)
  {
    case Preset::Even: startEaseOut=false; targetEaseIn=false; return true;
    case Preset::SlowStart: startEaseOut=true; targetEaseIn=false; return true;
    case Preset::SlowFinish: startEaseOut=false; targetEaseIn=true; return true;
    case Preset::SlowBothEnds: startEaseOut=true; targetEaseIn=true; return true;
    default: return false;
  }
}
}
