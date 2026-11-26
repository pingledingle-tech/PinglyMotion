/*
 * PinglyMotion
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "aboutcredits.h"

#include <QCoreApplication>

namespace
{
  const char* const addresses[] = {
    "github.com/RuthAndRoth/Ruth2",
    "github.com/RuthAndRoth/Roth2",
    "github.com/Mesh2Motion/mesh2motion-app",
    "github.com/pingledingle-tech/PinglyMotion/blob/main/CREDITS.md",
  };
}

QString AboutCredits::plainText()
{
  return QCoreApplication::translate("AboutCredits",
    "Thank you to everyone whose work PinglyMotion uses or learned from: QAvimator and "
    "Avimator, the Second Life and Firestorm viewers, the RuthAndRoth bodies, the CMU "
    "motion capture database, Mesh2Motion and Aurora. The full list is in CREDITS.md: "
    "github.com/pingledingle-tech/PinglyMotion/blob/main/CREDITS.md. "
    "Avatar body: Ruth2 v4 by Shin Ingen, Ada Radius and the RuthAndRoth contributors "
    "(github.com/RuthAndRoth/Ruth2). The mesh is licensed under the GNU Affero General "
    "Public License 3 or later; the UV map is CC BY Linden Lab and the rig parts are "
    "CC BY 3.0 Machinimatrix.org. Converted to PinglyMotion's own format. Licences and "
    "the full list of contributors: data/character/ruth2/. Male body: Roth2 v2 by Shin "
    "Ingen, Ada Radius and the RuthAndRoth contributors (github.com/RuthAndRoth/Roth2), "
    "AGPL 3 or later; the head is CC BY Linden Lab. Licences: data/character/roth2/. "
    "Optional Bento body: \"Maxine\" from the Max Avatar Project, Creative Commons "
    "Attribution 4.0 License, see data/character/max/LICENSE.txt. "
    "Test motions and reference library: CC0 clips from Mesh2Motion "
    "(github.com/Mesh2Motion/mesh2motion-app), CC0 1.0. Thank you to the Mesh2Motion project. "
    "Motion capture clips in the reference library and the test motions: "
    "The data used in this project was obtained from mocap.cs.cmu.edu. "
    "The database was created with funding from NSF EIA-0196217. "
    "CMU Graphics Lab Motion Capture Database.");
}

QString AboutCredits::html()
{
  QString text=plainText().toHtmlEscaped();
  for (const char* address : addresses)
    text.replace(QLatin1String(address),
                 QString("<a href=\"https://%1\">%1</a>").arg(address));
  return "<p>"+text+"</p>";
}

QStringList AboutCredits::requiredEntries()
{
  return {
    "Ruth2 v4", "Shin Ingen", "Ada Radius", "RuthAndRoth contributors",
    "github.com/RuthAndRoth/Ruth2", "GNU Affero General Public License 3 or later",
    "CC BY Linden Lab", "CC BY 3.0 Machinimatrix.org", "data/character/ruth2/",
    "Roth2 v2", "github.com/RuthAndRoth/Roth2", "AGPL 3 or later",
    "data/character/roth2/",
    "Maxine", "Max Avatar Project", "Creative Commons Attribution 4.0 License",
    "data/character/max/LICENSE.txt",
    "Thank you to everyone whose work PinglyMotion uses or learned from",
    "QAvimator and Avimator", "CREDITS.md",
    "github.com/pingledingle-tech/PinglyMotion/blob/main/CREDITS.md",
    "Mesh2Motion", "github.com/Mesh2Motion/mesh2motion-app", "CC0 1.0",
    "CMU Graphics Lab Motion Capture Database", "mocap.cs.cmu.edu", "NSF EIA-0196217",
  };
}
