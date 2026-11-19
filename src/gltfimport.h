/*
 * PinglyMotion - glTF 2.0 animation import, step 1 of issue #12
 *
 * Reads a .glb file or a .gltf (JSON) file from memory and turns one of its
 * animations into an AnimFile::Animation on the Second Life skeleton, through
 * the retargeting core (src/retarget.*, issue #16). It never touches the disk
 * or the window, and either returns a whole animation or an error message.
 * Nothing in the program uses it yet; the menu comes in a later job.
 *
 * What is read: the node tree, the skins (joint list, inverse bind matrices)
 * and the rotation and translation channels of the animations (LINEAR, STEP
 * and CUBICSPLINE). What is never read: meshes, pictures, materials, cameras.
 * Scale and shape key (morph target) animation is ignored and listed in the
 * notes, because Second Life cannot store it.
 *
 * Safety (docs/SECURITY_AUDIT.md section 8, the plan in issue #12):
 *  - only data inside the file is used: the binary chunk of a GLB file, or
 *    base64 "data:" URIs. A buffer that points to another file or to a web
 *    address is refused; no file is ever opened and nothing is downloaded.
 *  - every limit below is checked before memory is allocated for it.
 *  - every list of numbers (accessor) must end inside its buffer view, and
 *    the view inside its buffer (64-bit sums, no overflow).
 *  - all numbers must be finite; key times must rise; a turn of length zero
 *    is refused; the node tree must be a tree.
 *  - sparse accessors and compressed data (meshopt) are refused with a
 *    clear message.
 *
 * The JSON part is read with Qt's JSON reader (the choice of the plan in
 * issue #12: our own small reader on top of Qt, no new library).
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef GLTFIMPORT_H
#define GLTFIMPORT_H

#include "animfile.h"
#include "retarget.h"

#include <QByteArray>
#include <QMatrix4x4>
#include <QQuaternion>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QVector3D>

#include <optional>
#include <vector>

namespace GltfImport
{
  // limits for files that are read
  constexpr qsizetype MAX_FILE_BYTES=AnimFile::MAX_FILE_BYTES;  // whole file, 64 MB
  constexpr qsizetype MAX_JSON_BYTES=8*1024*1024;               // the JSON part
  constexpr int MAX_JSON_DEPTH=64;                              // nesting of [ and {
  constexpr int MAX_BUFFERS=64;
  constexpr int MAX_BUFFER_VIEWS=65536;
  constexpr int MAX_ACCESSORS=65536;
  constexpr int MAX_NODES=65536;
  constexpr int MAX_NODE_DEPTH=256;                             // nesting of all nodes
  constexpr int MAX_SKINS=256;
  constexpr int MAX_ANIMATIONS=256;
  constexpr int MAX_CHANNELS=4096;                              // channels or samplers of one animation
  constexpr int MAX_KEYS=1000000;                               // key times of one channel
  constexpr qint64 MAX_DECODED_FLOATS=16*1024*1024;             // all key data of one file together
  constexpr double MAX_KEY_TIME=3600.0;                         // seconds
  constexpr double MAX_COORDINATE=1.0e6;                        // translations, in file units
  constexpr float MIN_SCALE=1.0e-6f;
  constexpr float MAX_SCALE=1.0e6f;
  constexpr int MAX_JOINTS=AnimFile::MAX_JOINTS;
  constexpr int MAX_FRAMES=AnimFile::MAX_FRAMES;
  constexpr double MIN_FRAMES_PER_SECOND=1.0;
  constexpr double MAX_FRAMES_PER_SECOND=1.0/AnimFile::MIN_FRAME_TIME;
  constexpr double SL_MAX_SECONDS=60.0;                         // longer animations are refused by the SL upload

  struct Node
  {
    QString name;                 // may be empty
    int parent=-1;
    QVector<int> children;
    // the rest pose (a "matrix" in the file is split into these)
    QVector3D translation;
    QQuaternion rotation;         // unit length
    QVector3D scale{1.0f,1.0f,1.0f};  // every part > 0
    int skin=-1;                  // the skin of this node's mesh, -1 if none
  };

  struct Skin
  {
    QString name;
    QVector<int> joints;          // node indices, each once, 1..MAX_JOINTS of them
    std::vector<QMatrix4x4> inverseBind;  // empty, or one per joint
    int skeleton=-1;              // the node given as the skeleton root, -1 if none
  };

  enum class Path : quint8 { Translation, Rotation };
  enum class Interpolation : quint8 { Linear, Step, CubicSpline };

  struct Channel
  {
    int node=-1;
    Path path=Path::Rotation;
    Interpolation interpolation=Interpolation::Linear;
    std::vector<float> times;     // seconds, finite, >= 0, strictly rising
    // 3 numbers per value (translation) or 4 (rotation x y z w, unit length).
    // CUBICSPLINE stores in-tangent, value, out-tangent for every key.
    std::vector<float> values;

    int components() const { return path==Path::Rotation ? 4 : 3; }
  };

  struct Clip
  {
    QString name;
    QVector<Channel> channels;    // only rotation and translation; at most one per node and path
    double start=0.0;             // first and last key time of all channels
    double end=0.0;
    QStringList ignored;          // what was skipped, for the user
  };

  struct Document
  {
    QVector<Node> nodes;
    QVector<Skin> skins;
    QVector<Clip> animations;
  };

  struct ReadResult
  {
    std::optional<Document> document;
    QString error;                // why the file cannot be read, for the user

    bool ok() const { return document.has_value(); }
  };

  // read a .glb or .gltf file from memory (the format is found from the bytes)
  ReadResult read(const QByteArray& data);

  enum class RestPose
  {
    Nodes,          // the rest turns and positions stored on the nodes
    BindMatrices    // the pose the mesh was bound in (the skin's inverse bind matrices)
  };

  struct Options
  {
    int animation=0;              // index into Document::animations
    int skin=0;                   // index into Document::skins (ignored when the file has none)
    double framesPerSecond=30.0;
    RestPose restPose=RestPose::Nodes;
    Retarget::Options retarget;
  };

  struct SourceResult
  {
    // a BVH style animation on the skeleton of the file (metres, the axes of
    // the file): the rest pose has no turns, every joint has three rotation
    // channels, the root and every joint with translation keys have three
    // position channels too
    std::optional<AnimFile::Animation> animation;
    QString error;
    QStringList notes;            // what was skipped or might be wrong, for the user

    bool ok() const { return animation.has_value(); }
  };

  // sample one animation of a document to a fixed frame rate
  SourceResult toSource(const Document& document,const Options& options=Options());

  struct Result
  {
    std::optional<AnimFile::Animation> animation;  // on the target skeleton
    QString error;
    QStringList notes;
    Retarget::Mapping mapping;

    bool ok() const { return animation.has_value(); }
  };

  // read, sample and retarget onto the target (a Second Life skeleton, for
  // example bin/data/TPose.bvh)
  Result importAnimation(const QByteArray& data,const AnimFile::Animation& target,
                         const Options& options=Options());
}

#endif
