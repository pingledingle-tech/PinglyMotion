/*
 * PinglyMotion Script Lab - RLV suggestions for the script editor (lane 1, job R3)
 *
 * A provider for the suggestion engine (labsuggest.h). It answers only inside the text of the
 * owner-say call (llOwnerSay("@...") in LSL; the name comes from the language layer, so SLua's
 * ll.OwnerSay can be added later by letting handles() take SLua) when that text starts with "@":
 *  - after "@" or ",": the RLV commands of the behaviour table, each with one plain sentence, one
 *    example (Ctrl+Enter puts it in) and, for Expert mode, a dense line (restriction, force or
 *    reply; RLV and RLVa, RLVa only or RLV only; group; the option and its value type);
 *    the families (setdebug_, setenv_, getdebug_, getenv_) are listed with their ending;
 *  - after "<name>:": the option values the value type knows (attachment points, clothing
 *    layers, camera modes), or one sample value and what the option must be;
 *  - after "=": the forms the command takes, in the order n, y, add, rem, force, channel.
 * Commands that only RLVa knows are marked "RLVa only" while the viewer rule is RLV (and those
 * only RLV knows "RLV only" while it is RLVa). QtCore only, tested without a screen.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef RLVSUGGEST_H
#define RLVSUGGEST_H

#include "labsuggest.h"
#include "rlvbehaviours.h"

namespace LabSuggest
{
  class RlvProvider : public Provider
  {
    public:
      QString id() const override { return QStringLiteral("rlv"); }
      // LSL only for now; the owner-say name comes from the language, so SLua is one line later
      bool handles(const Language& language) const override { return language.id() == QLatin1String("lsl"); }
      QList<Item> suggest(const Context& context, const Language& language) const override;

      // the panel's viewer rule (RLVa at start), for the marks
      void setMode(Rlv::Mode mode) { m_mode = mode; }
      Rlv::Mode mode() const { return m_mode; }

      // where the cursor is inside an owner-say RLV text
      enum class Part { None, Name, Option, Form };
      struct Place
      {
        Part part = Part::None;
        QString behaviour;     // the command name before ":" or "=" (Option, Form)
        QString typed;         // what is typed of the part, lower case
        int partStart = -1;    // where the typed part starts in the text
        int commandStart = -1; // where the command starts (after "@" or ",")
      };
      static Place place(const Context& context, const Language& language);

      // the help item for a command name (also for a family member such as setenv_daytime);
      // empty name when the command is unknown
      Item commandItem(const QString& name) const;
      // the dense Expert line for a behaviour, e.g.
      // "Restriction, force · RLV and RLVa · Group: inventory · Option: attachment point, optional"
      static QString expertLine(const Rlv::Behaviour& behaviour);
      // the plain sentence for what an option must be, e.g. "A number, such as 1.5."
      static QString optionSentence(Rlv::ValueType type);

    private:
      Rlv::Mode m_mode = Rlv::Mode::Rlva;
      QString markFor(const Rlv::Behaviour& behaviour) const;
  };
}

#endif
