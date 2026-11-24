/*
 * PinglyMotion Script Lab - the channel map: who talks to whom in a run (job A4)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "labchannelmap.h"

#include <QHash>
#include <QMap>
#include <QObject>
#include <QSet>

#include <algorithm>

#include "lsl/ast.h"
#include "lsl/parser.h"
#include "lsl/sandbox.h"

namespace LabChannelMap
{

namespace
{
  const char* const NullKey = "00000000-0000-0000-0000-000000000000";

  // a listen of one script, as the log says
  struct Listen
  {
    int handle = 0;
    int channel = 0;
    QString name;
    QString id;
    QString message;
    int box = -1;
    QString script;
  };

  // the functions that write each kind of log entry
  QStringList functionsFor(const QString& type, const QJsonObject& entry)
  {
    if (type == QLatin1String("say")) return {QStringLiteral("llSay")};
    if (type == QLatin1String("whisper")) return {QStringLiteral("llWhisper")};
    if (type == QLatin1String("shout")) return {QStringLiteral("llShout")};
    if (type == QLatin1String("region_say")) return {QStringLiteral("llRegionSay")};
    if (type == QLatin1String("region_say_to")) return {QStringLiteral("llRegionSayTo")};
    if (type == QLatin1String("owner_say") || type == QLatin1String("rlv_command")) return {QStringLiteral("llOwnerSay")};
    if (type == QLatin1String("instant_message")) return {QStringLiteral("llInstantMessage")};
    if (type == QLatin1String("message_linked")) return {QStringLiteral("llMessageLinked")};
    if (type == QLatin1String("http_request")) return {QStringLiteral("llHTTPRequest")};
    if (type == QLatin1String("dialog"))
      return entry.value("text_box").toBool() ? QStringList{QStringLiteral("llTextBox")} : QStringList{QStringLiteral("llDialog")};
    return {};
  }

  void collectCalls(const lsl::Expr* e, const QStringList& functions, QVector<QPair<int, int>>& out)
  {
    if (!e) return;
    if (e->kind == lsl::ExprKind::Call && functions.contains(e->name)) out.append({e->line, e->column});
    for (const auto& a : e->args) collectCalls(a.get(), functions, out);
  }

  void collectCalls(const lsl::Stmt* s, const QStringList& functions, QVector<QPair<int, int>>& out)
  {
    if (!s) return;
    collectCalls(s->expr.get(), functions, out);
    for (const auto& e : s->init) collectCalls(e.get(), functions, out);
    for (const auto& e : s->step) collectCalls(e.get(), functions, out);
    for (const auto& b : s->body) collectCalls(b.get(), functions, out);
  }

  // the prims a link number reaches from the prim sender (Sandbox::messageLinked)
  QVector<int> linkTargets(int link, int sender, int prims)
  {
    const int LINK_SET = -1, LINK_ALL_OTHERS = -2, LINK_ALL_CHILDREN = -3, LINK_THIS = -4;
    QVector<int> out;
    for (int prim = 1; prim <= prims; ++prim)
    {
      bool hit = false;
      switch (link)
      {
        case LINK_SET: hit = true; break;
        case LINK_THIS: hit = prim == sender; break;
        case LINK_ALL_OTHERS: hit = prim != sender; break;
        case LINK_ALL_CHILDREN: hit = prims > 1 && prim > 1; break;
        default: hit = prims == 1 ? link == 0 : link == prim; break;
      }
      if (hit) out.append(prim);
    }
    return out;
  }
}

QString Box::label() const
{
  return hub ? QObject::tr("%1 (hub)").arg(name) : name;
}

QString Box::explain() const
{
  switch (kind)
  {
    case BoxKind::You: return QObject::tr("You, the avatar. You see menus and open chat, and you click.");
    case BoxKind::Viewer: return QObject::tr("Your viewer. It gets the RLV lines a script sends.");
    case BoxKind::Web: return QObject::tr("A web server outside Second Life. The sandbox never goes online.");
    case BoxKind::Part: break;
  }
  if (hub) return QObject::tr("This part listens and passes messages on, like a server.");
  return link > 0 ? QObject::tr("A part of the object (prim %1).").arg(link) : QObject::tr("An object in the run.");
}

QString kindName(BoxKind kind)
{
  switch (kind)
  {
    case BoxKind::Part: return QStringLiteral("part");
    case BoxKind::You: return QStringLiteral("you");
    case BoxKind::Viewer: return QStringLiteral("viewer");
    case BoxKind::Web: return QStringLiteral("web");
  }
  return QString();
}

QString kindName(ArrowKind kind)
{
  switch (kind)
  {
    case ArrowKind::Chat: return QStringLiteral("chat");
    case ArrowKind::LinkMessage: return QStringLiteral("link_message");
    case ArrowKind::Menu: return QStringLiteral("menu");
    case ArrowKind::MenuAnswer: return QStringLiteral("menu_answer");
    case ArrowKind::YouSay: return QStringLiteral("you_say");
    case ArrowKind::Touch: return QStringLiteral("touch");
    case ArrowKind::OwnerSay: return QStringLiteral("owner_say");
    case ArrowKind::Rlv: return QStringLiteral("rlv");
    case ArrowKind::RlvReply: return QStringLiteral("rlv_reply");
    case ArrowKind::Http: return QStringLiteral("http");
    case ArrowKind::HttpIn: return QStringLiteral("http_in");
  }
  return QString();
}

QString Arrow::label() const
{
  QString what;
  switch (kind)
  {
    case ArrowKind::Chat:
      if (how == QLatin1String("whisper")) what = QObject::tr("whisper on channel %1").arg(channel);
      else if (how == QLatin1String("shout")) what = QObject::tr("shout on channel %1").arg(channel);
      else if (how == QLatin1String("region_say")) what = QObject::tr("region say on channel %1").arg(channel);
      else if (how == QLatin1String("region_say_to")) what = QObject::tr("say to one on channel %1").arg(channel);
      else what = QObject::tr("say on channel %1").arg(channel);
      break;
    case ArrowKind::LinkMessage: what = QObject::tr("link message %1").arg(channel); break;
    case ArrowKind::Menu: what = QObject::tr("menu on channel %1").arg(channel); break;
    case ArrowKind::MenuAnswer: what = QObject::tr("menu answer on channel %1").arg(channel); break;
    case ArrowKind::YouSay: what = QObject::tr("you say on channel %1").arg(channel); break;
    case ArrowKind::Touch: what = QObject::tr("click"); break;
    case ArrowKind::OwnerSay: what = how == QLatin1String("instant_message") ? QObject::tr("message to you") : QObject::tr("owner say"); break;
    case ArrowKind::Rlv: what = QObject::tr("RLV line"); break;
    case ArrowKind::RlvReply: what = QObject::tr("answer on channel %1").arg(channel); break;
    case ArrowKind::Http: what = QObject::tr("web request"); break;
    case ArrowKind::HttpIn: what = QObject::tr("web request in"); break;
  }
  QString out = text.isEmpty() ? what : QObject::tr("%1: \"%2\"").arg(what, text);
  if (count > 1) out = QObject::tr("%1 (%2 times)").arg(out).arg(count);
  return out;
}

int Map::unheard() const
{
  int n = 0;
  for (const Arrow& a : arrows) if (a.to < 0) ++n;
  return n;
}

int Map::messages() const
{
  int n = 0;
  for (const Arrow& a : arrows) n += a.count;
  return n;
}

int Map::indexOf(BoxKind kind, const QString& name) const
{
  for (int i = 0; i < boxes.size(); ++i)
    if (boxes[i].kind == kind && (name.isNull() || boxes[i].name == name)) return i;
  return -1;
}

QJsonObject Map::toJson() const
{
  QJsonArray boxList;
  for (const Box& b : boxes)
  {
    QJsonObject o{{"kind", kindName(b.kind)}, {"name", b.name}};
    if (!b.object.isEmpty()) o["object"] = b.object;
    if (b.link > 0) o["link"] = b.link;
    if (b.hub) o["hub"] = true;
    boxList.append(o);
  }
  QJsonArray arrowList;
  for (const Arrow& a : arrows)
  {
    QJsonObject o{{"kind", kindName(a.kind)}, {"from", a.from}, {"to", a.to}, {"how", a.how}, {"text", a.text},
                  {"count", a.count}, {"t", a.time}};
    if (a.hasChannel) o["channel"] = a.channel;
    if (!a.error.isEmpty()) o["error"] = a.error;
    if (!a.file.isEmpty()) o["file"] = a.file;
    if (a.line > 0) { o["line"] = a.line; o["column"] = a.column; }
    arrowList.append(o);
  }
  QJsonObject out{{"boxes", boxList}, {"arrows", arrowList}};
  if (truncated) out["truncated"] = true;
  return out;
}

Place findCall(const QString& source, const QStringList& functions, const QString& text)
{
  Place place;
  if (source.isEmpty() || functions.isEmpty()) return place;
  const lsl::Parser::Result parsed = lsl::Parser::parse(source);
  if (!parsed.script) return place;
  QVector<QPair<int, int>> calls;
  for (const lsl::Global& g : parsed.script->globals) collectCalls(g.init.get(), functions, calls);
  for (const lsl::Function& f : parsed.script->functions) collectCalls(f.body.get(), functions, calls);
  for (const lsl::State& s : parsed.script->states)
    for (const lsl::EventHandler& h : s.events) collectCalls(h.body.get(), functions, calls);
  if (calls.isEmpty()) return place;
  std::sort(calls.begin(), calls.end());
  const QStringList lines = source.split(QLatin1Char('\n'));
  if (!text.isEmpty())
    for (const auto& c : calls)
      if (c.first >= 1 && c.first <= lines.size() && lines[c.first - 1].contains(text, Qt::CaseInsensitive))
        return Place{c.first, c.second};
  return Place{calls.first().first, calls.first().second};
}

Map build(const QJsonArray& log, const QJsonArray& objects, const QVector<Prim>& prims,
          const std::function<QString(const QString& file)>& source)
{
  Map map;
  // keys and owners of the objects, by name
  QHash<QString, QString> keyOf;
  QHash<QString, QString> objectOfKey;
  QString ownerKey = QString::fromLatin1(lsl::Sandbox::OWNER_KEY);
  for (const QJsonValue& v : objects)
  {
    const QJsonObject o = v.toObject();
    keyOf.insert(o.value("name").toString(), o.value("key").toString());
    objectOfKey.insert(o.value("key").toString(), o.value("name").toString());
    if (!o.value("owner").toString().isEmpty()) ownerKey = o.value("owner").toString();
  }
  QHash<int, QString> primName;
  int primCount = 0;
  for (const Prim& p : prims) { primName.insert(p.link, p.name); primCount = qMax(primCount, p.link); }

  QHash<QString, int> boxByKey;
  const auto box = [&](BoxKind kind, const QString& name, const QString& object = QString(), int link = 0) -> int
  {
    const QString key = kindName(kind) + QLatin1Char('\n') + name;
    auto it = boxByKey.constFind(key);
    if (it != boxByKey.constEnd()) return *it;
    Box b;
    b.kind = kind;
    b.name = name;
    b.object = object;
    b.link = link;
    map.boxes.append(b);
    boxByKey.insert(key, int(map.boxes.size()) - 1);
    return int(map.boxes.size()) - 1;
  };
  // the prims first, in link order, so the map reads as the tree
  QHash<int, int> primBox;
  for (const Prim& p : prims) primBox.insert(p.link, box(BoxKind::Part, p.name, QString(), p.link));
  const auto partOf = [&](const QJsonObject& e) -> int
  {
    if (e.contains("prim"))
    {
      const QString name = e.value("prim").toString();
      for (auto it = primName.constBegin(); it != primName.constEnd(); ++it)
        if (it.value() == name) return primBox.value(it.key());
      return box(BoxKind::Part, name, e.value("object").toString());
    }
    return box(BoxKind::Part, e.value("object").toString(QObject::tr("Object")), e.value("object").toString());
  };
  const auto linkOf = [&](const QJsonObject& e) -> int
  {
    const QString name = e.value("prim").toString();
    for (auto it = primName.constBegin(); it != primName.constEnd(); ++it)
      if (it.value() == name) return it.key();
    return 0;
  };
  const auto scriptId = [](const QJsonObject& e) -> QString
  {
    return e.value("object").toString() + QLatin1Char('\n') + e.value("script").toString();
  };

  // the sources, parsed once per file
  QHash<QString, QString> sources;
  const auto sourceOf = [&](const QString& file) -> QString
  {
    if (!source || file.isEmpty()) return QString();
    auto it = sources.constFind(file);
    if (it != sources.constEnd()) return *it;
    const QString text = source(file);
    sources.insert(file, text);
    return text;
  };
  QHash<QString, Place> places;
  const auto placeOf = [&](const QJsonObject& e, const QString& type, const QString& text) -> Place
  {
    const QString file = e.contains("file") ? e.value("file").toString() : e.value("script").toString();
    const QString key = file + QLatin1Char('\n') + type + QLatin1Char('\n') + text;
    auto it = places.constFind(key);
    if (it != places.constEnd()) return *it;
    const Place p = findCall(sourceOf(file), functionsFor(type, e), text);
    places.insert(key, p);
    return p;
  };

  QHash<QString, QVector<Listen>> listens;   // by script
  QSet<QString> stopped;
  QHash<QString, int> arrowByKey;
  const auto add = [&](Arrow a, const QJsonObject& e, const QString& type, const QString& findText)
  {
    const QString key = QStringLiteral("%1|%2|%3|%4|%5|%6|%7").arg(int(a.kind)).arg(a.from).arg(a.to).arg(a.hasChannel ? a.channel : 0)
                          .arg(a.how, a.error).arg(a.to < 0 ? a.text : QString());
    auto it = arrowByKey.constFind(key);
    if (it != arrowByKey.constEnd()) { ++map.arrows[*it].count; return; }
    if (map.arrows.size() >= MAX_ARROWS) { map.truncated = true; return; }
    a.time = e.value("t").toDouble();
    if (e.contains("script") && e.value("source").toString() != QLatin1String("sandbox"))
    {
      a.file = e.value("file").toString();
      a.script = e.value("script").toString();
      const Place p = placeOf(e, type, findText);
      a.line = p.line;
      a.column = p.column;
    }
    map.arrows.append(a);
    arrowByKey.insert(key, int(map.arrows.size()) - 1);
  };
  // the boxes whose listens hear a message on channel from a sender (name, key; a prim never hears itself)
  const auto hearers = [&](int channel, const QString& name, const QString& key, const QString& text, int senderBox,
                           const QString& onlyObject) -> QVector<int>
  {
    QVector<int> out;
    for (auto it = listens.constBegin(); it != listens.constEnd(); ++it)
    {
      if (stopped.contains(it.key())) continue;
      for (const Listen& l : it.value())
      {
        if (l.channel != channel || l.box == senderBox) continue;
        if (!onlyObject.isNull() && map.boxes[l.box].object != onlyObject && map.boxes[l.box].name != onlyObject) continue;
        if (!l.name.isEmpty() && l.name != name) continue;
        if (!l.id.isEmpty() && l.id != QLatin1String(NullKey) && l.id != key) continue;
        if (!l.message.isEmpty() && l.message != text) continue;
        if (!out.contains(l.box)) out.append(l.box);
      }
    }
    std::sort(out.begin(), out.end());
    return out;
  };
  const auto senderKey = [&](const QJsonObject& e) { return keyOf.value(e.value("object").toString()); };
  const auto senderName = [&](const QJsonObject& e) { return e.value("object").toString(); };

  for (const QJsonValue& value : log)
  {
    const QJsonObject e = value.toObject();
    const QString type = e.value("type").toString();
    const QString text = e.value("text").toString();
    if (type == QLatin1String("listen"))
    {
      Listen l;
      l.handle = e.value("handle").toInt();
      l.channel = e.value("channel").toInt();
      l.name = e.value("name").toString();
      l.id = e.value("id").toString();
      l.message = e.value("message").toString();
      l.box = partOf(e);
      l.script = scriptId(e);
      listens[l.script].append(l);
      continue;
    }
    if (type == QLatin1String("listen_remove"))
    {
      QVector<Listen>& list = listens[scriptId(e)];
      for (int i = 0; i < list.size(); ++i)
        if (list[i].handle == e.value("handle").toInt()) { list.remove(i); break; }
      continue;
    }
    // a state change or a reset removes the script's listens; a stopped script hears nothing
    if (type == QLatin1String("state") || type == QLatin1String("reset"))
    {
      listens.remove(scriptId(e));
      continue;
    }
    if (type == QLatin1String("error") || type == QLatin1String("limit") || type == QLatin1String("unsupported"))
    {
      stopped.insert(scriptId(e));
      continue;
    }

    if (type == QLatin1String("say") || type == QLatin1String("whisper") || type == QLatin1String("shout")
        || type == QLatin1String("region_say") || type == QLatin1String("region_say_to"))
    {
      const int from = partOf(e);
      const int channel = e.value("channel").toInt();
      Arrow a;
      a.kind = ArrowKind::Chat;
      a.from = from;
      a.how = type;
      a.channel = channel;
      a.hasChannel = true;
      a.text = text;
      QVector<int> to;
      if (type == QLatin1String("region_say_to"))
      {
        const QString target = e.value("to").toString();
        if (target == ownerKey && channel == 0) to.append(box(BoxKind::You, QObject::tr("You")));
        else if (objectOfKey.contains(target) && objectOfKey.value(target) != e.value("object").toString())
          to = hearers(channel, senderName(e), senderKey(e), text, from, objectOfKey.value(target));
      }
      else
      {
        // open chat reaches you too (llRegionSay never uses channel 0)
        if (channel == 0) to.append(box(BoxKind::You, QObject::tr("You")));
        to += hearers(channel, senderName(e), senderKey(e), text, from, QString());
      }
      if (to.isEmpty())
      {
        a.error = type == QLatin1String("region_say_to") ? QObject::tr("Nobody with that key listens on channel %1.").arg(channel)
                                                         : QObject::tr("Nobody listens on channel %1.").arg(channel);
        add(a, e, type, text);
      }
      for (int t : to) { a.to = t; add(a, e, type, text); }
      continue;
    }
    if (type == QLatin1String("message_linked"))
    {
      const int from = partOf(e);
      Arrow a;
      a.kind = ArrowKind::LinkMessage;
      a.from = from;
      a.how = type;
      a.channel = e.value("num").toInt();
      a.hasChannel = true;
      a.text = text;
      QVector<int> to;
      if (e.value("scripts").toInt() > 0)
        for (int prim : linkTargets(e.value("link").toInt(), linkOf(e), primCount))
        {
          bool scripted = false;
          for (const Prim& p : prims) if (p.link == prim) scripted = p.hasScripts;
          if (scripted) to.append(primBox.value(prim));
        }
      if (to.isEmpty() && e.value("scripts").toInt() > 0) to.append(from);   // a run that is not a folder: its own object
      if (to.isEmpty())
      {
        a.error = QObject::tr("No script gets link message %1.").arg(a.channel);
        add(a, e, type, text);
      }
      for (int t : to) { a.to = t; add(a, e, type, text); }
      continue;
    }
    if (type == QLatin1String("dialog"))
    {
      Arrow a;
      a.kind = ArrowKind::Menu;
      a.from = partOf(e);
      a.to = box(BoxKind::You, QObject::tr("You"));
      a.how = type;
      a.channel = e.value("channel").toInt();
      a.hasChannel = true;
      a.text = e.value("message").toString();
      add(a, e, type, a.text);
      continue;
    }
    if (type == QLatin1String("owner_say") || type == QLatin1String("instant_message"))
    {
      Arrow a;
      a.kind = ArrowKind::OwnerSay;
      a.from = partOf(e);
      a.to = box(BoxKind::You, QObject::tr("You"));
      a.how = type;
      a.text = text;
      add(a, e, type, text);
      continue;
    }
    if (type == QLatin1String("rlv_command"))
    {
      const int from = partOf(e);
      Arrow a;
      a.kind = ArrowKind::Rlv;
      a.from = from;
      a.to = box(BoxKind::Viewer, QObject::tr("Your viewer (RLV)"));
      a.how = type;
      a.text = text;
      if (e.contains("error"))
      {
        a.error = QObject::tr("Your viewer refused this line: %1").arg(e.value("error").toString());
        a.to = -1;
      }
      add(a, e, type, text);
      for (const QJsonValue& r : e.value("replies").toArray())
      {
        Arrow reply;
        reply.kind = ArrowKind::RlvReply;
        reply.from = box(BoxKind::Viewer, QObject::tr("Your viewer (RLV)"));
        reply.to = from;
        reply.how = QStringLiteral("rlv_reply");
        reply.channel = r.toObject().value("channel").toInt();
        reply.hasChannel = true;
        reply.text = r.toObject().value("text").toString();
        QJsonObject outside = e;
        outside.insert("source", "sandbox");
        add(reply, outside, type, QString());
      }
      continue;
    }
    if (type == QLatin1String("http_request"))
    {
      Arrow a;
      a.kind = ArrowKind::Http;
      a.from = partOf(e);
      a.to = box(BoxKind::Web, QObject::tr("Web server"));
      a.how = type;
      a.text = QStringLiteral("%1 %2").arg(e.value("method").toString(), e.value("url").toString());
      add(a, e, type, e.value("url").toString());
      continue;
    }
    if (type == QLatin1String("input") && !e.contains("error"))
    {
      const QString input = e.value("input").toString();
      const int you = input == QLatin1String("http_request") ? -1 : box(BoxKind::You, QObject::tr("You"));
      Arrow a;
      a.how = input;
      if (input == QLatin1String("touch"))
      {
        a.kind = ArrowKind::Touch;
        a.from = you;
        a.to = partOf(e);
        add(a, e, type, QString());
      }
      else if (input == QLatin1String("chat") || input == QLatin1String("dialog"))
      {
        a.kind = input == QLatin1String("chat") ? ArrowKind::YouSay : ArrowKind::MenuAnswer;
        a.from = you;
        a.channel = e.value("channel").toInt();
        a.hasChannel = true;
        a.text = text;
        const QVector<int> to = hearers(a.channel, e.value("name").toString(), ownerKey, text, you, QString());
        if (to.isEmpty())
        {
          a.error = a.kind == ArrowKind::MenuAnswer ? QObject::tr("Nobody listens on channel %1, so the menu answer is lost.").arg(a.channel)
                                                    : QObject::tr("Nobody listens on channel %1.").arg(a.channel);
          add(a, e, type, text);
        }
        for (int t : to) { a.to = t; add(a, e, type, text); }
      }
      else if (input == QLatin1String("http_request"))
      {
        a.kind = ArrowKind::HttpIn;
        a.from = box(BoxKind::Web, QObject::tr("Web server"));
        a.to = partOf(e);
        a.text = e.value("method").toString();
        add(a, e, type, QString());
      }
      continue;
    }
  }

  // hubs: a part that hears chat or a link message from one box and passes messages on to another box
  // (a part that only answers the box that spoke to it is not a hub)
  for (int i = 0; i < map.boxes.size(); ++i)
  {
    if (map.boxes[i].kind != BoxKind::Part) continue;
    QSet<int> heardFrom, passedTo;
    for (const Arrow& a : map.arrows)
    {
      const bool talk = a.kind == ArrowKind::Chat || a.kind == ArrowKind::LinkMessage;
      if (a.to == i && a.from != i && (talk || a.kind == ArrowKind::MenuAnswer || a.kind == ArrowKind::YouSay)) heardFrom.insert(a.from);
      if (a.from == i && a.to >= 0 && a.to != i && talk) passedTo.insert(a.to);
    }
    for (int from : heardFrom)
      for (int to : passedTo)
        if (from != to) map.boxes[i].hub = true;
  }
  return map;
}

Map fromFolderResult(const QJsonObject& folderResult, const std::function<QString(const QString& file)>& source)
{
  const QJsonObject run = folderResult.value("result").toObject();
  if (run.isEmpty()) return Map();
  QVector<Prim> prims;
  for (const QJsonValue& v : folderResult.value("linkset").toObject().value("prims").toArray())
  {
    const QJsonObject p = v.toObject();
    prims.append(Prim{p.value("link").toInt(), p.value("name").toString(), !p.value("scripts").toArray().isEmpty()});
  }
  return build(run.value("log").toArray(), run.value("objects").toArray(), prims, source);
}

} // namespace LabChannelMap
