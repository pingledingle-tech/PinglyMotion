/*
 * PinglyMotion Script Lab - what a script run does to the avatar (issue 293)
 *
 * Plain data, no drawing: reads the answer of one script run (the object that
 * lsl_run gives and a Live run keeps under "result") and works out what the
 * avatar shows at the end of the run:
 *   - the animations still playing (llStartAnimation without a later
 *     llStopAnimation), each with the simulated time it started;
 *   - the worn objects (the answer's "objects" with an attachment point) and
 *     whether an object locks itself with @detach=n;
 *   - attachment points locked with @detach:<point>=n.
 *
 * Also the safe lookup of an animation by its inventory name (a file
 * <name>.dingle, .bvh or .avm in a list of folders, compared with the folder
 * listing, never built into a path) and the frame an animation shows a given
 * number of seconds after it started.
 *
 * Attachment point numbers and names are Second Life's (the LSL ATTACH_*
 * constants); each point is drawn at the nearest joint of the SL skeleton.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef LABEFFECTS_H
#define LABEFFECTS_H

#include <QJsonObject>
#include <QList>
#include <QString>
#include <QStringList>

namespace LabEffects
{
  constexpr int MAX_NAME_CHARS = 63;        // Second Life inventory names
  constexpr int MAX_SEARCH_DEPTH = 2;       // folder levels below each search folder
  constexpr int MAX_SEARCH_ENTRIES = 5000;  // entries looked at in one search, in all
  constexpr qint64 MAX_FILE_BYTES = 16 * 1024 * 1024;

  // Second Life attachment points 1 to 55; "" for any other number
  QString pointName(int point);
  // the point of a name such as "chest" or "Left Hand" (case ignored); 0 when unknown
  int pointNumber(const QString& name);
  // HUD points (31 to 38) are on the screen, not on the avatar
  bool isHudPoint(int point);
  // the joint of the SL skeleton the point is drawn at, most fitting first; empty for HUD and unknown points
  QStringList jointsForPoint(int point);

  struct Playing
  {
    QString name;
    double started = 0;    // simulated seconds
  };

  struct Worn
  {
    QString object;        // the object's name
    QString key;
    int point = 0;
    bool locked = false;   // the object holds @detach=n
  };

  struct Effects
  {
    QList<Playing> playing;      // still playing at the end, oldest start first
    double endTime = 0;          // simulated seconds at the end of the run
    QList<Worn> worn;            // in the order of the answer's objects
    QList<int> lockedPoints;     // @detach:<point>=n, sorted, each once
    QStringList unwornLocks;     // names of objects holding @detach=n that are not worn
    QStringList unknownPoints;   // @detach:<option> options that are no attachment point
    bool logTruncated = false;   // the log was cut, so some effects may be missing
  };

  // reads one run answer ({"ok": true, "log": [...], "time": ..., "rlv": {...}, "objects": [...]}).
  // before: what was playing when this log began (an lsl_event runs on the world of the last lsl_run,
  // and its log has only the new entries)
  Effects fromRun(const QJsonObject& run, const QList<Playing>& before = QList<Playing>());

  // The frame (0-based) an animation shows elapsed seconds after it started: it starts on frame 1
  // (frame 0 is the reference pose; frame 0 for a one-frame animation), moves one frame each frameTime
  // and repeats between the loop points loopIn and loopOut (0-based, clamped to the frames).
  int frameAt(double elapsed, double frameTime, int frames, int loopIn, int loopOut);

  // true for a name a script may ask for: 1 to MAX_NAME_CHARS characters, no folder separators,
  // no characters a file name cannot hold, no control characters, not starting with a dot
  bool validAnimationName(const QString& name);

  struct Found
  {
    QString path;          // absolute path of the file
    QString error;         // empty when found
    bool ok() const { return error.isEmpty(); }
  };

  // Looks for <name>.dingle, <name>.bvh or <name>.avm (in that order) in each folder and up to
  // MAX_SEARCH_DEPTH levels below it, folders in the given order, shallower first. Hidden entries
  // and links are skipped; files over MAX_FILE_BYTES are not taken. Names are compared with the
  // folder listing (case matters, as in Second Life), so a name can never point outside the folders.
  Found findAnimation(const QString& name, const QStringList& folders);
}

#endif
