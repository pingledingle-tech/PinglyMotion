/*
 * PinglyMotion - the calibration table of the assistant guide (issue #129)
 *
 * From the rest pose (the skeleton of bin/data/SLFemale.bvh with every angle 0: arms out
 * to the side, legs straight down) each joint is turned by +30 degrees around X, Y and Z
 * in turn. The test measures where the end point of that joint (hand, foot, top of the
 * head, fingertip) goes in the world and writes it in words, from the figure's own point
 * of view (left and right are the figure's own sides). The table sits in
 * docs/ASSISTANT_GUIDE.md between two marker lines; this test makes it again and compares.
 *
 * Make the table again after a change:
 *   PINGLYMOTION_UPDATE_GUIDE=1 build/tests/calibration/pinglymotion_calibration_tests
 *
 * The angles are applied as the 3D view draws the figure: the joint's offset, then each
 * rotation channel in file order, multiplied on the right (see tests/tst_ik.cpp).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QFile>
#include <QMatrix4x4>
#include <QRegularExpression>
#include <QtTest>
#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

static const float ANGLE = 30.0f;
// An end point that moves less than TWIST_SHARE of its distance from the joint is "not moving":
// the turn is a twist around the line to that point and the table says where the front goes.
// A move below SMALL_SHARE is marked "(small)". A front direction that changes less than
// FRONT_MOVE (a unit vector turned 30 degrees changes by 0.52) is not a visible move at all.
static const float TWIST_SHARE = 0.06f;
static const float SMALL_SHARE = 0.27f;
static const float FRONT_MOVE = 0.15f;

static const char* const BEGIN_MARK = "<!-- calibration-table:begin -->";
static const char* const END_MARK = "<!-- calibration-table:end -->";

struct Node
{
  QString name;
  bool isEnd = false;
  QVector3D offset;
  QList<char> channels;  // 'X', 'Y', 'Z' rotation channels in file order
  Node* parent = nullptr;
  std::vector<std::unique_ptr<Node>> children;
};

class Skeleton
{
  public:
    bool load(const QString& path)
    {
      QFile file(path);
      if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;
      const QString text = QString::fromLatin1(file.readAll());
      const int motion = text.indexOf("MOTION");
      const QStringList tokens = text.left(motion < 0 ? text.size() : motion)
                                     .split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
      int i = 0;
      if (i >= tokens.size() || tokens[i++] != "HIERARCHY") return false;
      root = parse(tokens, i, nullptr);
      return root != nullptr;
    }

    Node* find(const QString& name) const
    {
      for (Node* n : all)
        if (n->name == name) return n;
      return nullptr;
    }

    // world matrix of every node when `turned` is rotated by (x, y, z) degrees and all else is 0
    QHash<Node*, QMatrix4x4> world(Node* turned, const QVector3D& angles) const
    {
      QHash<Node*, QMatrix4x4> out;
      place(root.get(), QMatrix4x4(), turned, angles, out);
      return out;
    }

    std::unique_ptr<Node> root;
    QList<Node*> all;

  private:
    std::unique_ptr<Node> parse(const QStringList& t, int& i, Node* parent)
    {
      auto node = std::make_unique<Node>();
      node->parent = parent;
      const QString kind = t.value(i++);
      if (kind == "End")
      {
        i++;  // "Site"
        node->isEnd = true;
        node->name = parent->name + "_end";
      }
      else
        node->name = t.value(i++);
      all << node.get();
      if (t.value(i++) != "{") return nullptr;
      while (i < t.size())
      {
        const QString word = t[i++];
        if (word == "OFFSET")
        {
          node->offset = QVector3D(t[i].toFloat(), t[i + 1].toFloat(), t[i + 2].toFloat());
          i += 3;
        }
        else if (word == "CHANNELS")
        {
          const int n = t[i++].toInt();
          for (int c = 0; c < n; c++)
          {
            const QString channel = t[i++];
            if (channel.endsWith("rotation")) node->channels << channel[0].toLatin1();
          }
        }
        else if (word == "JOINT" || word == "End")
        {
          i--;
          auto child = parse(t, i, node.get());
          if (!child) return nullptr;
          node->children.push_back(std::move(child));
        }
        else if (word == "}")
          return node;
        else
          return nullptr;
      }
      return nullptr;
    }

    void place(Node* node, const QMatrix4x4& parentMatrix, Node* turned, const QVector3D& angles,
               QHash<Node*, QMatrix4x4>& out) const
    {
      QMatrix4x4 m = parentMatrix;
      if (node != root.get()) m.translate(node->offset);  // the root stays at the origin
      if (node == turned)
        for (char channel : node->channels)
        {
          if (channel == 'X') m.rotate(angles.x(), 1, 0, 0);
          if (channel == 'Y') m.rotate(angles.y(), 0, 1, 0);
          if (channel == 'Z') m.rotate(angles.z(), 0, 0, 1);
        }
      out[node] = m;
      for (auto& child : node->children) place(child.get(), m, turned, angles, out);
    }
};

struct Limit
{
  float min = 0, max = 0;
  bool known = false;
};

// SL.lim lines: name weight AXIS min max AXIS min max AXIS min max
static QHash<QString, Limit> readLimits(const QString& path, bool* ok)
{
  QHash<QString, Limit> out;
  QFile file(path);
  *ok = file.open(QIODevice::ReadOnly | QIODevice::Text);
  if (!*ok) return out;
  const QStringList lines = QString::fromLatin1(file.readAll()).split('\n');
  for (const QString& line : lines)
  {
    const QStringList f = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    if (f.size() < 11) continue;
    for (int a = 0; a < 3; a++)
    {
      Limit l;
      l.min = f[3 + a * 3].toFloat();
      l.max = f[4 + a * 3].toFloat();
      l.known = true;
      out[f[0] + "." + f[2 + a * 3]] = l;
    }
  }
  return out;
}

struct Entry
{
  QString joint;
  QString endName;  // the node that moves; a leading * means: the end of the chain below that joint
  QString end;    // what moves, when the end point itself moves
  QString front;  // what moves, when the turn is a twist
};

static QList<Entry> entries()
{
  QList<Entry> list;
  for (const QString& joint : {QString("hip"), QString("abdomen"), QString("chest")})
    list << Entry{joint, "*head", "head", joint == "chest" ? "chest front" : "body front"};
  list << Entry{"neck", "*head", "top of head", "face"}
       << Entry{"head", "*head", "top of head", "face"};
  for (const QString& side : {QString("l"), QString("r")})
  {
    const QString word = side == "l" ? "left" : "right";
    const QString hand = side + "Hand";
    const QString foot = side + "Foot";
    list << Entry{side + "Collar", hand, word + " hand", "front of " + word + " arm"}
         << Entry{side + "Shldr", hand, word + " hand", "front of " + word + " arm"}
         << Entry{side + "ForeArm", hand, word + " hand", "front of " + word + " forearm"}
         << Entry{hand, "*" + hand, word + " hand tip", "front of " + word + " hand"}
         << Entry{side + "Thigh", foot, word + " foot", "front of " + word + " leg"}
         << Entry{side + "Shin", foot, word + " foot", "front of " + word + " lower leg"}
         << Entry{foot, "*" + foot, word + " toes", "front of " + word + " foot"};
  }
  list << Entry{"mSkull", "*mSkull", "top of head", "face"};
  list << Entry{"mEyeLeft", "*mEyeLeft", "left eye front", "left eye front"}
       << Entry{"mEyeRight", "*mEyeRight", "right eye front", "right eye front"}
       << Entry{"mFaceJaw", "*mFaceJaw", "chin", "jaw front"};
  for (const QString& side : {QString("Left"), QString("Right")})
  {
    const QString word = side.toLower();
    for (const QString& finger : {QString("Thumb"), QString("Index"), QString("Middle"), QString("Ring"), QString("Pinky")})
    {
      const QString joint = "mHand" + finger + "1" + side;
      list << Entry{joint, "*" + joint, word + " " + finger.toLower() + " tip", word + " " + finger.toLower()};
    }
  }
  return list;
}

// the last point of the chain below a joint: follow the first child down to the end
static Node* endPointOf(Node* joint)
{
  Node* n = joint;
  while (!n->children.empty()) n = n->children[0].get();
  return n;
}

static int sideOf(const QString& joint)
{
  if (joint.endsWith("Left") || QRegularExpression("^l[A-Z]").match(joint).hasMatch()) return 1;
  if (joint.endsWith("Right") || QRegularExpression("^r[A-Z]").match(joint).hasMatch()) return -1;
  return 0;  // on the middle line of the body
}

class TestCalibration : public QObject
{
  Q_OBJECT

  private slots:
    void initTestCase();
    void figureAxes();
    void kneeAndElbowBendTheRightWay();
    void tableMatchesTheGuide();

  private:
    QString table(float* smallestMargin = nullptr);

    Skeleton skeleton;
    QHash<QString, Limit> limits;
    QVector3D forward, left, up;
};

void TestCalibration::initTestCase()
{
  QVERIFY2(skeleton.load(QString(PINGLYMOTION_DATA) + "/SLFemale.bvh"), "cannot read SLFemale.bvh");
  bool ok = false;
  limits = readLimits(QString(PINGLYMOTION_DATA) + "/SL.lim", &ok);
  QVERIFY2(ok, "cannot read SL.lim");

  // the figure's own axes, found from the rest pose and not assumed
  const auto rest = skeleton.world(nullptr, QVector3D());
  auto at = [&](const QString& name) { return rest[skeleton.find(name)].map(QVector3D()); };
  up = QVector3D(0, 1, 0);
  left = (at("lThigh") - at("rThigh")).normalized();
  forward = QVector3D::crossProduct(left, up).normalized();
}

void TestCalibration::figureAxes()
{
  // the toes are in front of the ankle, the left hand is on the left, the head is above the hip
  const auto rest = skeleton.world(nullptr, QVector3D());
  auto at = [&](Node* n) { return rest[n].map(QVector3D()); };
  Node* toe = endPointOf(skeleton.find("lFoot"));
  QVERIFY(QVector3D::dotProduct(at(toe) - at(skeleton.find("lFoot")), forward) > 1);
  QVERIFY(QVector3D::dotProduct(at(skeleton.find("lHand")) - at(skeleton.find("hip")), left) > 10);
  QVERIFY(QVector3D::dotProduct(at(skeleton.find("head")) - at(skeleton.find("hip")), up) > 10);
  QVERIFY(QVector3D::dotProduct(at(skeleton.find("rHand")) - at(skeleton.find("hip")), left) < -10);
  int joints = 0;
  for (Node* n : skeleton.all) joints += n->isEnd ? 0 : 1;
  QCOMPARE(joints, 133);
}

void TestCalibration::kneeAndElbowBendTheRightWay()
{
  // Second Life's own limits (SL.lim) say both knees bend towards positive X (0 to 150), the left
  // elbow towards negative Y (-146 to 0) and the right elbow towards positive Y (0 to 146).
  // A bent knee takes the foot back and a bent elbow takes the hand forward, so +30 must do this.
  const QString t = table();
  QVERIFY2(t.contains("| lShin | X | + | left foot | back |"), qPrintable(t));
  QVERIFY2(t.contains("| rShin | X | + | right foot | back |"), qPrintable(t));
  QVERIFY2(t.contains("| lForeArm | Y | + | left hand | back |"), qPrintable(t));  // minus bends it
  QVERIFY2(t.contains("| rForeArm | Y | + | right hand | forward |"), qPrintable(t));
}

QString TestCalibration::table(float* smallestMargin)
{
  float margin = 1e9f;
  QString out;
  out += "| Joint | Axis | Sign | What moves | Direction | Allowed range |\n";
  out += "|---|---|---|---|---|---|\n";
  for (const Entry& entry : entries())
  {
    Node* joint = skeleton.find(entry.joint);
    Q_ASSERT_X(joint, "table", qPrintable(entry.joint));
    Node* end = entry.endName.startsWith('*') ? endPointOf(skeleton.find(entry.endName.mid(1)))
                                              : skeleton.find(entry.endName);
    Q_ASSERT_X(end, "table", qPrintable(entry.endName));
    const int side = sideOf(entry.joint);
    const bool isFace = entry.joint.startsWith("mEye") || entry.joint == "mFaceJaw";  // sideways = looks or turns
    const auto rest = skeleton.world(nullptr, QVector3D());
    const QVector3D endRest = rest[end].map(QVector3D());
    // a point in front of the end point, fixed to it: it shows a twist
    const QVector3D frontRest = rest[end].mapVector(QVector3D(0, 0, 1));
    const float lever = std::max((endRest - rest[joint].map(QVector3D())).length(), 0.5f);
    for (int axis = 0; axis < 3; axis++)
    {
      QVector3D angles;
      angles[axis] = ANGLE;
      const auto turned = skeleton.world(joint, angles);
      const QVector3D move = turned[end].map(QVector3D()) - endRest;
      const QVector3D frontMove = turned[end].mapVector(QVector3D(0, 0, 1)) - frontRest;
      const float share = move.length() / lever;
      const bool endMoves = share > TWIST_SHARE;
      margin = std::min({margin, std::abs(share - TWIST_SHARE), std::abs(share - SMALL_SHARE)});
      if (!endMoves) margin = std::min(margin, std::abs(frontMove.length() - FRONT_MOVE));

      const QVector3D d = endMoves ? move : frontMove;
      const float f = QVector3D::dotProduct(d, forward);
      const float l = QVector3D::dotProduct(d, left);
      const float u = QVector3D::dotProduct(d, up);
      const float af = std::abs(f), al = std::abs(l), au = std::abs(u);
      QString word;
      if (endMoves)
      {
        if (af >= al && af >= au) word = f > 0 ? "forward" : "back";
        else if (au >= al) word = u > 0 ? "up" : "down";
        else if (isFace) word = l > 0 ? "turn left" : "turn right";
        else if (side == 0) word = l > 0 ? "tilt left" : "tilt right";
        else word = (l * side > 0) ? "out to the side" : "in across the body";
      }
      else if (frontMove.length() < FRONT_MOVE)
        word = "nothing visible (rolls around its front)";
      else
      {
        // a twist: the front of the part turns; the end point stays
        if (al >= af && al >= au) word = l > 0 ? "turn left" : "turn right";
        else if (au >= af) word = u > 0 ? "front goes up" : "front goes down";
        else word = f > 0 ? "front goes forward" : "front goes back";
      }
      if (endMoves && share < SMALL_SHARE) word += " (small)";
      const QString name = QString(QChar('X' + axis));
      const Limit lim = limits.value(entry.joint + "." + name);
      const QString range = (lim.known && lim.max < 1000) ? QString("%1 to %2").arg(lim.min, 0, 'f', 0).arg(lim.max, 0, 'f', 0) : QString("none");
      out += QString("| %1 | %2 | + | %3 | %4 | %5 |\n")
                 .arg(entry.joint, name, endMoves ? entry.end : entry.front, word, range);
    }
  }
  if (smallestMargin) *smallestMargin = margin;
  return out;
}

void TestCalibration::tableMatchesTheGuide()
{
  float margin = 0;
  const QString made = table(&margin);
  // no result sits close to the "moves or twists" line, so another compiler cannot flip it
  QVERIFY2(margin > 0.02f, qPrintable(QString("a result is close to the limit: %1").arg(margin)));

  QFile file(PINGLYMOTION_GUIDE);
  QVERIFY2(file.open(QIODevice::ReadOnly | QIODevice::Text), "cannot read docs/ASSISTANT_GUIDE.md");
  QString guide = QString::fromUtf8(file.readAll());
  file.close();
  const int begin = guide.indexOf(BEGIN_MARK);
  const int end = guide.indexOf(END_MARK);
  QVERIFY2(begin >= 0 && end > begin, "the markers of the calibration table are missing in the guide");
  const int from = begin + int(strlen(BEGIN_MARK)) + 1;  // after the line end
  const QString inGuide = guide.mid(from, end - from);

  if (qEnvironmentVariableIsSet("PINGLYMOTION_UPDATE_GUIDE"))
  {
    guide.replace(from, end - from, made);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate));
    file.write(guide.toUtf8());
    qInfo("Calibration table written to the guide.");
    return;
  }
  QVERIFY2(inGuide == made,
           "The calibration table in docs/ASSISTANT_GUIDE.md is out of date. Make it again with "
           "PINGLYMOTION_UPDATE_GUIDE=1 (see the guide). The new table is:\n" + made.toUtf8());
}

QTEST_APPLESS_MAIN(TestCalibration)
#include "tst_calibration.moc"
