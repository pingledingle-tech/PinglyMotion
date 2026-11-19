// The credits and licence text of Help > About (issue 106).
// Kept apart from the dialog so a test can check it without a window.

#ifndef ABOUTCREDITS_H
#define ABOUTCREDITS_H

#include <QString>
#include <QStringList>

namespace AboutCredits
{
  // The credits as plain text: every entry in order, separated by a space.
  QString plainText();

  // The same text as HTML for a QTextBrowser; the project addresses are links.
  QString html();

  // Phrases that must be in the text (body, licences, data folders).
  QStringList requiredEntries();
}

#endif
