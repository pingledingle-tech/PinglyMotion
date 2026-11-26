/*
 * PinglyMotion Script Lab - reference answers for RLV (package B, quality plan)
 *
 * A reference file (.rlvref) holds what a real viewer answered to RLV questions, so the sandbox can be
 * compared with it. Plain text, one line each. Lines starting with # are notes. The header comes first
 * and has three lines that must all be there:
 *
 *   viewer Firestorm 7.1.14 (RLVa 2.4.2)      which viewer answered (free text)
 *   date 2026-11-26                           when the answers were recorded
 *   source recorded                           "recorded" (in Second Life) or "specification"
 *
 * A file with "source specification" was written from the RLV specification and not recorded in Second
 * Life; the runner says so each time. Then cases:
 *
 *   case getstatus_after_sendim               a case starts with its name (letters, digits, "_", "-", ".")
 *   mode rlva                                 optional: "rlva" (the default) or "rlv", the sandbox's dialect
 *   object collar                             optional: the object the next "send" lines come from ("ref")
 *   send @sendim=n                            an RLV line, as an object says it to the owner
 *   send @getstatus=2222                      a question: the viewer answers on the channel
 *   answer 2222 /sendim                       what the viewer said on that channel (the text may be empty)
 *   prefix 2222 RestrainedLife viewer v      the answer starts with this text (for answers that differ by viewer)
 *   number 2222                               the answer is a whole number, digits only (@versionnum)
 *   any 2222                                  the viewer answered something, whatever it was (for answers that
 *                                             hold personal data, such as the name of the active group)
 *   silent                                    the line before gave no answer at all
 *
 * "answer", "prefix", "number", "any" and "silent" belong to the "send" line above them. The answers of one "send" are compared in
 * order with what the sandbox answers to that line: the same channel, the same text letter for letter,
 * and no answer left over. A "send" with none of them below it is not checked (it only sets
 * something up). A case must have at least one of them, or it tests nothing.
 *
 * run() answers with a plain list of differences. A list of known differences (known.txt, one line each:
 * "<state> <file>/<case> <reason>", state fixed, intended or unclear) says which differences are known.
 * What a reference file says is a sandbox comparison, never a test in Second Life. Text from outside is
 * untrusted: it is only read and compared, never run.
 *
 * QtCore only. Licensed under the GNU General Public License version 2 or later.
 */

#ifndef RLVREFERENCE_H
#define RLVREFERENCE_H

#include <QString>
#include <QStringList>
#include <QVector>

#include <optional>

#include "rlvbehaviours.h"

namespace RlvReference
{
  static constexpr qint64 MAX_FILE_BYTES = 262144;
  static constexpr int MAX_LINE_CHARS = 4096;      // a long answer fits on one line
  static constexpr int MAX_CASES = 500;
  static constexpr int MAX_LINES_PER_CASE = 200;
  static constexpr int MAX_NAME_CHARS = 64;

  struct Line
  {
    enum class Kind { Send, Answer, Prefix, Number, Any, Silent };
    Kind kind = Kind::Send;
    QString text;            // send: the RLV line; answer: the text; prefix: the start of the text
    int channel = 0;         // answer
    QString object;          // send: the object the line comes from
    int line = 0;            // in the file, from 1
  };

  struct Case
  {
    QString name;
    Rlv::Mode mode = Rlv::Mode::Rlva;
    QVector<Line> lines;
    int line = 0;
  };

  struct Reference
  {
    QString viewer;
    QString date;
    QString source;          // "recorded" or "specification"
    QVector<Case> cases;
    bool fromSpecification() const { return source == QLatin1String("specification"); }
  };

  struct Result
  {
    std::optional<Reference> value;
    QString error;           // for the user, with the line number, empty when it worked
    bool ok() const { return error.isEmpty(); }
  };

  Result parse(const QString& text);
  // the file as text again (header, cases); parse(write(r)) gives r back
  QString write(const Reference& reference);
  // "" when the name is allowed for a case
  QString nameError(const QString& name);

  struct Difference
  {
    int line = 0;            // the answer, silent or send line in the file
    QString expected;        // "2222: /sendim" or "no answer"
    QString got;             // "2222: /sendim" or "no answer"
    QString text() const;    // "Line 7: expected 2222: "/sendim", but the sandbox answered ..."
  };

  struct CaseResult
  {
    QString name;
    QVector<Difference> differences;
    bool same() const { return differences.isEmpty(); }
  };

  // Runs every case of the reference in its own fresh sandbox state and lists the differences.
  QVector<CaseResult> run(const Reference& reference);

  // Known differences
  enum class KnownState { Fixed, Intended, Unclear };
  struct Known
  {
    KnownState state = KnownState::Unclear;
    QString key;             // "<file>/<case>", the file name without .rlvref
    QString reason;
    int line = 0;
  };
  struct KnownResult
  {
    QVector<Known> list;
    QString error;
    bool ok() const { return error.isEmpty(); }
  };
  KnownResult parseKnown(const QString& text);
  QString stateName(KnownState state);   // "fixed", "intended", "unclear"
}

#endif
