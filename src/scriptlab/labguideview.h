/*
 * PinglyMotion - the guide page of the Help view (job UX-D)
 *
 * The project owner, 24 November 2026: a whole guide page with samples and guidance in the Help
 * view, in plain words for someone new to animation and to scripting. It is a view of its own,
 * opened from the Help view (labhelpview.h) with the button "Guide page"; help does not sit
 * inside a function view.
 *
 * The page has: Start here, Animate in five steps (with the sample animations), Script Lab in five
 * steps, a picture of the Script Lab example folder with every file labelled, and a button that
 * makes that folder where the user chooses.
 *
 * The picture is built from the real example folder (the one the button copies), so it cannot go
 * out of date: a new file in the example shows up at once, with a label by its kind.
 *
 * The button never overwrites. When the folder name is taken in the chosen place, it makes
 * "<name> 2", "<name> 3" and so on. The copy is LabBeginner::copyExample, the same code as the
 * Make example project button.
 *
 * The picture is drawn with labels and icons (not colour alone), wraps its text, and sits in a
 * scroll area, so it works at 200 percent text size.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#ifndef LABGUIDEVIEW_H
#define LABGUIDEVIEW_H

#include <functional>

#include <QDialog>
#include <QList>
#include <QString>

#include "labbeginner.h"

class QLabel;
class QPushButton;
class QScrollArea;
class QVBoxLayout;

namespace LabGuide
{
  // one line of the folder picture
  struct FolderRow
  {
    QString path;     // relative to the example folder; "" is the example folder itself
    int depth = 0;    // 0 for the example folder, 1 for what is directly in it, and so on
    bool isFolder = false;
    QString label;    // what it is, in plain words
  };

  // every folder and file that the copy of the example holds, in the order of the picture. Uses the
  // same rules as LabBeginner::copyExample (scripts, notecards and example.json, two folders deep).
  QList<FolderRow> folderRows(const LabExample& example);

  // the name for a new folder: the example's own name, or with " 2", " 3" and so on when the name
  // is taken in root; empty when none of the first 99 is free
  QString freeName(const QString& root, const QString& object);

  // makes the example folder in root without touching anything that is there; returns its path, or
  // empty with the reason in error
  QString makeExampleFolder(const LabExample& example, const QString& root, QString* error);
}

class LabGuideView : public QDialog
{
  Q_OBJECT

  public:
    // examplesFolder: examples/objects (the first example in it is the one the page shows)
    explicit LabGuideView(const QString& examplesFolder, QWidget* parent = nullptr);

    // the picture's lines as shown: "<kind> <name>" and the label, one entry per row
    QList<LabGuide::FolderRow> pictureRows() const { return m_rows; }
    QStringList pictureNames() const;     // the name shown on each row
    QStringList pictureLabels() const;    // the label shown on each row
    // the whole page as plain text (for tests and screen readers)
    QString pageText() const;
    QStringList sampleFiles() const { return m_samples; }

    QPushButton* makeButton() const { return m_make; }
    QPushButton* openButton() const { return m_open; }
    QPushButton* closeButton() const { return m_close; }
    QLabel* statusLabel() const { return m_status; }
    QScrollArea* scrollArea() const { return m_scroll; }
    // the folder that was made last ("" when none)
    QString madeFolder() const { return m_made; }

    // the place where the folder is made: the button asks with a folder dialog; tests give their own
    void setFolderChooser(const std::function<QString()>& chooser) { m_chooser = chooser; }
    // makes the folder in root (what the button does after the user has chosen); true when made
    bool makeIn(const QString& root);

    void applyStyle(bool dark);

  signals:
    // the user pressed "Open in Script Lab" for the folder that was made
    void openRequested(const QString& folder);

  protected:
    void changeEvent(QEvent* event) override;

  private:
    void build();
    void chooseAndMake();
    QLabel* addText(QVBoxLayout* into, const QString& html, const QString& name = QString());
    QLabel* addHeading(QVBoxLayout* into, const QString& text, const QString& name = QString());

    QString m_examplesFolder;
    LabExample m_example;
    QList<LabGuide::FolderRow> m_rows;
    QStringList m_samples;
    QList<QWidget*> m_pictureLines;
    QWidget* m_picture = nullptr;
    QScrollArea* m_scroll = nullptr;
    QPushButton* m_make = nullptr;
    QPushButton* m_open = nullptr;
    QPushButton* m_close = nullptr;
    QLabel* m_status = nullptr;
    QString m_made;
    std::function<QString()> m_chooser;
    bool m_dark = true;
    bool m_styling = false;
};

#endif
