/*
 * PinglyMotion - test of the notes line under the timeline, UX phase 3c
 * (behaviour and texts)
 *
 * Exact answers only (src/timelinenotes.*, src/timelineview.*,
 * docs/UX_DECISIONS.md section 4): the wave example's notes read from its
 * file word for word (WAVE-01 to WAVE-06 of docs/TEACHING.md), the note for
 * every frame of the wave while stopped and while playing, the task at the
 * end, notes in a wrong shape left out, when the line shows (an animation
 * with notes, hints on, not hidden), Hide notes and Help > Show hints, the
 * line following the frame of the open animation, the notes kept when the
 * file is saved and opened again, the texts, icons and colours against the
 * mockup and the design tokens, and the window's wiring.
 *
 * How the line looks (spacing, the tone of its colour) is for the combined
 * Windows review.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include <QFile>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>

#include "animation.h"
#include "bvh.h"
#include "constants.h"
#include "dingle.h"
#include "easingtiles.h"
#include "jointgroups.h"
#include "lucideicons.h"
#include "theme.h"
#include "timeline.h"
#include "timelinenotes.h"
#include "timelineview.h"
#include "transportlook.h"

namespace
{
  QString readFile(const QString& path)
  {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return QString();
    return QString::fromUtf8(file.readAll());
  }
  QString source(const char* name) { return readFile(QStringLiteral(PINGLYMOTION_SOURCE_DIR "/src/") + QLatin1String(name)); }
  QString mockup() { return readFile(QStringLiteral(PINGLYMOTION_SOURCE_DIR "/docs/mockup/index.html")); }
  QString wavePath() { return QStringLiteral(PINGLYMOTION_SOURCE_DIR "/examples/wave.dingle"); }

  // WAVE-01 to WAVE-06 (docs/TEACHING.md, approved by the project owner), one row each
  const QString wave01 = QStringLiteral("A key is a saved pose. This one has the arm down. The wave starts and ends here, so it loops.");
  const QString wave02 = QStringLiteral("The arm lifts here. Slow both ends makes it start and stop softly.");
  const QString wave03 = QStringLiteral("The hand swings out, goes a little past, then settles. That is Overshoot, then settle.");
  const QString wave04 = QStringLiteral("The hand swings back at one steady speed. That is Even. It looks stiff, so try Slow both ends.");
  const QString wave05 = QStringLiteral("The arm slows down as it lands. It matches the first key, so the loop has no jump.");
  const QString wave06 = QStringLiteral("Your turn: make the arm stop more softly.");

  QJsonObject notesObject(const QJsonArray& keys, const QJsonValue& task = QJsonValue())
  {
    QJsonObject notes{{QStringLiteral("keys"), keys}};
    if (!task.isUndefined() && !task.isNull()) notes.insert(QStringLiteral("task"), task);
    return QJsonObject{{QLatin1String(NotesSpec::extensionKey), notes}};
  }
  QJsonObject key(const QJsonValue& frame, const QJsonValue& note) { return QJsonObject{{QStringLiteral("frame"), frame}, {QStringLiteral("note"), note}}; }

  void setUpJoints(Animation& anim)
  {
    Constants::setNumParts(BVH::countNodes(anim.getMotion()) + 1);
    QStringList names;
    for (int t = 0; t < Constants::numParts(); ++t) names << anim.getPartName(t);
    JointGroups::setTracks(names);
  }
}

// the wave example open in a timeline
struct WaveFixture
{
  BVH bvh;
  Animation anim;
  TimelineView view;
  WaveFixture() : anim(&bvh, wavePath())
  {
    setUpJoints(anim);
    view.getTimeline()->setAnimation(&anim);
    view.setNoAnimation(false);
  }
  NotesLine* line() const { return view.notesLine(); }
  bool shows() const { return line()->isVisibleTo(&view); }
  QString text() const { return line()->textLabel()->text(); }
};

class TimelineNotesTest : public QObject
{
  Q_OBJECT
private slots:
  void initTestCase() { QStandardPaths::setTestModeEnabled(true); }
  void waveNotesReadWordForWord();
  void noteForEveryFrameOfTheWave();
  void beforeTheFirstKeyAndWithoutNotes();
  void wrongShapesAreLeftOut();
  void limits();
  void whenTheLineShows();
  void lineFollowsTheFrame();
  void taskOnlyWhileStoppedAtTheEnd();
  void hideNotesUntilHintsAreTurnedOnAgain();
  void animationWithoutNotesHasNoLine();
  void noAnimationOpenHidesTheLine();
  void notesKeptWhenSavedAndOpenedAgain();
  void textsAsInTheMockup();
  void iconsAndButton();
  void coloursFromTheTokens();
  void windowWiring();
};

void TimelineNotesTest::waveNotesReadWordForWord()
{
  BVH bvh;
  Animation anim(&bvh, wavePath());
  QCOMPARE(anim.getNumberOfFrames(), 61);
  const NotesSpec::Notes notes = NotesSpec::fromExtensions(anim.extensions());
  QCOMPARE(notes.keys.size(), 5);
  const int frames[] = {0, 20, 30, 40, 60};
  const QString texts[] = {wave01, wave02, wave03, wave04, wave05};
  for (int i = 0; i < 5; ++i)
  {
    QCOMPARE(notes.keys[i].frame, frames[i]);
    QCOMPARE(notes.keys[i].text, texts[i]);
  }
  QCOMPARE(notes.task, wave06);
}

void TimelineNotesTest::noteForEveryFrameOfTheWave()
{
  BVH bvh;
  Animation anim(&bvh, wavePath());
  const NotesSpec::Notes notes = NotesSpec::fromExtensions(anim.extensions());
  for (int frame = 0; frame < 61; ++frame)
  {
    const QString expected = frame < 20 ? wave01 : frame < 30 ? wave02 : frame < 40 ? wave03 : frame < 60 ? wave04 : wave05;
    QCOMPARE(NotesSpec::noteFor(notes, frame, 61, true), expected);
    // stopped: the same, except the task at the last frame
    QCOMPARE(NotesSpec::noteFor(notes, frame, 61, false), frame == 60 ? wave06 : expected);
  }
}

void TimelineNotesTest::beforeTheFirstKeyAndWithoutNotes()
{
  const NotesSpec::Notes notes = NotesSpec::fromExtensions(notesObject({key(10, "Ten"), key(20, "Twenty")}, "Task"));
  QCOMPARE(NotesSpec::noteFor(notes, 0, 30, false), QStringLiteral("Ten"));
  QCOMPARE(NotesSpec::noteFor(notes, 9, 30, false), QStringLiteral("Ten"));
  QCOMPARE(NotesSpec::noteFor(notes, 10, 30, false), QStringLiteral("Ten"));
  QCOMPARE(NotesSpec::noteFor(notes, 19, 30, false), QStringLiteral("Ten"));
  QCOMPARE(NotesSpec::noteFor(notes, 20, 30, false), QStringLiteral("Twenty"));
  QCOMPARE(NotesSpec::noteFor(notes, 28, 30, false), QStringLiteral("Twenty"));
  QCOMPARE(NotesSpec::noteFor(notes, 29, 30, false), QStringLiteral("Task"));
  QCOMPARE(NotesSpec::noteFor(notes, 29, 30, true), QStringLiteral("Twenty"));
  // the last note's key after the last frame: no task (the end is before it)
  const NotesSpec::Notes late = NotesSpec::fromExtensions(notesObject({key(0, "Start"), key(50, "Late")}, "Task"));
  QCOMPARE(NotesSpec::noteFor(late, 29, 30, false), QStringLiteral("Start"));
  // no task: the last note stays
  const NotesSpec::Notes noTask = NotesSpec::fromExtensions(notesObject({key(0, "Only")}));
  QCOMPARE(noTask.task, QString());
  QCOMPARE(NotesSpec::noteFor(noTask, 29, 30, false), QStringLiteral("Only"));
  // no notes at all
  const NotesSpec::Notes none = NotesSpec::fromExtensions(QJsonObject());
  QVERIFY(none.isEmpty());
  QCOMPARE(NotesSpec::noteFor(none, 0, 30, false), QString());
}

void TimelineNotesTest::wrongShapesAreLeftOut()
{
  QVERIFY(NotesSpec::fromExtensions(QJsonObject{{QLatin1String(NotesSpec::extensionKey), QStringLiteral("text")}}).isEmpty());
  QVERIFY(NotesSpec::fromExtensions(QJsonObject{{QLatin1String(NotesSpec::extensionKey), QJsonObject{{"keys", "x"}}}}).isEmpty());
  QVERIFY(NotesSpec::fromExtensions(QJsonObject{{QStringLiteral("other.add-on"), QJsonObject{{"keys", QJsonArray{key(0, "x")}}}}}).isEmpty());

  const NotesSpec::Notes notes = NotesSpec::fromExtensions(notesObject({
    key(30, "Thirty"),
    key(-1, "Below zero"),
    key(2.5, "Not a whole frame"),
    key("4", "Frame as text"),
    key(5, 12),
    key(6, ""),
    key(7, "   "),
    QJsonValue(QStringLiteral("not an object")),
    key(10, "Ten,\nwith a line break  and   spaces"),
    key(30, "Second note for thirty"),
    key(0, "Zero"),
  }, 42));
  QCOMPARE(notes.keys.size(), 3);
  QCOMPARE(notes.keys[0].frame, 0);
  QCOMPARE(notes.keys[0].text, QStringLiteral("Zero"));
  QCOMPARE(notes.keys[1].frame, 10);
  QCOMPARE(notes.keys[1].text, QStringLiteral("Ten, with a line break and spaces"));
  QCOMPARE(notes.keys[2].frame, 30);
  QCOMPARE(notes.keys[2].text, QStringLiteral("Thirty"));
  // a task that is not text is left out
  QCOMPARE(notes.task, QString());
  // a task without notes is not kept either
  QCOMPARE(NotesSpec::fromExtensions(notesObject({}, "Task")).task, QString());
}

void TimelineNotesTest::limits()
{
  QJsonArray many;
  for (int i = 0; i < NotesSpec::maxNotes + 5; ++i) many.append(key(i, QStringLiteral("Note %1").arg(i)));
  const NotesSpec::Notes notes = NotesSpec::fromExtensions(notesObject(many));
  QCOMPARE(notes.keys.size(), NotesSpec::maxNotes);
  QCOMPARE(notes.keys.last().frame, NotesSpec::maxNotes - 1);

  const QString longest(NotesSpec::maxNoteLength, QLatin1Char('a'));
  const QString tooLong(NotesSpec::maxNoteLength + 1, QLatin1Char('a'));
  const NotesSpec::Notes sized = NotesSpec::fromExtensions(notesObject({key(0, longest), key(1, tooLong)}, tooLong));
  QCOMPARE(sized.keys.size(), 1);
  QCOMPARE(sized.keys[0].text, longest);
  QCOMPARE(sized.task, QString());
}

void TimelineNotesTest::whenTheLineShows()
{
  const NotesSpec::Notes notes = NotesSpec::fromExtensions(notesObject({key(0, "Zero")}));
  const NotesSpec::Notes none;
  QCOMPARE(NotesSpec::shows(notes, true, true, false), true);
  QCOMPARE(NotesSpec::shows(notes, false, true, false), false);
  QCOMPARE(NotesSpec::shows(notes, true, false, false), false);
  QCOMPARE(NotesSpec::shows(notes, true, true, true), false);
  QCOMPARE(NotesSpec::shows(none, true, true, false), false);
}

void TimelineNotesTest::lineFollowsTheFrame()
{
  WaveFixture f;
  QVERIFY(f.shows());
  QCOMPARE(f.anim.getFrame(), 0);
  QCOMPARE(f.text(), wave01);
  f.anim.setFrame(19);
  QCOMPARE(f.text(), wave01);
  f.anim.setFrame(20);
  QCOMPARE(f.text(), wave02);
  f.anim.setFrame(35);
  QCOMPARE(f.text(), wave03);
  f.anim.setFrame(40);
  QCOMPARE(f.text(), wave04);
  f.anim.setFrame(59);
  QCOMPARE(f.text(), wave04);
  f.anim.setFrame(60);
  QCOMPARE(f.text(), wave06);
  f.anim.setFrame(0);
  QCOMPARE(f.text(), wave01);
}

void TimelineNotesTest::taskOnlyWhileStoppedAtTheEnd()
{
  WaveFixture f;
  f.view.setPlaying(true);
  f.anim.setFrame(60);
  QCOMPARE(f.text(), wave05);
  // playing stops at the last frame: the task follows at once
  f.view.setPlaying(false);
  QCOMPARE(f.text(), wave06);
  f.view.setPlaying(true);
  QCOMPARE(f.text(), wave05);
}

void TimelineNotesTest::hideNotesUntilHintsAreTurnedOnAgain()
{
  WaveFixture f;
  QSignalSpy hidden(&f.view, &TimelineView::notesHidden);
  QVERIFY(f.shows());
  f.line()->hideButton()->click();
  QCOMPARE(hidden.count(), 1);
  QVERIFY(!f.shows());
  QVERIFY(f.line()->hiddenByButton());
  // the hints are on already: nothing changes, also not with another frame
  f.view.setHintsOn(true);
  f.anim.setFrame(30);
  QVERIFY(!f.shows());
  // Help > Show hints off: still hidden; on again: the notes are back
  f.view.setHintsOn(false);
  QVERIFY(!f.shows());
  f.view.setHintsOn(true);
  QVERIFY(f.shows());
  QVERIFY(!f.line()->hiddenByButton());
  QCOMPARE(f.text(), wave03);
  // hints off hides the line without the button
  f.view.setHintsOn(false);
  QVERIFY(!f.shows());
  QCOMPARE(hidden.count(), 1);
}

void TimelineNotesTest::animationWithoutNotesHasNoLine()
{
  WaveFixture f;
  BVH bvh;
  Animation plain(&bvh);
  QVERIFY(plain.extensions().isEmpty());
  f.view.getTimeline()->setAnimation(&plain);
  QVERIFY(!f.shows());
  // back to the wave: the line comes back with the wave's frame
  f.view.getTimeline()->setAnimation(&f.anim);
  QVERIFY(f.shows());
  QCOMPARE(f.text(), wave01);
  // the frame of the other animation no longer reaches the line
  f.view.getTimeline()->setAnimation(&plain);
  plain.setFrame(10);
  QVERIFY(!f.shows());
}

void TimelineNotesTest::noAnimationOpenHidesTheLine()
{
  WaveFixture f;
  f.view.setNoAnimation(true);
  QVERIFY(!f.shows());
  f.view.setNoAnimation(false);
  QVERIFY(f.shows());
}

void TimelineNotesTest::notesKeptWhenSavedAndOpenedAgain()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString path = dir.filePath(QStringLiteral("wave copy.dingle"));
  QJsonObject original;
  {
    BVH bvh;
    Animation anim(&bvh, wavePath());
    original = anim.extensions();
    QVERIFY(!original.isEmpty());
    QString error;
    QVERIFY2(anim.saveBVH(path, &error), qPrintable(error));
  }
  QFile saved(path);
  QVERIFY(saved.open(QIODevice::ReadOnly));
  const Dingle::Result read = Dingle::parse(saved.readAll());
  QVERIFY(read.ok());
  QCOMPARE(read.extensions, original);

  BVH bvh;
  Animation reopened(&bvh, path);
  QCOMPARE(reopened.extensions(), original);
  const NotesSpec::Notes notes = NotesSpec::fromExtensions(reopened.extensions());
  QCOMPARE(notes.keys.size(), 5);
  QCOMPARE(notes.keys[2].text, wave03);
  QCOMPARE(notes.task, wave06);

  // a BVH file has no extensions: opening one gives none
  const QString bvhPath = dir.filePath(QStringLiteral("wave.bvh"));
  QString error;
  QVERIFY2(reopened.saveBVH(bvhPath, &error), qPrintable(error));
  BVH bvh2;
  Animation fromBvh(&bvh2, bvhPath);
  QVERIFY(fromBvh.extensions().isEmpty());
}

void TimelineNotesTest::textsAsInTheMockup()
{
  QCOMPARE(NotesSpec::text(NotesSpec::hideButton), QStringLiteral("Hide notes"));
  QCOMPARE(NotesSpec::text(NotesSpec::hideTip), QStringLiteral("Hide the notes"));
  QCOMPARE(NotesSpec::text(NotesSpec::hiddenMessage), QStringLiteral("Notes hidden. Help > Show hints shows them again."));
  QCOMPARE(QLatin1String(NotesSpec::context), QLatin1String("TimelineNotes"));
  QCOMPARE(QLatin1String(NotesSpec::extensionKey), QLatin1String("pinglymotion.exampleNotes"));

  const QString page = mockup();
  QVERIFY(page.contains(QStringLiteral("<button class=\"btn compact\" data-cmd=\"hidenote\" data-name=\"Hide the notes\"><svg class=\"i\" data-i=\"eye-off\"></svg>Hide notes</button>")));
  QVERIFY(page.contains(QStringLiteral("<svg class=\"i\" data-i=\"book-open\"></svg><span class=\"txt\" id=\"noteTxt\"></span>")));
  QVERIFY(page.contains(QStringLiteral("feedback(\"Notes hidden. Help > Show hints shows them again.\")")));
  // the mockup's notes are the same texts as the wave example's
  for (const QString& text : {wave01, wave02, wave03, wave04, wave05, wave06})
    QVERIFY2(page.contains(QLatin1Char('"') + text + QLatin1Char('"')), qPrintable(text));
  // the register in docs/TEACHING.md has them too
  const QString teaching = readFile(QStringLiteral(PINGLYMOTION_SOURCE_DIR "/docs/TEACHING.md"));
  QVERIFY(teaching.contains(QStringLiteral("| A key is a saved pose. This one has the arm down.<br>The wave starts and ends here, so it loops. |")));
  QVERIFY(teaching.contains(QStringLiteral("| Your turn: make the arm stop more softly. |")));

  // the line's own widgets carry the texts
  NotesLine line;
  QCOMPARE(line.hideButton()->text(), QStringLiteral("Hide notes"));
  QCOMPARE(line.hideButton()->toolTip(), QStringLiteral("Hide the notes"));
  QCOMPARE(line.hideButton()->accessibleName(), QStringLiteral("Hide the notes"));
  QCOMPARE(line.textLabel()->textFormat(), Qt::PlainText);
  QVERIFY(line.textLabel()->wordWrap());
}

void TimelineNotesTest::iconsAndButton()
{
  QPushButton button;
  button.setObjectName(QStringLiteral("hideNotesButton"));
  QCOMPARE(TransportLook::iconName(&button), QStringLiteral("eye-off"));
  QCOMPARE(TransportLook::kind(&button), TransportLook::Kind::Normal);
  for (const char* name : {"book-open", "eye-off"})
  {
    QVERIFY2(LucideIcons::names().contains(QLatin1String(name)), name);
    QVERIFY2(!LucideIcons::path(QLatin1String(name)).isEmpty(), name);
  }
  // the open book fills most of the 24 x 24 grid, as in Lucide
  const QRectF book = LucideIcons::path(QStringLiteral("book-open")).boundingRect();
  QVERIFY2(qAbs(book.left() - 2) < 0.01 && qAbs(book.right() - 22) < 0.01, qPrintable(QStringLiteral("%1 %2").arg(book.left()).arg(book.right())));
  QVERIFY2(qAbs(book.top() - 3) < 0.01 && qAbs(book.bottom() - 21) < 0.01, qPrintable(QStringLiteral("%1 %2").arg(book.top()).arg(book.bottom())));
  const QRectF eye = LucideIcons::path(QStringLiteral("eye-off")).boundingRect();
  QVERIFY2(qAbs(eye.left() - 2) < 0.01 && qAbs(eye.right() - 22) < 0.01, qPrintable(QStringLiteral("%1 %2").arg(eye.left()).arg(eye.right())));

  NotesLine line;
  QCOMPARE(line.hideButton()->objectName(), QStringLiteral("hideNotesButton"));
  QVERIFY(!line.hideButton()->icon().isNull());
  QCOMPARE(line.objectName(), QStringLiteral("timelineNotes"));
  QVERIFY(!line.iconLabel()->pixmap().isNull());
  QCOMPARE(line.iconLabel()->size(), QSize(Theme::Size::iconSmall, Theme::Size::iconSmall));
}

void TimelineNotesTest::coloursFromTheTokens()
{
  using Theme::Token;
  for (bool dark : {true, false})
  {
    QCOMPARE(NotesLine::background(dark), Theme::mix(Theme::color(Token::Surface, dark), Theme::color(Token::AreaOther, dark), 0.08));
    QCOMPARE(NotesLine::iconColour(dark), Theme::color(Token::AreaOther, dark));
    QCOMPARE(NotesLine::topLine(dark), Theme::color(Token::Border, dark));
  }
  QCOMPARE(Theme::color(Token::AreaOther, true), QColor(QStringLiteral("#5CC8C8")));
  QCOMPARE(Theme::color(Token::AreaOther, false), QColor(QStringLiteral("#0F7C7C")));

  // the drawn line: the border on top, the background below, in both themes
  for (bool dark : {true, false})
  {
    Theme::apply(dark);
    WaveFixture f;
    f.view.resize(900, 400);
    f.view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&f.view));
    NotesLine* line = f.line();
    QVERIFY(line->isVisible());
    const QImage image = line->grab().toImage();
    const qreal ratio = image.devicePixelRatio();
    QCOMPARE(image.pixelColor(int(4 * ratio), 0).rgb(), NotesLine::topLine(dark).rgb());
    QCOMPARE(image.pixelColor(int(4 * ratio), image.height() - 1).rgb(), NotesLine::background(dark).rgb());
    // the layout's padding: spacing.m left and right, spacing.s top and bottom (docs/DESIGN_SYSTEM.md)
    const QMargins margins = line->layout()->contentsMargins();
    QCOMPARE(margins, QMargins(Theme::Size::spacingM, Theme::Size::spacingS, Theme::Size::spacingM, Theme::Size::spacingS));
    QCOMPARE(line->layout()->spacing(), Theme::Size::spacingS);
    // the line is under the easing tiles, at the bottom of the timeline
    QVERIFY(line->geometry().top() >= f.view.easingTiles()->geometry().bottom());
  }
  Theme::apply(true);
}

void TimelineNotesTest::windowWiring()
{
  // the window passes the hints switch, Help > Show hints and the play state on,
  // and shows the status line after Hide notes
  const QString window = source("pinglymotionwindow.cpp");
  QCOMPARE(window.count(QStringLiteral("ui->timelineView->setHintsOn(on);")), 2);
  QVERIFY(window.contains(QStringLiteral("ui->timelineView->setHintsOn(UserSettings::tipsOn());")));
  QVERIFY(window.contains(QStringLiteral("connect(ui->timelineView,&TimelineView::notesHidden,this,[this]()")));
  QVERIFY(window.contains(QStringLiteral("statusBar()->showMessage(NotesSpec::text(NotesSpec::hiddenMessage),5000);")));
  const QString playback = source("pinglymotionwindowplayback.cpp");
  QVERIFY(playback.contains(QStringLiteral("ui->timelineView->setPlaying(state!=PLAYSTATE_STOPPED);")));
  // a saved .dingle keeps the file's extensions
  QVERIFY(source("bvh.cpp").contains(QStringLiteral("Dingle::write(snapshot(anim,AnimFile::Format::Avm),anim->extensions(),QStringLiteral(\"PinglyMotion\"),anim->props())")));
}

QTEST_MAIN(TimelineNotesTest)
#include "tst_timelinenotes.moc"
