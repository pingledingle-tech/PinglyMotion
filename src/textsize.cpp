/*
 * PinglyMotion - the program's text size, 100 to 200 percent
 * Licensed under the GNU General Public License version 2 or later.
 */
#include <QApplication>

#include <cmath>

#include "textsize.h"

namespace
{
  int g_percent=TextSize::Default;
  bool g_haveBase=false;
  QFont g_base;
}

int TextSize::bounded(int percent)
{
  if(percent<Minimum) return Minimum;
  if(percent>Maximum) return Maximum;
  return percent;
}

QFont TextSize::scaledFont(const QFont& base,int percent)
{
  const int p=bounded(percent);
  QFont font=base;
  if(base.pointSizeF()>0)
    font.setPointSizeF(base.pointSizeF()*p/100.0);
  else if(base.pixelSize()>0)
    font.setPixelSize(int(std::lround(base.pixelSize()*p/100.0)));
  return font;
}

int TextSize::stepUp(int percent)
{
  return bounded(bounded(percent)+25);
}

int TextSize::stepDown(int percent)
{
  return bounded(bounded(percent)-25);
}

int TextSize::apply(int percent)
{
  const int p=bounded(percent);
  if(!g_haveBase)
  {
    g_base=QApplication::font();
    g_haveBase=true;
  }
  g_percent=p;
  const QFont font=scaledFont(g_base,p);
  if(!(QApplication::font()==font)) QApplication::setFont(font);
  return p;
}

int TextSize::percent()
{
  return g_percent;
}

int TextSize::scaled(int size,int percent)
{
  return int(std::lround(size*bounded(percent)/100.0));
}

int TextSize::scaled(int size)
{
  return scaled(size,g_percent);
}

void TextSize::resetForTests()
{
  if(g_haveBase) QApplication::setFont(g_base);
  g_haveBase=false;
  g_percent=Default;
}
