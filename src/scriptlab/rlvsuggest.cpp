/*
 * PinglyMotion Script Lab - RLV suggestions for the script editor (lane 1, job R3)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QCoreApplication>
#include <QSet>

#include "rlvsuggest.h"

namespace LabSuggest
{
  namespace
  {
    QString tr(const char* text) { return QCoreApplication::translate("LabSuggest", text); }

    // a made-up key in the right shape (the examples use the same one)
    const char* SampleKey = "a1b2c3d4-0000-4000-8000-000000000001";

    // the type an option of this behaviour has before the form is typed: the restriction's
    // when it has restriction forms, else the force form's; a type with a list of names wins
    Rlv::ValueType optionType(const Rlv::Behaviour& b)
    {
      QList<Rlv::ValueType> types;
      if (b.params & Rlv::AllowAddRemove) types << b.value.restriction;
      if (b.params & Rlv::AllowForce) types << b.value.force;
      for (Rlv::ValueType t : types)
        if (t == Rlv::ValueType::AttachPoint || t == Rlv::ValueType::Layer || t == Rlv::ValueType::CameraMode) return t;
      for (Rlv::ValueType t : types)
        if (t != Rlv::ValueType::Unchecked && t != Rlv::ValueType::None) return t;
      return Rlv::ValueType::Unchecked;
    }

    // the option of an example line ("@sittp:1.5=n" gives "1.5"), or empty
    QString exampleOption(const QString& example)
    {
      const Rlv::Command c = Rlv::parseCommand(example.mid(1));
      return c.valid ? c.option : QString();
    }

    QString sampleFor(Rlv::ValueType type)
    {
      switch (type)
      {
        case Rlv::ValueType::Uuid:
        case Rlv::ValueType::TextureUuid:
        case Rlv::ValueType::UuidOrDistance:
        case Rlv::ValueType::UuidOrNumber: return QString::fromLatin1(SampleKey);
        case Rlv::ValueType::Number: return QStringLiteral("1.5");
        case Rlv::ValueType::Integer: return QStringLiteral("2");
        case Rlv::ValueType::Channel: return QStringLiteral("5");
        case Rlv::ValueType::ReplyChannel:
        case Rlv::ValueType::Notify: return QStringLiteral("2222");
        case Rlv::ValueType::Vector3: return QStringLiteral("0/0/1");
        case Rlv::ValueType::Vector4: return QStringLiteral("8/6/0/0");
        default: return QString();
      }
    }
  }

  QString RlvProvider::optionSentence(Rlv::ValueType type)
  {
    switch (type)
    {
      case Rlv::ValueType::Uuid:
      case Rlv::ValueType::TextureUuid: return tr("A key (UUID): 36 letters and digits with dashes.");
      case Rlv::ValueType::UuidOrDistance: return tr("A key (UUID), or a distance in metres.");
      case Rlv::ValueType::UuidOrNumber: return tr("A key (UUID), or a number.");
      case Rlv::ValueType::Number: return tr("A number, such as 1.5.");
      case Rlv::ValueType::Integer: return tr("A whole number, such as 2.");
      case Rlv::ValueType::Channel:
      case Rlv::ValueType::ReplyChannel: return tr("A chat channel number, 1 or higher.");
      case Rlv::ValueType::Vector3: return tr("Three numbers with slashes, such as 0/0/1.");
      case Rlv::ValueType::Vector4: return tr("Four numbers with slashes, such as 8/6/0/0.");
      case Rlv::ValueType::AttachPoint: return tr("An attachment point on the avatar.");
      case Rlv::ValueType::Layer: return tr("A clothing layer.");
      case Rlv::ValueType::CameraMode: return tr("A camera mode.");
      case Rlv::ValueType::Notify: return tr("A channel number. Add ; and part of a rule name to filter.");
      case Rlv::ValueType::Unchecked: return tr("An example value. Change it to what you need.");
      case Rlv::ValueType::None: break;
    }
    return QString();
  }

  QString RlvProvider::expertLine(const Rlv::Behaviour& b)
  {
    QStringList kinds;
    if (b.params & Rlv::AllowAddRemove) kinds << tr("Restriction");
    if (b.params & Rlv::AllowForce) kinds << (kinds.isEmpty() ? tr("Force") : tr("force"));
    if (b.params & Rlv::AllowReply) kinds << (kinds.isEmpty() ? tr("Reply") : tr("reply"));
    QStringList parts{kinds.join(QStringLiteral(", "))};
    switch (b.dialect)
    {
      case Rlv::Dialect::Rlv: parts << tr("RLV and RLVa"); break;
      case Rlv::Dialect::RlvaOnly: parts << tr("RLVa only"); break;
      case Rlv::Dialect::RlvOnly: parts << tr("RLV only"); break;
    }
    if (b.rlvaOnlyParams & Rlv::AllowForce) parts.last() += tr(" (=force RLVa only)");
    parts << tr("Group: %1").arg(Rlv::groupName(b.group));
    if (b.option != Rlv::OptionUse::None)
    {
      const Rlv::ValueType type = optionType(b);
      const QString name = type == Rlv::ValueType::Unchecked ? tr("text") : Rlv::valueTypeName(type);
      parts << (b.option == Rlv::OptionUse::Required ? tr("Option: %1, needed").arg(name) : tr("Option: %1, optional").arg(name));
    }
    return parts.join(QStringLiteral(" · "));
  }

  QString RlvProvider::markFor(const Rlv::Behaviour& b) const
  {
    if (m_mode == Rlv::Mode::Rlv && b.dialect == Rlv::Dialect::RlvaOnly) return tr("RLVa only");
    if (m_mode == Rlv::Mode::Rlva && b.dialect == Rlv::Dialect::RlvOnly) return tr("RLV only");
    return QString();
  }

  RlvProvider::Place RlvProvider::place(const Context& context, const Language& language)
  {
    Place p;
    if (!context.inString || context.stringStart < 0) return p;
    const QString& text = context.text;
    // the string must be the first thing inside the owner-say call: name ( "
    int i = context.stringStart - 1;
    while (i >= 0 && text.at(i).isSpace()) --i;
    if (i < 0 || text.at(i) != QLatin1Char('(')) return p;
    --i;
    while (i >= 0 && text.at(i).isSpace()) --i;
    const int nameEnd = i + 1;
    while (i >= 0 && language.isNameChar(text.at(i))) --i;
    if (text.mid(i + 1, nameEnd - i - 1) != language.ownerSayName()) return p;
    if (i >= 0 && (text.at(i).isLetterOrNumber() || text.at(i) == QLatin1Char('_'))) return p;

    const int insideStart = context.stringStart + 1;
    if (context.cursor <= insideStart || text.at(insideStart) != QLatin1Char('@')) return p;
    const QString inside = text.mid(insideStart, context.cursor - insideStart);
    const int comma = int(inside.lastIndexOf(QLatin1Char(',')));
    p.commandStart = insideStart + (comma >= 0 ? comma + 1 : 1);
    const QString command = text.mid(p.commandStart, context.cursor - p.commandStart).toLower();
    const int equals = int(command.indexOf(QLatin1Char('=')));
    const int colon = int(command.indexOf(QLatin1Char(':')));
    if (equals >= 0)
    {
      p.part = Part::Form;
      p.behaviour = command.left(colon >= 0 && colon < equals ? colon : equals);
      p.typed = command.mid(equals + 1);
      p.partStart = p.commandStart + equals + 1;
    }
    else if (colon >= 0)
    {
      p.part = Part::Option;
      p.behaviour = command.left(colon);
      p.typed = command.mid(colon + 1);
      p.partStart = p.commandStart + colon + 1;
    }
    else
    {
      p.part = Part::Name;
      p.typed = command;
      p.partStart = p.commandStart;
    }
    return p;
  }

  Item RlvProvider::commandItem(const QString& name) const
  {
    Item it;
    const Rlv::Behaviour* b = Rlv::findBehaviour(name);
    for (const Rlv::Behaviour& family : Rlv::familyBehaviours())
      if (family.name == name) b = &family;   // the family itself ("setenv_")
    const Rlv::Help* help = Rlv::findHelp(name);
    if (!b || !help) return it;
    it.name = name;
    it.kind = Kind::Command;
    it.insert = name;
    it.plain = tr(help->sentence.toUtf8().constData());
    it.detail = expertLine(*b);
    it.example = help->example;
    it.exampleInsert = help->example.mid(1);
    it.mark = markFor(*b);
    it.provider = id();
    return it;
  }

  QList<Item> RlvProvider::suggest(const Context& context, const Language& language) const
  {
    QList<Item> out;
    const Place p = place(context, language);
    if (p.part == Part::None) return out;

    if (p.part == Part::Name)
    {
      if (p.typed.size() > Rlv::MAX_BEHAVIOUR_LENGTH) return out;
      QList<const Rlv::Behaviour*> all;
      for (const Rlv::Behaviour& b : Rlv::behaviours()) all << &b;
      for (const Rlv::Behaviour& b : Rlv::familyBehaviours()) all << &b;
      for (const Rlv::Behaviour* b : all)
      {
        const bool family = b->name.endsWith(QLatin1Char('_'));
        int tier = matchTier(b->name, p.typed);
        // a family stays in the list while its ending is typed: setenv_day still shows setenv_<setting>
        if (family && p.typed.size() > b->name.size() && p.typed.startsWith(b->name)) tier = 0;
        if (tier < 0 || tier == 2) continue;
        Item it = commandItem(b->name);
        if (it.name.isEmpty()) continue;
        if (family)
        {
          it.name = b->name + tr("<setting>");
          // taking it keeps an ending already typed
          it.insert = p.typed.size() > b->name.size() ? p.typed : b->name;
        }
        it.tier = tier;
        it.replaceFrom = p.partStart;
        it.exampleFrom = p.commandStart;
        out << it;
      }
      // @clear lifts this object's rules
      if (matchTier(QStringLiteral("clear"), p.typed) >= 0 && matchTier(QStringLiteral("clear"), p.typed) < 2)
      {
        Item it;
        it.name = QStringLiteral("clear");
        it.kind = Kind::Command;
        it.insert = it.name;
        it.plain = tr("Lifts every rule this object holds.");
        it.detail = tr("Clear · RLV and RLVa · @clear=<part of a name> lifts only those");
        it.example = QStringLiteral("@clear");
        it.exampleInsert = QStringLiteral("clear");
        it.exampleFrom = p.commandStart;
        it.replaceFrom = p.partStart;
        it.tier = matchTier(it.name, p.typed);
        it.provider = id();
        out << it;
      }
    }
    else if (p.part == Part::Option)
    {
      const Rlv::Behaviour* b = Rlv::findBehaviour(p.behaviour);
      if (!b || b->option == Rlv::OptionUse::None) return out;
      const Rlv::ValueType type = optionType(*b);
      const QString typeName = type == Rlv::ValueType::Unchecked ? tr("text") : Rlv::valueTypeName(type);
      QStringList values;
      if (type == Rlv::ValueType::AttachPoint) values = Rlv::rlvaAttachPointNames();
      else if (type == Rlv::ValueType::Layer) values = Rlv::rlvaLayerNames();
      else if (type == Rlv::ValueType::CameraMode) values = {QStringLiteral("mouselook"), QStringLiteral("thirdperson"), QStringLiteral("reset")};
      const bool list = !values.isEmpty();
      if (!list && p.typed.isEmpty())
      {
        const Rlv::Help* help = Rlv::findHelp(p.behaviour);
        QString sample = help ? exampleOption(help->example) : QString();
        if (sample.isEmpty() || (type != Rlv::ValueType::Unchecked && !Rlv::optionFits(type, sample))) sample = sampleFor(type);
        if (!sample.isEmpty()) values << sample;
      }
      for (const QString& v : values)
      {
        const int tier = matchTier(v, p.typed);
        if (tier < 0 || tier == 2) continue;
        Item it;
        it.name = v;
        it.kind = Kind::Command;
        it.insert = v;
        it.plain = optionSentence(type);
        it.detail = tr("Option of %1 · %2").arg(p.behaviour, typeName);
        it.tier = tier;
        it.replaceFrom = p.partStart;
        it.provider = id();
        out << it;
      }
    }
    else
    {
      const Rlv::Behaviour* b = Rlv::findBehaviour(p.behaviour);
      if (!b) return out;
      const QStringList forms = Rlv::formsOf(*b);
      for (int i = 0; i < forms.size(); ++i)
      {
        const QString& form = forms.at(i);
        const bool channel = form == QLatin1String("<channel>");
        const QString value = channel ? QStringLiteral("2222") : form;
        int tier = channel ? ((p.typed.isEmpty() || (p.typed.at(0).isDigit() && QStringLiteral("2222").startsWith(p.typed))) ? 0 : -1)
                           : matchTier(form, p.typed);
        if (tier < 0 || tier == 2) continue;
        Item it;
        it.name = value;
        it.kind = Kind::Command;
        it.insert = value;
        if (form == QLatin1String("n")) it.plain = tr("Turns the rule on.");
        else if (form == QLatin1String("y")) it.plain = tr("Lifts the rule again.");
        else if (form == QLatin1String("add")) it.plain = tr("The same as n: turns the rule on.");
        else if (form == QLatin1String("rem")) it.plain = tr("The same as y: lifts the rule again.");
        else if (form == QLatin1String("force")) it.plain = tr("Does it once, right now.");
        else it.plain = tr("A channel number. The answer comes back on it.");
        it.detail = channel ? tr("Reply · any channel from 1, here 2222") : tr("Form of %1").arg(p.behaviour);
        if (m_mode == Rlv::Mode::Rlv && Rlv::formIsRlvaOnly(*b, form)) it.mark = tr("RLVa only");
        it.tier = tier * 100 + i;   // keep the order n, y, add, rem, force, channel
        it.replaceFrom = p.partStart;
        it.provider = id();
        out << it;
      }
    }
    // a value typed in full alone shows no list (as for names)
    if (!context.requested && out.size() == 1 && out.first().name == p.typed) out.clear();
    return out;
  }
}
