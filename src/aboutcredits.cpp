#include "aboutcredits.h"

#include <QCoreApplication>

namespace
{
  const char* const addresses[] = {
    "github.com/RuthAndRoth/Ruth2",
    "github.com/RuthAndRoth/Roth2",
  };
}

QString AboutCredits::plainText()
{
  return QCoreApplication::translate("AboutCredits",
    "Avatar body: Ruth2 v4 by Shin Ingen, Ada Radius and the RuthAndRoth contributors "
    "(github.com/RuthAndRoth/Ruth2). The mesh is licensed under the GNU Affero General "
    "Public License 3 or later; the UV map is CC BY Linden Lab and the rig parts are "
    "CC BY 3.0 Machinimatrix.org. Converted to PinglyMotion's own format. Licences and "
    "the full list of contributors: data/character/ruth2/. Male body: Roth2 v2 by Shin "
    "Ingen, Ada Radius and the RuthAndRoth contributors (github.com/RuthAndRoth/Roth2), "
    "AGPL 3 or later; the head is CC BY Linden Lab. Licences: data/character/roth2/. "
    "Optional Bento body: \"Maxine\" from the Max Avatar Project, Creative Commons "
    "Attribution 4.0 License, see data/character/max/LICENSE.txt");
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
  };
}
