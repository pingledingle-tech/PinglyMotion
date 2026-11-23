/*
 * PinglyMotion - the notes line under the timeline (UX phase 3c), see
 * timelinenotes.h
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "timelinenotes.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QLabel>
#include <QPainter>
#include <QPushButton>

#include <algorithm>

#include "lucideicons.h"
#include "theme.h"
#include "transportlook.h"

using Theme::Token;

QString NotesSpec::text(const char* source)
{
  return QCoreApplication::translate(context,source);
}

NotesSpec::Notes NotesSpec::fromExtensions(const QJsonObject& extensions)
{
  Notes notes;
  const QJsonValue value=extensions.value(QLatin1String(extensionKey));
  if(!value.isObject()) return notes;
  const QJsonObject object=value.toObject();

  const QJsonArray keys=object.value(QLatin1String("keys")).toArray();
  for(const QJsonValue& entry : keys)
  {
    if(notes.keys.size()>=maxNotes) break;
    if(!entry.isObject()) continue;
    const QJsonObject key=entry.toObject();
    const QJsonValue frameValue=key.value(QLatin1String("frame"));
    const QJsonValue noteValue=key.value(QLatin1String("note"));
    if(!frameValue.isDouble() || !noteValue.isString()) continue;
    const double frame=frameValue.toDouble();
    if(frame<0 || frame>1e9 || frame!=double(int(frame))) continue;
    QString text=noteValue.toString().simplified();
    if(text.isEmpty() || text.size()>maxNoteLength) continue;
    const int f=int(frame);
    if(std::any_of(notes.keys.cbegin(),notes.keys.cend(),[f](const Note& n) { return n.frame==f; })) continue;
    notes.keys.append(Note{f,text});
  }
  std::sort(notes.keys.begin(),notes.keys.end(),[](const Note& a,const Note& b) { return a.frame<b.frame; });

  const QJsonValue task=object.value(QLatin1String("task"));
  if(task.isString() && !notes.keys.isEmpty())
  {
    const QString text=task.toString().simplified();
    if(text.size()<=maxNoteLength) notes.task=text;
  }
  return notes;
}

QString NotesSpec::noteFor(const Notes& notes,int frame,int frames,bool playing)
{
  if(notes.keys.isEmpty()) return QString();
  // stopped at the last frame, past the last note's key: the small task
  if(!playing && !notes.task.isEmpty() && frames>0 && frame==frames-1 && frame>=notes.keys.last().frame)
    return notes.task;
  QString note=notes.keys.first().text;
  for(const Note& n : notes.keys)
  {
    if(n.frame>frame) break;
    note=n.text;
  }
  return note;
}

bool NotesSpec::shows(const Notes& notes,bool animationOpen,bool hintsOn,bool hiddenByButton)
{
  return animationOpen && !notes.isEmpty() && hintsOn && !hiddenByButton;
}

NotesLine::NotesLine(QWidget* parent) : QFrame(parent)
{
  setObjectName(QStringLiteral("timelineNotes"));
  // the mockup's .note: padding s and m, gap s
  QHBoxLayout* layout=new QHBoxLayout(this);
  layout->setContentsMargins(Theme::Size::spacingM,Theme::Size::spacingS,Theme::Size::spacingM,Theme::Size::spacingS);
  layout->setSpacing(Theme::Size::spacingS);

  m_icon=new QLabel(this);
  m_icon->setObjectName(QStringLiteral("timelineNotesIcon"));
  m_icon->setFixedSize(Theme::Size::iconSmall,Theme::Size::iconSmall);
  layout->addWidget(m_icon,0,Qt::AlignVCenter);

  m_text=new QLabel(this);
  m_text->setObjectName(QStringLiteral("timelineNotesText"));
  m_text->setWordWrap(true);
  m_text->setTextFormat(Qt::PlainText);
  layout->addWidget(m_text,1);

  // the button in its own row widget, so it gets the program's button look
  QWidget* buttonRow=new QWidget(this);
  QHBoxLayout* buttonLayout=new QHBoxLayout(buttonRow);
  buttonLayout->setContentsMargins(0,0,0,0);
  m_hide=new QPushButton(NotesSpec::text(NotesSpec::hideButton),buttonRow);
  m_hide->setObjectName(QStringLiteral("hideNotesButton"));
  m_hide->setToolTip(NotesSpec::text(NotesSpec::hideTip));
  m_hide->setAccessibleName(NotesSpec::text(NotesSpec::hideTip));
  buttonLayout->addWidget(m_hide);
  TransportLook::apply(buttonRow);
  layout->addWidget(buttonRow,0,Qt::AlignVCenter);

  connect(m_hide,&QPushButton::clicked,this,[this]()
  {
    m_hiddenByButton=true;
    refresh();
    emit hidden();
  });

  updateIcon();
  refresh();
}

QColor NotesLine::background(bool dark)
{
  return Theme::mix(Theme::color(Token::Surface,dark),Theme::color(Token::AreaOther,dark),0.08);
}

QColor NotesLine::iconColour(bool dark)
{
  return Theme::color(Token::AreaOther,dark);
}

QColor NotesLine::topLine(bool dark)
{
  return Theme::color(Token::Border,dark);
}

void NotesLine::setNotes(const NotesSpec::Notes& notes)
{
  m_notes=notes;
  refresh();
}

void NotesLine::setFrame(int frame,int frames,bool playing)
{
  m_frame=frame;
  m_frames=frames;
  m_playing=playing;
  refresh();
}

void NotesLine::setPlaying(bool playing)
{
  m_playing=playing;
  refresh();
}

void NotesLine::setAnimationOpen(bool open)
{
  m_open=open;
  refresh();
}

void NotesLine::setHintsOn(bool on)
{
  if(on && !m_hintsOn) m_hiddenByButton=false;
  m_hintsOn=on;
  refresh();
}

void NotesLine::refresh()
{
  const bool show=NotesSpec::shows(m_notes,m_open,m_hintsOn,m_hiddenByButton);
  if(show)
  {
    const QString note=NotesSpec::noteFor(m_notes,m_frame,m_frames,m_playing);
    if(m_text->text()!=note) m_text->setText(note);
  }
  setVisible(show);
}

void NotesLine::updateIcon()
{
  const bool dark=Theme::isDark();
  const QIcon icon=LucideIcons::icon(QStringLiteral("book-open"),iconColour(dark),Theme::color(Token::TextDisabled,dark));
  m_icon->setPixmap(icon.pixmap(QSize(Theme::Size::iconSmall,Theme::Size::iconSmall),devicePixelRatioF()));
}

void NotesLine::paintEvent(QPaintEvent* event)
{
  const bool dark=Theme::isDark();
  QPainter painter(this);
  painter.fillRect(rect(),background(dark));
  painter.fillRect(QRect(0,0,width(),Theme::Size::borderWidth),topLine(dark));
  painter.end();
  QFrame::paintEvent(event);
}

void NotesLine::changeEvent(QEvent* event)
{
  // the theme was switched: the icon follows (the background is painted)
  if(event->type()==QEvent::PaletteChange || event->type()==QEvent::StyleChange)
  {
    updateIcon();
    update();
  }
  QFrame::changeEvent(event);
}
