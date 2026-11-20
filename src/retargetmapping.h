/*
 * PinglyMotion – hand-made joint mappings for BVH retargeting (issue #16, step 4)
 *
 * When File > Import BVH from another skeleton matches the joints by name, the
 * user may want to correct a few of them by hand. This file holds the core of
 * that, with no window: a small JSON file format (`.jointmap`, see
 * docs/RETARGET_MAPPING.md) with the source joint names and the Second Life
 * joint they go to, a reader that checks everything and a writer with a fixed
 * layout, a fingerprint that tells one source skeleton from another, a
 * function that puts the hand-made entries on top of the automatic mapping of
 * src/retarget.*, and a store that keeps one file per fingerprint in the
 * user's data folder. Editor holds the state of the dialog that edits the
 * mapping (src/retargetdialog.*), with no window, so it can be tested.
 *
 * Like the other readers, parse() takes any bytes and either returns a whole
 * mapping or an error message, never half a result and never a crash.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef RETARGETMAPPING_H
#define RETARGETMAPPING_H

#include "animfile.h"
#include "retarget.h"

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QVector>

#include <optional>

namespace RetargetMapping
{
  // the file format: docs/RETARGET_MAPPING.md
  constexpr int CURRENT_VERSION=1;
  constexpr const char* FORMAT_NAME="pinglymotion-jointmap";
  constexpr const char* FILE_SUFFIX="jointmap";

  // limits of the reader (a file over any of them is refused)
  constexpr qsizetype MAX_FILE_BYTES=1024*1024;
  constexpr int MAX_JSON_DEPTH=4;
  constexpr int MAX_ENTRIES=AnimFile::MAX_JOINTS;        // 256, one per source joint at most
  constexpr int MAX_SOURCE_NAME_BYTES=256;               // Retarget::MAX_NAME_LENGTH
  constexpr int MAX_TARGET_NAME_BYTES=64;                // SL joint names are short
  constexpr int MAX_LABEL_BYTES=128;                     // "generator" and "root"
  constexpr int FINGERPRINT_LENGTH=64;                   // SHA-256 as lower case hex

  // one hand-made entry: a joint of the user's file and the Second Life joint
  // it drives; an empty target means "leave this joint out"
  struct Entry
  {
    QString source;
    QString target;   // a canonical SL name (mShoulderLeft), or empty

    bool operator==(const Entry& o) const { return source==o.source && target==o.target; }
    bool operator!=(const Entry& o) const { return !(*this==o); }
  };

  struct Mapping
  {
    int version=CURRENT_VERSION;
    // fingerprint of the source skeleton the entries belong to (see
    // fingerprint() below); empty means "any skeleton with these joint names"
    QString fingerprint;
    // readable hints about that skeleton, written for people and assistants
    // who open the file; the reader does not check them against anything
    int jointCount=0;        // joints of the source skeleton without End Sites, 0 when unknown
    QString root;            // name of its root joint, may be empty
    QString generator;       // who wrote the file, may be empty
    QVector<Entry> entries;

    bool operator==(const Mapping& o) const
    { return version==o.version && fingerprint==o.fingerprint && jointCount==o.jointCount &&
             root==o.root && generator==o.generator && entries==o.entries; }
    bool operator!=(const Mapping& o) const { return !(*this==o); }
  };

  struct Result
  {
    std::optional<Mapping> mapping;   // set when the file was read; then error is empty
    QString error;                    // why it was refused, for the user
    bool newerVersion=false;          // the file needs a newer program

    bool ok() const { return mapping.has_value(); }
  };

  // ---- Second Life joint names -------------------------------------------

  // the canonical name of a Second Life joint ("mShoulderLeft") for a name
  // as it appears in files ("lShldr", "avatar_mShoulderLeft", "mshoulderleft"),
  // or empty when the name is not a joint of the Second Life skeleton
  QString canonicalSlName(const QString& name);
  // all canonical names, in skeleton order (133 joints)
  const QStringList& slJointNames();

  // ---- fingerprint ---------------------------------------------------------

  // a fingerprint of a source skeleton: SHA-256 of its joint names and tree
  // (End Sites, offsets and channels do not count), as 64 lower case hex
  // characters; empty when the skeleton is not usable
  QString fingerprint(const AnimFile::Animation& source);
  bool validFingerprint(const QString& text);

  // ---- file format ---------------------------------------------------------

  // read a .jointmap file from memory with every check of docs/RETARGET_MAPPING.md
  Result parse(const QByteArray& data);
  // the checks of parse() on a mapping made in the program; empty when it is fine
  QString validate(const Mapping& mapping);
  // the text of the file: fixed key order and layout, so the same mapping
  // gives the same bytes every time. The mapping must pass validate().
  QByteArray write(const Mapping& mapping);

  // ---- on top of the automatic mapping -------------------------------------

  struct Applied
  {
    Retarget::Mapping mapping;   // the automatic mapping with the entries on top
    int applied=0;               // entries that changed or confirmed a pair
    int leftOut=0;               // entries with an empty target that took a joint out
    QStringList notes;           // entries that could not be used, one line each
  };

  // put the hand-made entries on top of the automatic mapping (the result of
  // Retarget::mapSkeletons). A hand-made entry always wins: a target joint
  // the automatic mapping gave to another source joint is taken from it, and
  // an entry with an empty target takes the source joint out. Entries whose
  // source joint is not in the source, or whose target is not in the target
  // skeleton, are skipped with a note. The mapping and the fingerprint are
  // not compared here; see Store::load.
  Applied apply(const Retarget::Mapping& automatic,const Mapping& handMade,
                const AnimFile::Animation& source,const AnimFile::Animation& target);

  // the entries that describe a mapping the user changed: every pair whose
  // target differs from the automatic mapping, and every automatic pair that
  // was taken out (empty target). Used by Editor::toFile, and by tests.
  QVector<Entry> differences(const Retarget::Mapping& automatic,const Retarget::Mapping& edited,
                             const AnimFile::Animation& source,const AnimFile::Animation& target);

  // ---- store ----------------------------------------------------------------

  // one file per fingerprint, "<fingerprint>.jointmap", in a folder of the user
  class Store
  {
    public:
      // the user's folder: PINGLYMOTION_MAPPING_DIR when that environment
      // variable is set (tests), else "PinglyMotion/skeleton-maps" in the
      // data folder (next to the control connection file)
      static QString defaultFolder();

      explicit Store(const QString& folder=defaultFolder());

      QString folder() const { return m_folder; }
      // the file for a fingerprint; empty when the fingerprint is not valid
      QString filePath(const QString& fingerprint) const;
      bool exists(const QString& fingerprint) const;

      struct LoadResult
      {
        std::optional<Mapping> mapping;
        QString error;        // set when a file is there but cannot be used
        bool found=false;     // a file for the fingerprint is there

        bool ok() const { return mapping.has_value(); }
      };
      // the saved mapping for a skeleton; not found is no error (found false,
      // error empty). A file whose own fingerprint is another one is refused.
      LoadResult load(const QString& fingerprint) const;

      // write the mapping as the file for its fingerprint (the mapping must
      // have one); the folder is created. Empty when it worked, else the error.
      QString save(const Mapping& mapping) const;
      // delete the file for a fingerprint; true when there is none afterwards
      bool remove(const QString& fingerprint) const;

    private:
      QString m_folder;
  };

  // ---- editing (the state behind the mapping dialog) ----------------------

  // the automatic mapping of one source skeleton, the hand-made changes on
  // top of it, and saving and loading them per skeleton. No window: the
  // dialog in src/retargetdialog.* shows it, and the tests drive it directly.
  class Editor
  {
    public:
      Editor(const AnimFile::Animation& source,const AnimFile::Animation& target);

      // the fingerprint of the source skeleton; empty when the skeleton is not
      // usable (then nothing can be saved)
      QString fingerprint() const { return m_fingerprint; }

      const Retarget::Mapping& automatic() const { return m_automatic; }
      // the mapping with the hand-made changes; this is what the import uses
      const Retarget::Mapping& current() const { return m_current; }

      // the target joints a source joint can be given, in skeleton order:
      // joints of the Second Life skeleton with three rotation channels
      const QVector<int>& choices() const { return m_choices; }
      // a choice that the retargeting knows as part of the body (pelvis, spine,
      // arms, hands, fingers, legs, feet); the others are face, wings, tail...
      bool isBodyJoint(int targetJoint) const;

      // the target joint a source joint drives now, -1 when it is left out
      int targetOf(int sourceJoint) const;
      // the source joint that drives a target joint now, -1 when none
      int sourceOf(int targetJoint) const;

      // give a source joint another target joint (-1: leave it out). A target
      // joint that another source joint had is taken from it. False (and no
      // change) when a joint is not valid or not one of choices().
      bool setTarget(int sourceJoint,int targetJoint);
      // back to the automatic mapping
      void reset();

      // the source joint has another target than the automatic mapping gave it
      bool changedByHand(int sourceJoint) const;
      bool hasChanges() const;

      // the hand-made changes as a file, with the fingerprint and the hints
      Mapping toFile(const QString& generator=QString()) const;
      // put a saved mapping on top of the automatic one (replacing any change
      // made so far); the notes of apply() for entries that could not be used
      QStringList use(const Mapping& saved);

      struct Loaded
      {
        bool found=false;     // a saved mapping for this skeleton is there
        QString error;        // it is there but could not be used
        QStringList notes;    // entries of it that were skipped
      };
      // load the saved mapping for this skeleton from a store and use it
      Loaded load(const Store& store);
      // save the changes for this skeleton; with no changes, any saved file is
      // deleted, so the next import uses the automatic mapping again.
      // Empty when it worked, else the error.
      QString save(const Store& store,const QString& generator=QString()) const;

    private:
      AnimFile::Animation m_source;
      AnimFile::Animation m_target;
      QString m_fingerprint;
      Retarget::Mapping m_automatic;
      Retarget::Mapping m_current;
      QVector<int> m_choices;
      QVector<int> m_automaticTarget;   // per source joint
  };
}

#endif
