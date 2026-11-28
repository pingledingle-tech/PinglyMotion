/*
 * PinglyMotion Script Lab - sub-tabs on the left of a Script Lab tab (WP-01 Part B)
 *
 * The project owner's beta.2 test (28 Nov 2026): Script Lab got very tall with all its parts. The parts of
 * a tab are shown as sub-tabs in a list on the left of the tab, one at a time on the right, so the tab is
 * only as tall as the part in front. Parts can also be taken out into a window of their own: a whole
 * group at once (for example "Object and Script", the tree beside the editor) or one part alone (only
 * Object, or only Script). Closing that window, or Put back, puts it back where it was. Reset view in
 * Script Lab puts every group and part back and the first sub-tab in front.
 *
 *  - LabSubStack: a stacked widget as tall as its page in front (a QStackedWidget is as tall as its
 *    tallest page, which would bring the height back).
 *  - LabSubTabs: the list of groups on the left, the group's name and Pop out on top, the group in
 *    front under it; the windows of popped groups and parts (LabPopWindow, labtabs.h).
 *  - LabSubTabState: the remembered state ("sub=live:work;pop=log;place=log:10,20,600,400"), read and
 *    written without a window, so the tests can check it on its own. Only known keys and sane numbers
 *    are taken; anything else is left out.
 *
 * Colours come from the theme tokens (src/theme.h), sizes from docs/design_tokens.json.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef LABSUBTABS_H
#define LABSUBTABS_H

#include <QMap>
#include <QPointer>
#include <QRect>
#include <QStackedWidget>
#include <QStringList>
#include <QWidget>

class QBoxLayout;
class QLabel;
class QListWidget;
class QPushButton;
class QToolButton;
class QVBoxLayout;
class LabPopWindow;
class ScriptLabSection;

namespace LabSubTabState
{
  struct State
  {
    QMap<QString, QString> current;  // the sub-tab in front, per tab ("live" -> "work")
    QStringList popped;              // the groups and parts in windows of their own, in the order they were popped
    QMap<QString, QRect> places;     // the last place and size of each one's own window
    bool operator==(const State& other) const
    {
      return current == other.current && popped == other.popped && places == other.places;
    }
  };
  // reads the text; groups: the group keys of each tab; units: every group and part key that can pop out.
  // Unknown keys and broken entries are left out.
  State parse(const QString& text, const QMap<QString, QStringList>& groups, const QStringList& units);
  QString format(const State& state);
  // the longest text kept (the settings value is cut there)
  inline constexpr int MaxLength = 1000;
}

class LabSubStack : public QStackedWidget
{
  Q_OBJECT

  public:
    explicit LabSubStack(QWidget* parent = nullptr);
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
};

class LabSubTabs : public QWidget
{
  Q_OBJECT

  public:
    // tabKey: the Script Lab tab it belongs to ("live", "commands"), used in the object names
    explicit LabSubTabs(const QString& tabKey, QWidget* parent = nullptr);

    QString tabKey() const { return m_tabKey; }

    // a group: one sub-tab. Returns the layout its widgets go in. tip: one line on what it holds.
    QVBoxLayout* addGroup(const QString& key, const QString& name, const QString& tip);
    // a part of a group that can be popped out on its own (a group with more than one part). The part
    // stays where it is in the group's widgets; it gets a Pop out button in its header.
    void addPart(const QString& group, ScriptLabSection* part);
    // the parts of a group (sections); a group is listed while one of its parts is not hidden, or always
    // when it has none
    void setGroupParts(const QString& group, const QList<ScriptLabSection*>& parts);

    QStringList groupKeys() const { return m_order; }
    QString groupName(const QString& key) const;
    // every key that can pop out: the groups, then the parts
    QStringList unitKeys() const;
    QStringList partKeys() const;
    // the group a part is in ("" when none)
    QString groupOfPart(const QString& part) const;

    // the sub-tab in front ("" when none is listed)
    QString currentGroup() const;
    // brings a group to the front (its window, when it is popped out); false when it is not listed
    bool setCurrentGroup(const QString& key);
    // whether a group is in the list now (one of its parts can be seen)
    bool isListed(const QString& key) const;

    // pops a group or a part out into a window of its own; false when it is out already or unknown
    bool popOut(const QString& key);
    // puts a popped group or part back where it was; false when it is in
    bool putBack(const QString& key);
    bool isPoppedOut(const QString& key) const { return m_windows.contains(key); }
    LabPopWindow* popWindow(const QString& key) const { return m_windows.value(key); }
    // every group and part back, the first listed sub-tab in front, the places forgotten
    void reset();
    QStringList popped() const { return m_popOrder; }
    QMap<QString, QRect> places() const { return m_places; }
    void setPlaces(const QMap<QString, QRect>& places) { m_places = places; }

    // the popped windows show while Script Lab is open and hide while it is closed
    void setPopWindowsShown(bool shown);
    void applyStyle(bool dark);

    // read by the tests
    QListWidget* list() const { return m_list; }
    QLabel* groupTitle() const { return m_title; }
    QToolButton* popOutButton() const { return m_popOut; }
    LabSubStack* stack() const { return m_stack; }
    // what a group pops out (its widgets), and the line with Put back shown in its place meanwhile
    QWidget* groupContent(const QString& key) const;
    QWidget* groupPlaceholder(const QString& key) const;
    // a part's Pop out button in its header, and the line with Put back left in its place meanwhile
    QToolButton* partPopButton(const QString& part) const;
    QWidget* partPlaceholder(const QString& part) const;
    ScriptLabSection* part(const QString& key) const;

  signals:
    // the sub-tab in front, the popped groups and parts or a window's place changed
    void stateChanged();
    // a group or part came back or left: the panel may lay out its columns again
    void layoutChanged();

  protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void changeEvent(QEvent* event) override;

  private:
    struct Group
    {
      QString key;
      QString name;
      QWidget* page = nullptr;         // the page in the stack
      QWidget* content = nullptr;      // what pops out
      QVBoxLayout* contentLayout = nullptr;
      QWidget* placeholder = nullptr;  // "... is in a window of its own." and Put back
      QList<ScriptLabSection*> parts;
    };
    struct Part
    {
      QString key;
      QString group;
      ScriptLabSection* section = nullptr;
      QToolButton* pop = nullptr;
      QWidget* placeholder = nullptr;
      QPointer<QBoxLayout> layout;     // where it was, while it is out
      int index = -1;
    };
    QWidget* makePlaceholder(const QString& key, const QString& name, QWidget* parent);
    LabPopWindow* makeWindow(const QString& key, const QString& name);
    void closeWindow(const QString& key);
    void refreshList();
    void refreshHeader();
    void stylePopButton(QToolButton* button);
    int rowOf(const QString& key) const;

    QString m_tabKey;
    bool m_dark = true;
    bool m_windowsShown = false;       // the panel shows them with itself (setPopWindowsShown)
    bool m_autoPicked = true;          // the sub-tab in front was not picked by the user: the first listed one
    QListWidget* m_list = nullptr;
    QLabel* m_title = nullptr;
    QToolButton* m_popOut = nullptr;
    LabSubStack* m_stack = nullptr;
    QStringList m_order;
    QMap<QString, Group> m_groups;
    QMap<QString, Part> m_parts;
    QMap<QString, LabPopWindow*> m_windows;
    QStringList m_popOrder;
    QMap<QString, QRect> m_places;
    bool m_refreshing = false;
};

#endif
