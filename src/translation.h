/*
 * PinglyMotion - loading of the translations (docs/TRANSLATION.md)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef TRANSLATION_H
#define TRANSLATION_H

#include <QString>
#include <QStringList>

class QCoreApplication;

// The program's texts are English in the source code. A translation is a file
// pinglymotion_<language>.qm (made from translations/pinglymotion_<language>.ts) in the
// program's resources (":/i18n") or in a folder "translations" next to the program, and
// Qt's own texts (qtbase_<language>.qm: standard buttons, file dialogs) are loaded the same way.
namespace Translation
{
  // The language codes to try, best first, for a setting: "system" gives the computer's
  // languages, anything else that one language. "nb_NO" is tried as "nb_NO" then "nb";
  // "-" and "_" are the same. English and the "C" locale are not in the list: they need no file.
  QStringList candidates(const QString& setting, const QStringList& systemLanguages);

  // Folders searched for translation files, in this order.
  QStringList defaultFolders();

  // Installs the best translation for the setting into the application. Returns the code
  // that was loaded, or an empty string when the program stays English. Nothing is
  // installed twice: a second call replaces what the first one loaded.
  QString install(QCoreApplication& application, const QString& setting,
                  const QStringList& folders = defaultFolders(),
                  const QStringList& systemLanguages = QStringList());
}

#endif
