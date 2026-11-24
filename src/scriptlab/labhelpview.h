/*
 * PinglyMotion - Script Lab help, out of the working parts (UX small fixes)
 *
 * The project owner, 24 November 2026: "How to lay out this folder" looked like a function, and
 * help should not sit inside the function. The ? button in the Script Lab panel's top row opens
 * this bigger help view instead. Topics on the left, the text on the right:
 *
 *   Start here, How to lay out a folder (the maker guide that used to be a folding part of the
 *   Live scripts page), Words (the word tips), Hints, and one topic per example in
 *   examples/objects with its guide texts from example.json, word for word.
 *
 * The view does not block the window, so a user can read a step and do it. Up and Down move
 * between topics, Esc or Close closes it.
 *
 * LabHighlight is the short frame that shows where something is: the step button "Show Live
 * scripts" puts it round the Live scripts tab (and round the top of that page), so a press always
 * shows something, also when the page is already open. It goes away by itself after a moment.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#ifndef LABHELPVIEW_H
#define LABHELPVIEW_H

#include <QDialog>
#include <QList>
#include <QPointer>
#include <QRect>
#include <QStringList>
#include <QWidget>

#include "labguideview.h"

class QLabel;
class QListWidget;
class QPushButton;
class QTextBrowser;


class LabHelpView : public QDialog
{
  Q_OBJECT

  public:
    // examplesFolder: examples/objects, where each example folder has its example.json
    explicit LabHelpView(const QString& examplesFolder, QWidget* parent = nullptr);

    // the topics, in the order of the list: "start", "folder", "words", "hints", then
    // "example:<object>" for each example (by its order)
    QStringList topicKeys() const { return m_keys; }
    // the names in the list, in the same order (the heading "Examples" is not a topic)
    QStringList topicTitles() const;
    QString currentTopic() const;
    // shows a topic; false when there is no such topic
    bool showTopic(const QString& key);
    // the shown topic's heading and its text as plain text (for tests and screen readers)
    QString topicHeading() const;
    QString topicText() const;

    QListWidget* topicList() const { return m_list; }
    QTextBrowser* textView() const { return m_text; }
    QLabel* headingLabel() const { return m_heading; }
    QPushButton* closeButton() const { return m_close; }
    // the guide page (job UX-D, labguideview.h): a view of its own, opened with this button
    QPushButton* guideButton() const { return m_guideButton; }
    LabGuideView* guideView() const { return m_guideView; }
    void showGuide();

    // colours again after the theme changed
    void applyStyle(bool dark);

  signals:
    // the guide page made the example folder and the user pressed "Open in Script Lab"
    void openFolderRequested(const QString& folder);

  protected:
    void changeEvent(QEvent* event) override;

  private:
    void fill();
    QString topicHtml(const QString& key) const;

    QString m_examplesFolder;
    QStringList m_keys;
    QListWidget* m_list = nullptr;
    QLabel* m_heading = nullptr;
    QTextBrowser* m_text = nullptr;
    QPushButton* m_close = nullptr;
    QPushButton* m_guideButton = nullptr;
    QPointer<LabGuideView> m_guideView;
    bool m_dark = true;
    bool m_styling = false;
};

class LabHighlight : public QWidget
{
  Q_OBJECT

  public:
    // how long the frame shows, in milliseconds
    static constexpr int ShowMs = 1600;

    // a frame round `area` (in the coordinates of `target`), drawn over `target`; it removes
    // itself after ShowMs. A new highlight on the same target replaces the old one.
    static LabHighlight* show(QWidget* target, const QRect& area, bool dark);
    // the highlight on a widget now, or null
    static LabHighlight* on(QWidget* target);

    QRect area() const { return m_area; }

  protected:
    void paintEvent(QPaintEvent* event) override;

  private:
    LabHighlight(QWidget* target, const QRect& area, bool dark);
    QRect m_area;
    bool m_dark;
};

#endif
