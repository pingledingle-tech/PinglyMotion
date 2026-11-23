/*
 * PinglyMotion Script Lab - the control commands lsl_check, lsl_lint, lsl_run and lsl_event
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <cmath>

#include <QJsonArray>

#include "builtins.h"
#include "control.h"
#include "interp.h"
#include "lexer.h"
#include "lint.h"
#include "parser.h"
#include "rlvworld.h"
#include "sandbox.h"
#include "session.h"
#include "scriptlab/rlvstate.h"

namespace lsl
{
namespace Control
{

namespace
{
  QJsonObject fail(const QString& message)
  {
    return QJsonObject{{"ok", false}, {"error", message}};
  }

  QString clip(const QString& text, int length)
  {
    return text.size() > length ? text.left(length) + "..." : text;
  }

  QJsonArray diagnostics(const std::vector<Diagnostic>& list)
  {
    QJsonArray out;
    for (const Diagnostic& d : list)
      out.append(QJsonObject{{"line", d.line}, {"column", d.column}, {"message", d.message}});
    return out;
  }

  // checks the keys and the source; empty when fine
  QString checkRequest(const QJsonObject& request, const QStringList& known)
  {
    for (auto it = request.begin(); it != request.end(); ++it)
      if (!known.contains(it.key()))
        return "Unknown option \"" + clip(it.key(), 40) + "\". Options: " + known.mid(1).join(", ") + ".";
    if (!request.value("source").isString()) return "Missing \"source\": the LSL script as text.";
    if (request.value("source").toString().size() > Lexer::MAX_SOURCE_CHARS)
      return QString("\"source\" is too long (at most %1 characters).").arg(Lexer::MAX_SOURCE_CHARS);
    return QString();
  }
}

QJsonObject check(const QJsonObject& request)
{
  const QString problem = checkRequest(request, {"command", "source"});
  if (!problem.isEmpty()) return fail(problem);
  const Parser::Result parsed = Parser::parse(request.value("source").toString());
  QJsonObject answer{{"ok", true}, {"valid", parsed.ok()}, {"errors", diagnostics(parsed.errors)}, {"warnings", diagnostics(parsed.warnings)}};
  if (parsed.script)
  {
    QJsonArray states;
    for (const State& s : parsed.script->states) states.append(s.name);
    answer["states"] = states;
  }
  return answer;
}

QJsonObject lint(const QJsonObject& request)
{
  const QString problem = checkRequest(request, {"command", "source"});
  if (!problem.isEmpty()) return fail(problem);
  const Lint::Result result = Lint::lint(request.value("source").toString());
  QJsonArray warnings;
  for (const LintWarning& w : result.warnings)
    warnings.append(QJsonObject{{"line", w.line}, {"column", w.column}, {"id", w.id}, {"message", w.message}});
  return QJsonObject{{"ok", true}, {"valid", result.valid()}, {"errors", diagnostics(result.errors)}, {"warnings", warnings}};
}

namespace
{
  bool wholeNumber(const QJsonValue& v, double low, double high, int& out)
  {
    if (!v.isDouble()) return false;
    const double d = v.toDouble();
    if (!std::isfinite(d) || d != std::floor(d) || d < low || d > high) return false;
    out = int(d);
    return true;
  }

  bool validKey(const QString& key)
  {
    if (key.isEmpty() || key.size() > 36) return false;
    for (const QChar c : key)
      if (!(c.isDigit() || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '-')) return false;
    return true;
  }

  bool validName(const QString& name)
  {
    if (name.trimmed().isEmpty() || name.size() > 63) return false;
    for (const QChar c : name)
      if (c.category() == QChar::Other_Control) return false;
    return true;
  }

  // the "change" of a changed input (step 9b-2): a name ("inventory", "CHANGED_OWNER", any case), a list of
  // names, or the flags as a whole number; gives the CHANGED_* flags added together
  QString readChange(const QJsonValue& v, int& flags)
  {
    static const QString message = "\"change\" must be a name such as \"inventory\", \"allowed_drop\", \"color\", \"shape\", \"scale\", \"texture\", \"link\", "
                                   "\"owner\", \"region\", \"teleport\", \"region_start\", \"media\" or \"render_material\" (or \"CHANGED_OWNER\" and so on), "
                                   "a list of names, or the flags as a whole number.";
    static const QStringList names = {"INVENTORY", "ALLOWED_DROP", "COLOR", "SHAPE", "SCALE", "TEXTURE", "LINK", "OWNER", "REGION",
                                      "TELEPORT", "REGION_START", "MEDIA", "RENDER_MATERIAL"};
    auto flagOf = [](const QString& text) -> int
    {
      QString name = text.trimmed().toUpper();
      if (name.startsWith("CHANGED_")) name = name.mid(8);
      if (!names.contains(name)) return 0;
      const BuiltinConstant* c = Builtins::constant("CHANGED_" + name);
      return c ? c->value.i : 0;
    };
    int all = 0;
    for (const QString& n : names) all |= flagOf(n);
    flags = 0;
    if (v.isString())
    {
      flags = flagOf(v.toString());
      return flags != 0 ? QString() : message;
    }
    if (v.isArray())
    {
      const QJsonArray list = v.toArray();
      if (list.isEmpty() || list.size() > 16) return message;
      for (const QJsonValue& item : list)
      {
        const int f = item.isString() ? flagOf(item.toString()) : 0;
        if (f == 0) return message;
        flags |= f;
      }
      return QString();
    }
    if (!wholeNumber(v, 1, all, flags) || (flags & ~all) != 0) { flags = 0; return message; }
    return QString();
  }

  // the "levels" or "edges" of a control input (step 9b-6): a name (forward, back, left, right, up, down, rot_left,
  // rot_right, lbutton, ml_lbutton; also CONTROL_FWD and so on), a list of names, or the flags as a whole number
  QString readControls(const QJsonValue& v, const QString& field, int& flags)
  {
    static const QString names = "forward, back, left, right, up, down, rot_left, rot_right, lbutton or ml_lbutton";
    const QString message = "\"" + field + "\" must be a name (" + names + "; also CONTROL_FWD and so on), a list of names, or the flags as a whole number.";
    static const QStringList list = {"FWD", "BACK", "LEFT", "RIGHT", "UP", "DOWN", "ROT_LEFT", "ROT_RIGHT", "LBUTTON", "ML_LBUTTON"};
    auto flagOf = [](const QString& text) -> int
    {
      QString name = text.trimmed().toUpper();
      if (name.startsWith("CONTROL_")) name = name.mid(8);
      if (name == "FORWARD") name = "FWD";
      if (!list.contains(name)) return 0;
      return Builtins::constant("CONTROL_" + name)->value.i;
    };
    int all = 0;
    for (const QString& n : list) all |= flagOf(n);
    flags = 0;
    if (v.isString())
    {
      flags = flagOf(v.toString());
      return flags != 0 ? QString() : message;
    }
    if (v.isArray())
    {
      const QJsonArray items = v.toArray();
      if (items.isEmpty() || items.size() > 16) return message;
      for (const QJsonValue& item : items)
      {
        const int f = item.isString() ? flagOf(item.toString()) : 0;
        if (f == 0) return message;
        flags |= f;
      }
      return QString();
    }
    if (!wholeNumber(v, 0, all, flags) || (flags & ~all) != 0) { flags = 0; return message; }
    return QString();
  }

  // a vector field [x, y, z]: three finite numbers, each within low..high; false when not
  bool readVector3(const QJsonValue& v, double low, double high, float out[3])
  {
    if (!v.isArray()) return false;
    const QJsonArray p = v.toArray();
    if (p.size() != 3) return false;
    for (int i = 0; i < 3; ++i)
    {
      const double d = p[i].toDouble(std::nan(""));
      if (!p[i].isDouble() || !std::isfinite(d) || d < low || d > high) return false;
      out[i] = float(d);
    }
    return true;
  }

  // second gap list, step 6, part 2: the fields of the collision-family inputs (collision, land_collision, moving,
  // at_target, not_at_target); empty when fine
  QString readCollisionFamily(const QJsonObject& o, const QString& where, SandboxInput& in)
  {
    static const double MAX_COORD = 65536.0;
    const bool family = in.type == "collision" || in.type == "land_collision" || in.type == "moving";
    if (o.contains("phase"))
    {
      if (!family) return where + "\"phase\" is only for collision, land_collision and moving.";
      if (!o.value("phase").isString()) return where + "\"phase\" must be text.";
      in.stage = o.value("phase").toString();
      if (in.stage != "start" && in.stage != "end")
        return where + "\"phase\" must be \"start\" or \"end\"" + (in.type == "land_collision" ? " (leave it out for the continuous land_collision)." : ".");
    }
    else if (in.type == "collision" || in.type == "moving")
      return where + QString("a %1 input needs \"phase\": \"start\" or \"end\".").arg(in.type);
    if (o.contains("pos"))
    {
      if (in.type != "land_collision") return where + "\"pos\" is only for land_collision (the event's own vector argument).";
      if (!readVector3(o.value("pos"), -MAX_COORD, MAX_COORD, in.pos))
        return where + "\"pos\" must be [x, y, z], three numbers (the land_collision event's own vector argument).";
    }
    else if (in.type == "land_collision")
      return where + "a land_collision input needs \"pos\": [x, y, z] (the event's own vector argument).";
    if (in.type == "at_target")
    {
      if (!o.contains("tnum") || !wholeNumber(o.value("tnum"), 0, 2147483647.0, in.tnum))
        return where + "an at_target input needs \"tnum\": the target number, a whole number from 0 (llTarget's return value).";
      if (!o.contains("targetpos") || !readVector3(o.value("targetpos"), -MAX_COORD, MAX_COORD, in.targetpos))
        return where + "an at_target input needs \"targetpos\": [x, y, z] (the target position given to llTarget).";
      if (!o.contains("ourpos") || !readVector3(o.value("ourpos"), -MAX_COORD, MAX_COORD, in.ourpos))
        return where + "an at_target input needs \"ourpos\": [x, y, z] (where the object is now).";
    }
    else
      for (const char* field : {"tnum", "targetpos", "ourpos"})
        if (o.contains(field)) return where + "\"" + field + "\" is only for at_target.";
    return QString();
  }

  // gap list 2, step 4: the fields of money (amount), http_request (url, method, body, headers) and email (address,
  // subject, body); empty when fine
  QString readMoneyWebMail(const QJsonObject& o, const QString& where, SandboxInput& in)
  {
    static const int MAX_URL_CHARS = 2048;
    static const int MAX_HTTP_BODY_CHARS = 16384;
    static const int MAX_EMAIL_CHARS = 4096;
    static const int MAX_HEADERS = 32;
    for (const char* field : {"url", "method", "body", "address", "subject"})
      if (o.contains(field) && !o.value(field).isString()) return where + "\"" + field + "\" must be text.";
    if (in.type == "money")
    {
      if (!o.contains("amount") || !wholeNumber(o.value("amount"), 1, 2147483647.0, in.amount))
        return where + "a money input needs \"amount\": the L$ paid, a whole number from 1 to 2147483647.";
    }
    else if (o.contains("amount")) return where + "\"amount\" is only for money.";
    if (in.type == "http_request")
    {
      if (o.contains("url"))
      {
        in.url = o.value("url").toString();
        if (in.url.isEmpty() || in.url.size() > MAX_URL_CHARS)
          return where + QString("\"url\" must be text of 1 to %1 characters: a URL a script got, with any path and query (leave it out for the object's newest URL).").arg(MAX_URL_CHARS);
      }
      if (o.contains("method"))
      {
        in.method = o.value("method").toString().toUpper();
        if (in.method != "GET" && in.method != "POST" && in.method != "PUT" && in.method != "DELETE")
          return where + "\"method\" must be GET, POST, PUT or DELETE (default GET).";
      }
      if (o.contains("body"))
      {
        if (in.method != "POST" && in.method != "PUT") return where + "\"body\" is only for POST and PUT requests, as in Second Life.";
        in.body = o.value("body").toString();
        if (in.body.size() > MAX_HTTP_BODY_CHARS) return where + QString("\"body\" must be text of at most %1 characters (the script gets the first 2048 bytes).").arg(MAX_HTTP_BODY_CHARS);
      }
      if (o.contains("headers"))
      {
        const QString message = where + QString("\"headers\" must be an object of at most %1 headers, each name (letters, digits and \"-\", 1 to 64) with a text value of at most 1024 characters.").arg(MAX_HEADERS);
        if (!o.value("headers").isObject()) return message;
        const QJsonObject headers = o.value("headers").toObject();
        if (headers.size() > MAX_HEADERS) return message;
        for (auto it = headers.begin(); it != headers.end(); ++it)
        {
          const QString name = it.key();
          bool ok = !name.isEmpty() && name.size() <= 64 && it.value().isString() && it.value().toString().size() <= 1024;
          for (const QChar c : name)
            if (!(c.isDigit() || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '-')) ok = false;
          if (!ok) return message;
          in.headers.insert(name.toLower(), it.value().toString());
        }
      }
    }
    else
    {
      for (const char* field : {"url", "method", "headers"})
        if (o.contains(field)) return where + "\"" + field + "\" is only for http_request.";
    }
    if (in.type == "email")
    {
      in.address = o.value("address").toString();
      in.subject = o.value("subject").toString();
      in.body = o.value("body").toString();
      if (in.address.size() > 255) return where + "\"address\" must be text of at most 255 characters (the sender).";
      if (in.subject.size() > 255) return where + "\"subject\" must be text of at most 255 characters.";
      if (in.body.size() > MAX_EMAIL_CHARS) return where + QString("\"body\" (the message) must be text of at most %1 characters.").arg(MAX_EMAIL_CHARS);
    }
    else
    {
      for (const char* field : {"address", "subject"})
        if (o.contains(field)) return where + "\"" + field + "\" is only for email.";
      if (o.contains("body") && in.type != "http_request") return where + "\"body\" is only for http_request and email.";
    }
    return QString();
  }

  // one input of lsl_run or the event of lsl_event; empty when fine
  QString readInput(const QJsonObject& o, const QStringList& known, const QString& where, SandboxInput& in)
  {
    for (auto it = o.begin(); it != o.end(); ++it)
      if (!known.contains(it.key()))
        return where + "unknown field \"" + clip(it.key(), 40) + "\". Fields: " + known.mid(known.first() == "command" ? 1 : 0).join(", ") + ".";
    if (o.contains("at"))
    {
      if (!o.value("at").isDouble()) return where + "\"at\" must be a number of seconds.";
      in.at = o.value("at").toDouble();
      if (!std::isfinite(in.at) || in.at < 0 || in.at > MAX_SECONDS) return where + QString("\"at\" must be between 0 and %1.").arg(MAX_SECONDS);
    }
    if (o.contains("type") && !o.value("type").isString()) return where + "\"type\" must be text.";
    in.type = o.value("type").toString("chat");
    static const QStringList types = {"chat", "touch", "dialog", "attach", "detach", "changed", "sit", "stand", "control", "money", "http_request", "email",
                                       "collision", "land_collision", "moving", "at_target", "not_at_target", "step", "drop"};
    if (!types.contains(in.type))
      return where + "\"type\" must be \"chat\", \"touch\", \"dialog\", \"attach\", \"detach\", \"changed\", \"sit\", \"stand\", \"control\", \"money\", \"http_request\", "
                      "\"email\", \"collision\", \"land_collision\", \"moving\", \"at_target\", \"not_at_target\", \"step\" or \"drop\".";
    if (o.contains("channel") && !wholeNumber(o.value("channel"), -2147483648.0, 2147483647.0, in.channel))
      return where + "\"channel\" must be a whole number.";
    for (const char* field : {"name", "id", "text", "object", "kind", "button"})
      if (o.contains(field) && !o.value(field).isString()) return where + "\"" + field + "\" must be text.";
    in.name = o.value("name").toString().left(255);
    in.id = o.value("id").toString().left(36);
    in.text = o.value("text").toString().left(World::MAX_TEXT_CHARS);
    in.object = o.value("object").toString().left(63);
    if (o.contains("kind"))
    {
      in.kind = o.value("kind").toString();
      if (in.type != "chat") return where + "\"kind\" is only for chat.";
      if (in.kind != "say" && in.kind != "whisper" && in.kind != "shout") return where + "\"kind\" must be \"say\", \"whisper\" or \"shout\".";
    }
    if (in.type == "dialog")
    {
      if (o.contains("button")) in.text = o.value("button").toString().left(World::MAX_TEXT_CHARS);
      if (in.text.isEmpty()) return where + "a dialog answer needs \"button\": the text of the button pressed.";
    }
    else if (o.contains("button")) return where + "\"button\" is only for a dialog answer.";
    if (o.contains("link"))
    {
      if (in.type != "touch" && in.type != "sit" && in.type != "money" && in.type != "collision")
        return where + "\"link\" is only for touch (the prim touched), sit (the prim sat on), money (the prim paid) and collision (the prim hit).";
      if (!wholeNumber(o.value("link"), 1, Sandbox::MAX_PRIMS, in.link))
        return where + QString("\"link\" must be a whole number from 1 to %1 (the prim touched; 1 is the root).").arg(Sandbox::MAX_PRIMS);
    }
    const QString moneyWebMail = readMoneyWebMail(o, where, in);
    if (!moneyWebMail.isEmpty()) return moneyWebMail;
    const QString collisionFamily = readCollisionFamily(o, where, in);
    if (!collisionFamily.isEmpty()) return collisionFamily;
    if (o.contains("hold"))
    {
      if (in.type != "touch") return where + "\"hold\" is only for touch (the seconds the touch is held).";
      in.hold = o.value("hold").toDouble(-1.0);
      if (!o.value("hold").isDouble() || !std::isfinite(in.hold) || in.hold < 0.0 || in.hold > Sandbox::MAX_TOUCH_HOLD)
        return where + QString("\"hold\" must be a number of seconds from 0 to %1 (how long the touch is held; 0 is a click).").arg(Sandbox::MAX_TOUCH_HOLD);
    }
    // second gap list, step 6, part 3: the simple motion model's own step
    if (o.contains("dt"))
    {
      if (in.type != "step") return where + "\"dt\" is only for step (the seconds the motion model advances).";
      in.dt = o.value("dt").toDouble(-1.0);
      if (!o.value("dt").isDouble() || !std::isfinite(in.dt) || in.dt < 0.0 || in.dt > Sandbox::MAX_STEP_SECONDS)
        return where + QString("\"dt\" must be a number of seconds from 0 to %1.").arg(Sandbox::MAX_STEP_SECONDS);
    }
    else if (in.type == "step") return where + "a step input needs \"dt\": the seconds the motion model advances.";
    // several-objects job: an avatar drops an item into the object ("item": its name, "item_type": its type, default notecard)
    for (const char* field : {"item", "item_type"})
      if (o.contains(field) && in.type != "drop") return where + "\"" + field + "\" is only for drop.";
    if (in.type == "drop")
    {
      if (!o.value("item").isString() || !validName(o.value("item").toString()))
        return where + "a drop input needs \"item\": the name of the item dropped, text of 1 to 63 characters.";
      in.item = o.value("item").toString();
      if (o.contains("item_type"))
      {
        in.itemType = o.value("item_type").isString() ? Inventory::typeFromName(o.value("item_type").toString()) : -2;
        if (in.itemType == -2)
          return where + "\"item_type\" must be one of texture, sound, landmark, clothing, object, notecard, script, bodypart, animation, gesture, setting, material.";
      }
    }
    if (in.type == "changed")
    {
      if (!o.contains("change")) return where + "a changed input needs \"change\": what changed, such as \"inventory\", \"owner\", \"link\" or \"region_start\".";
      const QString problem = readChange(o.value("change"), in.change);
      if (!problem.isEmpty()) return where + problem;
    }
    else if (o.contains("change")) return where + "\"change\" is only for changed.";
    if (in.type == "control")
    {
      for (const char* field : {"levels", "edges"})
        if (o.contains(field))
        {
          const QString problem = readControls(o.value(field), field, field[0] == 'l' ? in.levels : in.edges);
          if (!problem.isEmpty()) return where + problem;
        }
      if (in.levels == 0 && in.edges == 0)
        return where + "a control input needs \"levels\" (the controls held now) or \"edges\" (the controls that changed), such as \"forward\".";
    }
    else if (o.contains("levels") || o.contains("edges")) return where + "\"levels\" and \"edges\" are only for control.";
    if (o.contains("point"))
    {
      if (in.type != "attach") return where + "\"point\" is only for attach.";
      if (!wholeNumber(o.value("point"), 1, Sandbox::MAX_ATTACH_POINT, in.point))
        return where + QString("\"point\" must be a whole number from 1 to %1.").arg(Sandbox::MAX_ATTACH_POINT);
    }
    return QString();
  }

  // step 9b-4: "inventory" of an object: [{name, type, key, lines}]; empty when fine
  QString readInventory(const QJsonValue& v, const QString& where, std::vector<InventoryItem>& out)
  {
    static const int MAX_GIVEN = Inventory::MAX_ITEMS - Sandbox::MAX_SCRIPTS_PER_OBJECT;
    static const int MAX_OBJECT_CHARS = 131072;   // all notecard lines of one object together
    const QString shape = where + "\"inventory\" must be a list of items, each {\"name\", \"type\" (texture, sound, landmark, clothing, object, notecard, script, "
                                   "bodypart, animation, gesture, setting or material), \"key\" (optional), \"lines\" (a notecard's lines), \"creator\" and "
                                   "\"description\" (optional)}.";
    if (!v.isArray()) return shape;
    const QJsonArray list = v.toArray();
    if (list.size() > MAX_GIVEN) return where + QString("\"inventory\" holds at most %1 items (an object holds %2 with its scripts).").arg(MAX_GIVEN).arg(Inventory::MAX_ITEMS);
    qint64 chars = 0;
    for (int i = 0; i < list.size(); ++i)
    {
      const QString at = where + QString("inventory item %1: ").arg(i + 1);
      if (!list[i].isObject()) return shape;
      const QJsonObject e = list[i].toObject();
      static const QStringList fields = {"name", "type", "key", "lines", "creator", "description"};
      for (auto it = e.begin(); it != e.end(); ++it)
        if (!fields.contains(it.key())) return at + "unknown field \"" + clip(it.key(), 40) + "\". Fields: " + fields.join(", ") + ".";
      InventoryItem item;
      if (!e.value("name").isString() || !validName(e.value("name").toString()))
        return at + "\"name\" must be text of 1 to 63 characters.";
      item.name = e.value("name").toString();
      if (!e.contains("type") && !e.contains("lines")) return at + "missing \"type\" (texture, sound, landmark, clothing, object, notecard, script, bodypart, animation, gesture, setting or material).";
      if (e.contains("type"))
      {
        item.type = e.value("type").isString() ? Inventory::typeFromName(e.value("type").toString()) : -2;
        if (item.type == -2) return at + "\"type\" must be one of texture, sound, landmark, clothing, object, notecard, script, bodypart, animation, gesture, setting, material.";
      }
      if (e.contains("key"))
      {
        if (!e.value("key").isString() || !validKey(e.value("key").toString())) return at + "\"key\" must be a key: 1 to 36 letters, digits or \"-\".";
        item.key = e.value("key").toString();
      }
      if (e.contains("creator"))
      {
        if (!e.value("creator").isString() || !validKey(e.value("creator").toString())) return at + "\"creator\" must be a key: 1 to 36 letters, digits or \"-\".";
        item.creator = e.value("creator").toString();
      }
      if (e.contains("description"))
      {
        if (!e.value("description").isString() || e.value("description").toString().size() > 127)
          return at + "\"description\" must be text of at most 127 characters.";
        item.description = e.value("description").toString();
      }
      if (e.contains("lines"))
      {
        if (item.type != 7) return at + "\"lines\" is only for a notecard.";
        if (!e.value("lines").isArray()) return at + "\"lines\" must be a list of text lines.";
        const QJsonArray lines = e.value("lines").toArray();
        if (lines.size() > Inventory::MAX_NOTECARD_LINES) return at + QString("a notecard has at most %1 lines.").arg(Inventory::MAX_NOTECARD_LINES);
        qint64 own = 0;
        for (const QJsonValue& line : lines)
        {
          if (!line.isString()) return at + "\"lines\" must be a list of text lines.";
          own += line.toString().size();
          item.lines << line.toString();
        }
        if (own > Inventory::MAX_NOTECARD_CHARS) return at + QString("a notecard has at most %1 characters in all.").arg(Inventory::MAX_NOTECARD_CHARS);
        chars += own;
        if (chars > MAX_OBJECT_CHARS) return where + QString("the notecards of one object have at most %1 characters in all.").arg(MAX_OBJECT_CHARS);
      }
      for (const InventoryItem& other : out)
        if (other.name == item.name) return at + "two items are named \"" + clip(item.name, 63) + "\"; each needs its own name.";
      out.push_back(item);
    }
    return QString();
  }

  bool readNumbers(const QJsonValue& v, int count, double low, double high, float* out)
  {
    const QJsonArray p = v.toArray();
    if (!v.isArray() || p.size() != count) return false;
    for (int i = 0; i < count; ++i)
    {
      const double d = p[i].toDouble(std::nan(""));
      if (!p[i].isDouble() || !std::isfinite(d) || d < low || d > high) return false;
      out[i] = float(d);
    }
    return true;
  }

  // step 9b-7: "avatars" of lsl_run: [{name, key, position, velocity, rotation, in_group}]; empty when fine
  QString readAvatars(const QJsonValue& v, std::vector<AvatarSpec>& out)
  {
    if (!v.isArray()) return "\"avatars\" must be a list of avatars, each {\"name\", \"key\", \"position\", \"velocity\", \"rotation\", \"in_group\", \"height\"}.";
    const QJsonArray list = v.toArray();
    if (list.size() > Sandbox::MAX_AVATARS) return QString("\"avatars\" holds at most %1 avatars.").arg(Sandbox::MAX_AVATARS);
    static const QStringList fields = {"name", "key", "position", "velocity", "rotation", "in_group", "height"};
    for (int i = 0; i < list.size(); ++i)
    {
      const QString where = QString("Avatar %1: ").arg(i + 1);
      if (!list[i].isObject()) return where + "must be an object with a name.";
      const QJsonObject o = list[i].toObject();
      for (auto it = o.begin(); it != o.end(); ++it)
        if (!fields.contains(it.key())) return where + "unknown field \"" + clip(it.key(), 40) + "\". Fields: " + fields.join(", ") + ".";
      AvatarSpec a;
      if (!o.value("name").isString() || !validName(o.value("name").toString())) return where + "\"name\" must be text of 1 to 63 characters.";
      a.name = o.value("name").toString();
      if (o.contains("key"))
      {
        if (!o.value("key").isString() || !validKey(o.value("key").toString())) return where + "\"key\" must be a key: 1 to 36 letters, digits or \"-\".";
        a.key = o.value("key").toString();
      }
      if (o.contains("position"))
      {
        const QJsonArray p = o.value("position").toArray();
        bool ok = o.value("position").isArray() && p.size() == 3;
        for (int k = 0; ok && k < 3; ++k)
        {
          const double d = p[k].toDouble(std::nan(""));
          ok = p[k].isDouble() && std::isfinite(d) && d >= 0.0 && d <= (k < 2 ? 256.0 : 4096.0);
          if (ok) a.position[k] = float(d);
        }
        if (!ok) return where + "\"position\" must be [x, y, z] in metres: x and y 0 to 256, z 0 to 4096.";
      }
      if (o.contains("velocity") && !readNumbers(o.value("velocity"), 3, -1000.0, 1000.0, a.velocity))
        return where + "\"velocity\" must be [x, y, z] in metres per second, each -1000 to 1000.";
      if (o.contains("rotation") && !readNumbers(o.value("rotation"), 4, -1000.0, 1000.0, a.rotation))
        return where + "\"rotation\" must be [x, y, z, s], four numbers.";
      if (o.contains("in_group"))
      {
        if (!o.value("in_group").isBool()) return where + "\"in_group\" must be true or false.";
        a.inGroup = o.value("in_group").toBool();
      }
      if (o.contains("height"))
      {
        const double h = o.value("height").toDouble(std::nan(""));
        if (!o.value("height").isDouble() || !std::isfinite(h) || h < 0.5 || h > 3.0)
          return where + "\"height\" must be the avatar's height in metres, 0.5 to 3 (default 1.8).";
        a.height = float(h);
      }
      out.push_back(a);
    }
    return QString();
  }

  // gap list 2, step 3: "region" of lsl_run: {name, corner, ground, water, day_length, time_of_day, unix_start}; empty when fine
  QString readRegion(const QJsonValue& v, RegionSettings& out)
  {
    static const QStringList fields = {"name", "corner", "ground", "water", "day_length", "time_of_day", "unix_start"};
    if (!v.isObject()) return "\"region\" must be an object with any of " + fields.join(", ") + ".";
    const QJsonObject o = v.toObject();
    for (auto it = o.begin(); it != o.end(); ++it)
      if (!fields.contains(it.key())) return "Region: unknown field \"" + clip(it.key(), 40) + "\". Fields: " + fields.join(", ") + ".";
    auto number = [&](const char* field, double low, double high, double& value)
    {
      if (!o.contains(field)) return true;
      const double d = o.value(field).toDouble(std::nan(""));
      if (!o.value(field).isDouble() || !std::isfinite(d) || d < low || d > high) return false;
      value = d;
      return true;
    };
    if (o.contains("name"))
    {
      const QString name = o.value("name").toString();
      if (!o.value("name").isString() || name.size() > 35 || !validName(name)) return "Region: \"name\" must be text of 1 to 35 characters.";
      out.name = name;
    }
    if (o.contains("corner"))
    {
      const QJsonArray c = o.value("corner").toArray();
      bool ok = o.value("corner").isArray() && c.size() == 3;
      for (int k = 0; ok && k < 3; ++k)
      {
        const double d = c[k].toDouble(std::nan(""));
        ok = c[k].isDouble() && std::isfinite(d) && d == std::floor(d) && d >= 0.0 && d <= (k < 2 ? 16777216.0 : 0.0);
        if (ok) out.corner[k] = float(d);
      }
      if (!ok) return "Region: \"corner\" must be [x, y, 0] in whole metres, x and y 0 to 16777216.";
    }
    double ground = out.ground, water = out.water, day = out.dayLength, time = out.timeOfDay, start = double(out.unixStart);
    if (!number("ground", 0.0, 4096.0, ground)) return "Region: \"ground\" must be the ground height in metres, 0 to 4096.";
    if (!number("water", 0.0, 4096.0, water)) return "Region: \"water\" must be the water height in metres, 0 to 4096.";
    if (!number("day_length", 60.0, 2592000.0, day)) return "Region: \"day_length\" must be the seconds of one day, 60 to 2592000.";
    if (!number("time_of_day", 0.0, 2592000.0, time) || time >= day)
      return "Region: \"time_of_day\" must be the seconds since the region's midnight at the start, 0 up to (not including) day_length.";
    if (!number("unix_start", 0.0, 2000000000.0, start) || start != std::floor(start))
      return "Region: \"unix_start\" must be the UTC time at the start in whole seconds since 1970, 0 to 2000000000.";
    out.ground = float(ground);
    out.water = float(water);
    out.dayLength = day;
    out.timeOfDay = time;
    out.unixStart = qint64(start);
    return QString();
  }

  // step 9b-9: "http" of lsl_run: canned replies to llHTTPRequest, [{url, method, status, body}]; empty when fine
  QString readHttp(const QJsonValue& v, std::vector<HttpReply>& out)
  {
    static const int MAX_URL_CHARS = 2048;
    static const int MAX_BODY_CHARS = 65536;
    if (!v.isArray()) return "\"http\" must be a list of canned replies, each {\"url\", \"method\", \"status\", \"body\"}.";
    const QJsonArray list = v.toArray();
    if (list.size() > Sandbox::MAX_HTTP_REPLIES) return QString("\"http\" holds at most %1 canned replies.").arg(Sandbox::MAX_HTTP_REPLIES);
    static const QStringList fields = {"url", "method", "status", "body"};
    for (int i = 0; i < list.size(); ++i)
    {
      const QString where = QString("HTTP reply %1: ").arg(i + 1);
      if (!list[i].isObject()) return where + "must be an object with a url.";
      const QJsonObject o = list[i].toObject();
      for (auto it = o.begin(); it != o.end(); ++it)
        if (!fields.contains(it.key())) return where + "unknown field \"" + clip(it.key(), 40) + "\". Fields: " + fields.join(", ") + ".";
      HttpReply r;
      if (!o.value("url").isString() || o.value("url").toString().isEmpty() || o.value("url").toString().size() > MAX_URL_CHARS)
        return where + QString("\"url\" must be text of 1 to %1 characters: the exact URL, or a prefix ending in * (\"*\" alone answers every URL).").arg(MAX_URL_CHARS);
      r.url = o.value("url").toString();
      if (o.contains("method"))
      {
        const QString method = o.value("method").toString().toUpper();
        if (!o.value("method").isString() || (method != "GET" && method != "POST" && method != "PUT" && method != "DELETE"))
          return where + "\"method\" must be GET, POST, PUT or DELETE (omit it to answer every method).";
        r.method = method;
      }
      if (o.contains("status") && !wholeNumber(o.value("status"), 100, 599, r.status))
        return where + "\"status\" must be a whole number from 100 to 599 (default 200).";
      if (o.contains("body"))
      {
        if (!o.value("body").isString() || o.value("body").toString().size() > MAX_BODY_CHARS)
          return where + QString("\"body\" must be text of at most %1 characters.").arg(MAX_BODY_CHARS);
        r.body = o.value("body").toString();
      }
      out.push_back(r);
    }
    return QString();
  }

  // step 9b-1: "description", "scale" and "start_parameter" of an object (and, step 9b-4, "inventory"); empty when fine
  QString readObjectState(const QJsonObject& o, const QString& where, ObjectSpec& spec)
  {
    if (o.contains("description"))
    {
      if (!o.value("description").isString() || o.value("description").toString().size() > 127)
        return where + "\"description\" must be text of at most 127 characters (what llGetObjectDesc answers).";
      spec.description = o.value("description").toString();
    }
    if (o.contains("scale"))
    {
      const QJsonArray p = o.value("scale").toArray();
      bool ok = o.value("scale").isArray() && p.size() == 3;
      for (int i = 0; ok && i < 3; ++i)
      {
        const double v = p[i].toDouble(std::nan(""));
        ok = p[i].isDouble() && std::isfinite(v) && v >= 0.001 && v <= 64.0;
        if (ok) spec.scale[i] = float(v);
      }
      if (!ok) return where + "\"scale\" must be [x, y, z] in metres, each 0.001 to 64 (what llGetScale answers).";
    }
    if (o.contains("start_parameter") && !wholeNumber(o.value("start_parameter"), -2147483648.0, 2147483647.0, spec.startParameter))
      return where + "\"start_parameter\" must be a whole number (what llGetStartParameter answers; 0 when rezzed by an avatar).";
    if (o.contains("inventory")) return readInventory(o.value("inventory"), where, spec.inventory);
    return QString();
  }

  // one entry of "objects"; empty when fine. Either "source" (one script, named "Script", in the root prim)
  // or "scripts" (step 9a: [{name, source, link}], each with its own memory and event queue)
  struct ScriptSource
  {
    QString name = "Script";
    int link = 1;
    QString source;
  };

  QString readObject(const QJsonObject& o, const QString& where, ObjectSpec& spec, std::vector<ScriptSource>& scripts)
  {
    static const QStringList known = {"name", "source", "scripts", "prims", "key", "owner", "owner_name", "position", "attached",
                                       "description", "scale", "start_parameter", "inventory"};
    for (auto it = o.begin(); it != o.end(); ++it)
      if (!known.contains(it.key())) return where + "unknown field \"" + clip(it.key(), 40) + "\". Fields: " + known.join(", ") + ".";
    if (!o.value("name").isString() || !validName(o.value("name").toString()))
      return where + "\"name\" must be text of 1 to 63 characters (the object's name, as llGetObjectName gives it).";
    spec.name = o.value("name").toString();
    if (o.contains("prims") && !wholeNumber(o.value("prims"), 1, Sandbox::MAX_PRIMS, spec.prims))
      return where + QString("\"prims\" must be a whole number from 1 to %1 (how many prims the object has).").arg(Sandbox::MAX_PRIMS);
    if (o.contains("source") && o.contains("scripts"))
      return where + "give either \"source\" (one script) or \"scripts\" (several scripts), not both.";
    if (o.contains("scripts"))
    {
      if (!o.value("scripts").isArray()) return where + "\"scripts\" must be a list of scripts, each with name and source.";
      const QJsonArray list = o.value("scripts").toArray();
      if (list.isEmpty() || list.size() > Sandbox::MAX_SCRIPTS_PER_OBJECT)
        return where + QString("\"scripts\" must hold 1 to %1 scripts.").arg(Sandbox::MAX_SCRIPTS_PER_OBJECT);
      int highest = 1;
      for (int i = 0; i < list.size(); ++i)
      {
        const QString at = where + QString("script %1: ").arg(i + 1);
        if (!list[i].isObject()) return at + "must be an object with name and source.";
        const QJsonObject e = list[i].toObject();
        static const QStringList fields = {"name", "source", "link"};
        for (auto it = e.begin(); it != e.end(); ++it)
          if (!fields.contains(it.key())) return at + "unknown field \"" + clip(it.key(), 40) + "\". Fields: " + fields.join(", ") + ".";
        ScriptSource script;
        if (!e.value("name").isString() || !validName(e.value("name").toString()))
          return at + "\"name\" must be text of 1 to 63 characters (the script's name, as llGetScriptName gives it).";
        script.name = e.value("name").toString();
        if (!e.value("source").isString()) return at + "missing \"source\": the script as text.";
        script.source = e.value("source").toString();
        if (script.source.size() > Lexer::MAX_SOURCE_CHARS) return at + QString("\"source\" is too long (at most %1 characters).").arg(Lexer::MAX_SOURCE_CHARS);
        if (e.contains("link") && !wholeNumber(e.value("link"), 1, Sandbox::MAX_PRIMS, script.link))
          return at + QString("\"link\" must be a whole number from 1 to %1 (the prim the script is in; 1 is the root).").arg(Sandbox::MAX_PRIMS);
        for (const ScriptSource& other : scripts)
          if (other.name == script.name) return at + "two scripts are named \"" + clip(script.name, 63) + "\"; each script of an object needs its own name.";
        highest = qMax(highest, script.link);
        scripts.push_back(script);
      }
      if (!o.contains("prims")) spec.prims = highest;
      else if (highest > spec.prims)
        return where + QString("a script is in link %1, but \"prims\" is %2.").arg(highest).arg(spec.prims);
    }
    else
    {
      if (!o.value("source").isString()) return where + "missing \"source\": the object's LSL script as text (or \"scripts\" for several).";
      ScriptSource script;
      script.source = o.value("source").toString();
      if (script.source.size() > Lexer::MAX_SOURCE_CHARS) return where + QString("\"source\" is too long (at most %1 characters).").arg(Lexer::MAX_SOURCE_CHARS);
      scripts.push_back(script);
    }
    for (const char* field : {"key", "owner"})
      if (o.contains(field) && (!o.value(field).isString() || !validKey(o.value(field).toString())))
        return where + "\"" + field + "\" must be a key: 1 to 36 letters, digits or \"-\".";
    spec.key = o.value("key").toString();
    spec.ownerKey = o.value("owner").toString();
    if (o.contains("owner_name"))
    {
      if (!o.value("owner_name").isString() || !validName(o.value("owner_name").toString()))
        return where + "\"owner_name\" must be text of 1 to 63 characters.";
      if (spec.ownerKey.isEmpty()) return where + "\"owner_name\" needs \"owner\" (the owner's key).";
      spec.ownerName = o.value("owner_name").toString();
    }
    if (o.contains("position"))
    {
      const QJsonArray p = o.value("position").toArray();
      bool ok = o.value("position").isArray() && p.size() == 3;
      for (int i = 0; ok && i < 3; ++i)
      {
        const double v = p[i].toDouble(std::nan(""));
        ok = p[i].isDouble() && std::isfinite(v) && v >= 0.0 && v <= (i < 2 ? 256.0 : 4096.0);
        if (ok) spec.position[i] = float(v);
      }
      if (!ok) return where + "\"position\" must be [x, y, z] in metres: x and y 0 to 256, z 0 to 4096.";
    }
    if (o.contains("attached") && !wholeNumber(o.value("attached"), 0, Sandbox::MAX_ATTACH_POINT, spec.attached))
      return where + QString("\"attached\" must be an attachment point from 1 to %1, or 0 (not attached).").arg(Sandbox::MAX_ATTACH_POINT);
    return readObjectState(o, where, spec);
  }

  double rounded(double t) { return std::round(t * 1000.0) / 1000.0; }

  QJsonObject rlvAnswer(const Sandbox& sandbox, const RlvState& state)
  {
    return QJsonObject{{"lines", sandbox.rlvLines()}, {"refused", sandbox.refusedLines()}, {"state", RlvWorld::stateJson(state)}};
  }
}

QJsonObject run(const QJsonObject& request, RlvState* rlv, Session* session)
{
  for (auto it = request.begin(); it != request.end(); ++it)
  {
    static const QStringList known = {"command", "source", "objects", "seconds", "inputs", "keep_rlv",
                                       "description", "scale", "start_parameter", "inventory", "avatars", "http", "region"};
    if (!known.contains(it.key()))
      return fail("Unknown option \"" + clip(it.key(), 40) + "\". Options: " + known.mid(1).join(", ") + ".");
  }
  const bool several = request.contains("objects");
  if (several && request.contains("source"))
    return fail("Give either \"source\" (one script) or \"objects\" (several scripts), not both.");
  if (several)
    for (const char* field : {"description", "scale", "start_parameter", "inventory"})
      if (request.contains(field)) return fail(QString("With \"objects\", give \"%1\" inside each object, not at the top.").arg(field));
  if (!several)
  {
    if (!request.value("source").isString()) return fail("Missing \"source\": the LSL script as text (or \"objects\" for several scripts).");
    if (request.value("source").toString().size() > Lexer::MAX_SOURCE_CHARS)
      return fail(QString("\"source\" is too long (at most %1 characters).").arg(Lexer::MAX_SOURCE_CHARS));
  }

  double seconds = DEFAULT_SECONDS;
  if (request.contains("seconds"))
  {
    if (!request.value("seconds").isDouble()) return fail("\"seconds\" must be a number.");
    seconds = request.value("seconds").toDouble();
    if (!std::isfinite(seconds) || seconds < 0 || seconds > MAX_SECONDS)
      return fail(QString("\"seconds\" must be between 0 and %1 (simulated seconds).").arg(MAX_SECONDS));
  }

  if (request.contains("keep_rlv") && !request.value("keep_rlv").isBool()) return fail("\"keep_rlv\" must be true or false.");
  const bool keepRlv = request.value("keep_rlv").toBool(false);

  // the objects and their scripts
  std::vector<ObjectSpec> specs;
  std::vector<std::vector<ScriptSource>> sources;
  std::vector<bool> severalScripts;
  if (several)
  {
    if (!request.value("objects").isArray()) return fail("\"objects\" must be a list of objects, each with name and source.");
    const QJsonArray list = request.value("objects").toArray();
    if (list.isEmpty() || list.size() > Sandbox::MAX_OBJECTS) return fail(QString("\"objects\" must hold 1 to %1 objects.").arg(Sandbox::MAX_OBJECTS));
    qint64 total = 0;
    int scriptCount = 0;
    for (int i = 0; i < list.size(); ++i)
    {
      const QString where = QString("Object %1: ").arg(i + 1);
      if (!list[i].isObject()) return fail(where + "must be an object with name and source.");
      ObjectSpec spec;
      std::vector<ScriptSource> source;
      const QString problem = readObject(list[i].toObject(), where, spec, source);
      if (!problem.isEmpty()) return fail(problem);
      for (const ObjectSpec& other : specs)
      {
        if (other.name == spec.name) return fail(where + "two objects are named \"" + clip(spec.name, 63) + "\"; each needs its own name.");
        if (!spec.key.isEmpty() && other.key == spec.key) return fail(where + "two objects have the key " + spec.key + "; each needs its own key.");
      }
      for (const ScriptSource& script : source) total += script.source.size();
      if (total > MAX_TOTAL_SOURCE_CHARS) return fail(QString("The scripts together are too long (at most %1 characters).").arg(MAX_TOTAL_SOURCE_CHARS));
      scriptCount += int(source.size());
      if (scriptCount > Sandbox::MAX_SCRIPTS) return fail(QString("At most %1 scripts in all objects together.").arg(Sandbox::MAX_SCRIPTS));
      specs.push_back(spec);
      severalScripts.push_back(list[i].toObject().contains("scripts"));
      sources.push_back(source);
    }
  }
  else
  {
    ObjectSpec spec;
    const QString problem = readObjectState(request, "", spec);
    if (!problem.isEmpty()) return fail(problem);
    specs.push_back(spec);
    ScriptSource script;
    script.source = request.value("source").toString();
    sources.push_back({script});
    severalScripts.push_back(false);
  }

  std::vector<AvatarSpec> avatars;
  if (request.contains("avatars"))
  {
    const QString problem = readAvatars(request.value("avatars"), avatars);
    if (!problem.isEmpty()) return fail(problem);
  }

  std::vector<HttpReply> http;
  if (request.contains("http"))
  {
    const QString problem = readHttp(request.value("http"), http);
    if (!problem.isEmpty()) return fail(problem);
  }

  RegionSettings region;
  if (request.contains("region"))
  {
    const QString problem = readRegion(request.value("region"), region);
    if (!problem.isEmpty()) return fail(problem);
  }

  std::vector<SandboxInput> inputs;
  if (request.contains("inputs"))
  {
    if (!request.value("inputs").isArray()) return fail("\"inputs\" must be a list of objects.");
    const QJsonArray list = request.value("inputs").toArray();
    if (list.size() > MAX_INPUTS) return fail(QString("At most %1 inputs.").arg(MAX_INPUTS));
    static const QStringList known = {"at", "type", "object", "channel", "kind", "name", "id", "text", "button", "point", "link", "hold", "change", "levels", "edges",
                                       "amount", "url", "method", "body", "headers", "address", "subject", "phase", "pos", "tnum", "targetpos", "ourpos", "dt", "item", "item_type"};
    for (int i = 0; i < list.size(); ++i)
    {
      const QString where = QString("Input %1: ").arg(i + 1);
      if (!list[i].isObject()) return fail(where + "must be an object.");
      SandboxInput in;
      const QString problem = readInput(list[i].toObject(), known, where, in);
      if (!problem.isEmpty()) return fail(problem);
      const ObjectSpec* touched = in.object.isEmpty() ? &specs.front() : nullptr;
      for (const ObjectSpec& spec : specs)
        if (spec.name == in.object) touched = &spec;
      if (!touched) return fail(where + "there is no object named \"" + clip(in.object, 63) + "\".");
      if (in.link > touched->prims)
        return fail(where + QString("the object \"%1\" has %2 prim(s); there is no link %3.").arg(clip(touched->name, 63)).arg(touched->prims).arg(in.link));
      inputs.push_back(in);
    }
  }

  // compile every script first: nothing runs unless all compile
  std::vector<std::vector<ScriptEntry>> scripts;
  QJsonArray warnings;
  for (size_t i = 0; i < sources.size(); ++i)
  {
    std::vector<ScriptEntry> entries;
    for (const ScriptSource& source : sources[i])
    {
      Parser::Result parsed = Parser::parse(source.source);
      if (!parsed.ok())
      {
        QJsonObject answer;
        if (!several) answer = fail("The script does not compile, so it was not run. The errors are listed (the same as lsl_check).");
        else if (severalScripts[i])
          answer = fail("The script \"" + source.name + "\" of object \"" + specs[i].name + "\" does not compile, so nothing was run. The errors are listed (the same as lsl_check).");
        else
          answer = fail("The script of object \"" + specs[i].name + "\" does not compile, so nothing was run. The errors are listed (the same as lsl_check).");
        if (several) answer["object"] = specs[i].name;
        if (severalScripts[i]) answer["script"] = source.name;
        answer["errors"] = diagnostics(parsed.errors);
        return answer;
      }
      for (const Diagnostic& d : parsed.warnings)
      {
        QJsonObject w{{"line", d.line}, {"column", d.column}, {"message", d.message}};
        if (several) w["object"] = specs[i].name;
        if (severalScripts[i]) w["script"] = source.name;
        warnings.append(w);
      }
      ScriptEntry entry;
      entry.name = source.name;
      entry.link = source.link;
      entry.script = std::shared_ptr<const Script>(std::move(parsed.script));
      entries.push_back(entry);
    }
    scripts.push_back(entries);
  }

  std::unique_ptr<RlvState> ownState;
  if (!rlv) ownState.reset(new RlvState);
  RlvState& state = rlv ? *rlv : *ownState;
  if (!keepRlv) state.reset();
  std::unique_ptr<Sandbox> sandbox(new Sandbox(state, several));
  for (size_t i = 0; i < specs.size(); ++i)
  {
    const QString problem = severalScripts[i] ? sandbox->addObject(specs[i], scripts[i]) : sandbox->addObject(specs[i], scripts[i].front().script);
    if (!problem.isEmpty()) return fail(QString("Object %1: ").arg(i + 1) + problem);
  }
  for (size_t i = 0; i < avatars.size(); ++i)
  {
    const QString problem = sandbox->addAvatar(avatars[i]);
    if (!problem.isEmpty()) return fail(QString("Avatar %1: ").arg(i + 1) + problem);
  }
  sandbox->setHttpReplies(http);
  sandbox->setRegion(region);
  for (const SandboxInput& in : inputs) sandbox->addInput(in);
  sandbox->start();
  sandbox->runUntil(seconds);

  QJsonObject answer{{"ok", true}, {"log", sandbox->log()}, {"log_truncated", sandbox->truncated()}};
  if (!several)
  {
    const Interpreter& interp = sandbox->interpreter(0);
    answer["stopped"] = interp.stopped();
    answer["state"] = interp.currentState();
    answer["time"] = rounded(interp.now());
    answer["steps"] = double(interp.steps());
    answer["open_listens"] = interp.openListens();
    answer["timer"] = interp.timerInterval();
    if (interp.stopped()) answer["stop_reason"] = interp.stopReason();
  }
  else
  {
    answer["stopped"] = sandbox->allStopped();
    answer["time"] = rounded(sandbox->now());
  }
  answer["objects"] = sandbox->objectsJson();
  if (!sandbox->lastDialog().isEmpty()) answer["dialog"] = sandbox->lastDialog();
  answer["warnings"] = warnings;
  answer["rlv"] = rlvAnswer(*sandbox, state);
  answer["note"] = "A sandbox with a simulated clock, not Second Life: test the final script in Second Life too.";
  // keep the world for lsl_event (only with the shared RLV state, which outlives it)
  if (session && rlv) session->set(std::move(sandbox));
  return answer;
}

QJsonObject event(const QJsonObject& request, Session* session)
{
  Sandbox* sandbox = session ? session->sandbox() : nullptr;
  static const QStringList known = {"command", "type", "object", "channel", "kind", "name", "id", "text", "button", "point", "link", "hold", "change", "levels", "edges",
                                    "amount", "url", "method", "body", "headers", "address", "subject", "seconds", "phase", "pos", "tnum", "targetpos", "ourpos", "dt", "item", "item_type"};
  SandboxInput in;
  const QString problem = readInput(request, known, QString(), in);
  if (!problem.isEmpty())
  {
    QString message = problem;
    if (message.startsWith("unknown field")) message = "Unknown option" + message.mid(13);
    message[0] = message[0].toUpper();
    return fail(message);
  }
  if (!request.contains("type"))
    return fail("Missing \"type\": chat, touch, dialog, attach, detach, changed, sit, stand, control, money, http_request, email, collision, land_collision, "
                "moving, at_target, not_at_target, step or drop.");
  double seconds = DEFAULT_EVENT_SECONDS;
  if (request.contains("seconds"))
  {
    if (!request.value("seconds").isDouble()) return fail("\"seconds\" must be a number.");
    seconds = request.value("seconds").toDouble();
    if (!std::isfinite(seconds) || seconds < 0 || seconds > MAX_SECONDS)
      return fail(QString("\"seconds\" must be between 0 and %1 (simulated seconds).").arg(MAX_SECONDS));
  }
  // a held touch without "seconds" runs on until it is let go, so its touch_end is in this answer
  else if (in.hold > seconds) seconds = in.hold;
  if (!sandbox) return fail("There is no running world to send the event to. Start one with lsl_run first (with \"source\" or \"objects\").");
  if (!in.object.isEmpty() && sandbox->indexOf(in.object) < 0)
    return fail("There is no object named \"" + clip(in.object, 63) + "\" in the world of the last lsl_run.");
  if (in.link > 0)
  {
    const int prims = sandbox->prims(in.object.isEmpty() ? 0 : sandbox->indexOf(in.object));
    if (in.link > prims) return fail(QString("The object has %1 prim(s); there is no link %2.").arg(prims).arg(in.link));
  }
  if (sandbox->allStopped()) return fail("Every script in the world of the last lsl_run has stopped; run lsl_run again.");

  sandbox->newCommand();
  in.at = sandbox->now();
  sandbox->addInput(in);
  sandbox->runUntil(sandbox->now() + seconds);

  QJsonObject answer{{"ok", true}, {"log", sandbox->log()}, {"log_truncated", sandbox->truncated()},
                     {"stopped", sandbox->allStopped()}, {"time", rounded(sandbox->now())}, {"objects", sandbox->objectsJson()}};
  if (!sandbox->lastDialog().isEmpty()) answer["dialog"] = sandbox->lastDialog();
  answer["rlv"] = rlvAnswer(*sandbox, sandbox->rlv());
  answer["note"] = "A sandbox with a simulated clock, not Second Life: test the final script in Second Life too.";
  session->notify();
  return answer;
}

}
}
