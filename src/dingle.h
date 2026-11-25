/*
 * PinglyMotion - the .dingle working file format (issue #26, docs/DINGLE_FORMAT.md)
 *
 * Reader and writer on top of AnimFile::Animation. The reader works from
 * memory, returns the whole animation or an error (never half a result, never
 * a crash) and checks every count before allocating. The writer is a small
 * emitter of its own with a fixed key order, so a saved file is byte-identical
 * every time; QJsonDocument is used only for reading.
 *
 * Only the key frames are stored. The frames in between are computed on load
 * with the interpolation of BVHNode::interpolate (src/bvhnode.cpp): linear, or a
 * sine curve on the ease flags.
 *
 * A parsed animation has format Avm: every joint has keys (frame 0 first),
 * hasPositionKeys is set and positionKeys is never empty. Joints are in depth
 * first order (the reader refuses other orders; the data model needs it).
 * The position keys of the model are shared by all joints that have position
 * channels, so those joints must list the same frames and ease flags.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef DINGLE_H
#define DINGLE_H

#include <QByteArray>
#include <QJsonObject>
#include <QString>

#include <optional>

#include "animfile.h"
#include "propfile.h"

namespace Dingle
{
  constexpr int CURRENT_VERSION=1;

  // limits for files that are read (the ones of AnimFile apply as well)
  constexpr qsizetype MAX_FILE_BYTES=16*1024*1024;
  constexpr int MAX_JSON_DEPTH=16;
  constexpr int MAX_NAME_BYTES=128;
  constexpr int MAX_GENERATOR_BYTES=128;
  constexpr qsizetype MAX_EXTENSIONS_BYTES=1024*1024;

  struct Result
  {
    // set when the file was read; then error is empty
    std::optional<AnimFile::Animation> animation;
    // keys this program does not know, in objects it knows. Save should warn
    // once when this is not 0, because they are lost on save.
    int ignoredKeys=0;
    // the "extensions" object, kept untouched (empty when there is none)
    QJsonObject extensions;
    // the props saved with the animation (the "props" list; empty when there is none, as in older files)
    QList<PropRecord> props;
    // why the file cannot be read, for the user
    QString error;
    // true when the file is from a newer version of the format
    bool newerVersion=false;

    bool ok() const { return animation.has_value(); }
  };

  Result parse(const QByteArray& data);

  // The text of the .dingle file. The animation must follow the rules of a
  // parsed one (see animfile.h). BVH animations (no keys) get a key at every frame.
  QByteArray write(const AnimFile::Animation& anim,
                   const QJsonObject& extensions=QJsonObject(),
                   const QString& generator=QStringLiteral("PinglyMotion"),
                   const QList<PropRecord>& props=QList<PropRecord>());

  // Same joints, timing, keys, loop, figure, scale and every value; the
  // format field is not compared.
  bool equal(const AnimFile::Animation& a,const AnimFile::Animation& b);
}

#endif
