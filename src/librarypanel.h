/*
 * PinglyMotion - the Library panel (roadmap item 6, issue 19; docs/UX_DECISIONS.md 6l)
 *
 * Ready-made moves to start from, as drawn in the approved mockup
 * (docs/mockup/index.html, "Library"): a search field that works like the
 * control command find_reference, the kinds of move with their counts, the
 * list of clips (name, length and tags, an "Open now" chip on the clip that is
 * open), the picked clip with its description, a small preview that plays the
 * clip itself, its facts, and "Use this", which opens the clip by the same
 * route as load_reference. Empty states for a search without hits and for a
 * missing library folder.
 *
 * A panel like Script Lab: View > Panels > Library (Ctrl+Shift+B), the Library
 * button in the top bar and the close button in its title open and close it;
 * it docks on the right as a tab next to Selected joint, floats with the
 * pop-out button, and its place, size and float are saved with the layout.
 *
 * Colours come from the theme tokens only (docs/DESIGN_SYSTEM.md); the texts
 * are LibrarySpec (libraryindex.h). tests/librarypanel checks the behaviour.
 * "Add as a second figure" belongs to the two figures job (6m) and is not here.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#ifndef LIBRARYPANEL_H
#define LIBRARYPANEL_H

#include <QWidget>

#include "libraryindex.h"

class ControlApi;
class QAction;
class LibraryPreview;
class QAbstractButton;
class QButtonGroup;
class QDockWidget;
class QFrame;
class QLabel;
class QLineEdit;
class QListWidget;
class QMainWindow;
class QMessageBox;
class QPushButton;
class QSizeGrip;
class QStackedWidget;

class LibraryPanel : public QWidget
{
  Q_OBJECT

  public:
    static const char* const DockName;        // "libraryDock"
    static const char* const ViewActionName;  // "viewLibraryAction"

    // the dock, added hidden on the right of `window`, with the shared title bar;
    // its toggle action carries the name, the key and the tip
    static QDockWidget* makeDock(QMainWindow* window, LibraryPanel* panel);
    static QSize defaultSize();
    static QSize minimumPanelSize();
    QSize sizeHint() const override { return defaultSize(); }
    void setFloatingGrip(bool floating);
    QSizeGrip* sizeGrip() const { return m_sizeGrip; }

    explicit LibraryPanel(QWidget* parent = nullptr);

    // reads <folder>/index.json; without it the panel shows "No library found"
    bool setLibraryFolder(const QString& folder);
    const LibraryIndex& index() const { return m_index; }

    // the line on top shows while hints are on
    void setHintsOn(bool on);
    bool hintsOn() const { return m_hintsOn; }

    // the file that is open in the window: a library clip gets the "Open now" chip
    // and its Use this is greyed
    void setOpenFile(const QString& path);
    QString openClipId() const { return m_openClipId; }

    // search and kind of move ("" for All)
    void setSearchText(const QString& text);
    QString searchText() const;
    void setKind(const QString& kind);
    QString kind() const { return m_kind; }
    // Show all clips: clears the search and the kind
    void showAll();

    // the clips in the list now (ids, in list order) and the picked one
    QStringList shownClipIds() const;
    QString selectedClipId() const { return m_selected; }
    bool selectClip(const QString& id);

    bool noHitShown() const;
    bool missingShown() const;

    // Use this: asks the window to open the picked clip (nothing when it is greyed)
    bool useSelected();
    void togglePreview();
    // the keyboard goes to the search field (the panel was opened)
    void focusSearch();

    // the parts, for the tests
    QLineEdit* searchField() const { return m_search; }
    QListWidget* list() const { return m_list; }
    QPushButton* kindButton(const QString& kind) const;
    QLabel* countLabel() const { return m_count; }
    QLabel* explainLabel() const { return m_explain; }
    QWidget* explainRow() const { return m_explainRow; }
    QLabel* nameLabel() const { return m_name; }
    QLabel* descriptionLabel() const { return m_description; }
    QLabel* factValue(int row) const { return m_facts.value(row); }
    QPushButton* useButton() const { return m_use; }
    QPushButton* previewButton() const { return m_previewButton; }
    QPushButton* showAllButton() const { return m_showAll; }
    LibraryPreview* preview() const { return m_preview; }
    QLabel* noHitTitle() const { return m_noHitTitle; }
    QLabel* missingTitle() const { return m_missingTitle; }
    QLabel* missingLine() const { return m_missingLine; }

    // the question before Use this when the animation has unsaved changes (LIB-22)
    enum class UnsavedChoice { Cancel, OpenWithoutSaving, SaveThenOpen };
    static QMessageBox* unsavedBox(QWidget* parent, const QString& clipName);
    static UnsavedChoice choiceOf(QMessageBox* box);

    // opens a clip by the route of load_reference; `message` is the status line
    // (LIB-23 on success, the command's reason otherwise)
    static bool openClip(ControlApi* api, const QString& id, QString& message);

  signals:
    void useRequested(const QString& id);
    void statusMessage(const QString& text);

  protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void changeEvent(QEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

  private:
    void buildList();
    void refreshDetails();
    void refreshPreviewClip();
    void refreshKinds();
    void applyStyle();
    void placeSizeGrip();
    void moveKind(int step, bool toEnd);

    LibraryIndex m_index;
    QString m_kind;
    QString m_selected;
    QString m_openClipId;
    QString m_openFile;
    bool m_hintsOn = true;
    bool m_previewDirty = false;

    QWidget* m_explainRow = nullptr;
    QLabel* m_explainIcon = nullptr;
    QLabel* m_explain = nullptr;
    QStackedWidget* m_pages = nullptr;
    QLineEdit* m_search = nullptr;
    QAction* m_searchIcon = nullptr;
    QLabel* m_kindLabel = nullptr;
    QButtonGroup* m_kinds = nullptr;
    QList<QPushButton*> m_kindButtons;
    QLabel* m_count = nullptr;
    QStackedWidget* m_listPages = nullptr;
    QListWidget* m_list = nullptr;
    QFrame* m_noHit = nullptr;
    QLabel* m_noHitIcon = nullptr;
    QLabel* m_noHitTitle = nullptr;
    QLabel* m_noHitLine = nullptr;
    QPushButton* m_showAll = nullptr;
    QFrame* m_details = nullptr;
    QLabel* m_name = nullptr;
    QLabel* m_description = nullptr;
    LibraryPreview* m_preview = nullptr;
    QPushButton* m_previewButton = nullptr;
    QList<QLabel*> m_factNames;
    QList<QLabel*> m_facts;
    QPushButton* m_use = nullptr;
    QFrame* m_missing = nullptr;
    QLabel* m_missingIcon = nullptr;
    QLabel* m_missingTitle = nullptr;
    QLabel* m_missingLine = nullptr;
    QSizeGrip* m_sizeGrip = nullptr;
};

#endif
