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

#ifndef FLOOR_H
#define FLOOR_H

#include <QObject>
#include <QVector>
#include <QVector3D>
#include <QVector4D>

#include <functional>

class Floor : public QObject
{
  Q_OBJECT

  public:
    explicit Floor(QObject* parent = nullptr);
    ~Floor();

    void draw();

    void protectFrame(bool on);

    // The 3D view draws the floor with its shaders: triangles, normals and
    // colours. Without a hook nothing is drawn.
    using DrawHook = std::function<bool(const QVector<QVector3D>& positions,
                                        const QVector<QVector3D>& normals,
                                        const QVector<QVector4D>& colors)>;
    static void setDrawHook(DrawHook hook);

  signals:
    void repaint();

  private:
    bool frameProtected;
    static DrawHook s_drawHook;
};

#endif
