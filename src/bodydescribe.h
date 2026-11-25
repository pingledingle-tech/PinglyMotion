/*
 * PinglyMotion - describe_body: the body as text (issue #129)
 *
 * describe_pose and describe_motion say what the figure does. An AI assistant
 * also needs to know the figure itself: which body is shown, how tall it is,
 * how long the arms and legs are, how wide the shoulders and hips are, and the
 * proportions, so it can judge reach ("can the hand touch the floor?"), plan a
 * sit at a given seat height, or describe the body to the user in words.
 *
 * All sizes are measured on the skeleton in the start pose (every angle 0,
 * standing straight, arms out): between joint centres, from the floor (the
 * lowest point of the feet) to the top of the skull. The skin adds a little.
 * The program's units are inches; every size is also given in metres. The
 * skeleton is the Second Life (Bento) skeleton, the same for the female and the
 * male figure and for both bodies (Ruth2 and Max): the choice only changes what
 * the 3D view draws, so the sizes are the same.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef BODYDESCRIBE_H
#define BODYDESCRIBE_H

#include <QJsonObject>
#include <QString>

class Animation;

namespace BodyDescribe
{
  // "male" is the figure shown (state's figure), "body" the body setting
  // ("ruth2" or "max", UserSettings::body()). The answer has ok true.
  QJsonObject describe(Animation* animation, bool male, const QString& body);
}

#endif
