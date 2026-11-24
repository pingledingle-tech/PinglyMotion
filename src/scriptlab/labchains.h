/*
 * PinglyMotion Script Lab - chain and ribbon lines between two parts (job A4, part 2)
 *
 * A script makes a chain or a ribbon in Second Life with particles that fly
 * from one prim to a target: llParticleSystem or llLinkParticleSystem with
 * PSYS_SRC_TARGET_KEY (LockGuard and LockMeister chains work this way). The
 * sandbox logs the prim that emits and the target's link when the target is a
 * prim of the same object (lsl::Interpreter, "emitter_link" and "target_link").
 * This file reads a run's log and says which chains are there at the end:
 * one per emitting prim, the newest wins, an empty rule list removes it.
 *
 * A chain is locked when the object holds @detach (the same rule as the
 * recipe's facts, labrecipe.h), free otherwise. The 3D view draws a straight
 * line between the two props (no physics): solid in the danger colour when
 * locked, dashed in the success colour when free, always with the word
 * Locked or Free beside it (design charter P10).
 *
 * QtCore only. Licensed under the GNU General Public License version 2 or later.
 */

#ifndef LABCHAINS_H
#define LABCHAINS_H

#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QVector>

class RlvState;

namespace LabChains
{
  struct Chain
  {
    int fromLink = 0;
    QString from;          // the prim that emits (the name of its prop)
    int toLink = 0;
    QString to;            // the target prim
    bool locked = false;
    QString file;          // the script that made it
    // "Locked" or "Free": the word drawn beside the line
    QString word() const;
    // "Cuff_left to Collar: locked."
    QString words() const;
    bool operator==(const Chain& o) const
    {
      return fromLink == o.fromLink && from == o.from && toLink == o.toLink && to == o.to && locked == o.locked && file == o.file;
    }
  };

  // the chains at the end of a run's log; prims: link number to name (link 1 first)
  QVector<Chain> fromLog(const QJsonArray& log, const QList<QPair<int, QString>>& prims, bool locked);
  // the chains of LabWatcher::folderResult; locked: the object holds @detach
  QVector<Chain> fromFolderResult(const QJsonObject& folderResult, bool locked);
  // true when the object of the folder result holds @detach (without an option) in rlv
  bool objectLocked(const QJsonObject& folderResult, const RlvState& rlv);
}

#endif
