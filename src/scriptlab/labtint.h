/*
 * PinglyMotion Script Lab - props as prims (lane 1, job E)
 *
 * When the folder runs as one object (labfolder.h), its scripts can give a
 * prim a colour and a transparency (llSetColor, llSetAlpha, llSetLinkColor,
 * llSetLinkAlpha, PRIM_COLOR). A prop in the scene with the same name as the
 * prim shows them while "Show effects in 3D view" is ticked: the prop stands
 * in for the prim. fromFolder() reads the run's log and gives, for every prim
 * a script coloured, the last colour and alpha given to the whole prim
 * (ALL_SIDES) or to face 0. Props have one colour, so other faces are left out.
 *
 * QtCore only.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef LABTINT_H
#define LABTINT_H

#include <QJsonObject>
#include <QList>
#include <QString>

namespace LabTint
{
  struct Tint
  {
    QString prim;            // the prim's name: its folder's name (the object's name for a root without "Root")
    int link = 1;
    bool hasColour = false;  // a script set the colour (red, green, blue from 0 to 1)
    float colour[3] = {1.0f, 1.0f, 1.0f};
    bool hasAlpha = false;   // a script set the transparency (1 solid, 0 clear)
    float alpha = 1.0f;
    bool operator==(const Tint& other) const;
  };

  // The tints of a folder run (LabWatcher::folderResult()); empty when the folder did not run.
  // In link order.
  QList<Tint> fromFolder(const QJsonObject& folder);
}

#endif
