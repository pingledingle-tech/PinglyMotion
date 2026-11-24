/*
 * PinglyMotion - two figures in one scene, Expert mode only (docs/UX_DECISIONS.md 6m)
 *
 * See figures.h.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QAbstractButton>
#include <QButtonGroup>
#include <QEvent>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHelpEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QRadioButton>
#include <QSizePolicy>
#include <QToolButton>
#include <QToolTip>
#include <QVBoxLayout>

#include "animation.h"
#include "bvhnode.h"
#include "constants.h"
#include "dialogfit.h"
#include "lucideicons.h"
#include "menuspec.h"
#include "textsize.h"
#include "theme.h"
#include "timelinelook.h"

#include "figures.h"

using Token = Theme::Token;
namespace Size = Theme::Size;

// ---------------------------------------------------------------- texts

QString FigureSpec::tr(const char* source)
{
  return QCoreApplication::translate(context, source);
}

QString FigureSpec::shownName(const QString& path)
{
  return QFileInfo(path).fileName();
}

QString FigureSpec::addBlocked(bool animationOpen, int figures)
{
  if (!animationOpen) return QCoreApplication::translate("MenuSpec", MenuSpec::openFirst);
  if (figures >= maxFigures) return tr(sceneFull);
  return QString();
}

QString FigureSpec::addedMessage(int number, const QString& path)
{
  return tr(added).arg(number).arg(shownName(path));
}

QString FigureSpec::activeMessage(int number, const QString& path)
{
  return tr(madeActive).arg(number).arg(shownName(path));
}

QString FigureSpec::removedMessage(int number, const QString& otherPath)
{
  QString text = tr(removed).arg(number);
  if (!otherPath.isEmpty()) text += QLatin1Char(' ') + tr(renumbered).arg(shownName(otherPath));
  return text;
}

QString FigureSpec::titleText(int number, int figures)
{
  if (figures < 2) return QString();
  return tr(titleSuffix).arg(number).arg(figures);
}

QString FigureSpec::chipText(int number, int figures)
{
  if (figures < 2) return QString();
  return tr(chip).arg(number);
}

QString FigureSpec::tagText(int number, const QString& path)
{
  return QStringLiteral("%1 · %2").arg(number).arg(shownName(path));
}

int FigureSpec::freePlace(const QList<int>& used)
{
  int place = 0;
  while (used.contains(place)) ++place;
  return place;
}

float FigureSpec::placeOffset(int place)
{
  return float(place) * spacingUnits;
}

// ---------------------------------------------------------------- shared looks

namespace
{
  QString hex(Token token) { return Theme::color(token).name(); }

  // the three button kinds of docs/DESIGN_SYSTEM.md, compact (24 px, the timeline's height)
  QString buttonStyle(bool danger, bool main)
  {
    const QString focus = hex(Token::FocusRing);
    if (danger || main)
      return QStringLiteral(
        "QPushButton { background: %1; color: %2; border: 1px solid %1; border-radius: %3px; padding: 0 %4px; }"
        "QPushButton:disabled { background: %5; color: %6; border: 1px solid %7; }"
        "QPushButton:focus { border: 2px solid %8; }")
        .arg(hex(danger ? Token::DangerFill : Token::Accent), hex(Token::OnAccent)).arg(Size::radiusControl).arg(Size::spacingS)
        .arg(hex(Token::SurfaceRaised), hex(Token::TextDisabled), hex(Token::Border), focus);
    return QStringLiteral(
      "QPushButton { background: %1; color: %2; border: 1px solid %3; border-radius: %4px; padding: 0 %5px; }"
      "QPushButton:disabled { color: %6; }"
      "QPushButton:focus { border: 2px solid %7; }")
      .arg(hex(Token::SurfaceRaised), hex(Token::Text), hex(Token::Border)).arg(Size::radiusControl).arg(Size::spacingS)
      .arg(hex(Token::TextDisabled), focus);
  }

  QString chipStyle()
  {
    return QStringLiteral("QLabel { background: %1; color: %2; border-radius: %3px; padding: 0 %4px; font-weight: bold; }")
      .arg(hex(Token::Selection), hex(Token::OnAccent)).arg(Size::radiusSmall).arg(Size::spacingS);
  }

  QString numberStyle(bool active)
  {
    return QStringLiteral("QLabel { background: %1; color: %2; border: 1px solid %3; border-radius: %4px; padding: 0 %5px; font-weight: bold; }")
      .arg(hex(active ? Token::Selection : Token::SurfaceRaised), hex(active ? Token::OnAccent : Token::TextMuted), hex(Token::Border))
      .arg(Size::radiusSmall).arg(Size::spacingXs);
  }
}

// ---------------------------------------------------------------- Add a second figure window

AddFigureDialog::AddFigureDialog(QWidget* parent)
  : QDialog(parent)
{
  using namespace FigureSpec;
  setObjectName(QStringLiteral("addFigureDialog"));
  setWindowTitle(FigureSpec::tr(dialogTitle));
  // in a scroll area that fits the screen, with the buttons under it (src/dialogfit.h; job B4: the
  // window was cut off on a small screen at 200 percent text)
  QVBoxLayout* layout = DialogFit::contentLayout(this);
  layout->setContentsMargins(Size::spacingXl, Size::spacingXl, Size::spacingXl, Size::spacingXl);
  layout->setSpacing(Size::spacingM);

  QHBoxLayout* titleRow = new QHBoxLayout;
  QLabel* icon = new QLabel(this);
  icon->setPixmap(LucideIcons::icon(QStringLiteral("user-plus"), Theme::color(Token::Text), Theme::color(Token::TextDisabled))
                    .pixmap(Size::iconGrid, Size::iconGrid));
  m_title = new QLabel(FigureSpec::tr(dialogTitle), this);
  m_title->setObjectName(QStringLiteral("addFigureTitle"));
  QFont titleFont = m_title->font();
  titleFont.setWeight(QFont::DemiBold);
  titleFont.setPointSizeF(titleFont.pointSizeF() * 1.25);
  m_title->setFont(titleFont);
  titleRow->addWidget(icon);
  titleRow->addWidget(m_title, 1);
  layout->addLayout(titleRow);

  m_line = new QLabel(FigureSpec::tr(dialogLine), this);
  m_line->setObjectName(QStringLiteral("addFigureLine"));
  m_line->setWordWrap(true);
  layout->addWidget(m_line);

  // two choices: the name, then a line under it (the mockup's .choice buttons, drawn as the
  // welcome card's start choices are)
  const auto choiceButton = [this](const char* name, const char* line, bool main)
  {
    QToolButton* button = new QToolButton(this);
    button->setToolButtonStyle(Qt::ToolButtonTextOnly);
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    button->setText(FigureSpec::tr(name) + QLatin1Char('\n') + FigureSpec::tr(line));
    button->setMinimumHeight(TextSize::scaled(Size::controlLarge) + Size::spacingM);
    const QString fill = hex(main ? Token::Accent : Token::SurfaceRaised);
    const QString fg = hex(main ? Token::OnAccent : Token::Text);
    const QString edge = hex(main ? Token::Accent : Token::Border);
    button->setStyleSheet(QStringLiteral(
      "QToolButton { background: %1; color: %2; border: %5px solid %3; border-radius: %4px; padding: %8px %9px; }"
      "QToolButton:focus { border: %6px solid %7; }")
      .arg(fill, fg, edge).arg(Size::radiusControl).arg(Size::borderWidth).arg(Size::focusRingWidth)
      .arg(hex(Token::FocusRing)).arg(Size::spacingS).arg(Size::spacingL));
    return button;
  };
  m_library = choiceButton(fromLibrary, fromLibraryLine, true);
  m_library->setObjectName(QStringLiteral("addFigureFromLibrary"));
  m_file = choiceButton(fromFile, fromFileLine, false);
  m_file->setObjectName(QStringLiteral("addFigureFromFile"));
  layout->addWidget(m_library);
  layout->addWidget(m_file);

  QHBoxLayout* actions = new QHBoxLayout;
  actions->addStretch(1);
  m_cancel = new QPushButton(FigureSpec::tr(cancel), this);
  m_cancel->setObjectName(QStringLiteral("addFigureCancel"));
  m_cancel->setMinimumHeight(TextSize::scaled(Size::controlDefault));
  m_cancel->setAutoDefault(false);
  m_cancel->setStyleSheet(buttonStyle(false, false));
  actions->addWidget(m_cancel);
  DialogFit::finish(this, actions);

  // the scroll area's body is the same colour as the dialog, so the window looks as before
  setStyleSheet(QStringLiteral("QDialog#addFigureDialog, QDialog#addFigureDialog QWidget#dialogFitBody { background: %1; } QLabel { color: %2; }")
                  .arg(hex(Token::Surface), hex(Token::Text)));

  connect(m_library, &QToolButton::clicked, this, [this]() { m_choice = Choice::Library; accept(); });
  connect(m_file, &QToolButton::clicked, this, [this]() { m_choice = Choice::File; accept(); });
  connect(m_cancel, &QPushButton::clicked, this, [this]() { m_choice = Choice::Cancel; reject(); });
  m_library->setFocus();
}

// ---------------------------------------------------------------- Figures box

FiguresBox::FiguresBox(QWidget* parent)
  : QFrame(parent)
{
  setObjectName(QStringLiteral("figuresBox"));
  m_column = new QVBoxLayout(this);
  m_column->setContentsMargins(Size::spacingS, Size::spacingS, Size::spacingS, Size::spacingS);
  m_column->setSpacing(Size::spacingXs);
  m_heading = new QLabel(FigureSpec::tr(FigureSpec::boxHeading), this);
  m_heading->setObjectName(QStringLiteral("figuresBoxHeading"));
  m_column->addWidget(m_heading);
  m_group = new QButtonGroup(this);
  m_group->setExclusive(true);
  connect(m_group, &QButtonGroup::idClicked, this, [this](int id)
  {
    if (id != m_active) emit activateRequested(id);
  });
  if (parent) parent->installEventFilter(this);
  hide();
  applyLook();
}

void FiguresBox::setFigures(const QStringList& paths, int active)
{
  while (m_buttons.size() > paths.size())
  {
    QRadioButton* gone = m_buttons.takeLast();
    m_group->removeButton(gone);
    delete gone;
  }
  while (m_buttons.size() < paths.size())
  {
    QRadioButton* button = new QRadioButton(this);
    button->setObjectName(QStringLiteral("figuresBoxFigure%1").arg(m_buttons.size() + 1));
    button->setMinimumHeight(TextSize::scaled(Size::controlCompact));
    m_group->addButton(button, int(m_buttons.size()));
    m_column->addWidget(button);
    m_buttons << button;
  }
  m_active = active;
  for (int i = 0; i < m_buttons.size(); ++i)
  {
    QRadioButton* button = m_buttons.at(i);
    const bool on = i == active;
    button->setText(QStringLiteral("%1  %2").arg(i + 1).arg(FigureSpec::shownName(paths.at(i))));
    button->setAccessibleName(FigureSpec::tr(FigureSpec::figureName).arg(i + 1).arg(FigureSpec::shownName(paths.at(i))));
    button->setToolTip(button->accessibleName() + QStringLiteral("  Alt+%1\n").arg(i + 1)
                       + FigureSpec::tr(on ? FigureSpec::tipActive : FigureSpec::tipOther));
    QFont font = button->font();
    font.setBold(on);
    button->setFont(font);
    button->setChecked(on);
  }
  adjustSize();
  place();
}

void FiguresBox::place()
{
  QWidget* parent = parentWidget();
  if (!parent) return;
  adjustSize();
  move(parent->width() - width() - Size::spacingS, Size::spacingS);
  raise();
}

void FiguresBox::applyLook()
{
  setStyleSheet(QStringLiteral(
    "QFrame#figuresBox { background: %1; border: 1px solid %2; border-radius: %3px; }"
    "QLabel#figuresBoxHeading { color: %4; font-weight: bold; }"
    "QRadioButton { color: %5; }"
    "QRadioButton:checked { color: %6; }"
    "QRadioButton:focus { border: 2px solid %7; border-radius: %8px; }")
    .arg(hex(Token::Surface), hex(Token::Border)).arg(Size::radiusPanel)
    .arg(hex(Token::TextMuted), hex(Token::TextMuted), hex(Token::Text), hex(Token::FocusRing)).arg(Size::radiusSmall));
}

bool FiguresBox::eventFilter(QObject* watched, QEvent* event)
{
  if (watched == parentWidget() && event->type() == QEvent::Resize) place();
  return QFrame::eventFilter(watched, event);
}

// ---------------------------------------------------------------- name tag

FigureTag::FigureTag(QWidget* parent)
  : QLabel(parent)
{
  setObjectName(QStringLiteral("figureTag"));
  setCursor(Qt::PointingHandCursor);
  hide();
}

void FigureTag::setTag(int number, const QString& path, bool active)
{
  m_number = number;
  m_active = active;
  setText((active ? QStringLiteral("\u2713 ") : QString()) + FigureSpec::tagText(number, path));
  setToolTip(FigureSpec::tr(active ? FigureSpec::tipActive : FigureSpec::tipOther));
  setStyleSheet(QStringLiteral("QLabel { background: %1; color: %2; border: 1px solid %3; border-radius: %4px; padding: 2px %5px; font-weight: %6; }")
                  .arg(hex(active ? Token::Selection : Token::Surface), hex(active ? Token::OnAccent : Token::TextMuted),
                       hex(active ? Token::Selection : Token::Border))
                  .arg(Size::radiusSmall).arg(Size::spacingS).arg(active ? QStringLiteral("bold") : QStringLiteral("normal")));
  adjustSize();
}

void FigureTag::mousePressEvent(QMouseEvent* event)
{
  if (event->button() == Qt::LeftButton) emit clicked();
  QLabel::mousePressEvent(event);
}

// ---------------------------------------------------------------- timeline header row

FigureHeader::FigureHeader(QWidget* parent)
  : QFrame(parent)
{
  setObjectName(QStringLiteral("figureHeader"));
  QHBoxLayout* row = new QHBoxLayout(this);
  row->setContentsMargins(Size::spacingS, Size::spacingXs, Size::spacingS, Size::spacingXs);
  row->setSpacing(Size::spacingS);
  m_numberLabel = new QLabel(this);
  m_numberLabel->setObjectName(QStringLiteral("figureHeaderNumber"));
  m_name = new QLabel(this);
  m_name->setObjectName(QStringLiteral("figureHeaderName"));
  m_chip = new QLabel(this);
  m_chip->setObjectName(QStringLiteral("figureHeaderActive"));
  m_makeActive = new QPushButton(FigureSpec::tr(FigureSpec::makeActive), this);
  m_makeActive->setObjectName(QStringLiteral("figureHeaderMakeActive"));
  m_remove = new QPushButton(FigureSpec::tr(FigureSpec::remove), this);
  m_remove->setObjectName(QStringLiteral("figureHeaderRemove"));
  for (QPushButton* button : {m_makeActive, m_remove})
  {
    button->setFixedHeight(TextSize::scaled(Size::controlCompact));
    button->setAutoDefault(false);
    // Beginner mode hides them without moving anything (6m)
    QSizePolicy policy = button->sizePolicy();
    policy.setRetainSizeWhenHidden(true);
    button->setSizePolicy(policy);
  }
  row->addWidget(m_numberLabel);
  row->addWidget(m_name);
  row->addWidget(m_chip);
  row->addWidget(m_makeActive);
  row->addStretch(1);
  row->addWidget(m_remove);
  connect(m_makeActive, &QPushButton::clicked, this, &FigureHeader::makeActiveClicked);
  connect(m_remove, &QPushButton::clicked, this, &FigureHeader::removeClicked);
  applyLook();
}

void FigureHeader::setFigure(int number, const QString& path, bool active)
{
  m_number = number;
  m_active = active;
  m_numberLabel->setText(QString::number(number));
  m_name->setText(FigureSpec::shownName(path));
  m_chip->setText(FigureSpec::tr(FigureSpec::activeChip));
  m_makeActive->setToolTip(FigureSpec::tr(FigureSpec::makeActiveName).arg(number) + QStringLiteral("  Alt+%1").arg(number));
  m_makeActive->setAccessibleName(FigureSpec::tr(FigureSpec::makeActiveName).arg(number));
  m_remove->setToolTip(FigureSpec::tr(FigureSpec::removeName).arg(number) + QLatin1Char('\n') + FigureSpec::tr(FigureSpec::removeTip));
  m_remove->setAccessibleName(FigureSpec::tr(FigureSpec::removeName).arg(number));
  refresh();
  applyLook();
}

void FigureHeader::setExpert(bool expert)
{
  m_expert = expert;
  refresh();
}

void FigureHeader::refresh()
{
  m_chip->setVisible(m_active);
  // the Make active button keeps its room on the active row too, so both rows line up
  QSizePolicy policy = m_makeActive->sizePolicy();
  policy.setRetainSizeWhenHidden(!m_active);
  m_makeActive->setSizePolicy(policy);
  m_makeActive->setVisible(!m_active && m_expert);
  m_remove->setVisible(m_expert);
}

void FigureHeader::applyLook()
{
  m_numberLabel->setStyleSheet(numberStyle(m_active));
  QFont font = m_name->font();
  font.setBold(m_active);
  m_name->setFont(font);
  m_name->setStyleSheet(QStringLiteral("QLabel { color: %1; }").arg(hex(m_active ? Token::Text : Token::TextMuted)));
  m_chip->setStyleSheet(chipStyle());
  m_makeActive->setStyleSheet(buttonStyle(false, false));
  m_remove->setStyleSheet(buttonStyle(true, false));
  const int icon = Size::iconSmall;
  m_chip->setText(FigureSpec::tr(FigureSpec::activeChip));
  m_remove->setIcon(LucideIcons::icon(QStringLiteral("user-minus"), Theme::color(Token::OnAccent), Theme::color(Token::TextDisabled)));
  m_remove->setIconSize(QSize(icon, icon));
  setStyleSheet(QStringLiteral("QFrame#figureHeader { background: %1; border-top: 1px solid %2; }")
                  .arg(hex(m_active ? Token::SurfaceRaised : Token::Surface), hex(Token::Border)));
}

// ---------------------------------------------------------------- the other figure's rows

FigureRows::FigureRows(QWidget* parent)
  : QWidget(parent)
{
  setObjectName(QStringLiteral("figureRows"));
  setMouseTracking(true);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void FigureRows::setAnimation(Animation* anim, int number)
{
  m_animation = anim;
  m_number = number;
  if (anim) m_rows.update(anim, -1);
  else m_rows.clear();
  updateGeometry();
  update();
}

void FigureRows::setNamesWidth(int width)
{
  m_namesWidth = width;
  update();
}

void FigureRows::setScrollX(int x)
{
  m_scrollX = x;
  update();
}

QSize FigureRows::sizeHint() const
{
  return QSize(m_namesWidth + 100, m_rows.rowCount() * Constants::lineHeight());
}

int FigureRows::trackAt(const QPoint& pos) const
{
  if (pos.y() < 0 || Constants::lineHeight() <= 0) return -1;
  return m_rows.trackOfRow(pos.y() / Constants::lineHeight());
}

QString FigureRows::shownName(int track) const
{
  if (!m_animation) return QString();
  const QString name = m_animation->getPartName(track);
  if (m_nameOf)
  {
    const QString shown = m_nameOf(name);
    if (!shown.isEmpty()) return shown;
  }
  return name;
}

void FigureRows::paintEvent(QPaintEvent*)
{
  QPainter p(this);
  const bool dark = Theme::isDark();
  p.fillRect(rect(), TimelineLook::row(false, dark));
  if (!m_animation) return;
  const QColor grey = Theme::color(Token::TextDisabled);
  const int line = Constants::lineHeight();
  const int keyWidth = Constants::keyWidth();
  const int gap = Size::spacingS;
  const int frames = m_animation->getNumberOfFrames();
  for (int track : m_rows.tracks())
  {
    const int y = m_rows.rowOfTrack(track) * line;
    // the name, greyed (the area stripe too: the other figure is not active)
    p.fillRect(0, y, TimelineLook::areaStripeWidth(), line, grey);
    p.setPen(grey);
    p.drawText(QRect(TimelineLook::areaStripeWidth() + gap, y, m_namesWidth - TimelineLook::areaStripeWidth() - 2 * gap, line),
               Qt::AlignVCenter | Qt::AlignLeft, shownName(track));
    // small grey keys at their frames, under the active figure's ruler
    p.save();
    p.setClipRect(QRect(m_namesWidth, y, width() - m_namesWidth, line));
    p.fillRect(m_namesWidth, y + line / 2, width() - m_namesWidth, 1, Theme::mix(TimelineLook::row(false, dark), grey, 0.5));
    BVHNode* joint = m_animation->getNode(track);
    if (joint)
    {
      const int size = qMax(4, line / 3);
      for (int frame : joint->keyframeList())
      {
        if (frame < 0 || frame >= frames) continue;
        const int x = m_namesWidth - m_scrollX + frame * keyWidth + (keyWidth - size) / 2;
        p.fillRect(x, y + (line - size) / 2, size, size, grey);
      }
    }
    p.restore();
  }
}

void FigureRows::mousePressEvent(QMouseEvent* event)
{
  const int track = trackAt(event->position().toPoint());
  if (event->button() == Qt::LeftButton && track >= 0) emit rowClicked(track);
  QWidget::mousePressEvent(event);
}

bool FigureRows::event(QEvent* event)
{
  if (event->type() == QEvent::ToolTip)
  {
    QHelpEvent* help = static_cast<QHelpEvent*>(event);
    const int track = trackAt(help->pos());
    if (track >= 0)
      QToolTip::showText(help->globalPos(), shownName(track) + QLatin1Char('\n') + FigureSpec::tr(FigureSpec::otherRowTip).arg(m_number), this);
    else
      QToolTip::hideText();
    return true;
  }
  return QWidget::event(event);
}

// ---------------------------------------------------------------- Remove with unsaved changes

QMessageBox* FigureRemove::box(QWidget* parent, int number, const QString& path)
{
  QMessageBox* box = new QMessageBox(QMessageBox::Warning, FigureSpec::tr(FigureSpec::removeTitle).arg(number),
                                     FigureSpec::tr(FigureSpec::removeLine).arg(FigureSpec::shownName(path)), QMessageBox::NoButton, parent);
  box->setObjectName(QStringLiteral("removeFigureBox"));
  box->setTextFormat(Qt::PlainText);
  QPushButton* cancel = box->addButton(FigureSpec::tr(FigureSpec::cancel), QMessageBox::RejectRole);
  cancel->setObjectName(QStringLiteral("removeFigureCancel"));
  QPushButton* without = box->addButton(FigureSpec::tr(FigureSpec::removeWithout), QMessageBox::DestructiveRole);
  without->setObjectName(QStringLiteral("removeFigureWithout"));
  QPushButton* save = box->addButton(FigureSpec::tr(FigureSpec::saveThenRemove), QMessageBox::AcceptRole);
  save->setObjectName(QStringLiteral("removeFigureSave"));
  box->setDefaultButton(save);
  box->setEscapeButton(cancel);
  return box;
}

FigureRemove::Choice FigureRemove::choiceOf(QMessageBox* box)
{
  QAbstractButton* clicked = box ? box->clickedButton() : nullptr;
  if (!clicked) return Choice::Cancel;
  if (clicked->objectName() == QLatin1String("removeFigureWithout")) return Choice::RemoveWithoutSaving;
  if (clicked->objectName() == QLatin1String("removeFigureSave")) return Choice::SaveThenRemove;
  return Choice::Cancel;
}
