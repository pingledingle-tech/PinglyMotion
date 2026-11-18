/*
 * Pingmator - reads the answer from GitHub's "latest release" API
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef RELEASEINFO_H
#define RELEASEINFO_H

#include <QByteArray>
#include <QString>

namespace ReleaseInfo
{
  // where the program asks for the newest release
  QString apiUrl();
  // the page a user downloads the newest release from
  // version number of the release in GitHub's JSON answer ("v0.2.0" gives "0.2.0");
  // empty if the answer is not a release
  QString latestVersion(const QByteArray& json);
}

#endif
