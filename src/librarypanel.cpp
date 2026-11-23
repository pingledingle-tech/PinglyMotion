/*
 * PinglyMotion - the Library panel (see librarypanel.h)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#include "librarypanel.h"

#include <QAction>
#include <QApplication>
#include <QButtonGroup>
#include <QDockWidget>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QJsonObject>
#include <QKeyEvent>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QSizeGrip>
#include <QStackedWidget>
#include <QStyledItemDelegate>
#include <QVBoxLayout>

#include "controlapi.h"
#include "figures.h"
#include "librarypreview.h"
#include "lucideicons.h"
#include "paneltitlebar.h"
#include "textsize.h"
#include "theme.h"
#include "usersettings.h"

const char* const LibraryPanel::DockName = "libraryDock";
const char* const LibraryPanel::ViewActionName = "viewLibraryAction";

namespace
{
  using Theme::Token;
  namespace Size = Theme::Size;

  const int IdRole = Qt::UserRole;
  const int LineRole = Qt::UserRole + 1;
  const int OpenRole = Qt::UserRole + 2;
  // the list takes the room the panel has, at least this tall (the mockup's .liblist)
  const int ListMinimum = 120;

  QString hex(Token token) { return Theme::color(token).name(QColor::HexRgb); }

  // A layout that puts its items in a row and goes on in the next row when the
  // row is full, so the kinds of move wrap at a narrow width
  class FlowLayout : public QLayout
  {
    public:
      FlowLayout(QWidget* parent, int spacing) : QLayout(parent) { setContentsMargins(0, 0, 0, 0); setSpacing(spacing); }
      ~FlowLayout() override { while (QLayoutItem* item = takeAt(0)) delete item; }
      void addItem(QLayoutItem* item) override { m_items << item; }
      int count() const override { return int(m_items.size()); }
      QLayoutItem* itemAt(int index) const override { return m_items.value(index); }
      QLayoutItem* takeAt(int index) override { return index >= 0 && index < m_items.size() ? m_items.takeAt(index) : nullptr; }
      Qt::Orientations expandingDirections() const override { return {}; }
      bool hasHeightForWidth() const override { return true; }
      int heightForWidth(int width) const override { return place(QRect(0, 0, width, 0), false); }
      void setGeometry(const QRect& rect) override { QLayout::setGeometry(rect); place(rect, true); }
      QSize sizeHint() const override { return minimumSize(); }
      QSize minimumSize() const override
      {
        QSize size;
        for (const QLayoutItem* item : m_items) size = size.expandedTo(item->minimumSize());
        return size;
      }

    private:
      int place(const QRect& rect, bool apply) const
      {
        int x = rect.x(), y = rect.y(), rowHeight = 0;
        for (QLayoutItem* item : m_items)
        {
          const QSize hint = item->sizeHint().expandedTo(item->minimumSize());
          if (x > rect.x() && x + hint.width() > rect.right() + 1)
          {
            x = rect.x();
            y += rowHeight + spacing();
            rowHeight = 0;
          }
          if (apply) item->setGeometry(QRect(QPoint(x, y), hint));
          x += hint.width() + spacing();
          rowHeight = qMax(rowHeight, hint.height());
        }
        return y + rowHeight - rect.y();
      }

      QList<QLayoutItem*> m_items;
  };

  // a card with a rounded border, dashed for the empty states (the mockup's .emptycard),
  // solid for the picked clip (.libprev)
  class Card : public QFrame
  {
    public:
      Card(bool dashed, QWidget* parent) : QFrame(parent), m_dashed(dashed) {}

    protected:
      void paintEvent(QPaintEvent*) override
      {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        QPen pen(Theme::color(Token::Border), Size::borderWidth, m_dashed ? Qt::DashLine : Qt::SolidLine);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        const int radius = m_dashed ? Size::radiusPanel : Size::radiusControl;
        painter.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), radius, radius);
      }

    private:
      bool m_dashed;
  };

  // a row of the clip list: the name (semibold) with the "Open now" chip, then the length and
  // the tags in caption size, muted; the picked row has the selection fill and on_accent text
  class RowDelegate : public QStyledItemDelegate
  {
    public:
      using QStyledItemDelegate::QStyledItemDelegate;

      QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex&) const override
      {
        const QFontMetrics name(boldFont(option.font)), line(captionFont(option.font));
        const int h = name.height() + line.height() + 2 * Size::spacingXs;
        return QSize(option.rect.width(), std::max(h, TextSize::scaled(Size::controlDefault)));
      }

      void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
      {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        const bool picked = option.state & QStyle::State_Selected;
        const bool hover = option.state & QStyle::State_MouseOver;
        const QRectF box = QRectF(option.rect).adjusted(0, 1, 0, -1);
        if (picked || hover)
        {
          painter->setPen(Qt::NoPen);
          painter->setBrush(Theme::color(picked ? Token::Selection : Token::SurfaceRaised));
          painter->drawRoundedRect(box, Size::radiusSmall, Size::radiusSmall);
        }
        const QColor text = Theme::color(picked ? Token::OnAccent : Token::Text);
        const QColor muted = Theme::color(picked ? Token::OnAccent : Token::TextMuted);
        const QFont bold = boldFont(option.font), caption = captionFont(option.font);
        const QFontMetrics nm(bold), cm(caption);
        const int x = option.rect.x() + Size::spacingS;
        int y = option.rect.y() + Size::spacingXs;
        painter->setFont(bold);
        painter->setPen(text);
        const QString name = index.data(Qt::DisplayRole).toString();
        painter->drawText(QPoint(x, y + nm.ascent()), name);
        if (index.data(OpenRole).toBool())
        {
          // the chip: caption text with the check icon, a round border
          const QString chip = LibrarySpec::tr(LibrarySpec::openNowChip);
          const int icon = cm.height();
          const int w = Size::spacingXs * 3 + icon + cm.horizontalAdvance(chip);
          const QRectF r(x + nm.horizontalAdvance(name) + Size::spacingS, y, w, nm.height());
          painter->setPen(QPen(picked ? Theme::color(Token::OnAccent) : Theme::color(Token::Border), Size::borderWidth));
          painter->setBrush(Qt::NoBrush);
          painter->drawRoundedRect(r.adjusted(0.5, 0.5, -0.5, -0.5), r.height() / 2, r.height() / 2);
          const QIcon check = LucideIcons::icon(QStringLiteral("check"), text, text);
          check.paint(painter, QRect(int(r.x()) + Size::spacingXs, int(r.center().y() - icon / 2.0), icon, icon));
          painter->setFont(caption);
          painter->setPen(text);
          painter->drawText(QRectF(r.x() + Size::spacingXs * 2 + icon, r.y(), w, r.height()), Qt::AlignVCenter | Qt::AlignLeft, chip);
        }
        y += nm.height();
        painter->setFont(caption);
        painter->setPen(muted);
        painter->drawText(QRect(x, y, option.rect.width() - 2 * Size::spacingS, cm.height()), Qt::AlignLeft | Qt::AlignVCenter,
                          cm.elidedText(index.data(LineRole).toString(), Qt::ElideRight, option.rect.width() - 2 * Size::spacingS));
        if (option.state & QStyle::State_HasFocus && option.widget && option.widget->hasFocus())
        {
          painter->setPen(QPen(Theme::color(Token::FocusRing), Size::focusRingWidth));
          painter->setBrush(Qt::NoBrush);
          painter->drawRoundedRect(box.adjusted(1, 1, -1, -1), Size::radiusSmall, Size::radiusSmall);
        }
        painter->restore();
      }

    private:
      static QFont boldFont(QFont font) { font.setWeight(QFont::DemiBold); return font; }
      static QFont captionFont(QFont font)
      {
        // the caption type token: a step smaller than the body text
        if (font.pointSizeF() > 0) font.setPointSizeF(font.pointSizeF() * 0.85);
        else font.setPixelSize(std::max(1, int(font.pixelSize() * 0.85)));
        return font;
      }
  };

  QLabel* bold(const QString& text, QWidget* parent)
  {
    QLabel* label = new QLabel(text, parent);
    QFont font = label->font();
    font.setWeight(QFont::DemiBold);
    label->setFont(font);
    return label;
  }
}

LibraryPanel::LibraryPanel(QWidget* parent) : QWidget(parent)
{
  setObjectName(QStringLiteral("libraryPanel"));
  setMinimumSize(minimumPanelSize());
  QVBoxLayout* outer = new QVBoxLayout(this);
  outer->setContentsMargins(Size::spacingM, Size::spacingS, Size::spacingM, Size::spacingM);
  outer->setSpacing(Size::spacingS);

  // the line on top (while hints are on)
  m_explainRow = new QWidget(this);
  m_explainRow->setObjectName(QStringLiteral("libraryExplainRow"));
  QHBoxLayout* explainRow = new QHBoxLayout(m_explainRow);
  explainRow->setContentsMargins(0, 0, 0, 0);
  explainRow->setSpacing(Size::spacingS);
  m_explainIcon = new QLabel(m_explainRow);
  explainRow->addWidget(m_explainIcon, 0, Qt::AlignTop);
  m_explain = new QLabel(LibrarySpec::tr(LibrarySpec::explain), m_explainRow);
  m_explain->setObjectName(QStringLiteral("libraryExplain"));
  m_explain->setWordWrap(true);
  explainRow->addWidget(m_explain, 1);
  outer->addWidget(m_explainRow);

  m_pages = new QStackedWidget(this);
  outer->addWidget(m_pages, 1);

  // ---- the library
  QWidget* main = new QWidget(m_pages);
  QVBoxLayout* mainLayout = new QVBoxLayout(main);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(Size::spacingS);

  m_search = new QLineEdit(main);
  m_search->setObjectName(QStringLiteral("librarySearch"));
  m_search->setPlaceholderText(LibrarySpec::tr(LibrarySpec::search));
  m_search->setAccessibleName(LibrarySpec::tr(LibrarySpec::search));
  m_search->setToolTip(LibrarySpec::tr(LibrarySpec::searchTip));
  m_search->setClearButtonEnabled(true);
  m_search->setMinimumHeight(TextSize::scaled(Size::controlDefault));
  m_search->installEventFilter(this);
  m_searchIcon = m_search->addAction(QIcon(), QLineEdit::LeadingPosition);
  mainLayout->addWidget(m_search);

  m_kindLabel = bold(LibrarySpec::tr(LibrarySpec::kindLabel), main);
  mainLayout->addWidget(m_kindLabel);
  QWidget* kindsBox = new QWidget(main);
  kindsBox->setObjectName(QStringLiteral("libraryKinds"));
  kindsBox->setAccessibleName(LibrarySpec::tr(LibrarySpec::kindLabel));
  FlowLayout* kinds = new FlowLayout(kindsBox, Size::spacingXs);
  m_kinds = new QButtonGroup(this);
  m_kinds->setExclusive(true);
  for (const LibrarySpec::Kind& k : LibrarySpec::kinds)
  {
    QPushButton* b = new QPushButton(kindsBox);
    b->setObjectName(QStringLiteral("libraryKind_") + (k.id[0] ? QLatin1String(k.id) : QLatin1String("all")));
    b->setProperty("kind", QLatin1String(k.id));
    b->setCheckable(true);
    b->setMinimumHeight(TextSize::scaled(Size::controlCompact));
    b->installEventFilter(this);
    kinds->addWidget(b);
    m_kinds->addButton(b);
    m_kindButtons << b;
    const QString id = QLatin1String(k.id);
    connect(b, &QPushButton::clicked, this, [this, id]() { setKind(id); });
  }
  mainLayout->addWidget(kindsBox);

  m_count = new QLabel(main);
  m_count->setObjectName(QStringLiteral("libraryCount"));
  mainLayout->addWidget(m_count);

  m_listPages = new QStackedWidget(main);
  m_list = new QListWidget(m_listPages);
  m_list->setObjectName(QStringLiteral("libraryList"));
  m_list->setAccessibleName(LibrarySpec::tr(LibrarySpec::clipsName));
  m_list->setItemDelegate(new RowDelegate(m_list));
  m_list->setMouseTracking(true);
  m_list->setUniformItemSizes(true);
  m_list->setMinimumHeight(TextSize::scaled(ListMinimum));
  m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  m_list->installEventFilter(this);
  m_listPages->addWidget(m_list);
  connect(m_list, &QListWidget::currentItemChanged, this, [this](QListWidgetItem* item)
  {
    if (!item) return;
    const QString id = item->data(IdRole).toString();
    if (id == m_selected) return;
    m_selected = id;
    refreshDetails();
  });
  connect(m_list, &QListWidget::itemDoubleClicked, this, [this]() { useSelected(); });

  m_noHit = new Card(true, m_listPages);
  m_noHit->setObjectName(QStringLiteral("libraryNoHit"));
  QVBoxLayout* noHit = new QVBoxLayout(m_noHit);
  noHit->setContentsMargins(Size::spacingL, Size::spacingL, Size::spacingL, Size::spacingL);
  noHit->setSpacing(Size::spacingS);
  QHBoxLayout* noHitHead = new QHBoxLayout;
  m_noHitIcon = new QLabel(m_noHit);
  noHitHead->addWidget(m_noHitIcon);
  m_noHitTitle = bold(LibrarySpec::tr(LibrarySpec::noHitTitle), m_noHit);
  noHitHead->addWidget(m_noHitTitle, 1);
  noHit->addLayout(noHitHead);
  m_noHitLine = new QLabel(LibrarySpec::tr(LibrarySpec::noHitLine), m_noHit);
  m_noHitLine->setWordWrap(true);
  noHit->addWidget(m_noHitLine);
  m_showAll = new QPushButton(LibrarySpec::tr(LibrarySpec::showAll), m_noHit);
  m_showAll->setObjectName(QStringLiteral("libraryShowAll"));
  m_showAll->setMinimumHeight(TextSize::scaled(Size::controlDefault));
  noHit->addWidget(m_showAll, 0, Qt::AlignLeft);
  noHit->addStretch(1);
  m_listPages->addWidget(m_noHit);
  connect(m_showAll, &QPushButton::clicked, this, &LibraryPanel::showAll);
  mainLayout->addWidget(m_listPages, 1);

  // the picked clip
  m_details = new Card(false, main);
  m_details->setObjectName(QStringLiteral("libraryDetails"));
  QVBoxLayout* details = new QVBoxLayout(m_details);
  details->setContentsMargins(Size::spacingM, Size::spacingM, Size::spacingM, Size::spacingM);
  details->setSpacing(Size::spacingS);
  m_name = bold(QString(), m_details);
  m_name->setObjectName(QStringLiteral("libraryClipName"));
  m_name->setWordWrap(true);
  details->addWidget(m_name);
  m_description = new QLabel(m_details);
  m_description->setObjectName(QStringLiteral("libraryClipDescription"));
  m_description->setWordWrap(true);
  details->addWidget(m_description);
  QHBoxLayout* previewRow = new QHBoxLayout;
  previewRow->setSpacing(Size::spacingM);
  m_preview = new LibraryPreview(m_details);
  previewRow->addWidget(m_preview, 0, Qt::AlignTop);
  QVBoxLayout* column = new QVBoxLayout;
  column->setSpacing(Size::spacingS);
  m_previewButton = new QPushButton(LibrarySpec::tr(LibrarySpec::playPreview), m_details);
  m_previewButton->setObjectName(QStringLiteral("libraryPreviewButton"));
  m_previewButton->setMinimumHeight(TextSize::scaled(Size::controlCompact));
  column->addWidget(m_previewButton, 0, Qt::AlignLeft);
  connect(m_previewButton, &QPushButton::clicked, this, &LibraryPanel::togglePreview);
  connect(m_preview, &LibraryPreview::playingChanged, this, [this](bool on)
  {
    m_previewButton->setText(LibrarySpec::tr(on ? LibrarySpec::stopPreview : LibrarySpec::playPreview));
    applyStyle();
  });
  QGridLayout* facts = new QGridLayout;
  facts->setHorizontalSpacing(Size::spacingM);
  facts->setVerticalSpacing(Size::spacingXs);
  const char* names[] = {LibrarySpec::factLength, LibrarySpec::factSkeleton, LibrarySpec::factFrom, LibrarySpec::factLicence};
  for (int row = 0; row < 4; row++)
  {
    QLabel* name = new QLabel(LibrarySpec::tr(names[row]), m_details);
    QLabel* value = new QLabel(m_details);
    value->setWordWrap(true);
    value->setObjectName(QStringLiteral("libraryFact%1").arg(row));
    facts->addWidget(name, row, 0, Qt::AlignTop);
    facts->addWidget(value, row, 1);
    m_factNames << name;
    m_facts << value;
  }
  facts->setColumnStretch(1, 1);
  column->addStretch(1);
  previewRow->addLayout(column, 1);
  details->addLayout(previewRow);
  // the facts under the preview, so they keep their lines at the smallest docked width
  details->addLayout(facts);
  m_use = new QPushButton(LibrarySpec::tr(LibrarySpec::use), m_details);
  m_use->setObjectName(QStringLiteral("libraryUse"));
  m_use->setMinimumHeight(TextSize::scaled(Size::controlDefault));
  m_use->setDefault(false);
  m_use->setAutoDefault(false);
  connect(m_use, &QPushButton::clicked, this, &LibraryPanel::useSelected);
  // Add as a second figure (Expert mode only, docs/UX_DECISIONS.md 6m): beside Use this,
  // under it when the panel is too narrow for both
  m_addFigure = new QPushButton(FigureSpec::tr(FigureSpec::libraryAdd), m_details);
  m_addFigure->setObjectName(QStringLiteral("libraryAddFigure"));
  m_addFigure->setMinimumHeight(TextSize::scaled(Size::controlDefault));
  m_addFigure->setAutoDefault(false);
  connect(m_addFigure, &QPushButton::clicked, this, &LibraryPanel::addSelectedAsFigure);
  m_useRow = new QBoxLayout(QBoxLayout::LeftToRight);
  m_useRow->setContentsMargins(0, 0, 0, 0);
  m_useRow->addWidget(m_use, 0, Qt::AlignLeft);
  m_useRow->addWidget(m_addFigure, 0, Qt::AlignLeft);
  m_useRow->addStretch(1);
  details->addLayout(m_useRow);
  refreshAddFigure();
  setExpertMode(UserSettings::expertMode());
  UserSettings::watchExpertMode(this, [this](bool on) { setExpertMode(on); });
  mainLayout->addWidget(m_details);
  m_pages->addWidget(main);

  // ---- no library folder
  QWidget* missingPage = new QWidget(m_pages);
  QVBoxLayout* missingLayout = new QVBoxLayout(missingPage);
  missingLayout->setContentsMargins(0, 0, 0, 0);
  m_missing = new Card(true, missingPage);
  m_missing->setObjectName(QStringLiteral("libraryMissing"));
  QVBoxLayout* missing = new QVBoxLayout(m_missing);
  missing->setContentsMargins(Size::spacingL, Size::spacingL, Size::spacingL, Size::spacingL);
  missing->setSpacing(Size::spacingS);
  QHBoxLayout* missingHead = new QHBoxLayout;
  m_missingIcon = new QLabel(m_missing);
  missingHead->addWidget(m_missingIcon);
  m_missingTitle = bold(LibrarySpec::tr(LibrarySpec::missingTitle), m_missing);
  missingHead->addWidget(m_missingTitle, 1);
  missing->addLayout(missingHead);
  m_missingLine = new QLabel(LibrarySpec::tr(LibrarySpec::missingLine), m_missing);
  m_missingLine->setWordWrap(true);
  missing->addWidget(m_missingLine);
  missingLayout->addWidget(m_missing);
  missingLayout->addStretch(1);
  m_pages->addWidget(missingPage);

  // Tab: search field, kind of move (one stop), list (one stop), Play preview, Use this (P8)
  setTabOrder(m_search, m_kindButtons.first());
  for (int i = 1; i < m_kindButtons.size(); i++) setTabOrder(m_kindButtons.at(i - 1), m_kindButtons.at(i));
  setTabOrder(m_kindButtons.last(), m_list);
  setTabOrder(m_list, m_showAll);
  setTabOrder(m_showAll, m_previewButton);
  setTabOrder(m_previewButton, m_use);
  setTabOrder(m_use, m_addFigure);

  connect(m_search, &QLineEdit::textChanged, this, [this]() { buildList(); });

  m_sizeGrip = new QSizeGrip(this);
  m_sizeGrip->setObjectName(QStringLiteral("librarySizeGrip"));
  m_sizeGrip->hide();

  setLibraryFolder(QString());
  applyStyle();
}

QDockWidget* LibraryPanel::makeDock(QMainWindow* window, LibraryPanel* panel)
{
  QDockWidget* dock = new QDockWidget(LibrarySpec::tr(LibrarySpec::title), window);
  dock->setObjectName(QString::fromLatin1(DockName));
  dock->setWidget(panel);
  dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);
  dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
  dock->setTitleBarWidget(new PanelTitleBar(dock));

  QAction* action = dock->toggleViewAction();
  action->setObjectName(QString::fromLatin1(ViewActionName));
  action->setText(LibrarySpec::tr(LibrarySpec::title));
  action->setShortcut(QKeySequence(QLatin1String(LibrarySpec::key)));
  action->setShortcutContext(Qt::ApplicationShortcut);
  action->setToolTip(LibrarySpec::tr(LibrarySpec::topTip));
  action->setStatusTip(action->toolTip());

  // hidden until asked for, on the right side
  window->addDockWidget(Qt::RightDockWidgetArea, dock);
  dock->hide();
  connect(dock, &QDockWidget::topLevelChanged, panel, [panel](bool floating) { panel->setFloatingGrip(floating); });
  return dock;
}

QSize LibraryPanel::defaultSize()
{
  return QSize(TextSize::scaled(320), TextSize::scaled(640));
}

QSize LibraryPanel::minimumPanelSize()
{
  // the smallest right column (docs/UX_DECISIONS.md 6l: 240 pixels)
  return QSize(TextSize::scaled(240), TextSize::scaled(240));
}

void LibraryPanel::setFloatingGrip(bool floating)
{
  m_sizeGrip->setVisible(floating);
  placeSizeGrip();
}

void LibraryPanel::placeSizeGrip()
{
  const QSize hint = m_sizeGrip->sizeHint();
  m_sizeGrip->setGeometry(width() - hint.width(), height() - hint.height(), hint.width(), hint.height());
  m_sizeGrip->raise();
}

void LibraryPanel::resizeEvent(QResizeEvent* event)
{
  QWidget::resizeEvent(event);
  placeSizeGrip();
  fitUseRow();
}

void LibraryPanel::fitUseRow()
{
  if (!m_useRow || !m_details) return;
  const int both = m_use->sizeHint().width() + m_useRow->spacing() + m_addFigure->sizeHint().width();
  const QMargins margins = m_details->layout() ? m_details->layout()->contentsMargins() : QMargins();
  const bool wrap = m_addFigure->isVisibleTo(this) && both > m_details->width() - margins.left() - margins.right();
  const QBoxLayout::Direction direction = wrap ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight;
  if (m_useRow->direction() != direction) m_useRow->setDirection(direction);
}

bool LibraryPanel::useRowWrapped() const
{
  return m_useRow && m_useRow->direction() == QBoxLayout::TopToBottom;
}

void LibraryPanel::setExpertMode(bool expert)
{
  m_expert = expert;
  m_addFigure->setVisible(expert);
  fitUseRow();
}

void LibraryPanel::setSecondFigureBlocked(const QString& why)
{
  m_addFigureBlocked = why;
  refreshAddFigure();
}

void LibraryPanel::refreshAddFigure()
{
  m_addFigure->setEnabled(m_addFigureBlocked.isEmpty());
  m_addFigure->setToolTip(m_addFigureBlocked.isEmpty() ? FigureSpec::tr(FigureSpec::libraryAddTip) : m_addFigureBlocked);
  m_addFigure->setStatusTip(m_addFigure->toolTip());
}

bool LibraryPanel::addSelectedAsFigure()
{
  if (!m_expert || !m_index.find(m_selected) || !m_addFigure->isEnabled() || m_details->isHidden()) return false;
  emit addFigureRequested(m_selected);
  return true;
}

bool LibraryPanel::setLibraryFolder(const QString& folder)
{
  const bool ok = m_index.load(folder);
  m_pages->setCurrentIndex(ok ? 0 : 1);
  m_explainRow->setVisible(m_hintsOn && ok);
  if (ok && !m_index.find(m_selected)) m_selected = m_index.clips().first().id;
  refreshKinds();
  buildList();
  setOpenFile(m_openFile);
  return ok;
}

void LibraryPanel::setHintsOn(bool on)
{
  m_hintsOn = on;
  m_explainRow->setVisible(on && m_index.loaded());
}

void LibraryPanel::setOpenFile(const QString& path)
{
  m_openFile = path;
  const LibraryClip* clip = m_index.findByPath(path);
  const QString id = clip ? clip->id : QString();
  if (id == m_openClipId && m_list->count() > 0) return;
  m_openClipId = id;
  for (int i = 0; i < m_list->count(); i++)
    m_list->item(i)->setData(OpenRole, m_list->item(i)->data(IdRole).toString() == m_openClipId && !id.isEmpty());
  refreshDetails();
}

void LibraryPanel::setSearchText(const QString& text)
{
  m_search->setText(text);
}

QString LibraryPanel::searchText() const
{
  return m_search->text();
}

void LibraryPanel::setKind(const QString& kind)
{
  bool known = false;
  for (QPushButton* b : m_kindButtons)
    if (b->property("kind").toString() == kind) known = true;
  m_kind = known ? kind : QString();
  refreshKinds();
  buildList();
}

void LibraryPanel::showAll()
{
  const QSignalBlocker block(m_search);
  m_search->clear();
  m_kind.clear();
  refreshKinds();
  buildList();
  emit statusMessage(LibrarySpec::tr(LibrarySpec::showingAll).arg(m_index.size()));
  m_list->setFocus();
}

QStringList LibraryPanel::shownClipIds() const
{
  QStringList ids;
  if (m_listPages->currentWidget() != m_list) return ids;
  for (int i = 0; i < m_list->count(); i++) ids << m_list->item(i)->data(IdRole).toString();
  return ids;
}

bool LibraryPanel::selectClip(const QString& id)
{
  for (int i = 0; i < m_list->count(); i++)
    if (m_list->item(i)->data(IdRole).toString() == id)
    {
      m_list->setCurrentRow(i);
      m_list->scrollToItem(m_list->item(i));
      return true;
    }
  return false;
}

bool LibraryPanel::noHitShown() const
{
  return m_index.loaded() && m_listPages->currentWidget() == m_noHit;
}

bool LibraryPanel::missingShown() const
{
  return m_pages->currentIndex() == 1;
}

QPushButton* LibraryPanel::kindButton(const QString& kind) const
{
  for (QPushButton* b : m_kindButtons)
    if (b->property("kind").toString() == kind) return b;
  return nullptr;
}

void LibraryPanel::refreshKinds()
{
  for (QPushButton* b : m_kindButtons)
  {
    const QString id = b->property("kind").toString();
    const char* text = "";
    for (const LibrarySpec::Kind& k : LibrarySpec::kinds)
      if (id == QLatin1String(k.id)) text = k.text;
    const int n = m_index.count(id);
    // the number after the name (the mockup's .n); the chosen kind is bold, not colour alone
    b->setText(QStringLiteral("%1  %2").arg(LibrarySpec::tr(text)).arg(n));
    b->setAccessibleName(LibrarySpec::tr(LibrarySpec::kindName).arg(LibrarySpec::tr(text)).arg(n));
    b->setToolTip(b->accessibleName());
    const bool chosen = id == m_kind;
    b->setChecked(chosen);
    QFont font = b->font();
    font.setWeight(chosen ? QFont::DemiBold : QFont::Normal);
    b->setFont(font);
    // one Tab stop for the row: only the chosen kind takes Tab, arrows move between them
    b->setFocusPolicy(chosen ? Qt::StrongFocus : Qt::ClickFocus);
  }
}

void LibraryPanel::moveKind(int step, bool toEnd)
{
  int at = 0;
  for (int i = 0; i < m_kindButtons.size(); i++)
    if (m_kindButtons.at(i)->property("kind").toString() == m_kind) at = i;
  const int last = int(m_kindButtons.size()) - 1;
  int next = toEnd ? (step < 0 ? 0 : last) : at + step;
  next = std::clamp(next, 0, last);
  setKind(m_kindButtons.at(next)->property("kind").toString());
  m_kindButtons.at(next)->setFocus(Qt::TabFocusReason);
}

void LibraryPanel::buildList()
{
  const QList<int> hits = m_index.search(m_search->text(), m_kind);
  m_count->setText(LibraryIndex::countText(int(hits.size()), m_index.size()));
  m_count->setVisible(m_index.loaded());
  {
    const QSignalBlocker block(m_list);
    m_list->clear();
    for (int i : hits)
    {
      const LibraryClip& c = m_index.clips().at(i);
      QListWidgetItem* item = new QListWidgetItem(c.name, m_list);
      item->setData(IdRole, c.id);
      item->setData(LineRole, LibraryIndex::rowLine(c));
      item->setData(OpenRole, !m_openClipId.isEmpty() && c.id == m_openClipId);
      item->setToolTip(c.description);
    }
  }
  if (hits.isEmpty())
  {
    m_listPages->setCurrentWidget(m_noHit);
    m_details->hide();
    m_preview->setPlaying(false);
    return;
  }
  m_listPages->setCurrentWidget(m_list);
  m_details->show();
  // the picked clip stays picked while it is in the list, otherwise the first one
  bool keep = false;
  for (int i : hits)
    if (m_index.clips().at(i).id == m_selected) keep = true;
  const QString want = keep ? m_selected : m_index.clips().at(hits.first()).id;
  {
    const QSignalBlocker block(m_list);
    for (int i = 0; i < m_list->count(); i++)
      if (m_list->item(i)->data(IdRole).toString() == want) { m_list->setCurrentRow(i); m_list->scrollToItem(m_list->item(i)); }
  }
  if (want != m_selected || !keep || m_preview->clip().isEmpty())
  {
    m_selected = want;
    refreshDetails();
  }
}

void LibraryPanel::refreshDetails()
{
  const LibraryClip* c = m_index.find(m_selected);
  if (!c) return;
  m_name->setText(c->name);
  m_description->setText(c->description);
  m_facts.at(0)->setText(LibraryIndex::lengthText(*c));
  m_facts.at(1)->setText(LibraryIndex::skeletonText(*c));
  m_facts.at(2)->setText(LibraryIndex::fromText(*c));
  m_facts.at(3)->setText(LibraryIndex::licenceText(*c));
  const bool open = !m_openClipId.isEmpty() && c->id == m_openClipId;
  m_use->setEnabled(!open);
  m_use->setToolTip(LibrarySpec::tr(open ? LibrarySpec::openNowReason : LibrarySpec::useTip));
  m_use->setStatusTip(m_use->toolTip());
  m_preview->setAccessibleName(LibrarySpec::tr(LibrarySpec::previewName).arg(c->name));
  refreshPreviewClip();
}

void LibraryPanel::refreshPreviewClip()
{
  const LibraryClip* c = m_index.find(m_selected);
  const QString path = c ? m_index.absolutePath(*c) : QString();
  if (path == m_preview->clip() && !m_previewDirty) return;
  // the clip is read only while the panel shows, so a hidden panel costs nothing at start-up
  if (!isVisible())
  {
    m_preview->setPlaying(false);
    m_previewDirty = true;
    return;
  }
  m_previewDirty = false;
  m_preview->setClip(path);
  m_previewButton->setEnabled(m_preview->frameCount() > 1);
}

void LibraryPanel::showEvent(QShowEvent* event)
{
  QWidget::showEvent(event);
  if (m_previewDirty || m_preview->clip().isEmpty()) refreshPreviewClip();
}

bool LibraryPanel::useSelected()
{
  if (!m_index.find(m_selected) || !m_use->isEnabled() || m_details->isHidden()) return false;
  emit useRequested(m_selected);
  return true;
}

void LibraryPanel::togglePreview()
{
  if (m_previewDirty) refreshPreviewClip();
  m_preview->setPlaying(!m_preview->playing());
}

void LibraryPanel::focusSearch()
{
  if (missingShown()) return;
  m_search->setFocus(Qt::OtherFocusReason);
  m_search->selectAll();
}

bool LibraryPanel::eventFilter(QObject* watched, QEvent* event)
{
  if (event->type() != QEvent::KeyPress) return QWidget::eventFilter(watched, event);
  QKeyEvent* key = static_cast<QKeyEvent*>(event);
  if (watched == m_search)
  {
    // Esc clears the field; Enter or Down arrow goes to the list
    if (key->key() == Qt::Key_Escape && !m_search->text().isEmpty()) { m_search->clear(); return true; }
    if ((key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter || key->key() == Qt::Key_Down) && m_list->count() > 0)
    {
      m_list->setFocus(Qt::OtherFocusReason);
      return true;
    }
  }
  else if (watched == m_list)
  {
    // Enter is Use this, Space plays or stops the preview
    if (key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter) { useSelected(); return true; }
    if (key->key() == Qt::Key_Space) { togglePreview(); return true; }
  }
  else if (m_kindButtons.contains(static_cast<QPushButton*>(watched)))
  {
    switch (key->key())
    {
      case Qt::Key_Left: case Qt::Key_Up: moveKind(-1, false); return true;
      case Qt::Key_Right: case Qt::Key_Down: moveKind(1, false); return true;
      case Qt::Key_Home: moveKind(-1, true); return true;
      case Qt::Key_End: moveKind(1, true); return true;
      default: break;
    }
  }
  return QWidget::eventFilter(watched, event);
}

void LibraryPanel::changeEvent(QEvent* event)
{
  QWidget::changeEvent(event);
  if (event->type() == QEvent::FontChange)
  {
    setMinimumSize(minimumPanelSize());
    updateGeometry();
  }
  if (event->type() == QEvent::PaletteChange) applyStyle();
}

void LibraryPanel::applyStyle()
{
  const QString focus = hex(Token::FocusRing);
  // a normal button: surface_raised fill, border, text; greyed with text_disabled
  const QString normal = QStringLiteral(
    "QPushButton { background: %1; color: %2; border: 1px solid %3; border-radius: %4px; padding: 0 %5px; }"
    "QPushButton:disabled { color: %6; }"
    "QPushButton:focus { border: 2px solid %7; }")
    .arg(hex(Token::SurfaceRaised), hex(Token::Text), hex(Token::Border)).arg(Size::radiusControl).arg(Size::spacingM)
    .arg(hex(Token::TextDisabled), focus);
  // the main button: accent fill, on_accent text
  const QString mainButton = QStringLiteral(
    "QPushButton { background: %1; color: %2; border: 1px solid %1; border-radius: %3px; padding: 0 %4px; font-weight: 600; }"
    "QPushButton:disabled { background: %5; color: %6; border: 1px solid %7; }"
    "QPushButton:focus { border: 2px solid %8; }")
    .arg(hex(Token::Accent), hex(Token::OnAccent)).arg(Size::radiusControl).arg(Size::spacingM)
    .arg(hex(Token::SurfaceRaised), hex(Token::TextDisabled), hex(Token::Border), focus);
  // the kinds: compact buttons; the chosen one has the selection fill and on_accent text (and bold)
  const QString kind = QStringLiteral(
    "QPushButton { background: %1; color: %2; border: 1px solid %3; border-radius: %4px; padding: 0 %5px; }"
    "QPushButton:checked { background: %6; color: %7; border: 1px solid %6; }"
    "QPushButton:focus { border: 2px solid %8; }")
    .arg(hex(Token::SurfaceRaised), hex(Token::Text), hex(Token::Border)).arg(Size::radiusControl).arg(Size::spacingS)
    .arg(hex(Token::Selection), hex(Token::OnAccent), focus);
  for (QPushButton* b : m_kindButtons) b->setStyleSheet(kind);
  m_showAll->setStyleSheet(normal);
  m_previewButton->setStyleSheet(normal);
  m_use->setStyleSheet(mainButton);
  m_search->setStyleSheet(QStringLiteral(
    "QLineEdit { background: %1; color: %2; border: 1px solid %3; border-radius: %4px; padding: 0 %5px; }"
    "QLineEdit:focus { border: 2px solid %6; }")
    .arg(hex(Token::Background), hex(Token::Text), hex(Token::Border)).arg(Size::radiusControl).arg(Size::spacingS).arg(focus));
  m_list->setStyleSheet(QStringLiteral("QListWidget { background: %1; color: %2; border: 1px solid %3; border-radius: %4px; padding: %5px; }"
                                       "QListWidget:focus { border: 2px solid %6; }")
                        .arg(hex(Token::Background), hex(Token::Text), hex(Token::Border)).arg(Size::radiusControl).arg(Size::spacingXs).arg(focus));

  const QColor text = Theme::color(Token::Text), disabled = Theme::color(Token::TextDisabled);
  const QColor accentText = Theme::color(Token::AccentText);
  m_searchIcon->setIcon(LucideIcons::icon(QStringLiteral("search"), Theme::color(Token::TextMuted), disabled));
  m_showAll->setIcon(LucideIcons::icon(QStringLiteral("library-big"), text, disabled));
  m_use->setIcon(LucideIcons::icon(QStringLiteral("import"), Theme::color(Token::OnAccent), disabled));
  m_addFigure->setStyleSheet(normal);
  m_addFigure->setIcon(LucideIcons::icon(QStringLiteral("user-plus"), text, disabled));
  m_previewButton->setIcon(LucideIcons::icon(m_preview->playing() ? QStringLiteral("square") : QStringLiteral("play"), text, disabled));
  const int icon = Size::iconSmall;
  m_explainIcon->setPixmap(LucideIcons::icon(QStringLiteral("info"), accentText, disabled).pixmap(icon, icon));
  m_noHitIcon->setPixmap(LucideIcons::icon(QStringLiteral("search"), accentText, disabled).pixmap(20, 20));
  m_missingIcon->setPixmap(LucideIcons::icon(QStringLiteral("library-big"), accentText, disabled).pixmap(20, 20));

  QPalette muted = palette();
  muted.setColor(QPalette::WindowText, Theme::color(Token::TextMuted));
  for (QLabel* label : {m_explain, m_count, m_description, m_noHitLine, m_missingLine}) label->setPalette(muted);
  for (QLabel* label : m_factNames) label->setPalette(muted);
}

QMessageBox* LibraryPanel::unsavedBox(QWidget* parent, const QString& clipName)
{
  QMessageBox* box = new QMessageBox(parent);
  box->setObjectName(QStringLiteral("libraryUnsavedBox"));
  box->setIcon(QMessageBox::NoIcon);
  box->setWindowTitle(LibrarySpec::tr(LibrarySpec::unsavedTitle).arg(clipName));
  box->setText(LibrarySpec::tr(LibrarySpec::unsavedTitle).arg(clipName));
  box->setInformativeText(LibrarySpec::tr(LibrarySpec::unsavedLine));
  QPushButton* cancel = box->addButton(LibrarySpec::tr(LibrarySpec::unsavedCancel), QMessageBox::RejectRole);
  cancel->setObjectName(QStringLiteral("libraryUnsavedCancel"));
  QPushButton* discard = box->addButton(LibrarySpec::tr(LibrarySpec::unsavedDiscard), QMessageBox::DestructiveRole);
  discard->setObjectName(QStringLiteral("libraryUnsavedDiscard"));
  QPushButton* save = box->addButton(LibrarySpec::tr(LibrarySpec::unsavedSave), QMessageBox::AcceptRole);
  save->setObjectName(QStringLiteral("libraryUnsavedSave"));
  box->setDefaultButton(save);
  box->setEscapeButton(cancel);
  return box;
}

LibraryPanel::UnsavedChoice LibraryPanel::choiceOf(QMessageBox* box)
{
  const QAbstractButton* clicked = box ? box->clickedButton() : nullptr;
  if (!clicked) return UnsavedChoice::Cancel;
  if (clicked->objectName() == QLatin1String("libraryUnsavedDiscard")) return UnsavedChoice::OpenWithoutSaving;
  if (clicked->objectName() == QLatin1String("libraryUnsavedSave")) return UnsavedChoice::SaveThenOpen;
  return UnsavedChoice::Cancel;
}

bool LibraryPanel::openClip(ControlApi* api, const QString& id, QString& message)
{
  const QJsonObject answer = api->execute(QJsonObject{{"command", QStringLiteral("load_reference")}, {"id", id}});
  if (!answer.value(QStringLiteral("ok")).toBool())
  {
    message = answer.value(QStringLiteral("error")).toString();
    return false;
  }
  const QString name = answer.value(QStringLiteral("reference")).toObject().value(QStringLiteral("name")).toString(id);
  message = LibrarySpec::tr(LibrarySpec::opened).arg(name);
  return true;
}
