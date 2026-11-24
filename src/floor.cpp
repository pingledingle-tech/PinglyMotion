/***************************************************************************
 *   Copyright (C) 2006 by Zi Ree   *
 *   Zi Ree @ SecondLife   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <qopengl.h>

#include "usersettings.h"
#include "viewcolours.h"

#include "floor.h"

Floor::Floor(QObject* parent)
  : QObject(parent)
{
  frameProtected = false;
}

Floor::~Floor()
{
}

Floor::DrawHook Floor::s_drawHook;

void Floor::setDrawHook(DrawHook hook)
{
  s_drawHook = std::move(hook);
}

void Floor::draw()
{
  float alpha = (100 - UserSettings::floorTranslucency()) / 100.0;

  if (s_drawHook)
  {
    // 20 x 20 tiles of 40 units, two triangles each, facing up
    // the theme's colours, asked for at every picture (src/viewcolours.h)
    QVector4D strong = ViewColours::vector(ViewColours::floorTile(true, frameProtected));
    QVector4D faint = ViewColours::vector(ViewColours::floorTile(false, frameProtected));
    strong.setW(alpha);
    faint.setW(alpha);
    QVector<QVector3D> positions;
    QVector<QVector3D> normals;
    QVector<QVector4D> colors;
    positions.reserve(20 * 20 * 6);
    for (int i = -10; i < 10; i++)
    {
      for (int j = -10; j < 10; j++)
      {
        QVector4D color = (i + j) % 2 ? faint : strong;
        const QVector3D a(i * 40, 0, j * 40);
        const QVector3D b(i * 40, 0, (j + 1) * 40);
        const QVector3D c((i + 1) * 40, 0, (j + 1) * 40);
        const QVector3D d((i + 1) * 40, 0, j * 40);
        for (const QVector3D& p : {a, b, c, a, c, d})
        {
          positions << p;
          normals << QVector3D(0, 1, 0);
          colors << color;
        }
      }
    }
    glEnable(GL_DEPTH_TEST);
    s_drawHook(positions, normals, colors);
  }
}

// set the current frame's visual protection status
void Floor::protectFrame(bool on)
{
  // only redraw if we need to
  if (frameProtected != on)
  {
    frameProtected = on;
    emit repaint();
  }
}
