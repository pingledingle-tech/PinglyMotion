/*
 * PinglyMotion - conversion between slider steps and values
 *
 * The rotation and position sliders in the Properties panel work in whole
 * steps of 0.01 (degrees or units). These helpers convert in both directions
 * without losing the decimals: divide as a double, and round (not truncate)
 * when going back to a slider step (issue #192).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef SLIDERSCALE_H
#define SLIDERSCALE_H

#include <cmath>

namespace SliderScale
{
  // slider steps per degree or unit
  constexpr int precision = 100;

  // slider step to value, e.g. 1234 -> 12.34
  inline double toValue(int sliderValue)
  {
    return sliderValue / static_cast<double>(precision);
  }

  // value to the nearest slider step, e.g. 12.34 -> 1234 (12.34 * 100 is 1233.999...)
  inline int toSlider(double value)
  {
    return static_cast<int>(std::lround(value * precision));
  }
}

#endif
