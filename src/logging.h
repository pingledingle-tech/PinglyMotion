/*
 * PinglyMotion
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

/***************************************************************************
 *   Logging categories. Debug output can be turned on per area with       *
 *   QT_LOGGING_RULES, for example "pinglymotion.animation.debug=false".   *
 ***************************************************************************/

#ifndef LOGGING_H
#define LOGGING_H

#include <QLoggingCategory>

#define PINGLYMOTION_LOG_CATEGORY(function, name) \
  inline const QLoggingCategory& function()       \
  {                                               \
    static const QLoggingCategory category(name); \
    return category;                              \
  }

// reading and writing files
PINGLYMOTION_LOG_CATEGORY(lcFile, "pinglymotion.file")
// the animation model: keys, frames, joints
PINGLYMOTION_LOG_CATEGORY(lcAnimation, "pinglymotion.animation")
// the 3D view, the timeline and the window
PINGLYMOTION_LOG_CATEGORY(lcView, "pinglymotion.view")
// the control commands
PINGLYMOTION_LOG_CATEGORY(lcControl, "pinglymotion.control")

#endif
