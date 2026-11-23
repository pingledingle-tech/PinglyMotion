/*
 * PinglyMotion Script Lab - help for someone who has never scripted (job U3)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QLabel>
#include <QPushButton>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QToolButton>
#include <QVBoxLayout>

#include "labwatcher.h"
#include "lucideicons.h"
#include "theme.h"

#include "labbeginner.h"

namespace
{
  // docs/design_tokens.json: spacing and sizes
  const int SpacingXs = Theme::Size::spacingXs;
  const int SpacingS = Theme::Size::spacingS;
  const int SpacingM = Theme::Size::spacingM;
  const int IconSmall = Theme::Size::iconSmall;
  const qint64 MaxExampleFile = 64 * 1024;   // example.json is far smaller

  QVector3D vector(const QJsonValue& value)
  {
    const QJsonArray a = value.toArray();
    return QVector3D(float(a.at(0).toDouble()), float(a.at(1).toDouble()), float(a.at(2).toDouble()));
  }

  QFont semibold(QFont font)
  {
    font.setWeight(QFont::DemiBold);
    return font;
  }

  // the style of a small icon button that closes a card (normal button colours, square)
  QString closeStyle(bool dark)
  {
    return QStringLiteral("QToolButton { background: transparent; border: 1px solid transparent; border-radius: %1px; }"
                          "QToolButton:hover { background: %2; }"
                          "QToolButton:focus { border: 2px solid %3; }")
      .arg(Theme::Size::radiusControl)
      .arg(Theme::mix(Theme::color(Theme::Token::SurfaceRaised, dark), Theme::color(Theme::Token::Text, dark), 0.08).name(),
           Theme::color(Theme::Token::FocusRing, dark).name());
  }

  QToolButton* makeClose(QWidget* parent, const QString& tip)
  {
    QToolButton* close = new QToolButton(parent);
    close->setToolTip(tip);
    close->setAccessibleName(tip);
    close->setFixedSize(Theme::Size::controlCompact, Theme::Size::controlCompact);
    close->setIconSize(QSize(IconSmall, IconSmall));
    close->setFocusPolicy(Qt::StrongFocus);
    return close;
  }

  bool reservedWindowsName(const QString& name)
  {
    static const QRegularExpression reserved(QStringLiteral("^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])$"),
                                             QRegularExpression::CaseInsensitiveOption);
    return reserved.match(name.trimmed()).hasMatch();
  }
}

QStringList LabExample::texts(const QString& part) const
{
  QStringList out;
  for (const Entry& e : guide)
    if (e.part == part) out << e.text;
  return out;
}

QList<LabExample::Entry> LabExample::steps() const
{
  QList<Entry> out;
  for (const Entry& e : guide)
    if (e.part == QLatin1String("step") && out.size() < MaxSteps) out << e;
  return out;
}

LabExample LabBeginner::readExample(const QString& folder, QString* error)
{
  LabExample example;
  QFile file(QDir(folder).filePath(QStringLiteral("example.json")));
  if (QFileInfo(file.fileName()).size() > MaxExampleFile || !file.open(QIODevice::ReadOnly))
  {
    if (error) *error = QObject::tr("The example could not be read.");
    return example;
  }
  const QJsonObject root = QJsonDocument::fromJson(file.read(MaxExampleFile)).object();
  if (root.value("format").toInt() != 1)
  {
    if (error) *error = QObject::tr("The example could not be read.");
    return example;
  }
  example.folder = QDir(folder).absolutePath();
  example.object = root.value("object").toString();
  example.title = root.value("title").toString();
  example.order = root.value("order").toInt();
  example.worksNow = root.value("works_now").toBool();
  for (const QJsonValue& v : root.value("props").toArray())
  {
    const QJsonObject p = v.toObject();
    example.props << LabExample::Prop{p.value("name").toString(), p.value("shape").toString(), p.value("joint").toString(),
                                      vector(p.value("position")), vector(p.value("scale")), vector(p.value("rotation"))};
  }
  for (const QJsonValue& v : root.value("guide").toArray())
  {
    const QJsonObject g = v.toObject();
    example.guide << LabExample::Entry{g.value("id").toString(), g.value("part").toString(), g.value("text").toString(),
                                       g.value("control").toString()};
  }
  // the object's name is a folder name: refuse anything that could leave the folder
  if (!checkFolderName(example.object).isEmpty())
  {
    if (error) *error = QObject::tr("The example could not be read.");
    return LabExample();
  }
  return example;
}

LabExample LabBeginner::firstExample(const QString& objectsFolder, QString* error)
{
  LabExample best;
  const QDir dir(objectsFolder);
  for (const QString& name : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name))
  {
    const LabExample example = readExample(dir.filePath(name));
    if (example.isValid() && (!best.isValid() || example.order < best.order)) best = example;
  }
  if (!best.isValid() && error) *error = QObject::tr("The example objects are not next to the program.");
  return best;
}

QString LabBeginner::defaultScriptsRoot()
{
  QString documents = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
  if (documents.isEmpty()) documents = QDir::homePath();
  return QDir(documents).filePath(QStringLiteral("PinglyMotion/Scripts"));
}

QString LabBeginner::checkFolderName(const QString& name)
{
  if (name.trimmed().isEmpty()) return QObject::tr("Type a name for the folder first.");
  if (name.size() > MaxNameLength) return QObject::tr("The name is too long. Use at most %1 characters.").arg(MaxNameLength);
  static const QRegularExpression allowed(QStringLiteral("^[A-Za-z0-9_][A-Za-z0-9 _-]*$"));
  if (!allowed.match(name).hasMatch() || name.endsWith(QLatin1Char(' ')))
    return QObject::tr("Use only letters, digits, spaces, - and _ in the name.");
  if (reservedWindowsName(name)) return QObject::tr("Windows keeps this name for itself. Pick another name.");
  return QString();
}

QString LabBeginner::makeFolder(const QString& root, const QString& name, QString* error)
{
  const QString why = checkFolderName(name);
  if (!why.isEmpty())
  {
    if (error) *error = why;
    return QString();
  }
  const QString path = QDir::cleanPath(QDir(root).absolutePath() + QLatin1Char('/') + name);
  if (path.size() > MaxPathLength)
  {
    if (error) *error = QObject::tr("The folder path would be too long. Use a shorter name.");
    return QString();
  }
  if (QFileInfo::exists(path))
  {
    if (error) *error = QObject::tr("A folder with this name is already there. Pick another name.");
    return QString();
  }
  if (!QDir().mkpath(path))
  {
    if (error) *error = QObject::tr("Could not make the folder. Check that Documents can be written.");
    return QString();
  }
  return path;
}

QString LabBeginner::copyExample(const LabExample& example, const QString& root, QString* error)
{
  if (!example.isValid() || !checkFolderName(example.object).isEmpty())
  {
    if (error) *error = QObject::tr("The example could not be read.");
    return QString();
  }
  const QString dest = QDir::cleanPath(QDir(root).absolutePath() + QLatin1Char('/') + example.object);
  if (dest.size() > MaxPathLength)
  {
    if (error) *error = QObject::tr("The folder path would be too long. Use a shorter name.");
    return QString();
  }
  // made before: open it as it is, so the user's own changes stay
  if (QFileInfo(dest).isDir()) return dest;
  if (QFileInfo::exists(dest))
  {
    if (error) *error = QObject::tr("A folder with this name is already there. Pick another name.");
    return QString();
  }

  // what is copied: the scripts, notecards and example.json, at most two folders deep, no links
  const QDir source(example.folder);
  QStringList files;
  QDirIterator it(example.folder, QDir::Files | QDir::NoSymLinks | QDir::Hidden, QDirIterator::Subdirectories);
  while (it.hasNext())
  {
    const QString path = it.next();
    const QFileInfo info(path);
    if (info.isSymLink()) continue;
    const QString relative = source.relativeFilePath(path);
    const QString suffix = info.suffix().toLower();
    if (relative.count(QLatin1Char('/')) > 1 || relative.startsWith(QLatin1String(".."))) continue;
    if (suffix != QLatin1String("lsl") && suffix != QLatin1String("txt") && suffix != QLatin1String("notecard")
        && relative != QLatin1String("example.json")) continue;
    if (info.size() > LabWatcher::MAX_FILE_BYTES) continue;
    files << relative;
  }
  if (files.size() > MaxCopiedFiles)
  {
    if (error) *error = QObject::tr("The example could not be read.");
    return QString();
  }
  if (!QDir().mkpath(dest))
  {
    if (error) *error = QObject::tr("Could not make the folder. Check that Documents can be written.");
    return QString();
  }
  files.sort();
  for (const QString& relative : files)
  {
    const QString target = QDir(dest).filePath(relative);
    QDir().mkpath(QFileInfo(target).absolutePath());
    // QFile::copy never overwrites, and the folder is new
    if (!QFile::copy(source.filePath(relative), target))
    {
      if (error) *error = QObject::tr("Could not make the folder. Check that Documents can be written.");
      return QString();
    }
    QFile::setPermissions(target, QFile::permissions(target) | QFileDevice::WriteOwner);
  }
  return dest;
}

QList<LabBeginner::Tip> LabBeginner::jargonTips()
{
  // docs/TEACHING.md, LABNEW-30 to LABNEW-37, in the order they show (one at a time)
  return {
    {"prim", QObject::tr("Prim"), QObject::tr("One part of an object. Each folder is one prim.")},
    {"rootPrim", QObject::tr("Root prim"), QObject::tr("The main part of the object. It is the folder Root.")},
    {"notecard", QObject::tr("Notecard"), QObject::tr("A text file that a script can read. Here it is a .txt file.")},
    {"sandbox", QObject::tr("Sandbox"), QObject::tr("A safe place to test. Nothing here reaches Second Life.")},
    {"viewerRules", QObject::tr("Viewer rules"), QObject::tr("Which kind of RLV the test viewer follows.")},
    {"rlv", QObject::tr("RLV"), QObject::tr("Rules a script sends to your viewer, like no detaching.")},
    {"rlva", QObject::tr("RLVa"), QObject::tr("A newer kind of RLV, used in Firestorm and other viewers.")},
    {"held", QObject::tr("N held"), QObject::tr("How many RLV rules hold the avatar right now.")},
  };
}

LabBeginner::Tip LabBeginner::jargonTip(const QString& key)
{
  for (const Tip& tip : jargonTips())
    if (QLatin1String(tip.key) == key) return tip;
  return {"", QString(), QString()};
}

// --- the step card -------------------------------------------------------

LabStepCard::LabStepCard(QWidget* parent) : ScriptLabFrame(parent)
{
  setObjectName("scriptLabStepCard");
  setFocusPolicy(Qt::NoFocus);
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(SpacingM, SpacingM, SpacingM, SpacingM);
  layout->setSpacing(SpacingS);

  QHBoxLayout* top = new QHBoxLayout;
  top->setSpacing(SpacingS);
  m_title = new QLabel(this);
  m_title->setObjectName("scriptLabStepTitle");
  QFont title = semibold(m_title->font());   // the "title" type token: 16 px next to the 13 px body
  if (title.pointSizeF() > 0) title.setPointSizeF(title.pointSizeF() * 16.0 / 13.0);
  m_title->setFont(title);
  m_title->setWordWrap(true);
  top->addWidget(m_title, 1);
  m_count = new QLabel(this);
  m_count->setObjectName("scriptLabStepCount");
  top->addWidget(m_count);
  m_close = makeClose(this, tr("Close the steps. Help, Script Lab steps shows them again."));
  m_close->setObjectName("scriptLabStepClose");
  top->addWidget(m_close, 0, Qt::AlignTop);
  layout->addLayout(top);

  // what you should see comes first: the result before the explanation
  m_seeHeading = new QLabel(tr("You should see"), this);
  m_seeHeading->setObjectName("scriptLabStepSeeHeading");
  m_seeHeading->setFont(semibold(m_seeHeading->font()));
  layout->addWidget(m_seeHeading);
  m_see = new QLabel(this);
  m_see->setObjectName("scriptLabStepSee");
  m_see->setWordWrap(true);
  m_see->setTextFormat(Qt::PlainText);
  layout->addWidget(m_see);

  m_stepsLayout = new QVBoxLayout;
  m_stepsLayout->setSpacing(SpacingS);
  layout->addLayout(m_stepsLayout);

  m_nextHeading = new QLabel(tr("Then try"), this);
  m_nextHeading->setObjectName("scriptLabStepNextHeading");
  m_nextHeading->setFont(semibold(m_nextHeading->font()));
  layout->addWidget(m_nextHeading);
  m_next = new QLabel(this);
  m_next->setObjectName("scriptLabStepNext");
  m_next->setWordWrap(true);
  m_next->setTextFormat(Qt::PlainText);
  layout->addWidget(m_next);

  connect(m_close, &QToolButton::clicked, this, &LabStepCard::closeRequested);
}

void LabStepCard::setExample(const QString& title, const QStringList& see, const QStringList& next)
{
  m_title->setText(tr("Steps: %1").arg(title));
  m_see->setText(see.join(QLatin1Char(' ')));
  m_see->setVisible(!see.isEmpty());
  m_seeHeading->setVisible(!see.isEmpty());
  m_next->setText(next.join(QLatin1Char(' ')));
  m_next->setVisible(!next.isEmpty());
  m_nextHeading->setVisible(!next.isEmpty());
}

int LabStepCard::doneCount() const
{
  int done = 0;
  for (const Step& s : m_steps)
    if (s.done) ++done;
  return done;
}

void LabStepCard::setSteps(const QList<Step>& steps)
{
  // the rows are made again only when the number of steps changes
  if (steps.size() != m_steps.size())
  {
    while (QLayoutItem* item = m_stepsLayout->takeAt(0))
    {
      if (QLayout* row = item->layout())
        while (QLayoutItem* inner = row->takeAt(0))
        {
          if (QLayout* column = inner->layout())
            while (QLayoutItem* c = column->takeAt(0)) { delete c->widget(); delete c; }
          delete inner->widget();
          delete inner;
        }
      delete item;
    }
    m_ticks.clear();
    m_texts.clear();
    m_reasons.clear();
    m_buttons.clear();
    for (int i = 0; i < steps.size(); ++i)
    {
      QHBoxLayout* row = new QHBoxLayout;
      row->setSpacing(SpacingS);
      QLabel* tick = new QLabel(this);
      tick->setObjectName("scriptLabStepTick");
      tick->setFixedSize(IconSmall, IconSmall);
      row->addWidget(tick, 0, Qt::AlignTop);
      QVBoxLayout* column = new QVBoxLayout;
      column->setSpacing(SpacingXs);
      QLabel* text = new QLabel(this);
      text->setObjectName("scriptLabStepText");
      text->setWordWrap(true);
      text->setTextFormat(Qt::PlainText);
      column->addWidget(text);
      QLabel* reason = new QLabel(this);
      reason->setObjectName("scriptLabStepReason");
      reason->setWordWrap(true);
      reason->setTextFormat(Qt::PlainText);
      QFont caption = reason->font();   // the "caption" type token
      if (caption.pointSizeF() > 0) caption.setPointSizeF(caption.pointSizeF() * 11.0 / 13.0);
      reason->setFont(caption);
      column->addWidget(reason);
      row->addLayout(column, 1);
      QPushButton* button = new QPushButton(this);
      button->setObjectName("scriptLabStepButton");
      button->setMinimumHeight(Theme::Size::controlCompact);
      row->addWidget(button, 0, Qt::AlignTop);
      m_stepsLayout->addLayout(row);
      connect(button, &QPushButton::clicked, this, [this, i]() { emit stepRequested(i); });
      m_ticks << tick;
      m_texts << text;
      m_reasons << reason;
      m_buttons << button;
    }
  }
  m_steps = steps;
  for (int i = 0; i < steps.size(); ++i)
  {
    const Step& s = steps.at(i);
    m_texts[i]->setText(tr("%1. %2").arg(i + 1).arg(s.text));
    m_buttons[i]->setText(s.button);
    m_buttons[i]->setEnabled(s.enabled);
    m_buttons[i]->setToolTip(s.enabled ? s.text : s.reason);
    m_buttons[i]->setStatusTip(s.enabled ? QString() : s.reason);
    m_reasons[i]->setText(s.enabled ? QString() : s.reason);
    m_reasons[i]->setVisible(!s.enabled && !s.reason.isEmpty());
    m_ticks[i]->setToolTip(s.done ? tr("Done") : tr("Not done"));
    m_ticks[i]->setAccessibleName(s.done ? tr("Done") : tr("Not done"));
  }
  m_count->setText(tr("%1 of %2 done").arg(doneCount()).arg(steps.size()));
  applyStyle(m_dark);
}

void LabStepCard::restyleTicks()
{
  const QColor success = Theme::color(Theme::Token::Success, m_dark);
  const QColor muted = Theme::color(Theme::Token::TextMuted, m_dark);
  for (int i = 0; i < m_ticks.size() && i < m_steps.size(); ++i)
  {
    // done: a circle with a check in the success colour; not done: an empty square (shape, not only colour)
    const bool done = m_steps.at(i).done;
    m_ticks[i]->setPixmap(LucideIcons::icon(done ? QStringLiteral("circle-check") : QStringLiteral("square"),
                                            done ? success : muted, muted).pixmap(IconSmall, IconSmall));
    m_ticks[i]->setProperty("done", done);
  }
}

void LabStepCard::applyStyle(bool dark)
{
  m_dark = dark;
  // a first-run card: radius.panel, surface_raised fill and the border token (DESIGN_SYSTEM.md, "Guidance")
  setLook(Theme::color(Theme::Token::Border, dark), Theme::color(Theme::Token::SurfaceRaised, dark), Theme::Size::radiusPanel, false);
  const QColor text = Theme::color(Theme::Token::Text, dark);
  const QColor muted = Theme::color(Theme::Token::TextMuted, dark);
  for (QLabel* label : QList<QLabel*>{m_title, m_seeHeading, m_see, m_nextHeading, m_next})
  {
    QPalette p = label->palette();
    p.setColor(QPalette::WindowText, text);
    label->setPalette(p);
  }
  QPalette mutedPalette = m_count->palette();
  mutedPalette.setColor(QPalette::WindowText, muted);
  m_count->setPalette(mutedPalette);
  for (QLabel* reason : m_reasons) reason->setPalette(mutedPalette);
  for (QLabel* label : m_texts)
  {
    QPalette p = label->palette();
    p.setColor(QPalette::WindowText, text);
    label->setPalette(p);
  }
  // the step buttons are normal buttons; greyed ones keep text_muted, so the word stays readable (4.5:1)
  const QString normal = QStringLiteral(
    "QPushButton { background: %1; color: %2; border: 1px solid %3; border-radius: %4px; padding: 0 %5px; }"
    "QPushButton:disabled { color: %6; border: 1px solid %7; }"
    "QPushButton:focus { border: 2px solid %8; }")
    .arg(Theme::color(Theme::Token::Surface, dark).name(), text.name(), Theme::color(Theme::Token::Border, dark).name())
    .arg(Theme::Size::radiusControl).arg(SpacingM)
    .arg(muted.name(), Theme::mix(Theme::color(Theme::Token::Surface, dark), Theme::color(Theme::Token::Border, dark), 0.5).name(),
         Theme::color(Theme::Token::FocusRing, dark).name());
  for (QPushButton* b : m_buttons) b->setStyleSheet(normal);
  m_close->setStyleSheet(closeStyle(dark));
  m_close->setIcon(LucideIcons::icon(QStringLiteral("x"), text, muted));
  restyleTicks();
}

void LabStepCard::keyPressEvent(QKeyEvent* event)
{
  if (event->key() == Qt::Key_Escape)
  {
    emit closeRequested();
    return;
  }
  ScriptLabFrame::keyPressEvent(event);
}

// --- one word explained ----------------------------------------------------

LabTipCard::LabTipCard(const LabBeginner::Tip& tip, QWidget* parent) : ScriptLabFrame(parent), m_key(QLatin1String(tip.key))
{
  setObjectName(QStringLiteral("scriptLabTip_%1").arg(m_key));
  QHBoxLayout* row = new QHBoxLayout(this);
  row->setContentsMargins(SpacingS, SpacingS, SpacingS, SpacingS);
  row->setSpacing(SpacingS);
  QVBoxLayout* column = new QVBoxLayout;
  column->setSpacing(SpacingXs);
  m_word = new QLabel(tip.word, this);
  m_word->setObjectName("scriptLabTipWord");
  m_word->setFont(semibold(m_word->font()));
  column->addWidget(m_word);
  m_text = new QLabel(tip.text, this);
  m_text->setObjectName("scriptLabTipText");
  m_text->setWordWrap(true);
  m_text->setTextFormat(Qt::PlainText);
  column->addWidget(m_text);
  row->addLayout(column, 1);
  m_close = makeClose(this, tr("Close this tip. It does not come back."));
  m_close->setObjectName("scriptLabTipClose");
  row->addWidget(m_close, 0, Qt::AlignTop);
  setAccessibleName(tip.word + QLatin1String(": ") + tip.text);
  connect(m_close, &QToolButton::clicked, this, [this]() { emit closed(m_key); });
}

void LabTipCard::applyStyle(bool dark)
{
  setLook(Theme::color(Theme::Token::Border, dark), Theme::color(Theme::Token::SurfaceRaised, dark), Theme::Size::radiusPanel, false);
  const QColor text = Theme::color(Theme::Token::Text, dark);
  for (QLabel* label : {m_word, m_text})
  {
    QPalette p = label->palette();
    p.setColor(QPalette::WindowText, text);
    label->setPalette(p);
  }
  m_close->setStyleSheet(closeStyle(dark));
  m_close->setIcon(LucideIcons::icon(QStringLiteral("x"), text, Theme::color(Theme::Token::TextMuted, dark)));
}

void LabTipCard::keyPressEvent(QKeyEvent* event)
{
  if (event->key() == Qt::Key_Escape)
  {
    emit closed(m_key);
    return;
  }
  ScriptLabFrame::keyPressEvent(event);
}
