/*
 * PinglyMotion - the guide page of the Help view (see labguideview.h)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#include "labguideview.h"

#include <algorithm>

#include <QDir>
#include <QDirIterator>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QSet>
#include <QTextDocument>
#include <QVBoxLayout>

#include "dialogfit.h"
#include "lucideicons.h"
#include "theme.h"

namespace
{
  QString esc(const QString& text)
  {
    return text.toHtmlEscaped();
  }

  QString paragraphs(const QStringList& lines)
  {
    QString out;
    for (const QString& line : lines) out += QStringLiteral("<p>%1</p>").arg(esc(line));
    return out;
  }

  QString numbered(const QStringList& lines)
  {
    QString out = QStringLiteral("<ol>");
    for (const QString& line : lines) out += QStringLiteral("<li>%1</li>").arg(esc(line));
    return out + QStringLiteral("</ol>");
  }

  // the files of the example folder that the copy holds: the same rule as LabBeginner::copyExample
  QStringList copiedFiles(const QString& folder)
  {
    QStringList files;
    const QDir source(folder);
    QDirIterator it(folder, QDir::Files | QDir::NoSymLinks | QDir::Hidden, QDirIterator::Subdirectories);
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
      files << relative;
    }
    files.sort();
    return files;
  }

  QString labelForFile(const QString& relative)
  {
    const QString name = QFileInfo(relative).fileName();
    const QString suffix = QFileInfo(relative).suffix().toLower();
    if (name == QLatin1String("example.json"))
      return QObject::tr("The guide and the props of this example. The program reads it. You do not change it.");
    if (name == QLatin1String("colour.lsl")) return QObject::tr("The script. It changes the colour of its prim. Open it and change a number.");
    if (suffix == QLatin1String("lsl")) return QObject::tr("A script. It runs on its prim.");
    return QObject::tr("A notecard. A text file that a script can read.");
  }

  QString labelForFolder(int depth)
  {
    if (depth == 0) return QObject::tr("The object. One folder is one object. Script Lab watches this folder.");
    return QObject::tr("A prim, one part of the object. It has a prop of the same name in 3D view.");
  }

  // the samples next to the program, found by their files
  QStringList sampleAnimations(const QString& examplesRoot)
  {
    QStringList out;
    for (const QString& name : QDir(examplesRoot).entryList({QStringLiteral("*.dingle")}, QDir::Files, QDir::Name))
      out << name;
    return out;
  }
}

QList<LabGuide::FolderRow> LabGuide::folderRows(const LabExample& example)
{
  QList<FolderRow> rows;
  FolderRow top;
  top.path = QString();
  top.depth = 0;
  top.isFolder = true;
  top.label = labelForFolder(0);
  rows << top;
  QSet<QString> seen;
  for (const QString& relative : copiedFiles(example.folder))
  {
    const QStringList parts = relative.split(QLatin1Char('/'));
    if (parts.size() == 2 && !seen.contains(parts.first()))
    {
      seen.insert(parts.first());
      FolderRow folder;
      folder.path = parts.first();
      folder.depth = 1;
      folder.isFolder = true;
      folder.label = labelForFolder(1);
      rows << folder;
    }
    FolderRow file;
    file.path = relative;
    file.depth = parts.size();
    file.isFolder = false;
    file.label = labelForFile(relative);
    rows << file;
  }
  // folders first inside each level, so the picture reads like a file manager
  QList<FolderRow> ordered;
  ordered << rows.first();
  for (int i = 1; i < rows.size(); ++i)
    if (rows.at(i).depth == 1 && rows.at(i).isFolder)
    {
      ordered << rows.at(i);
      for (int j = 1; j < rows.size(); ++j)
        if (rows.at(j).depth == 2 && rows.at(j).path.startsWith(rows.at(i).path + QLatin1Char('/'))) ordered << rows.at(j);
    }
  for (int i = 1; i < rows.size(); ++i)
    if (rows.at(i).depth == 1 && !rows.at(i).isFolder) ordered << rows.at(i);
  return ordered;
}

QString LabGuide::freeName(const QString& root, const QString& object)
{
  const QDir dir(root);
  if (!QFileInfo::exists(dir.filePath(object))) return object;
  for (int n = 2; n < 100; ++n)
  {
    const QString name = QStringLiteral("%1 %2").arg(object).arg(n);
    if (!QFileInfo::exists(dir.filePath(name))) return name;
  }
  return QString();
}

QString LabGuide::makeExampleFolder(const LabExample& example, const QString& root, QString* error)
{
  if (!example.isValid())
  {
    if (error) *error = QObject::tr("The example is not next to the program.");
    return QString();
  }
  const QString name = freeName(root, example.object);
  if (name.isEmpty())
  {
    if (error) *error = QObject::tr("Too many folders with this name are there. Pick another place.");
    return QString();
  }
  LabExample copy = example;
  copy.object = name;   // copyExample names the new folder after the object
  return LabBeginner::copyExample(copy, root, error);
}

LabGuideView::LabGuideView(const QString& examplesFolder, QWidget* parent) :
  QDialog(parent), m_examplesFolder(examplesFolder)
{
  setObjectName(QStringLiteral("scriptLabGuidePage"));
  setWindowTitle(tr("Guide page"));
  setModal(false);
  setAttribute(Qt::WA_DeleteOnClose, false);
  m_example = LabBeginner::firstExample(examplesFolder);
  m_rows = m_example.isValid() ? LabGuide::folderRows(m_example) : QList<LabGuide::FolderRow>();
  m_samples = sampleAnimations(QDir::cleanPath(examplesFolder + QStringLiteral("/..")));
  m_chooser = [this]() {
    return QFileDialog::getExistingDirectory(this, tr("Choose where to make the example folder"), LabBeginner::defaultScriptsRoot());
  };
  build();
  applyStyle(Theme::color(Theme::Token::Background).lightness() < 128);
}

QLabel* LabGuideView::addText(QVBoxLayout* into, const QString& html, const QString& name)
{
  QLabel* label = new QLabel(html, this);
  label->setTextFormat(Qt::RichText);
  label->setWordWrap(true);
  label->setTextInteractionFlags(Qt::TextSelectableByMouse);
  if (!name.isEmpty()) label->setObjectName(name);
  into->addWidget(label);
  return label;
}

QLabel* LabGuideView::addHeading(QVBoxLayout* into, const QString& text, const QString& name)
{
  QLabel* label = new QLabel(text, this);
  label->setWordWrap(true);
  label->setObjectName(name.isEmpty() ? QStringLiteral("scriptLabGuideHeading") : name);
  QFont f = font();
  f.setWeight(QFont::DemiBold);
  if (f.pointSizeF() > 0) f.setPointSizeF(f.pointSizeF() * 16.0 / 13.0);
  label->setFont(f);
  into->addWidget(label);
  return label;
}

void LabGuideView::build()
{
  QVBoxLayout* layout = DialogFit::contentLayout(this);
  layout->setSpacing(Theme::Size::spacingM);
  m_scroll = findChild<QScrollArea*>();

  QLabel* title = addHeading(layout, tr("Guide page"), QStringLiteral("scriptLabGuideTitle"));
  QFont big = title->font();
  if (big.pointSizeF() > 0) big.setPointSizeF(big.pointSizeF() * 20.0 / 16.0);
  title->setFont(big);

  addHeading(layout, tr("Start here"));
  addText(layout, paragraphs({tr("This page is for you if you are new to animation and to scripts."),
                              tr("Animation: you pose a figure at a few moments. The program fills in the rest."),
                              tr("Script Lab: you test a Second Life script on your PC. Nothing reaches Second Life.")}));

  addHeading(layout, tr("Animate in five steps"));
  addText(layout, numbered({tr("Choose File > New. A figure appears."),
                            tr("Drag the time marker to the middle of the timeline."),
                            tr("Click an arm. Drag a coloured ring to turn it. A key is saved at once."),
                            tr("Drag the time marker to a later frame. Turn the arm again."),
                            tr("Press Start. The arm moves between your two keys.")}));
  if (!m_samples.isEmpty())
  {
    QStringList names;
    for (const QString& name : m_samples) names << QFileInfo(name).completeBaseName();
    addText(layout, paragraphs({tr("Samples to look at: %1.").arg(names.join(QStringLiteral(", "))),
                                tr("Choose File > Open. The dialog starts in the examples folder next to the program.")}),
            QStringLiteral("scriptLabGuideSamples"));
  }

  addHeading(layout, tr("Script Lab in five steps"));
  addText(layout, numbered({tr("Make the example folder below, or press Make example project in Script Lab."),
                            tr("Open the folder in Script Lab. It watches the folder."),
                            tr("Press Show effects in 3D view, so it shows a tick."),
                            tr("Click a prop in the 3D view. Its script runs."),
                            tr("Change a number in the script and save. Look at the 3D view.")}));

  addHeading(layout, tr("The example folder"));
  if (m_rows.isEmpty())
  {
    addText(layout, paragraphs({tr("The example is not next to the program.")}), QStringLiteral("scriptLabGuideNoExample"));
  }
  else
  {
    addText(layout, paragraphs({tr("This is the folder the button makes. Every file has a label.")}));
    m_picture = new QFrame(this);
    m_picture->setObjectName(QStringLiteral("scriptLabGuidePicture"));
    QVBoxLayout* pic = new QVBoxLayout(m_picture);
    pic->setSpacing(Theme::Size::spacingS);
    for (const LabGuide::FolderRow& row : m_rows)
    {
      QWidget* line = new QWidget(m_picture);
      line->setObjectName(QStringLiteral("scriptLabGuideRow"));
      QHBoxLayout* lineLayout = new QHBoxLayout(line);
      lineLayout->setContentsMargins(row.depth * Theme::Size::spacingXl, 0, 0, 0);
      lineLayout->setSpacing(Theme::Size::spacingS);
      QLabel* icon = new QLabel(line);
      icon->setObjectName(QStringLiteral("scriptLabGuideRowIcon"));
      icon->setFixedSize(Theme::Size::iconGrid, Theme::Size::iconGrid);
      icon->setProperty("iconName", row.isFolder ? QStringLiteral("folder-open") : QStringLiteral("file-code"));
      lineLayout->addWidget(icon, 0, Qt::AlignTop);
      QVBoxLayout* words = new QVBoxLayout;
      words->setSpacing(0);
      lineLayout->addLayout(words, 1);
      QLabel* name = new QLabel(line);
      name->setObjectName(QStringLiteral("scriptLabGuideRowName"));
      name->setWordWrap(true);
      const QString shown = row.path.isEmpty() ? m_example.object : QFileInfo(row.path).fileName();
      // the kind in words as well as the icon, so the picture does not rely on the icon alone
      name->setText(row.isFolder ? tr("Folder %1").arg(shown) : (shown.endsWith(QLatin1String(".json")) ? tr("Data file %1").arg(shown)
                                                                                                         : tr("File %1").arg(shown)));
      name->setProperty("shownName", shown);
      name->setProperty("folder", row.isFolder);
      name->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
      QLabel* what = new QLabel(row.label, line);
      what->setObjectName(QStringLiteral("scriptLabGuideRowLabel"));
      what->setWordWrap(true);
      words->addWidget(name);
      words->addWidget(what);
      pic->addWidget(line);
      m_pictureLines << line;
    }
    layout->addWidget(m_picture);

    m_make = new QPushButton(tr("Make the example folder"), this);
    m_make->setObjectName(QStringLiteral("scriptLabGuideMake"));
    m_make->setMinimumHeight(Theme::Size::controlLarge);
    m_make->setToolTip(tr("Choose a place. The program makes the folder there. It never replaces a folder."));
    connect(m_make, &QPushButton::clicked, this, [this]() { chooseAndMake(); });
    layout->addWidget(m_make, 0, Qt::AlignLeft);
    m_status = new QLabel(this);
    m_status->setObjectName(QStringLiteral("scriptLabGuideStatus"));
    m_status->setWordWrap(true);
    m_status->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_status->setAccessibleName(tr("Result of making the folder"));
    layout->addWidget(m_status);
    m_open = new QPushButton(tr("Open it in Script Lab"), this);
    m_open->setObjectName(QStringLiteral("scriptLabGuideOpen"));
    m_open->setMinimumHeight(Theme::Size::controlDefault);
    m_open->setEnabled(false);
    m_open->setToolTip(tr("Make the folder first."));
    connect(m_open, &QPushButton::clicked, this, [this]() {
      if (!m_made.isEmpty()) emit openRequested(m_made);
    });
    layout->addWidget(m_open, 0, Qt::AlignLeft);
  }

  addHeading(layout, tr("More help"));
  addText(layout, paragraphs({tr("The Help view has the words, the hints and a guide for every example."),
                              tr("Press Esc to close this page.")}));
  layout->addStretch(1);

  QHBoxLayout* buttons = new QHBoxLayout;
  buttons->addStretch(1);
  m_close = new QPushButton(tr("Close"), this);
  m_close->setObjectName(QStringLiteral("scriptLabGuideClose"));
  m_close->setMinimumHeight(Theme::Size::controlDefault);
  connect(m_close, &QPushButton::clicked, this, &QDialog::close);
  buttons->addWidget(m_close);
  DialogFit::finish(this, buttons, QSize(640, 560));
  if (m_scroll) m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  if (m_make) setTabOrder(m_make, m_open);
}

void LabGuideView::chooseAndMake()
{
  const QString root = m_chooser ? m_chooser() : QString();
  if (root.isEmpty()) return;   // the user closed the dialog: nothing happens
  makeIn(root);
}

bool LabGuideView::makeIn(const QString& root)
{
  QString error;
  const QString made = LabGuide::makeExampleFolder(m_example, root, &error);
  if (made.isEmpty())
  {
    m_status->setText(tr("Error: %1").arg(error));
    return false;
  }
  m_made = made;
  m_status->setText(tr("Made the folder %1. Nothing was replaced.").arg(QDir::toNativeSeparators(made)));
  m_open->setEnabled(true);
  m_open->setToolTip(tr("Watches the folder in Script Lab."));
  return true;
}

QStringList LabGuideView::pictureNames() const
{
  QStringList out;
  for (QWidget* line : m_pictureLines)
    if (QLabel* name = line->findChild<QLabel*>(QStringLiteral("scriptLabGuideRowName"))) out << name->property("shownName").toString();
  return out;
}

QStringList LabGuideView::pictureLabels() const
{
  QStringList out;
  for (QWidget* line : m_pictureLines)
    if (QLabel* what = line->findChild<QLabel*>(QStringLiteral("scriptLabGuideRowLabel"))) out << what->text();
  return out;
}

QString LabGuideView::pageText() const
{
  QString out;
  for (QLabel* label : findChildren<QLabel*>())
  {
    QString text = label->text();
    if (label->textFormat() == Qt::RichText || text.contains(QLatin1Char('<')))
    {
      QTextDocument doc;
      doc.setHtml(text);
      text = doc.toPlainText();
    }
    out += text + QLatin1Char('\n');
  }
  return out;
}

void LabGuideView::applyStyle(bool dark)
{
  if (m_styling) return;
  m_styling = true;
  m_dark = dark;
  const QColor raised = Theme::color(Theme::Token::SurfaceRaised, dark);
  const QColor border = Theme::color(Theme::Token::Border, dark);
  const QColor muted = Theme::color(Theme::Token::TextMuted, dark);
  const QColor text = Theme::color(Theme::Token::Text, dark);
  if (m_picture)
    m_picture->setStyleSheet(QStringLiteral("QFrame#scriptLabGuidePicture { background: %1; border: 1px solid %2; border-radius: %3px; }"
                                            "QLabel#scriptLabGuideRowLabel { color: %4; }"
                                            "QLabel#scriptLabGuideRowName { color: %5; font-weight: 600; }")
                               .arg(raised.name(), border.name()).arg(Theme::Size::radiusPanel).arg(text.name(), text.name()));
  const QColor iconColour = Theme::color(Theme::Token::AccentText, dark);
  for (QLabel* icon : findChildren<QLabel*>(QStringLiteral("scriptLabGuideRowIcon")))
    icon->setPixmap(LucideIcons::icon(icon->property("iconName").toString(), iconColour, muted).pixmap(Theme::Size::iconSmall, Theme::Size::iconSmall));
  m_styling = false;
}

void LabGuideView::changeEvent(QEvent* event)
{
  QDialog::changeEvent(event);
  if (event->type() == QEvent::PaletteChange) applyStyle(Theme::color(Theme::Token::Background).lightness() < 128);
}
