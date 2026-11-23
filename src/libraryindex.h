/*
 * PinglyMotion - the reference library for the Library panel (roadmap item 6, issue 19)
 *
 * Reads library/index.json (the same file as the control commands find_reference
 * and load_reference) and answers what the Library panel shows: the clips in
 * library order, a search that works like find_reference (each word must begin
 * a word in the clip's id, name, description or tags, case ignored), the kinds
 * of move with their counts, and the facts of a clip in plain words.
 *
 * The texts are the drafts LIB-01 to LIB-27 of docs/TEACHING.md, kept in one
 * table (LibrarySpec) for the panel and for tests/librarypanel, which checks
 * them letter for letter. No widgets here.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#ifndef LIBRARYINDEX_H
#define LIBRARYINDEX_H

#include <QCoreApplication>
#include <QList>
#include <QString>
#include <QStringList>

namespace LibrarySpec
{
  struct Kind
  {
    const char* id;    // the tag in library/index.json ("" for All)
    const char* text;  // the choice
  };

  // LIB-01 to LIB-27 (docs/TEACHING.md)
  inline constexpr const char* title = QT_TRANSLATE_NOOP("LibraryPanel", "Library");
  inline constexpr const char* explain = QT_TRANSLATE_NOOP("LibraryPanel", "Ready-made moves to start from. Pick one, then press Use this.");
  inline constexpr const char* search = QT_TRANSLATE_NOOP("LibraryPanel", "Search the library");
  inline constexpr const char* searchTip = QT_TRANSLATE_NOOP("LibraryPanel", "Type words like walk, wave or sit.");
  inline constexpr const char* kindLabel = QT_TRANSLATE_NOOP("LibraryPanel", "Kind of move");
  inline constexpr const char* countAll = QT_TRANSLATE_NOOP("LibraryPanel", "%1 clips");
  inline constexpr const char* countSome = QT_TRANSLATE_NOOP("LibraryPanel", "%1 of %2 clips");
  inline constexpr const char* noHitTitle = QT_TRANSLATE_NOOP("LibraryPanel", "No clip found");
  inline constexpr const char* noHitLine = QT_TRANSLATE_NOOP("LibraryPanel", "Try another word, or show all clips.");
  inline constexpr const char* showAll = QT_TRANSLATE_NOOP("LibraryPanel", "Show all clips");
  inline constexpr const char* missingTitle = QT_TRANSLATE_NOOP("LibraryPanel", "No library found");
  inline constexpr const char* missingLine = QT_TRANSLATE_NOOP("LibraryPanel", "The library folder next to PinglyMotion is missing. Install PinglyMotion again to get it back.");
  inline constexpr const char* playPreview = QT_TRANSLATE_NOOP("LibraryPanel", "Play preview");
  inline constexpr const char* stopPreview = QT_TRANSLATE_NOOP("LibraryPanel", "Stop preview");
  inline constexpr const char* factLength = QT_TRANSLATE_NOOP("LibraryPanel", "Length");
  inline constexpr const char* factSkeleton = QT_TRANSLATE_NOOP("LibraryPanel", "Skeleton");
  inline constexpr const char* factFrom = QT_TRANSLATE_NOOP("LibraryPanel", "From");
  inline constexpr const char* factLicence = QT_TRANSLATE_NOOP("LibraryPanel", "Licence");
  inline constexpr const char* lengthValue = QT_TRANSLATE_NOOP("LibraryPanel", "%1 s, %2 frames");
  inline constexpr const char* skeletonValue = QT_TRANSLATE_NOOP("LibraryPanel", "Second Life, %1 joints");
  inline constexpr const char* cmuFrom = QT_TRANSLATE_NOOP("LibraryPanel", "CMU motion capture database");
  inline constexpr const char* cmuLicence = QT_TRANSLATE_NOOP("LibraryPanel", "No restrictions on use. No formal licence text.");
  inline constexpr const char* m2mFrom = QT_TRANSLATE_NOOP("LibraryPanel", "Mesh2Motion");
  inline constexpr const char* m2mLicence = QT_TRANSLATE_NOOP("LibraryPanel", "CC0 1.0: free to use for anything.");
  inline constexpr const char* use = QT_TRANSLATE_NOOP("LibraryPanel", "Use this");
  inline constexpr const char* useTip = QT_TRANSLATE_NOOP("LibraryPanel", "Opens this move as your animation. Save it under your own name.");
  inline constexpr const char* openNowReason = QT_TRANSLATE_NOOP("LibraryPanel", "This move is open now.");
  inline constexpr const char* openNowChip = QT_TRANSLATE_NOOP("LibraryPanel", "Open now");
  inline constexpr const char* unsavedTitle = QT_TRANSLATE_NOOP("LibraryPanel", "Open %1?");
  inline constexpr const char* unsavedLine = QT_TRANSLATE_NOOP("LibraryPanel", "Your animation has changes that are not saved.");
  inline constexpr const char* unsavedCancel = QT_TRANSLATE_NOOP("LibraryPanel", "Cancel");
  inline constexpr const char* unsavedDiscard = QT_TRANSLATE_NOOP("LibraryPanel", "Open without saving");
  inline constexpr const char* unsavedSave = QT_TRANSLATE_NOOP("LibraryPanel", "Save, then open");
  inline constexpr const char* opened = QT_TRANSLATE_NOOP("LibraryPanel", "Opened %1 from the library. Save it under your own name.");
  inline constexpr const char* topTip = QT_TRANSLATE_NOOP("LibraryPanel", "Library: ready-made moves to start from");
  inline constexpr const char* openedPanel = QT_TRANSLATE_NOOP("LibraryPanel", "Library open. It shares the right side with Selected joint: switch with the tabs.");
  inline constexpr const char* closedPanel = QT_TRANSLATE_NOOP("LibraryPanel", "Library closed. View > Library opens it again.");
  inline constexpr const char* showingAll = QT_TRANSLATE_NOOP("LibraryPanel", "Showing all %1 clips.");
  inline constexpr const char* clipsName = QT_TRANSLATE_NOOP("LibraryPanel", "Clips");
  inline constexpr const char* previewName = QT_TRANSLATE_NOOP("LibraryPanel", "Preview of %1");
  inline constexpr const char* kindName = QT_TRANSLATE_NOOP("LibraryPanel", "%1: %2 clips");
  inline constexpr const char* closeName = QT_TRANSLATE_NOOP("LibraryPanel", "Close the library");

  // the kinds of move of the approved plan, plus All (docs/UX_DECISIONS.md 6l)
  inline constexpr Kind kinds[] = {
    {"", QT_TRANSLATE_NOOP("LibraryPanel", "All")},
    {"walk", QT_TRANSLATE_NOOP("LibraryPanel", "Walk")},
    {"dance", QT_TRANSLATE_NOOP("LibraryPanel", "Dance")},
    {"sit", QT_TRANSLATE_NOOP("LibraryPanel", "Sit")},
    {"stand", QT_TRANSLATE_NOOP("LibraryPanel", "Stand")},
    {"gesture", QT_TRANSLATE_NOOP("LibraryPanel", "Gesture")},
    {"idle", QT_TRANSLATE_NOOP("LibraryPanel", "Idle")},
    {"wave", QT_TRANSLATE_NOOP("LibraryPanel", "Wave")},
  };

  // View > Panels > Library and the top bar button
  inline constexpr const char* key = "Ctrl+Shift+B";

  inline QString tr(const char* text) { return QCoreApplication::translate("LibraryPanel", text); }
}

struct LibraryClip
{
  QString id;
  QString name;
  QString description;
  QStringList tags;
  QString source;      // "cmu", "mesh2motion" or another key of "sources"
  QString sourceName;  // the source's name from the index, for an unknown source
  QString licence;     // the clip's licence text from the index, for an unknown source
  QString file;        // relative to the library folder
  int frames = 0;
  int joints = 0;
  double seconds = 0.0;
};

class LibraryIndex
{
  public:
    // reads <folder>/index.json; false (and no clips) when it is missing or not a library index
    bool load(const QString& folder);
    bool loaded() const { return m_loaded; }
    QString folder() const { return m_folder; }

    const QList<LibraryClip>& clips() const { return m_clips; }
    int size() const { return int(m_clips.size()); }
    // the clip with this id, or nullptr
    const LibraryClip* find(const QString& id) const;
    // the clip whose file is this path (absolute, as the window shows the open file), or nullptr
    const LibraryClip* findByPath(const QString& path) const;
    QString absolutePath(const LibraryClip& clip) const;

    // the clips that match the search text and the kind ("" for All), in library order (indexes)
    QList<int> search(const QString& text, const QString& kind) const;
    // number of clips of a kind ("" for All)
    int count(const QString& kind) const;

    // find_reference's rule: every word of the text begins a word of the clip's id, name, description or tags
    static bool matches(const LibraryClip& clip, const QString& text);

    // the second line of a list row: "1.4 s · walk, calm, loop"
    static QString rowLine(const LibraryClip& clip);
    // the facts: "1.4 s, 43 frames", "Second Life, 19 joints", where it is from and its licence
    static QString lengthText(const LibraryClip& clip);
    static QString skeletonText(const LibraryClip& clip);
    static QString fromText(const LibraryClip& clip);
    static QString licenceText(const LibraryClip& clip);
    // the count over the list: "106 clips" or "12 of 106 clips"
    static QString countText(int shown, int all);

  private:
    QList<LibraryClip> m_clips;
    QString m_folder;
    bool m_loaded = false;
};

#endif
