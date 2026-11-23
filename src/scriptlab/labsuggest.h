/*
 * PinglyMotion Script Lab - the suggestion engine of the script editor (lane 1, job F2)
 *
 * Given the text and the cursor, the engine returns the names to suggest, each with its help:
 * one plain sentence (Beginner and Expert) and one dense line (Expert: kind, types, parameters).
 * QtCore only, so it is tested without a screen and the editor (labeditor.*) only shows it.
 *
 * Three layers, so later jobs add to it without changing the rest:
 *  - the language (LabSuggest::Language): which characters make a name, what is code and what
 *    is a string or comment, and which name a built-in has in that language (the name that
 *    means "send text to the owner" is llOwnerSay in LSL and ll.OwnerSay in SLua);
 *  - the providers (LabSuggest::Provider): each one gives the items it knows for a context.
 *    Now: Linden Lab's LSL definitions (functions, events, constants, types, keywords) and the
 *    names made in the script itself, and (job R3, rlvsuggest.h) the RLV commands after "@"
 *    inside the owner-say text; a Lua layer later adds its own language and providers;
 *  - the engine (LabSuggest::Engine): asks the providers that handle the language, sorts and
 *    joins their items.
 *
 * Help texts: the plain sentence is our own words where lslhelp_plain.inc has one (checked by
 * tests/test_gen_lsl_help.py: at most 12 words); otherwise the first sentence of the
 * definition's description (lslhelp_data.inc, made by tools/gen_lsl_help.py).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef LABSUGGEST_H
#define LABSUGGEST_H

#include <QList>
#include <QString>
#include <memory>
#include <vector>

namespace LabSuggest
{
  enum class Kind { Function, Event, Constant, Type, Keyword, ScriptName, Command };

  // the word for a kind, as the help shows it ("Function", "Event", ...)
  QString kindWord(Kind kind);

  // one suggestion
  struct Item
  {
    QString name;          // what the list shows and what is matched
    Kind kind = Kind::Function;
    QString insert;        // what replaces the typed part of the name
    int cursorBack = 0;    // how far the cursor goes back from the end of insert (inside "()")
    bool callable = false; // a function: "()" is left out when a "(" already follows
    QString plain;         // help: one plain sentence
    QString detail;        // help, Expert only: kind, types, parameters
    QString provider;      // the provider's id
    int tier = 0;          // how well it matches (0 best); set by the provider with matchTier()
    // RLV items (job R3) and other providers whose typed part is not a plain name:
    int replaceFrom = -1;  // 0 or more: insert replaces the text from here to the cursor, not the prefix
    QString mark;          // shown after the name in the list, e.g. "RLVa only" (a word, not a colour)
    QString example;       // a whole example line shown under the help, e.g. "@detach=n"
    QString exampleInsert; // what Ctrl+Enter puts in, from exampleFrom to the cursor
    int exampleFrom = -1;
  };

  // where the cursor is
  struct Context
  {
    QString text;          // the whole script
    int cursor = 0;        // position in text
    QString prefix;        // the part of a name typed right before the cursor (may be empty)
    int prefixStart = 0;   // where prefix starts in text
    bool inCode = true;    // not inside a string or a comment
    bool inString = false;
    bool inComment = false;
    int stringStart = -1;  // inside a string: position of its opening quote
    bool afterDot = false; // the prefix follows a "." (a vector's .x and so on)
    bool requested = false;// the user asked (Ctrl+Space): suggest even with nothing typed
  };

  // how a name matches a typed prefix: 0 starts with it (same case), 1 starts with it (any case),
  // 2 starts with it after "ll" ("OwnerSay" finds llOwnerSay), -1 no match. Empty prefix: 1.
  int matchTier(const QString& name, const QString& prefix);

  // the text and the cursor position after taking item, with `after` the text right of the
  // cursor (to leave out "()" when a "(" already follows)
  struct Insertion
  {
    QString text;
    int cursorBack = 0;
  };
  Insertion insertionFor(const Item& item, const QString& after);

  // --- the language layer
  class Language
  {
    public:
      virtual ~Language() = default;
      virtual QString id() const = 0;                 // "lsl", "slua"
      virtual QString displayName() const = 0;        // "LSL", "SLua"
      virtual bool isNameChar(QChar c) const;
      // the name a built-in has in this language, from its LSL name
      virtual QString builtinName(const QString& lslName) const = 0;
      // the name that sends text to the owner (the RLV job reads "@" lines in it)
      QString ownerSayName() const { return builtinName(QStringLiteral("llOwnerSay")); }
      // the context at the cursor
      virtual Context context(const QString& text, int cursor, bool requested) const;
    protected:
      // marks strings and comments: line comment start, block comment start and end
      virtual QString lineComment() const = 0;
  };

  class LslLanguage : public Language
  {
    public:
      QString id() const override { return QStringLiteral("lsl"); }
      QString displayName() const override { return QStringLiteral("LSL"); }
      QString builtinName(const QString& lslName) const override { return lslName; }
    protected:
      QString lineComment() const override { return QStringLiteral("//"); }
  };

  // SLua (Second Life's Lua): only the language choice exists (job F3). No provider handles it
  // yet, so it gives no suggestions; the names follow Linden Lab's SLua definitions (ll.OwnerSay).
  class SluaLanguage : public Language
  {
    public:
      QString id() const override { return QStringLiteral("slua"); }
      QString displayName() const override { return QStringLiteral("SLua"); }
      bool isNameChar(QChar c) const override;
      QString builtinName(const QString& lslName) const override;
    protected:
      QString lineComment() const override { return QStringLiteral("--"); }
  };

  // --- the providers
  class Provider
  {
    public:
      virtual ~Provider() = default;
      virtual QString id() const = 0;
      // true for the languages it knows
      virtual bool handles(const Language& language) const = 0;
      virtual QList<Item> suggest(const Context& context, const Language& language) const = 0;
    protected:
      // in code, not after ".", and asked for or at least two characters typed
      static bool wantsNames(const Context& context);
  };

  // Linden Lab's definitions: functions, events, constants, and the types and keywords
  class LslDefinitionsProvider : public Provider
  {
    public:
      QString id() const override { return QStringLiteral("lsl-definitions"); }
      bool handles(const Language& language) const override { return language.id() == QLatin1String("lsl"); }
      QList<Item> suggest(const Context& context, const Language& language) const override;
      // the item for a name (for help on a name; an empty name when unknown)
      static Item item(const QString& name);
      // all items (the tests count them)
      static const QList<Item>& all();
  };

  // global variables and functions made in the script itself
  class ScriptNamesProvider : public Provider
  {
    public:
      QString id() const override { return QStringLiteral("script-names"); }
      bool handles(const Language& language) const override { return language.id() == QLatin1String("lsl"); }
      QList<Item> suggest(const Context& context, const Language& language) const override;
  };

  // --- the engine
  class Engine
  {
    public:
      static constexpr int MaxItems = 2500;  // a longer list is cut (all LSL names fit)

      // starts with LSL and the two LSL providers
      Engine();
      void setLanguage(std::shared_ptr<Language> language);
      const Language& language() const { return *m_language; }
      void addProvider(std::shared_ptr<Provider> provider);
      QStringList providerIds() const;

      // the context, and the sorted items for it. Empty when nothing fits, or when the only
      // item is exactly what is typed already (and the user did not ask).
      Context context(const QString& text, int cursor, bool requested) const;
      QList<Item> suggest(const Context& context) const;
      QList<Item> suggest(const QString& text, int cursor, bool requested) const
      { return suggest(context(text, cursor, requested)); }

    private:
      std::shared_ptr<Language> m_language;
      std::vector<std::shared_ptr<Provider>> m_providers;
  };
}

#endif
