/*
 * Pingmator - reads the answer from GitHub's "latest release" API
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QJsonDocument>
#include <QJsonObject>

#include "releaseinfo.h"
#include "versionnumber.h"

QString ReleaseInfo::apiUrl()
{
  return QString("https://api.github.com/repos/pingledingle-tech/Pingmator/releases/latest");
}

QString ReleaseInfo::latestVersion(const QByteArray& json)
{
  const QJsonDocument document = QJsonDocument::fromJson(json);
  if (!document.isObject()) return QString();

  const QJsonObject release = document.object();
  // drafts and pre-releases are not offered as updates
  if (release.value("draft").toBool() || release.value("prerelease").toBool()) return QString();

  QString tag = release.value("tag_name").toString().trimmed();
  if (tag.startsWith('v') || tag.startsWith('V')) tag.remove(0, 1);

  if (!VersionNumber(tag).isValid()) return QString();
  return tag;
}
