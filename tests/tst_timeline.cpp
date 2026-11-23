/*
 * PinglyMotion - test of the timeline's key and frame logic (src/timeline.cpp,
 * issue 202)
 *
 * Clicks, drags, the wheel and the Delete and Insert keys on an offscreen
 * timeline, checked against the animation's frame and keys. Since UX phase
 * 3a only changed joints have a row (tests/timelinerows): the two tracks used
 * here get a key, so TRACK has the first row and TRACK + 1 the second.
 */
#include <QtTest>
#include <QSignalSpy>
#include <QWheelEvent>

#include "animation.h"
#include "bvh.h"
#include "constants.h"
#include "timeline.h"

class TimelineTest : public QObject
{
  Q_OBJECT
private slots:
  void init();
  void cleanup();

  void clickSelectsTrackAndFrame();
  void dragMovesKey();
  void dragKeepsFirstAndLastFrame();
  void insertAndDeleteFrame();
  void wheelStepsFiveFrames();
  void setCurrentFrameCentres();

private:
  QPoint cell(int track, int frame) const;
  BVH* m_bvh = nullptr;
  Animation* m_anim = nullptr;
  Timeline* m_timeline = nullptr;
};

static const int FRAMES = 20;
static const int TRACK = 3;

QPoint TimelineTest::cell(int track, int frame) const
{
  const int row = m_timeline->rows().rowOfTrack(track);
  return QPoint(frame * Constants::keyWidth() + Constants::keyWidth() / 2,
                row * Constants::lineHeight() + Constants::lineHeight() / 2);
}

void TimelineTest::init()
{
  m_bvh = new BVH;
  m_anim = new Animation(m_bvh);
  Constants::setNumParts(BVH::countNodes(m_anim->getMotion()) + 1);
  QVERIFY(Constants::numParts() > TRACK);

  m_timeline = new Timeline;
  m_timeline->resize(FRAMES * Constants::keyWidth(), Constants::numParts() * Constants::lineHeight());
  m_timeline->setAnimation(m_anim);
  m_anim->setNumberOfFrames(FRAMES);
  m_timeline->show();

  // one key in the middle of the track, besides the first frame
  m_anim->setMirrored(false);
  m_anim->setFrame(5);
  m_anim->setRotation(m_anim->getNode(TRACK), 10, 20, 30);
  QVERIFY(m_anim->isKeyFrame(TRACK, 5));
  // the next track changed late in the animation, so it has a row too
  m_anim->setFrame(15);
  m_anim->setRotation(m_anim->getNode(TRACK + 1), 5, 0, 0);
  m_anim->setFrame(0);
  QCOMPARE(m_timeline->rows().rowOfTrack(TRACK), 0);
  QCOMPARE(m_timeline->rows().rowOfTrack(TRACK + 1), 1);
}

void TimelineTest::cleanup()
{
  delete m_timeline;
  delete m_anim;
  delete m_bvh;
}

void TimelineTest::clickSelectsTrackAndFrame()
{
  QSignalSpy clicked(m_timeline, &Timeline::trackClicked);
  QTest::mouseClick(m_timeline, Qt::LeftButton, Qt::NoModifier, cell(TRACK, 7));
  QCOMPARE(clicked.count(), 1);
  QCOMPARE(clicked.at(0).at(0).toInt(), TRACK);
  QCOMPARE(m_anim->getFrame(), 7);

  QTest::mouseClick(m_timeline, Qt::LeftButton, Qt::NoModifier, cell(TRACK + 1, 2));
  QCOMPARE(clicked.count(), 2);
  QCOMPARE(clicked.at(1).at(0).toInt(), TRACK + 1);
  QCOMPARE(m_anim->getFrame(), 2);
}

void TimelineTest::dragMovesKey()
{
  QTest::mousePress(m_timeline, Qt::LeftButton, Qt::NoModifier, cell(TRACK, 5));
  QTest::mouseMove(m_timeline, cell(TRACK, 8));
  QTest::mouseRelease(m_timeline, Qt::LeftButton, Qt::NoModifier, cell(TRACK, 8));

  QVERIFY(!m_anim->isKeyFrame(TRACK, 5));
  QVERIFY(m_anim->isKeyFrame(TRACK, 8));
  const QVector3D r = m_anim->getNode(TRACK)->frameData(8).rotation();
  QCOMPARE(r, QVector3D(10, 20, 30));
}

