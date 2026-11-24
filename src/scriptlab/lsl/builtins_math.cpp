/*
 * PinglyMotion Script Lab - LSL interpreter: the built-in functions for rotations and maths
 *
 * What each function does follows the Second Life wiki and OpenSimulator (BSD); written from that
 * description, no code copied. See builtins.cpp for the tables and builtins_internal.h for the helpers.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "builtins_internal.h"

namespace lsl
{

using namespace builtin_detail;

Value Interpreter::callBuiltinMath(Fn fn, const QString& name, std::vector<Value>& a, bool& handled)
{
  handled = true;
  [[maybe_unused]] const qint32 DEBUG_CHANNEL = 0x7FFFFFFF;
  [[maybe_unused]] const QString NULL_KEY = "00000000-0000-0000-0000-000000000000";
  (void)name;

  switch (fn)
  {
    // -------------------------------------------- rotations (step 9b-1)
    case Fn::Euler2Rot: return rotationOf(eulerToQuat(a[0].f[0], a[0].f[1], a[0].f[2]));
    case Fn::Rot2Euler:
    {
      float e[3];
      quatToEuler(quatOf(a[0]), e);
      return Value::vector(e[0], e[1], e[2]);
    }
    case Fn::Rot2Fwd: return rotatedAxis(quatOf(a[0]), 1.0f, 0.0f, 0.0f);
    case Fn::Rot2Left: return rotatedAxis(quatOf(a[0]), 0.0f, 1.0f, 0.0f);
    case Fn::Rot2Up: return rotatedAxis(quatOf(a[0]), 0.0f, 0.0f, 1.0f);
    case Fn::AxisAngle2Rot: return rotationOf(axisAngleToQuat(a[0].f, a[1].f[0]));
    case Fn::RotBetween: return rotationOf(shortestArc(a[0].f, a[1].f));
    case Fn::Axes2Rot: return rotationOf(axesToQuat(a[0].f, a[1].f, a[2].f));
    case Fn::Rot2Axis:
    {
      // the wiki's reference: llVecNorm(<x, y, z>), turned round when s is negative; ZERO_ROTATION gives <0, 0, 0>
      const float x = a[0].f[0], y = a[0].f[1], z = a[0].f[2];
      const float m = std::sqrt(x * x + y * y + z * z);
      if (!(m > 0.0f) || !std::isfinite(m)) return Value::vector(0.0f, 0.0f, 0.0f);
      const float sign = a[0].f[3] < 0.0f ? -1.0f : 1.0f;
      return Value::vector(sign * x / m, sign * y / m, sign * z / m);
    }
    case Fn::AngleBetween:
    {
      // the wiki's reference: r = b / a, then the angle from r's scalar and vector parts, 0 to PI
      const Quat r = lslMul(quatOf(a[1]), conj(quatOf(a[0])));
      const float s2 = r.s * r.s, v2 = r.x * r.x + r.y * r.y + r.z * r.z;
      if (!(s2 + v2 > 0.0f)) return Value::number(0.0f);
      if (s2 < v2) return Value::number(2.0f * std::acos(std::sqrt(s2 / (s2 + v2))));
      if (v2 > 0.0f) return Value::number(2.0f * std::asin(std::sqrt(v2 / (s2 + v2))));
      return Value::number(0.0f);
    }

    // -------------------------------------------- maths
    case Fn::Abs: return Value::integer(a[0].i < 0 ? qint32(0u - quint32(a[0].i)) : a[0].i);
    case Fn::Fabs: return Value::number(std::fabs(a[0].f[0]));
    case Fn::Round: return Value::integer(floatToInteger(std::floor(a[0].f[0] + 0.5f)));
    case Fn::Floor: return Value::integer(floatToInteger(std::floor(a[0].f[0])));
    case Fn::Ceil: return Value::integer(floatToInteger(std::ceil(a[0].f[0])));
    case Fn::Sqrt: return Value::number(std::sqrt(a[0].f[0]));
    case Fn::Pow: return Value::number(std::pow(a[0].f[0], a[1].f[0]));
    case Fn::Sin: return Value::number(std::sin(a[0].f[0]));
    case Fn::Cos: return Value::number(std::cos(a[0].f[0]));
    case Fn::Tan: return Value::number(std::tan(a[0].f[0]));
    case Fn::Atan2: return Value::number(std::atan2(a[0].f[0], a[1].f[0]));
    case Fn::Frand: return Value::number(nextRandom() * a[0].f[0]);
    case Fn::Log: return Value::number(a[0].f[0] > 0.0f ? std::log(a[0].f[0]) : 0.0f);   // the wiki: 0 for a value of 0 or less
    case Fn::Asin: return Value::number(std::asin(a[0].f[0]));   // outside -1..1 the answer is NaN, as for llAcos

    default:
      break;
  }
  handled = false;
  return Value();
}

} // namespace lsl
