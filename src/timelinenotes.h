/*
 * PinglyMotion - the notes line under the timeline (UX phase 3c,
 * docs/UX_DECISIONS.md section 4)
 *
 * An animation can carry teaching notes, one per key, and a small task for
 * the end: the wave example (examples/wave.dingle) has the notes WAVE-01 to
 * WAVE-06 of docs/TEACHING.md in its "extensions" object under
 * "pinglymotion.exampleNotes". One thin line under the timeline shows the
 * note of the current frame: the note of the last key at or before it (the
 * first note before the first key). Stopped at the last frame, at or after
 * the last note's key, it shows the task instead (the mockup's noteFor()).
 *
 * The line shows only while an animation with notes is open, the hints are
 * on (Help > Show hints) and its Hide notes button has not been clicked.
 * Hide notes hides it until the hints are turned on again.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef TIMELINENOTES_H
#define TIMELINENOTES_H

#include <QCoreApplication>
#include <QFrame>
#include <QJsonObject>
#include <QString>
#include <QVector>

class QLabel;
class QPushButton;

// the texts and rules of the notes line, in one place for the tests
namespace NotesSpec
{
  inline constexpr const char* context = "TimelineNotes";
  // the button and its tooltip (the mockup's "Hide notes", data-name "Hide the notes")
  inline constexpr const char* hideButton = QT_TRANSLATE_NOOP("TimelineNotes", "Hide notes");
  inline constexpr const char* hideTip = QT_TRANSLATE_NOOP("TimelineNotes", "Hide the notes");
  // the status line after Hide notes (the mockup's feedback)
  inline constexpr const char* hiddenMessage = QT_TRANSLATE_NOOP("TimelineNotes", "Notes hidden. Help > Show hints shows them again.");
  // where the notes are in the file's "extensions" object
  inline constexpr const char* extensionKey = "pinglymotion.exampleNotes";
  // limits for notes read from a file: more notes, or longer ones, are left out
  constexpr int maxNotes = 1000;
  constexpr int maxNoteLength = 1000;

  QString text(const char* source);

  struct Note
  {
    int frame = 0;
    QString text;
  };

  struct Notes
  {
    // sorted by frame, one per frame
    QVector<Note> keys;
    // shown at the end (WAVE-06); may be empty
    QString task;
    bool isEmpty() const { return keys.isEmpty(); }
  };

  // the notes in a .dingle file's "extensions" object; empty when there are
  // none or they are not in the expected shape. A note with no text, a
  // frame below 0 or a second note for the same frame is left out; line
  // breaks become spaces, as the line has one row.
  Notes fromExtensions(const QJsonObject& extensions);

  // the note for this frame (frames: the animation's number of frames);
  // empty when there are no notes
  QString noteFor(const Notes& notes,int frame,int frames,bool playing);

  // whether the line shows
  bool shows(const Notes& notes,bool animationOpen,bool hintsOn,bool hiddenByButton);
}

// the line itself: the book-open icon, the note and Hide notes
class NotesLine : public QFrame
{
  Q_OBJECT

  public:
    explicit NotesLine(QWidget* parent=nullptr);

    void setNotes(const NotesSpec::Notes& notes);
    const NotesSpec::Notes& notes() const { return m_notes; }
    // the current frame, the number of frames and whether it plays
    void setFrame(int frame,int frames,bool playing);
    void setPlaying(bool playing);
    void setAnimationOpen(bool open);
    // Help > Show hints; turning it on shows hidden notes again
    void setHintsOn(bool on);
    bool hiddenByButton() const { return m_hiddenByButton; }

    QLabel* textLabel() const { return m_text; }
    QLabel* iconLabel() const { return m_icon; }
    QPushButton* hideButton() const { return m_hide; }

    // the colours (docs/DESIGN_SYSTEM.md, "The notes line"): surface with 8
    // percent area_other, the icon in area_other, a border line on top
    static QColor background(bool dark);
    static QColor iconColour(bool dark);
    static QColor topLine(bool dark);

  signals:
    // Hide notes was clicked (the window shows NotesSpec::hiddenMessage)
    void hidden();

  protected:
    void paintEvent(QPaintEvent* event) override;
    void changeEvent(QEvent* event) override;

  private:
    void refresh();
    void updateIcon();

    NotesSpec::Notes m_notes;
    int m_frame=0;
    int m_frames=0;
    bool m_playing=false;
    bool m_open=false;
    bool m_hintsOn=true;
    bool m_hiddenByButton=false;

    QLabel* m_icon=nullptr;
    QLabel* m_text=nullptr;
    QPushButton* m_hide=nullptr;
};

#endif
