/*
 * PinglyMotion Script Lab - the watched folder as a tree (lane 1, job D)
 *
 * The Live page watches a folder as one object (LabFolder, labfolder.h): the
 * top folder is the object, each subfolder a prim, and the scripts and
 * notecards sit in their prim. This tree shows that object after each run:
 *
 *   MyObject      Object, 2 prims     1 error, 2 OK    12.0 KB
 *     Root        Root prim, link 1   2 OK              8.0 KB
 *       main      LSL script          OK                4.0 KB
 *       settings  Notecard, 4 lines
 *     Lid         Prim, link 2        1 error
 *       lid       LSL script          Error
 *
 * Icons: a box for the object, a square for each prim, file-code for a
 * script and notebook-text for a notecard (Lucide, docs/DESIGN_SYSTEM.md,
 * "Icons"). Each script has a colour dot for its state (success, warning,
 * danger or text_muted from docs/design_tokens.json, through theme.h) and
 * the state as a word as well (design charter P10). A prim and the object
 * sum up their scripts: how many of each state, with the dot of the worst,
 * and the memory of all their scripts. Memory is what each script's values
 * used at the end of the run (the sandbox's rough count, code not counted;
 * Second Life gives a script 64 KB). A script the folder refused (a name
 * used twice, a file too large) has an Error row with the reason.
 *
 * Picking a script (click, or Enter on its row) sends scriptPicked with its
 * file, so the panel shows that script in the editor.
 *
 * Job U3: picking a prim or a script also sends primPicked with the prim's
 * name, so the panel can light up the prop of the same name in the 3D view;
 * selectPrim() picks a prim's row from outside (a click on a prop) without
 * sending anything back.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef LABTREE_H
#define LABTREE_H

#include <QJsonObject>
#include <QTreeWidget>

class LabFolderTree : public QTreeWidget
{
  Q_OBJECT

  public:
    // a script's state, from the least to the most serious
    enum State { NoScripts = 0, Ok = 1, NotRun = 2, Warning = 3, Stopped = 4, Error = 5 };
    // what a row is (stored under KindRole)
    enum Kind { ObjectRow = 0, PrimRow = 1, ScriptRow = 2, NotecardRow = 3 };
    enum Column { NameColumn = 0, KindColumn = 1, StateColumn = 2, MemoryColumn = 3 };
    enum Role { FileRole = Qt::UserRole, KindRole = Qt::UserRole + 1, StateRole = Qt::UserRole + 2, MemoryRole = Qt::UserRole + 3,
                PrimRole = Qt::UserRole + 4 };

    // the memory one script may use in Second Life (llGetMemoryLimit at the start)
    static constexpr qint64 SCRIPT_MEMORY = 65536;

    explicit LabFolderTree(QWidget* parent = nullptr);

    // Builds the tree from a folder run (LabWatcher::folderResult(): "linkset", "check", "result",
    // "ok", "error"). An empty object, or one whose folder could not be read, empties the tree.
    // The picked script stays picked while it is still in the folder.
    void setFolderResult(const QJsonObject& folder);

    // the state of one script of the last folder run (NoScripts when it is not in the tree)
    State scriptState(const QString& file) const;
    // the row of a file (a script or a notecard), nullptr when there is none
    QTreeWidgetItem* itemForFile(const QString& file) const;
    // the object's row (nullptr while the tree is empty)
    QTreeWidgetItem* objectItem() const { return topLevelItemCount() ? topLevelItem(0) : nullptr; }

    // picks a script's row, as a click does (sends scriptPicked); false when it is not a script of the tree
    bool pickScript(const QString& file);
    // the picked script's file ("" when none)
    QString pickedScript() const;

    // --- prims (job U3): the row of a prim by its name (the prop's name), nullptr when none
    QTreeWidgetItem* itemForPrim(const QString& name) const;
    // the prim a row belongs to: its own name for a prim, its prim's for a script or notecard; "" for the object
    static QString primOf(const QTreeWidgetItem* item);
    // picks a prim's row as a click on its prop in the 3D view does; sends nothing. False when it is not in the tree
    bool selectPrim(const QString& name);
    // the prim of the picked row ("" when none, or the object row)
    QString pickedPrim() const { return primOf(currentItem()); }

    // the part's count: "3 scripts in 2 prims" ("" while the tree is empty)
    QString countText() const { return m_count; }

    // the words, for the tests and the panel
    static QString stateWord(State state);
    // "4.0 KB" (one decimal, 1 KB = 1024 bytes)
    static QString memoryText(qint64 bytes);
    // the sum of a prim or the object: "1 error, 2 OK" (most serious first; "No scripts" for none)
    static QString sumText(const QList<State>& states);
    // the colour token of a state's dot (theme.h), as a colour in the given theme
    static QColor stateColour(State state, bool dark);

    // icons and dots in the theme's colours (called when the palette changes)
    void applyStyle(bool dark);

  signals:
    void scriptPicked(const QString& file);
    // a prim or one of its files was picked by the user (job U3)
    void primPicked(const QString& name);

  protected:
    void changeEvent(QEvent* event) override;

  private:
    void restyleItem(QTreeWidgetItem* item, bool dark);
    QString m_count;
    bool m_dark = true;
    bool m_building = false;
};

#endif
