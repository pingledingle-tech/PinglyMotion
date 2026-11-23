/*
 * PinglyMotion - rows of the joint list in the properties panel (#14, step 2)
 *
 * sideOf() and the optional side filter in rows() are for the Joints panel's
 * Left / Right / Mirror control (posing UX phase 2a).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "jointlist.h"

#include <QCoreApplication>
#include <QHash>
#include <algorithm>

#include "jointgroups.h"
#include "jointinfo.h"

const QString JointList::POSITION_TRACK=QStringLiteral("position");

static QString tr(const char* text)
{
  return QCoreApplication::translate("JointList",text);
}

QString JointList::friendlyName(const JointInfo& info,const QString& name)
{
  if(name==POSITION_TRACK) return tr("Avatar Position");
  const QString friendly=info.friendlyName(name);
  return friendly.isEmpty() ? name : friendly;
}

QString JointList::heading(const JointInfo& info,const QString& name)
{
  if(name==POSITION_TRACK) return tr("Body");
  const JointInfo::Joint* j=info.joint(name);
  if(!j) return tr("Other joints");
  // Firestorm lists the pelvis, chest and torso without a heading, and the
  // skull under "Body" although it is on the Face tab
  if(j->firestormSection.isEmpty() || j->firestormSection==QLatin1String("Body"))
    return j->tab==QLatin1String("Face") ? tr("Head") : tr("Body");
  return j->firestormSection;
}

bool JointList::matches(const JointInfo& info,const QString& name,const QString& search)
{
  const QStringList words=search.split(QLatin1Char(' '),Qt::SkipEmptyParts);
  if(words.isEmpty()) return true;

  QString text=friendlyName(info,name)+QLatin1Char(' ')+heading(info,name)+QLatin1Char(' ')+name;
  if(const JointInfo::Joint* j=info.joint(name))
    text+=QLatin1Char(' ')+j->sl+QLatin1Char(' ')+j->bvh;

  for(const QString& word : words)
    if(!text.contains(word,Qt::CaseInsensitive)) return false;
  return true;
}

QChar JointList::sideOf(const JointInfo& info,const QString& name)
{
  if(const JointInfo::Joint* j=info.joint(name))
  {
    if(j->sl.endsWith(QLatin1String("Left"))) return QLatin1Char('L');
    if(j->sl.endsWith(QLatin1String("Right"))) return QLatin1Char('R');
    return QChar();
  }
  // a joint the data does not know: fall back to the BVH file name convention
  // (lShldr, rShldr, ...), the same one BVHNode::mirror() relies on
  if(name.size()>1 && name.at(1).isUpper())
  {
    if(name.startsWith(QLatin1Char('l'))) return QLatin1Char('L');
    if(name.startsWith(QLatin1Char('r'))) return QLatin1Char('R');
  }
  return QChar();
}

QVector<JointList::Row> JointList::rows(const JointInfo& info,const QStringList& trackNames,int tab,
                                        const QString& search,const QVector<bool>& hidden,bool leaveOutHidden,
                                        QChar side)
{
  const bool searching=!search.trimmed().isEmpty();
  const int known=info.joints().count();

  // place of each heading: the order in which the joint data first uses it
  QHash<QString,int> headingOrder;
  headingOrder.insert(heading(info,POSITION_TRACK),-1);
  for(const JointInfo::Joint& j : info.joints())
  {
    const QString h=heading(info,j.sl);
    if(!headingOrder.contains(h)) headingOrder.insert(h,headingOrder.count());
  }
  const QString otherHeading=tr("Other joints");
  if(!headingOrder.contains(otherHeading)) headingOrder.insert(otherHeading,headingOrder.count()+known);

  struct Entry
  {
    int headingPlace;
    int jointPlace;
    Row row;
  };
  QVector<Entry> entries;

  for(int track=0;track<trackNames.count();track++)
  {
    const QString& name=trackNames[track];
    if(name==QLatin1String("Site")) continue;
    if(searching ? !matches(info,name,search) : JointGroups::tabOf(name)!=tab) continue;
    if(!side.isNull())
    {
      const QChar jointSide=sideOf(info,name);
      if(!jointSide.isNull() && jointSide!=side) continue;
    }

    const bool isHidden=track<hidden.count() && hidden[track];
    if(isHidden && leaveOutHidden) continue;

    const JointInfo::Joint* j=info.joint(name);
    Entry e;
    e.headingPlace=headingOrder.value(heading(info,name));
    // the position track first; joints the data does not know in track order at the end
    e.jointPlace=(name==POSITION_TRACK) ? -1 : (j ? int(j-info.joints().constData()) : known+track);
    e.row.text=friendlyName(info,name);
    e.row.slName=j ? j->sl : QString();
    e.row.fileName=name;
    e.row.track=track;
    e.row.dimmed=isHidden;
    entries << e;
  }

  std::stable_sort(entries.begin(),entries.end(),[](const Entry& a,const Entry& b)
  {
    if(a.headingPlace!=b.headingPlace) return a.headingPlace<b.headingPlace;
    return a.jointPlace<b.jointPlace;
  });

  QVector<Row> result;
  int lastHeading=-2;
  for(const Entry& e : entries)
  {
    if(e.headingPlace!=lastHeading)
    {
      Row h;
      h.heading=true;
      h.text=heading(info,e.row.fileName);
      result << h;
      lastHeading=e.headingPlace;
    }
    result << e.row;
  }
  return result;
}