void TimelineTest::dragKeepsFirstAndLastFrame()
{
  // a key is never dragged onto the first or the last frame
  QTest::mousePress(m_timeline, Qt::LeftButton, Qt::NoModifier, cell(TRACK, 5));
  QTest::mouseMove(m_timeline, cell(TRACK, FRAMES - 1));
  QTest::mouseRelease(m_timeline, Qt::LeftButton, Qt::NoModifier, cell(TRACK, FRAMES - 1));
  QVERIFY(m_anim->isKeyFrame(TRACK, 5));
  QVERIFY(!m_anim->isKeyFrame(TRACK, FRAMES - 1));

  // without a key under the mouse, a drag only moves the current frame
  QTest::mousePress(m_timeline, Qt::LeftButton, Qt::NoModifier, cell(TRACK, 2));
  QTest::mouseMove(m_timeline, cell(TRACK, 4));
  QTest::mouseRelease(m_timeline, Qt::LeftButton, Qt::NoModifier, cell(TRACK, 4));
  QCOMPARE(m_anim->getFrame(), 4);
  QVERIFY(m_anim->isKeyFrame(TRACK, 5));
  QVERIFY(!m_anim->isKeyFrame(TRACK, 2));
  QVERIFY(!m_anim->isKeyFrame(TRACK, 4));
}

void TimelineTest::insertAndDeleteFrame()
{
  QSignalSpy inserted(m_timeline, &Timeline::insertFrame);
  QSignalSpy deleted(m_timeline, &Timeline::deleteFrame);

  // Insert on frame 3 of the track pushes the key on frame 5 to frame 6
  QTest::mouseClick(m_timeline, Qt::LeftButton, Qt::NoModifier, cell(TRACK, 3));
  QTest::keyClick(m_timeline, Qt::Key_Insert);
  QCOMPARE(inserted.count(), 1);
  QCOMPARE(inserted.at(0).at(0).toInt(), TRACK);
  QCOMPARE(inserted.at(0).at(1).toInt(), 3);
  QVERIFY(!m_anim->isKeyFrame(TRACK, 5));
  QVERIFY(m_anim->isKeyFrame(TRACK, 6));

  // Delete on frame 3 pulls it back
  QTest::keyClick(m_timeline, Qt::Key_Delete);
  QCOMPARE(deleted.count(), 1);
  QCOMPARE(deleted.at(0).at(0).toInt(), TRACK);
  QCOMPARE(deleted.at(0).at(1).toInt(), 3);
  QVERIFY(m_anim->isKeyFrame(TRACK, 5));
  QVERIFY(!m_anim->isKeyFrame(TRACK, 6));

  // other tracks are not touched
  QVERIFY(!m_anim->isKeyFrame(TRACK + 1, 5));
}

static void wheel(QWidget* widget, int angle)
{
  const QPointF pos(5, 5);
  QWheelEvent event(pos, widget->mapToGlobal(pos), QPoint(), QPoint(0, angle),
                    Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
  QApplication::sendEvent(widget, &event);
}

void TimelineTest::wheelStepsFiveFrames()
{
  QTest::mouseClick(m_timeline, Qt::LeftButton, Qt::NoModifier, cell(TRACK, 7));
  wheel(m_timeline, -120);  // down: forward
  QCOMPARE(m_anim->getFrame(), 12);
  wheel(m_timeline, -120);
  QCOMPARE(m_anim->getFrame(), 17);
  wheel(m_timeline, -120);  // stops on the last frame
  QCOMPARE(m_anim->getFrame(), FRAMES - 1);
  wheel(m_timeline, 120);   // up: back
  QCOMPARE(m_anim->getFrame(), FRAMES - 6);
  for (int i = 0; i < 5; ++i) wheel(m_timeline, 120);
  QCOMPARE(m_anim->getFrame(), 0);  // stops on the first frame
}

void TimelineTest::setCurrentFrameCentres()
{
  QSignalSpy centre(m_timeline, &Timeline::positionCenter);
  m_timeline->setCurrentFrame(9);
  QCOMPARE(centre.count(), 1);
  QCOMPARE(centre.at(0).at(0).toInt(), 9 * Constants::keyWidth());
}

QTEST_MAIN(TimelineTest)
#include "tst_timeline.moc"
