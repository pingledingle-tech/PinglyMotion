/*
 * PinglyMotion - loading of the translations (docs/TRANSLATION.md)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "translation.h"

#include <QCoreApplication>
#include <QDir>
#include <QLibraryInfo>
#include <QLocale>
#include <QTranslator>

namespace
{
  const QString programBase = QStringLiteral("pinglymotion");
  const QString qtBase = QStringLiteral("qtbase");

  // translators of the last install, removed again by the next one
  QTranslator* programTranslator = nullptr;
  QTranslator* qtTranslator = nullptr;

  void removeOld(QCoreApplication& application)
  {
    for (QTranslator** t : {&programTranslator, &qtTranslator})
    {
      if (!*t) continue;
      application.removeTranslator(*t);
      delete *t;
      *t = nullptr;
    }
  }

  bool isEnglish(const QString& code)
  {
    return code == QLatin1String("en") || code.startsWith(QLatin1String("en_"));
  }

  // true and the translator filled when <base>_<code>.qm is in one of the folders
  bool loadFrom(QTranslator& translator, const QString& base, const QString& code, const QStringList& folders)
  {
    for (const QString& folder : folders)
      if (translator.load(base + QLatin1Char('_') + code, folder)) return true;
    return false;
  }
}

QStringList Translation::candidates(const QString& setting, const QStringList& systemLanguages)
{
  QStringList source;
  if (setting.isEmpty() || setting.compare(QLatin1String("system"), Qt::CaseInsensitive) == 0)
    source = systemLanguages;
  else
    source << setting;

  QStringList result;
  for (QString language : source)
  {
    language = language.trimmed();
    language.replace(QLatin1Char('-'), QLatin1Char('_'));
    // codes are folder and file name parts: letters, digits and "_" only, so a setting can
    // never point outside the translation folders
    if (language.isEmpty() || language.size() > 32) continue;
    bool clean = true;
    for (const QChar c : language)
      if (c.unicode() > 127 || !(c.isLetterOrNumber() || c == QLatin1Char('_'))) clean = false;
    if (!clean) continue;
    if (language == QLatin1String("C") || language == QLatin1String("POSIX")) continue;
    // "nb_NO" then "nb"
    for (QString part = language; !part.isEmpty();)
    {
      if (isEnglish(part)) return result;   // English is the source text: nothing after it is wanted
      if (!result.contains(part)) result << part;
      const int cut = part.lastIndexOf(QLatin1Char('_'));
      if (cut <= 0) break;
      part.truncate(cut);
    }
  }
  return result;
}

QStringList Translation::defaultFolders()
{
  QStringList folders;
  folders << QStringLiteral(":/i18n");
  if (QCoreApplication::instance())
    folders << QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("translations"));
  return folders;
}

QString Translation::install(QCoreApplication& application, const QString& setting,
                             const QStringList& folders, const QStringList& systemLanguages)
{
  removeOld(application);

  const QStringList codes = candidates(setting,
    systemLanguages.isEmpty() ? QLocale::system().uiLanguages() : systemLanguages);

  for (const QString& code : codes)
  {
    auto* translator = new QTranslator;
    if (!loadFrom(*translator, programBase, code, folders))
    {
      delete translator;
      continue;
    }
    programTranslator = translator;
    application.installTranslator(programTranslator);

    // Qt's own texts, from the folders and from the Qt installation; missing is fine
    auto* qt = new QTranslator;
    QStringList qtFolders = folders;
    qtFolders << QLibraryInfo::path(QLibraryInfo::TranslationsPath);
    if (loadFrom(*qt, qtBase, code, qtFolders))
    {
      qtTranslator = qt;
      application.installTranslator(qtTranslator);
    }
    else
      delete qt;
    return code;
  }
  return QString();
}
