/*
 * PinglyMotion Script Lab - LSL interpreter: the built-in functions for chat, listens and timers, link messages and Linkset Data, HTTP, money, web server, mail, llCloud and remote data
 *
 * What each function does follows the Second Life wiki and OpenSimulator (BSD); written from that
 * description, no code copied. See builtins.cpp for the tables and builtins_internal.h for the helpers.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "builtins_internal.h"

namespace lsl
{

using namespace builtin_detail;

Value Interpreter::callBuiltinComm(Fn fn, const QString& name, std::vector<Value>& a, bool& handled)
{
  handled = true;
  [[maybe_unused]] const qint32 DEBUG_CHANNEL = 0x7FFFFFFF;
  [[maybe_unused]] const QString NULL_KEY = "00000000-0000-0000-0000-000000000000";
  (void)name;

  switch (fn)
  {
    // -------------------------------------------- chat
    case Fn::OwnerSay:
      m_world.ownerSay(m_now, a[0].s);
      return Value();
    case Fn::Say:
    case Fn::Whisper:
    case Fn::Shout:
    {
      const char* kind = fn == Fn::Say ? "say" : (fn == Fn::Whisper ? "whisper" : "shout");
      m_world.chat(m_now, kind, a[0].i, a[1].s);
      return Value();
    }
    case Fn::RegionSay:
      if (a[0].i == 0)
      {
        m_world.log(m_now, QJsonObject{{"type", "debug"}, {"channel", DEBUG_CHANNEL}, {"text", "llRegionSay cannot be used on channel 0."}});
        return Value();
      }
      m_world.chat(m_now, "region_say", a[0].i, a[1].s);
      return Value();
    case Fn::RegionSayTo:
      m_world.chatTo(m_now, a[0].s, a[1].i, a[2].s);
      return Value();
    case Fn::InstantMessage:
      m_world.instantMessage(m_now, a[0].s, a[1].s);
      return Value();
    case Fn::Dialog:
    {
      // the checks and messages of Second Life's llDialog; a refused dialog is not shown
      QString problem;
      QStringList buttons;
      for (const Value& v : a[2].list) buttons << toLslString(v);
      if (a[1].s.isEmpty()) problem = "llDialog: must supply a message";
      else if (a[1].s.toUtf8().size() >= 512) problem = "llDialog: message too long, must be less than 512 characters";
      else if (buttons.size() > 12) problem = "llDialog: button list too long, must be 12 or fewer entries";
      for (const QString& b : buttons)
        if (problem.isEmpty() && b.trimmed().isEmpty()) problem = "llDialog: all buttons must have label strings";
        else if (problem.isEmpty() && b.toUtf8().size() > 24) problem = "llDialog: button labels must be 24 or fewer characters long";
      if (!problem.isEmpty())
      {
        m_world.log(m_now, QJsonObject{{"type", "debug"}, {"channel", DEBUG_CHANNEL}, {"text", problem}});
        return Value();
      }
      if (buttons.isEmpty()) buttons << "OK";
      m_world.dialog(m_now, a[0].s, a[1].s, buttons, a[3].i);
      return Value();
    }
    case Fn::GetPos:
    {
      float p[3];
      primWorldPosition(m_world, qMax(1, m_world.linkNumber()), p);
      return Value::vector(p[0], p[1], p[2]);
    }
    case Fn::GetAttached: return Value::integer(m_world.attachPoint());
    case Fn::GetOwnerKey:
    {
      const QString owner = m_world.ownerOf(a[0].s);
      return Value::key(owner.isEmpty() ? a[0].s : owner);
    }

    // -------------------------------------------- listen and timer
    case Fn::Listen:
    {
      if (int(m_listens.size()) >= m_limits.maxListens)
        fail(QString("Too many listens: at most %1 at a time, as in Second Life.").arg(m_limits.maxListens));
      Listen l;
      l.handle = m_nextListen++;
      l.channel = a[0].i;
      l.name = a[1].s;
      l.id = a[2].s;
      l.message = a[3].s;
      l.active = true;
      m_listens.push_back(l);
      m_world.log(m_now, QJsonObject{{"type", "listen"}, {"handle", l.handle}, {"channel", l.channel},
                                     {"name", World::clip(l.name)}, {"id", World::clip(l.id)}, {"message", World::clip(l.message)}});
      return Value::integer(l.handle);
    }
    case Fn::ListenRemove:
      for (size_t i = 0; i < m_listens.size(); ++i)
        if (m_listens[i].handle == a[0].i)
        {
          m_listens.erase(m_listens.begin() + qint64(i));
          m_world.log(m_now, QJsonObject{{"type", "listen_remove"}, {"handle", a[0].i}});
          break;
        }
      return Value();
    case Fn::ListenControl:
      for (Listen& l : m_listens)
        if (l.handle == a[0].i) l.active = a[1].i != 0;
      return Value();
    case Fn::SetTimerEvent:
    {
      const float seconds = a[0].f[0];
      if (!(seconds > 0.0f) || !std::isfinite(seconds)) m_timerInterval = 0.0;
      else
      {
        m_timerInterval = qMax(double(seconds), 0.01);
        m_nextTimer = m_now + m_timerInterval;
      }
      m_world.log(m_now, QJsonObject{{"type", "timer_set"}, {"seconds", m_timerInterval}});
      return Value();
    }
    case Fn::Sleep:
    {
      const float seconds = a[0].f[0];
      if (seconds > 0.0f && std::isfinite(seconds))
      {
        m_now = qMin(m_now + double(seconds), 1.0e7);
        m_world.log(m_now, QJsonObject{{"type", "sleep"}, {"seconds", double(seconds)}});
      }
      return Value();
    }

    // -------------------------------------------- link messages and Linkset Data (step 9a)
    case Fn::MessageLinked:
      m_world.messageLinked(m_now, a[0].i, a[1].i, a[2].s, a[3].s);
      return Value();
    case Fn::GetLinkNumber: return Value::integer(m_world.linkNumber());
    case Fn::LinksetDataWrite:
    {
      LinksetData& data = m_world.linksetData();
      const bool existed = data.pairs().contains(a[0].s);
      const int result = data.write(a[0].s, a[1].s);
      const bool deleted = a[1].s.isEmpty();
      QJsonObject entry{{"type", "linkset_data"}, {"op", deleted ? "delete" : "write"}, {"name", World::clip(a[0].s)},
                        {"result", LinksetData::resultName(result)}, {"used", data.used()}};
      if (!deleted) entry["value"] = World::clip(a[1].s);
      m_world.log(m_now, entry);
      if (result == LinksetData::Ok)
      {
        if (deleted && existed) m_world.linksetDataChanged(m_now, LinksetData::ActionDelete, a[0].s, QString());
        else if (!deleted) m_world.linksetDataChanged(m_now, LinksetData::ActionUpdate, a[0].s, a[1].s);
      }
      return Value::integer(result);
    }
    case Fn::LinksetDataRead: return Value::string(m_world.linksetData().read(a[0].s));
    case Fn::LinksetDataDelete:
    {
      LinksetData& data = m_world.linksetData();
      const int result = data.remove(a[0].s);
      m_world.log(m_now, QJsonObject{{"type", "linkset_data"}, {"op", "delete"}, {"name", World::clip(a[0].s)},
                                     {"result", LinksetData::resultName(result)}, {"used", data.used()}});
      if (result == LinksetData::Ok) m_world.linksetDataChanged(m_now, LinksetData::ActionDelete, a[0].s, QString());
      return Value::integer(result);
    }
    case Fn::LinksetDataReset:
      m_world.linksetData().reset();
      m_world.log(m_now, QJsonObject{{"type", "linkset_data"}, {"op", "reset"}, {"used", 0}});
      m_world.linksetDataChanged(m_now, LinksetData::ActionReset, QString(), QString());
      return Value();
    case Fn::LinksetDataListKeys:
    {
      const QStringList names = m_world.linksetData().keys(a[0].i, a[1].i);
      if (names.size() > m_limits.maxListLength)
        fail(QString("llLinksetDataListKeys would return %1 keys, more than a list can hold in the sandbox (%2); ask for fewer with count.")
               .arg(names.size()).arg(m_limits.maxListLength));
      Value out = Value::defaultFor(Type::List);
      for (const QString& n : names) out.list.push_back(Value::string(n));
      checkSize(out);
      return out;
    }
    case Fn::LinksetDataAvailable: return Value::integer(m_world.linksetData().available());
    case Fn::LinksetDataCountKeys: return Value::integer(m_world.linksetData().count());

    // -------------------------------------------- HTTP with canned replies (step 9b-9); the sandbox never goes on the network
    case Fn::HTTPRequest:
    {
      const QString url = a[0].s;
      auto refuse = [&](const QString& text)
      {
        m_world.log(m_now, QJsonObject{{"type", "debug"}, {"channel", DEBUG_CHANNEL}, {"text", "llHTTPRequest: " + text}});
        return Value::key(NULL_KEY);
      };
      if (!httpUrlValid(url)) return refuse("the URL must start with http:// or https://, name a host and have no spaces or control characters.");
      QString method = "GET", mimetype = "text/plain;charset=utf-8";
      int maxLength = HTTP_DEFAULT_BODY_LENGTH, headerChars = 0;
      bool verbose = true;
      QJsonArray headers;
      const std::vector<Value>& p = a[1].list;
      for (size_t i = 0; i < p.size();)
      {
        if (p[i].type != Type::Integer || p[i].i < HTTP_METHOD || p[i].i > HTTP_EXTENDED_ERROR)
          return refuse(QString("parameter %1 of the list is not one of the HTTP_* flags.").arg(i + 1));
        const int flag = p[i].i;
        const QString flagName = httpFlagName(flag);
        const bool text = flag == HTTP_METHOD || flag == HTTP_MIMETYPE || flag == HTTP_CUSTOM_HEADER || flag == HTTP_USER_AGENT || flag == HTTP_ACCEPT;
        const size_t count = flag == HTTP_CUSTOM_HEADER ? 2 : 1;
        if (i + count >= p.size()) return refuse(flagName + " is missing its value.");
        for (size_t k = 1; k <= count; ++k)
          if (p[i + k].type != (text ? Type::String : Type::Integer)) return refuse(flagName + (text ? " needs a string." : " needs an integer."));
        const Value& v = p[i + 1];
        switch (flag)
        {
          case HTTP_METHOD:
            if (v.s != "GET" && v.s != "POST" && v.s != "PUT" && v.s != "DELETE")
              return refuse("HTTP_METHOD must be \"GET\", \"POST\", \"PUT\" or \"DELETE\".");
            method = v.s;
            break;
          case HTTP_MIMETYPE: mimetype = v.s; break;
          case HTTP_BODY_MAXLENGTH:
            if (v.i < 0 || v.i > HTTP_MAX_BODY_LENGTH) return refuse(QString("HTTP_BODY_MAXLENGTH must be 0 to %1.").arg(HTTP_MAX_BODY_LENGTH));
            maxLength = v.i;
            break;
          case HTTP_VERBOSE_THROTTLE: verbose = v.i != 0; break;
          case HTTP_CUSTOM_HEADER:
          {
            const QString& header = v.s;
            if (header.isEmpty()) return refuse("HTTP_CUSTOM_HEADER needs a header name.");
            if (header.compare("Content-Type", Qt::CaseInsensitive) == 0) return refuse("set Content-Type with HTTP_MIMETYPE, not HTTP_CUSTOM_HEADER.");
            headerChars += int(header.size() + p[i + 2].s.size());
            if (headerChars > HTTP_MAX_HEADER_CHARS) return refuse(QString("the custom headers together have more than %1 characters.").arg(HTTP_MAX_HEADER_CHARS));
            headers.append(QJsonArray{World::clip(header), World::clip(p[i + 2].s)});
            break;
          }
          case HTTP_USER_AGENT:
            if (v.s.contains(' ')) return refuse("HTTP_USER_AGENT may not have spaces.");
            break;
          default: break;   // HTTP_VERIFY_CERT, HTTP_PRAGMA_NO_CACHE, HTTP_ACCEPT, HTTP_EXTENDED_ERROR: accepted, nothing to simulate
        }
        i += count + 1;
      }
      QJsonObject entry{{"type", "http_request"}, {"method", method}, {"url", World::clip(url)}, {"mimetype", World::clip(mimetype)}};
      if (!a[2].s.isEmpty()) entry["body"] = World::clip(a[2].s);
      if (!headers.isEmpty()) entry["headers"] = headers;
      // the throttle: at most 25 requests per object in 20 seconds
      std::deque<double>& times = m_world.httpTimes();
      while (!times.empty() && times.front() <= m_now - HTTP_THROTTLE_SECONDS) times.pop_front();
      if (int(times.size()) >= HTTP_THROTTLE_REQUESTS)
      {
        entry["result"] = "throttled";
        if (!m_world.logFull()) m_world.log(m_now, entry);
        if (verbose)
          m_world.log(m_now, QJsonObject{{"type", "debug"}, {"channel", DEBUG_CHANNEL},
                                         {"text", QString("llHTTPRequest: too many requests (more than %1 in %2 seconds from this object), so this one was refused.").arg(HTTP_THROTTLE_REQUESTS).arg(HTTP_THROTTLE_SECONDS)}});
        return Value::key(NULL_KEY);
      }
      times.push_back(m_now);
      const HttpReply* reply = m_world.httpReply(method, url);
      const QString id = nextKey();
      const int status = reply ? reply->status : 499;
      const QString full = reply ? reply->body : QString();
      const QString body = utf8Prefix(full, maxLength);
      const int truncatedAt = body.size() < full.size() ? maxLength : -1;
      entry["request"] = id;
      entry["result"] = reply ? "canned" : "no_reply";
      entry["status"] = status;
      if (truncatedAt >= 0) entry["truncated_at"] = truncatedAt;
      if (!m_world.logFull()) m_world.log(m_now, entry);
      if (!m_world.httpResponse(m_now, id, status, truncatedAt, body)) httpResponseEvent(id, status, truncatedAt, body);
      return Value::key(id);
    }

    // -------------------------------------------- money, web server and mail stubs (gap list 2, step 4)
    case Fn::GiveMoney:
    {
      // logic and messages only: no L$ ever moves. llGiveMoney always answers 0 (Second Life wiki).
      QString result = "ok";
      if (!(m_permissions & 0x2))
      {
        m_world.log(m_now, QJsonObject{{"type", "debug"}, {"channel", DEBUG_CHANNEL},
                                       {"text", "Script trying to pay L$ but PERMISSION_DEBIT permission not set"}});
        result = "MISSING_PERMISSION_DEBIT";
      }
      else if (!keyIsValid(a[0].s)) result = "INVALID_DESTINATION";
      else if (a[1].i <= 0) result = "INVALID_AMOUNT";
      else if (a[0].s != m_world.ownerKey() && !m_world.ownerOf(a[0].s).isEmpty()) result = "LINDENDOLLAR_ENTITYDOESNOTEXIST";   // an object, not an avatar
      if (!m_world.logFull())
        m_world.log(m_now, QJsonObject{{"type", "give_money"}, {"to", a[0].s}, {"amount", a[1].i}, {"result", result},
                                       {"note", "Simulated: no L$ moved. The sandbox owner always has enough L$."}});
      return Value::integer(0);
    }
    case Fn::Email:
    {
      // record-only: no mail is sent. Second Life puts the object's name, region and position in front of the message,
      // allows 4096 bytes for all of it together, and makes the script sleep 20 seconds.
      const RegionSettings& region = m_world.region();
      float p[3];
      m_world.position(p);
      const QString header = QString("Object-Name: %1\nRegion: %2 (%3, %4)\nLocal-Position: (%5, %6, %7)\n\n")
                               .arg(m_world.objectName(), region.name).arg(qint64(region.corner[0])).arg(qint64(region.corner[1]))
                               .arg(qint64(std::floor(p[0]))).arg(qint64(std::floor(p[1]))).arg(qint64(std::floor(p[2])));
      const qint64 bytes = qint64(a[0].s.toUtf8().size() + a[1].s.toUtf8().size() + header.toUtf8().size() + a[2].s.toUtf8().size());
      const bool tooLong = bytes > EMAIL_MAX_BYTES;
      if (tooLong)
        m_world.log(m_now, QJsonObject{{"type", "debug"}, {"channel", DEBUG_CHANNEL},
                                       {"text", QString("llEmail: the address, subject and message are %1 bytes with the header, more than %2, so the email was not sent.").arg(bytes).arg(EMAIL_MAX_BYTES)}});
      if (!m_world.logFull())
        m_world.log(m_now, QJsonObject{{"type", "email_sent"}, {"address", World::clip(a[0].s)}, {"subject", World::clip(a[1].s)},
                                       {"message", World::clip(a[2].s)}, {"bytes", double(bytes)}, {"result", tooLong ? "too_long" : "recorded"},
                                       {"note", "Simulated: no email was sent."}});
      m_now = qMin(m_now + EMAIL_DELAY, 1.0e7);
      return Value();
    }
    case Fn::GetNextEmail:
    {
      Email mail;
      int left = 0;
      const bool found = m_world.nextEmail(a[0].s, a[1].s, mail, left);
      QJsonObject entry{{"type", "email_check"}, {"address", World::clip(a[0].s)}, {"subject", World::clip(a[1].s)}, {"found", found}};
      if (found) entry["left"] = left;
      if (!m_world.logFull()) m_world.log(m_now, entry);
      if (found)
        queueEvent(Event{"email", {Value::string(mail.time), Value::string(mail.address), Value::string(mail.subject),
                                   Value::string(mail.message), Value::integer(left)}, QString(), QString()});
      return Value();
    }
    case Fn::RequestURL:
    case Fn::RequestSecureURL:
    {
      // a URL of the sandbox's fake web server, never a real one; the answer comes as http_request
      const bool secure = fn == Fn::RequestSecureURL;
      const QString id = nextKey();
      const QString url = m_world.grantUrl(secure);
      QJsonObject entry{{"type", "url_request"}, {"secure", secure}, {"request", id}, {"result", url.isEmpty() ? "denied" : "granted"}};
      if (!url.isEmpty()) entry["url"] = url;
      if (!m_world.logFull()) m_world.log(m_now, entry);
      queueEvent(Event{"http_request", {Value::key(id), Value::string(url.isEmpty() ? "URL_REQUEST_DENIED" : "URL_REQUEST_GRANTED"),
                                        Value::string(url)}, QString(), QString()});
      return Value::key(id);
    }
    case Fn::ReleaseURL:
    {
      const bool released = m_world.releaseUrl(a[0].s);
      if (!m_world.logFull())
        m_world.log(m_now, QJsonObject{{"type", "url_release"}, {"url", World::clip(a[0].s)}, {"result", released ? "released" : "not_found"}});
      return Value();
    }
    case Fn::GetFreeURLs: return Value::integer(m_world.freeUrls());
    case Fn::HTTPResponse:
    {
      // the answer to a request from outside: recorded, never sent anywhere
      dropExpiredRequests();
      QString result = "unknown_request";
      for (auto r = m_incoming.begin(); r != m_incoming.end(); ++r)
        if (r->id == a[0].s)
        {
          result = m_now - r->at <= HTTP_RESPONSE_TIMEOUT ? "sent" : "timed_out";
          m_incoming.erase(r);
          break;
        }
      if (!m_world.logFull())
        m_world.log(m_now, QJsonObject{{"type", "http_server_response"}, {"request", a[0].s}, {"status", a[1].i}, {"body", World::clip(a[2].s)}, {"result", result}});
      return Value();
    }
    case Fn::GetHTTPHeader:
    {
      // the header names must be lower case, as in Second Life; "" once answered or after HTTP_HEADER_TIMEOUT seconds
      dropExpiredRequests();
      for (const IncomingRequest& r : m_incoming)
        if (r.id == a[0].s) return Value::string(r.headers.value(a[1].s));
      return Value::string(QString());
    }

    // -------------------------------------------- steps 16 and 17: llCloud and remote data (XML-RPC, deprecated)
    case Fn::Cloud:
      // the wiki: since 13.02.08.270166 it only returns 0.0
      return Value::number(0.0f);
    case Fn::OpenRemoteData:
    {
      // one channel per script: a second call gives the same channel again (the wiki: the same ID is kept). The channel
      // is announced by remote_data(REMOTE_DATA_CHANNEL, channel, NULL_KEY, "", 0, ""). Nothing is opened on the network.
      const bool fresh = m_remoteChannel.isEmpty();
      if (fresh) m_remoteChannel = nextKey();
      if (!m_world.logFull())
        m_world.log(m_now, QJsonObject{{"type", "remote_data"}, {"op", "open"}, {"channel", m_remoteChannel}, {"new", fresh},
                                       {"note", "Simulated: no XML-RPC channel exists; a request comes from a remote_data input."}});
      queueEvent(Event{"remote_data", {Value::integer(1), Value::key(m_remoteChannel), Value::key("00000000-0000-0000-0000-000000000000"),
                                       Value::string(QString()), Value::integer(0), Value::string(QString())}, QString(), QString()});
      return Value();
    }
    case Fn::CloseRemoteData:
    {
      const bool found = !m_remoteChannel.isEmpty() && a[0].s == m_remoteChannel;
      if (found)
      {
        m_remoteChannel.clear();
        m_remoteReceived.clear();
      }
      if (!m_world.logFull())
        m_world.log(m_now, QJsonObject{{"type", "remote_data"}, {"op", "close"}, {"channel", World::clip(a[0].s)}, {"result", found ? "closed" : "not_found"}});
      return Value();
    }
    case Fn::RemoteDataSetRegion:
      if (!m_world.logFull())
        m_world.log(m_now, QJsonObject{{"type", "remote_data"}, {"op", "set_region"}, {"channel", m_remoteChannel},
                                       {"note", "Simulated: the sandbox has one region, so there is nothing to register again."}});
      return Value();
    case Fn::SendRemoteData:
    {
      // the request is only recorded, never sent. The answer is a remote_data input of kind "reply" with this message ID.
      const QString id = nextKey();
      m_remoteSent.append(id);
      m_remoteSentChannel.insert(id, a[0].s);
      while (m_remoteSent.size() > MAX_REMOTE_MESSAGES) m_remoteSentChannel.remove(m_remoteSent.takeFirst());
      if (!m_world.logFull())
        m_world.log(m_now, QJsonObject{{"type", "remote_data"}, {"op", "send"}, {"channel", World::clip(a[0].s)}, {"dest", World::clip(a[1].s)},
                                       {"idata", a[2].i}, {"sdata", World::clip(a[3].s)}, {"message", id},
                                       {"note", "Simulated: nothing is sent; send a remote_data input of kind reply to answer it."}});
      return Value::key(id);
    }
    case Fn::RemoteDataReply:
    {
      QString result = "sent";
      if (m_remoteChannel.isEmpty() || a[0].s != m_remoteChannel) result = "unknown_channel";
      else if (!m_remoteReceived.contains(a[1].s)) result = "unknown_message";
      else m_remoteReceived.removeAll(a[1].s);
      if (!m_world.logFull())
        m_world.log(m_now, QJsonObject{{"type", "remote_data"}, {"op", "reply"}, {"channel", World::clip(a[0].s)}, {"message", World::clip(a[1].s)},
                                       {"sdata", World::clip(a[2].s)}, {"idata", a[3].i}, {"result", result}});
      return Value();
    }

    default:
      break;
  }
  handled = false;
  return Value();
}

} // namespace lsl
