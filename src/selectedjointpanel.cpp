/*
 * PinglyMotion - the Selected joint panel (see selectedjointpanel.h)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#include "selectedjointpanel.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QSignalBlocker>
#include <QSlider>
#include <QStackedWidget>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>

#include "lucideicons.h"
#include "sliderscale.h"
#include "theme.h"
#include "usersettings.h"

namespace
{
  QString tr2(const char* text)
  {
    return QCoreApplication::translate("SelectedJointPanel", text);
  }

  // semibold, the size still follows the program's text size (only the weight is set)
  QFont labelFont()
  {
    QFont font;
    font.setWeight(QFont::DemiBold);
    return font;
  }

  // A label that breaks its text into lines. It asks only for the width of its
  // longest word, so the controls set the panel's width, and for the height its
  // lines need at the width it has (Qt's own wrapped label asks for a guessed
  // width and height that the layout then does not keep).
  class WrapLabel : public QLabel
  {
    public:
      WrapLabel(const QString& text, QWidget* parent) : QLabel(text, parent)
      {
        setWordWrap(true);
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
      }

      QSize sizeHint() const override
      {
        int word = 0;
        const QFontMetrics metrics = fontMetrics();
        for (const QString& part : text().split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts))
          word = std::max(word, metrics.horizontalAdvance(part));
        const QMargins m = contentsMargins();
        const int least = word + m.left() + m.right() + 2 * margin() + 1;
        const int wide = std::max(width(), least);
        return QSize(least, heightForWidth(wide));
      }

      // at least one line; the layout asks for the height at the real width (heightForWidth)
      QSize minimumSizeHint() const override
      {
        const QMargins m = contentsMargins();
        return QSize(sizeHint().width(), fontMetrics().height() + m.top() + m.bottom() + 2 * margin());
      }
  };

  QLabel* wrapLabel(const QString& text, QWidget* parent)
  {
    return new WrapLabel(text, parent);
  }

  Theme::Token axisToken(int axis)
  {
    return axis == 0 ? Theme::Token::AxisX : (axis == 1 ? Theme::Token::AxisY : Theme::Token::AxisZ);
  }

  QPushButton* segmentButton(const QString& text, QWidget* parent)
  {
    QPushButton* button = new QPushButton(text, parent);
    button->setCheckable(true);
    button->setAutoDefault(false);
    button->setFocusPolicy(Qt::StrongFocus);
    return button;
  }
}

NumberField::NumberField(QWidget* parent) : QLineEdit(parent)
{
  setAlignment(Qt::AlignRight | Qt::AlignVCenter);
}

QSize NumberField::sizeHint() const
{
  const QSize normal = QLineEdit::sizeHint();
  const QMargins m = textMargins();
  const int text = fontMetrics().horizontalAdvance(QStringLiteral("-000.00"));
  // the frame and the inner margins as Qt's own field has them
  const int frame = normal.width() - fontMetrics().horizontalAdvance(QLatin1Char('x')) * 17;
  return QSize(text + m.left() + m.right() + std::max(frame, Theme::Size::spacingS), normal.height());
}

QSize NumberField::minimumSizeHint() const
{
  return sizeHint();
}

QSize SelectedJointPanel::sizeHint() const
{
  QSize hint = QWidget::sizeHint();
  if (layout() && layout()->hasHeightForWidth())
    hint.setHeight(std::max(hint.height(), layout()->totalHeightForWidth(hint.width())));
  return hint;
}

SelectedJointPanel::SelectedJointPanel(QWidget* parent, const Controls* controls) : QWidget(parent)
{
  setObjectName(QStringLiteral("selectedJointPanel"));
  m_limitsOn = UserSettings::jointLimits();
  m_hintsOn = UserSettings::tipsOn();

  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  m_pages = new QStackedWidget(this);
  m_pages->setObjectName(QStringLiteral("selectedJointPages"));
  m_pages->addWidget(buildNothingPage());
  m_pages->addWidget(buildPickPage());
  m_pages->addWidget(buildJointPage(controls));
  layout->addWidget(m_pages);
  layout->addStretch(1);

  applyLook();
  showNothing();
}

QWidget* SelectedJointPanel::buildNothingPage()
{
  QWidget* page = new QWidget(this);
  page->setObjectName(QStringLiteral("selectedJointNothing"));
  QVBoxLayout* outer = new QVBoxLayout(page);
  outer->setContentsMargins(0, 0, 0, 0);
  QFrame* card = new QFrame(page);
  card->setObjectName(QStringLiteral("nothingCard"));
  m_cards << card;
  QVBoxLayout* column = new QVBoxLayout(card);
  column->setContentsMargins(Theme::Size::spacingL, Theme::Size::spacingL, Theme::Size::spacingL, Theme::Size::spacingL);
  column->setSpacing(Theme::Size::spacingS);
  QHBoxLayout* head = new QHBoxLayout;
  head->setSpacing(Theme::Size::spacingS);
  m_nothingIcon = new QLabel(card);
  m_nothingHeading = new QLabel(tr2(SelectedJointSpec::nothingHeading), card);
  m_nothingHeading->setObjectName(QStringLiteral("nothingHeading"));
  m_nothingHeading->setFont(labelFont());
  head->addWidget(m_nothingIcon);
  head->addWidget(m_nothingHeading, 1);
  column->addLayout(head);
  m_nothingLine = wrapLabel(tr2(SelectedJointSpec::nothingLine), card);
  m_nothingLine->setObjectName(QStringLiteral("nothingLine"));
  column->addWidget(m_nothingLine);
  outer->addWidget(card);
  outer->addStretch(1);
  return page;
}

QWidget* SelectedJointPanel::buildPickPage()
{
  QWidget* page = new QWidget(this);
  page->setObjectName(QStringLiteral("selectedJointPick"));
  QVBoxLayout* outer = new QVBoxLayout(page);
  outer->setContentsMargins(0, 0, 0, 0);
  QFrame* card = new QFrame(page);
  card->setObjectName(QStringLiteral("pickCard"));
  m_cards << card;
  QVBoxLayout* column = new QVBoxLayout(card);
  column->setContentsMargins(Theme::Size::spacingL, Theme::Size::spacingL, Theme::Size::spacingL, Theme::Size::spacingL);
  column->setSpacing(Theme::Size::spacingS);
  QHBoxLayout* head = new QHBoxLayout;
  head->setSpacing(Theme::Size::spacingS);
  m_pickIcon = new QLabel(card);
  m_pickHeading = new QLabel(tr2(SelectedJointSpec::pickHeading), card);
  m_pickHeading->setObjectName(QStringLiteral("pickHeading"));
  m_pickHeading->setFont(labelFont());
  head->addWidget(m_pickIcon);
  head->addWidget(m_pickHeading, 1);
  column->addLayout(head);
  m_pickLine = wrapLabel(tr2(SelectedJointSpec::pickLine), card);
  m_pickLine->setObjectName(QStringLiteral("pickLine"));
  column->addWidget(m_pickLine);
  int number = 1;
  for (const char* step : SelectedJointSpec::pickSteps)
  {
    QLabel* label = wrapLabel(QStringLiteral("%1. %2").arg(number).arg(tr2(step)), card);
    label->setObjectName(QStringLiteral("pickStep%1").arg(number));
    m_pickSteps << label;
    column->addWidget(label);
    ++number;
  }
  m_pickDemo = new QPushButton(tr2(SelectedJointSpec::pickDemo), card);
  m_pickDemo->setObjectName(QStringLiteral("pickDemoButton"));
  m_pickDemo->setAutoDefault(false);
  m_pickDemo->setMinimumHeight(Theme::Size::controlLarge);
  m_pickDemo->setProperty("main", true);
  connect(m_pickDemo, &QPushButton::clicked, this, &SelectedJointPanel::pickDemoRequested);
  column->addWidget(m_pickDemo);
  outer->addWidget(card);
  outer->addStretch(1);
  return page;
}

QWidget* SelectedJointPanel::axisRow(int axis, QSlider*& slider, QLineEdit*& field, QSlider* given,
                                     QLineEdit* givenField, Mode mode)
{
  const SelectedJointSpec::Axis& spec = SelectedJointSpec::axes[axis];
  const bool turn = mode == Mode::Turn;
  QWidget* row = new QWidget(this);
  row->setObjectName(QStringLiteral("%1Row%2").arg(turn ? QStringLiteral("turn") : QStringLiteral("move")).arg(QLatin1String(spec.letter)));
  // the badge, the name and the number on top, the slider under them, the range line last
  QVBoxLayout* rows = new QVBoxLayout(row);
  rows->setContentsMargins(0, 0, 0, 0);
  rows->setSpacing(Theme::Size::spacingXs);

  QLabel* badge = new QLabel(QLatin1String(spec.letter), row);
  badge->setObjectName(QStringLiteral("axisBadge%1").arg(QLatin1String(spec.letter)));
  badge->setAlignment(Qt::AlignCenter);
  badge->setFont(labelFont());
  badge->setMinimumWidth(Theme::Size::iconSmall + Theme::Size::spacingXs);
  QLabel* name = wrapLabel(tr2(turn ? spec.turnName : spec.moveName), row);
  name->setObjectName(QStringLiteral("axisName%1").arg(QLatin1String(spec.letter)));
  name->setFont(labelFont());

  slider = given ? given : new QSlider(Qt::Horizontal, row);
  slider->setParent(row);
  slider->setOrientation(Qt::Horizontal);
  slider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  // the arrow keys turn one degree (or move one unit), Page Up / Page Down ten
  slider->setSingleStep(SliderScale::precision);
  slider->setPageStep(10 * SliderScale::precision);
  slider->setFocusPolicy(Qt::StrongFocus);
  // room for the round handle (control_height.compact)
  slider->setMinimumHeight(Theme::Size::controlCompact);
  if (!given) slider->setRange(-180 * SliderScale::precision, 180 * SliderScale::precision);
  const QString what = tr2(turn ? spec.turnName : spec.moveName);
  slider->setAccessibleName(QStringLiteral("%1: %2").arg(QLatin1String(spec.letter), what));
  slider->setProperty("reasonName", slider->accessibleName());
  field = givenField ? givenField : new NumberField(row);
  field->setParent(row);
  field->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  field->setAccessibleName(turn ? QCoreApplication::translate("SelectedJointPanel", "%1, %2, in degrees").arg(QLatin1String(spec.letter), what)
                                : QStringLiteral("%1, %2").arg(QLatin1String(spec.letter), what));
  field->setProperty("reasonName", field->accessibleName());
  if (!givenField) field->setText(QStringLiteral("0"));
  QLabel* unit = new QLabel(turn ? QStringLiteral("°") : QString(), row);

  QHBoxLayout* top = new QHBoxLayout;
  top->setSpacing(Theme::Size::spacingS);
  top->addWidget(badge);
  top->addWidget(name, 1);
  rows->addLayout(top);
  QHBoxLayout* numberRow = new QHBoxLayout;
  numberRow->setSpacing(2);
  numberRow->addWidget(field);
  numberRow->addWidget(unit);
  top->addLayout(numberRow);
  rows->addWidget(slider);

  QHBoxLayout* rangeRow = new QHBoxLayout;
  rangeRow->setContentsMargins(0, 0, 0, 0);
  QLabel** range = turn ? m_range[axis] : m_moveRange[axis];
  for (int i = 0; i < 3; ++i)
  {
    range[i] = new QLabel(row);
    range[i]->setObjectName(QStringLiteral("range%1%2").arg(QLatin1String(spec.letter)).arg(i));
    range[i]->setAlignment(i == 0 ? Qt::AlignLeft : (i == 1 ? Qt::AlignHCenter : Qt::AlignRight));
    rangeRow->addWidget(range[i], i == 1 ? 1 : 0);
  }
  rows->addLayout(rangeRow);

  connect(slider, &QSlider::rangeChanged, this, [this]() { refresh(); });
  connect(slider, &QSlider::valueChanged, this, [this]() { refresh(); });
  // without the window's code behind it (the tests), the slider and its field follow each other
  if (!given && !givenField)
  {
    QSlider* s = slider;
    QLineEdit* f = field;
    connect(s, &QSlider::valueChanged, f, [f](int value) { f->setText(SelectedJointPanel::number(SliderScale::toValue(value))); });
    connect(f, &QLineEdit::editingFinished, s, [s, f]() { s->setValue(SliderScale::toSlider(f->text().toDouble())); });
  }

  if (turn)
  {
    m_badge[axis] = badge;
    m_axisName[axis] = name;
    m_unit[axis] = unit;
  }
  else
  {
    m_moveBadge[axis] = badge;
    m_moveName[axis] = name;
  }
  slider->setToolTip(QStringLiteral("%1 (%2): %3\n%4").arg(QLatin1String(spec.letter), tr2(spec.ring), what,
                                                           tr2(SelectedJointSpec::sliderKeys)));
  return row;
}

QWidget* SelectedJointPanel::buildJointPage(const Controls* controls)
{
  QWidget* page = new QWidget(this);
  page->setObjectName(QStringLiteral("selectedJointPage"));
  QVBoxLayout* column = new QVBoxLayout(page);
  column->setContentsMargins(0, 0, 0, 0);
  column->setSpacing(Theme::Size::spacingM);

  // the joint's name, body area, key count and Second Life name
  QFrame* head = new QFrame(page);
  head->setObjectName(QStringLiteral("jointHead"));
  QVBoxLayout* headColumn = new QVBoxLayout(head);
  headColumn->setContentsMargins(Theme::Size::spacingM, Theme::Size::spacingXs, 0, Theme::Size::spacingXs);
  headColumn->setSpacing(Theme::Size::spacingXs);
  m_name = wrapLabel(QString(), head);
  m_name->setObjectName(QStringLiteral("jointName"));
  m_name->setFont(labelFont());
  headColumn->addWidget(m_name);
  QHBoxLayout* chips = new QHBoxLayout;
  chips->setSpacing(Theme::Size::spacingXs);
  m_area = new QLabel(head);
  m_area->setObjectName(QStringLiteral("areaChip"));
  m_keys = new QLabel(head);
  m_keys->setObjectName(QStringLiteral("keysChip"));
  chips->addWidget(m_area);
  chips->addWidget(m_keys);
  chips->addStretch(1);
  headColumn->addLayout(chips);
  m_slName = wrapLabel(QString(), head);
  m_slName->setObjectName(QStringLiteral("slName"));
  headColumn->addWidget(m_slName);
  column->addWidget(head);

  m_hint = wrapLabel(tr2(SelectedJointSpec::hint), page);
  m_hint->setObjectName(QStringLiteral("jointHint"));
  column->addWidget(m_hint);

  // Turn (or Move): the three axis rows, the warning and the limits switch
  m_section = new QLabel(tr2(SelectedJointSpec::turnHeading), page);
  m_section->setObjectName(QStringLiteral("sectionHeading"));
  m_section->setFont(labelFont());
  column->addWidget(m_section);
  m_turnRows = new QWidget(page);
  m_turnRows->setObjectName(QStringLiteral("turnRows"));
  QVBoxLayout* turnColumn = new QVBoxLayout(m_turnRows);
  turnColumn->setContentsMargins(0, 0, 0, 0);
  turnColumn->setSpacing(Theme::Size::spacingM);
  m_moveRows = new QWidget(page);
  m_moveRows->setObjectName(QStringLiteral("moveRows"));
  QVBoxLayout* moveColumn = new QVBoxLayout(m_moveRows);
  moveColumn->setContentsMargins(0, 0, 0, 0);
  moveColumn->setSpacing(Theme::Size::spacingM);
  for (int axis = 0; axis < 3; ++axis)
  {
    turnColumn->addWidget(axisRow(axis, m_turn[axis], m_turnValue[axis], controls ? controls->turn[axis] : nullptr,
                                  controls ? controls->turnValue[axis] : nullptr, Mode::Turn));
    moveColumn->addWidget(axisRow(axis, m_move[axis], m_moveValue[axis], controls ? controls->move[axis] : nullptr,
                                  controls ? controls->moveValue[axis] : nullptr, Mode::Move));
  }
  column->addWidget(m_turnRows);
  column->addWidget(m_moveRows);

  m_warning = new QFrame(page);
  m_warning->setObjectName(QStringLiteral("limitWarning"));
  m_warning->setAccessibleName(tr2(SelectedJointSpec::nearTitle).arg(QString()));
  QHBoxLayout* warningRow = new QHBoxLayout(m_warning);
  warningRow->setContentsMargins(Theme::Size::spacingS, Theme::Size::spacingS, Theme::Size::spacingS, Theme::Size::spacingS);
  warningRow->setSpacing(Theme::Size::spacingS);
  m_warningIcon = new QLabel(m_warning);
  warningRow->addWidget(m_warningIcon, 0, Qt::AlignTop);
  QVBoxLayout* warningColumn = new QVBoxLayout;
  warningColumn->setSpacing(Theme::Size::spacingXs);
  m_warningTitle = wrapLabel(QString(), m_warning);
  m_warningTitle->setObjectName(QStringLiteral("limitWarningTitle"));
  m_warningTitle->setFont(labelFont());
  m_warningText = wrapLabel(tr2(SelectedJointSpec::nearText), m_warning);
  m_warningText->setObjectName(QStringLiteral("limitWarningText"));
  m_limitsOff = new QPushButton(tr2(SelectedJointSpec::turnLimitsOff), m_warning);
  m_limitsOff->setObjectName(QStringLiteral("turnLimitsOffButton"));
  m_limitsOff->setAutoDefault(false);
  warningColumn->addWidget(m_warningTitle);
  warningColumn->addWidget(m_warningText);
  warningColumn->addWidget(m_limitsOff, 0, Qt::AlignLeft);
  warningRow->addLayout(warningColumn, 1);
  column->addWidget(m_warning);
  connect(m_limitsOff, &QPushButton::clicked, this, [this]()
  {
    setLimitsOn(false);
    // the focus was on the button that is gone now: the switch says what happened
    m_limitsSwitch->setFocus();
    emit limitsToggled(false);
  });

  m_limitsSwitch = new QCheckBox(tr2(SelectedJointSpec::limitsSwitch), page);
  m_limitsSwitch->setObjectName(QStringLiteral("limitsSwitch"));
  m_limitsSwitch->setToolTip(tr2(SelectedJointSpec::limitsTip));
  m_limitsSwitch->setChecked(m_limitsOn);
  column->addWidget(m_limitsSwitch);
  connect(m_limitsSwitch, &QCheckBox::toggled, this, [this](bool on)
  {
    if (on == m_limitsOn) return;
    setLimitsOn(on);
    emit limitsToggled(on);
  });

  // Reset joint and Pull with keys
  QHBoxLayout* buttons = new QHBoxLayout;
  buttons->setSpacing(Theme::Size::spacingS);
  m_reset = new QPushButton(tr2(SelectedJointSpec::resetJoint), page);
  m_reset->setObjectName(QStringLiteral("resetJointButton"));
  m_reset->setAutoDefault(false);
  connect(m_reset, &QPushButton::clicked, this, &SelectedJointPanel::resetRequested);
  m_pullKeys = new QPushButton(tr2(SelectedJointSpec::pullKeys), page);
  m_pullKeys->setObjectName(QStringLiteral("pullKeysButton"));
  m_pullKeys->setAutoDefault(false);
  connect(m_pullKeys, &QPushButton::clicked, this, &SelectedJointPanel::pullKeysRequested);
  buttons->addWidget(m_reset);
  buttons->addWidget(m_pullKeys);
  buttons->addStretch(1);
  column->addLayout(buttons);

  // what a drag in the 3D view does
  m_dragHeading = wrapLabel(tr2(SelectedJointSpec::dragHeading), page);
  m_dragHeading->setObjectName(QStringLiteral("dragHeading"));
  m_dragHeading->setFont(labelFont());
  column->addWidget(m_dragHeading);
  QHBoxLayout* drag = new QHBoxLayout;
  drag->setSpacing(0);
  m_pullLimb = segmentButton(tr2(SelectedJointSpec::pullLimb), page);
  m_pullLimb->setObjectName(QStringLiteral("pullLimbButton"));
  m_turnOnly = segmentButton(tr2(SelectedJointSpec::turnOnly), page);
  m_turnOnly->setObjectName(QStringLiteral("turnOnlyButton"));
  m_dragGroup = new QButtonGroup(this);
  m_dragGroup->setExclusive(true);
  m_dragGroup->addButton(m_pullLimb);
  m_dragGroup->addButton(m_turnOnly);
  (UserSettings::pullLimbOnDrag() ? m_pullLimb : m_turnOnly)->setChecked(true);
  connect(m_pullLimb, &QPushButton::toggled, this, [this](bool on)
  {
    UserSettings::setPullLimbOnDrag(on);
    m_dragCaption->setText(dragCaptionText());
    emit dragChoiceChanged(on);
  });
  m_pullLimb->setToolTip(QStringLiteral("%1 (%2)").arg(tr2(SelectedJointSpec::pullLimbName), tr2(SelectedJointSpec::pullLimbKey)));
  m_turnOnly->setToolTip(tr2(SelectedJointSpec::turnOnly));
  drag->addWidget(m_pullLimb);
  drag->addWidget(m_turnOnly);
  drag->addStretch(1);
  column->addLayout(drag);
  m_dragCaption = wrapLabel(dragCaptionText(), page);
  m_dragCaption->setObjectName(QStringLiteral("dragCaption"));
  column->addWidget(m_dragCaption);

  // More options: Turn or Move
  QHBoxLayout* moreRow = new QHBoxLayout;
  m_more = new QToolButton(page);
  m_more->setObjectName(QStringLiteral("moreOptionsButton"));
  m_more->setText(tr2(SelectedJointSpec::moreOptions));
  m_more->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  m_more->setCheckable(true);
  m_more->setAutoRaise(true);
  m_more->setFocusPolicy(Qt::StrongFocus);
  m_more->setFont(labelFont());
  m_moreCaption = new QLabel(tr2(SelectedJointSpec::moreCaption), page);
  m_moreCaption->setObjectName(QStringLiteral("moreCaption"));
  moreRow->addWidget(m_more);
  moreRow->addStretch(1);
  moreRow->addWidget(m_moreCaption);
  column->addLayout(moreRow);
  m_moreBody = new QWidget(page);
  m_moreBody->setObjectName(QStringLiteral("moreBody"));
  QVBoxLayout* moreColumn = new QVBoxLayout(m_moreBody);
  moreColumn->setContentsMargins(0, 0, 0, 0);
  moreColumn->setSpacing(Theme::Size::spacingXs);
  m_changeLabel = new QLabel(tr2(SelectedJointSpec::changeLabel), m_moreBody);
  m_changeLabel->setObjectName(QStringLiteral("changeLabel"));
  m_changeLabel->setFont(labelFont());
  moreColumn->addWidget(m_changeLabel);
  QHBoxLayout* modes = new QHBoxLayout;
  modes->setSpacing(0);
  m_turnMode = segmentButton(tr2(SelectedJointSpec::turnMode), m_moreBody);
  m_turnMode->setObjectName(QStringLiteral("turnModeButton"));
  m_moveMode = segmentButton(tr2(SelectedJointSpec::moveMode), m_moreBody);
  m_moveMode->setObjectName(QStringLiteral("moveModeButton"));
  m_modeGroup = new QButtonGroup(this);
  m_modeGroup->setExclusive(true);
  m_modeGroup->addButton(m_turnMode);
  m_modeGroup->addButton(m_moveMode);
  m_turnMode->setChecked(true);
  modes->addWidget(m_turnMode);
  modes->addWidget(m_moveMode);
  modes->addStretch(1);
  moreColumn->addLayout(modes);
  column->addWidget(m_moreBody);
  connect(m_turnMode, &QPushButton::clicked, this, [this]() { setMode(Mode::Turn); });
  connect(m_moveMode, &QPushButton::clicked, this, [this]() { setMode(Mode::Move); });
  connect(m_more, &QToolButton::toggled, this, [this](bool open)
  {
    UserSettings::setJointMoreOptions(open);
    m_moreBody->setVisible(open);
    m_more->setIcon(LucideIcons::icon(open ? QStringLiteral("chevron-down") : QStringLiteral("chevron-right"),
                                      Theme::color(Theme::Token::Text), Theme::color(Theme::Token::TextDisabled)));
  });
  m_more->setChecked(UserSettings::jointMoreOptions());
  m_moreBody->setVisible(m_more->isChecked());

  m_moveRows->hide();
  return page;
}

void SelectedJointPanel::showNothing()
{
  m_page = Page::Nothing;
  m_pages->setCurrentIndex(0);
}

void SelectedJointPanel::showPickJoint()
{
  m_page = Page::PickJoint;
  m_pages->setCurrentIndex(1);
}

bool SelectedJointPanel::Joint::operator==(const Joint& other) const
{
  return name == other.name && slName == other.slName && area == other.area && keys == other.keys &&
         canTurn == other.canTurn && canMove == other.canMove && changed == other.changed;
}

void SelectedJointPanel::showJoint(const Joint& joint)
{
  // the window asks often (every slider step): nothing to do when nothing changed
  if (m_page == Page::Joint && joint == m_joint) return;
  const bool sameLook = m_page == Page::Joint && joint.area == m_joint.area;
  m_joint = joint;
  m_page = Page::Joint;
  m_pages->setCurrentIndex(2);
  m_name->setText(joint.name);
  const int area = (joint.area >= 0 && joint.area < 4) ? joint.area : 3;
  m_area->setText(tr2(SelectedJointSpec::areas[area]));
  m_keys->setText(joint.keys == 0 ? tr2(SelectedJointSpec::noKeys)
                  : (joint.keys == 1 ? tr2(SelectedJointSpec::oneKey) : tr2(SelectedJointSpec::manyKeys).arg(joint.keys)));
  m_slName->setText(tr2(SelectedJointSpec::slName).arg(joint.slName));
  m_slName->setVisible(!joint.slName.isEmpty());
  // a joint that cannot move shows Turn, the Avatar Position shows Move
  if (!joint.canMove) m_mode = Mode::Turn;
  else if (!joint.canTurn) m_mode = Mode::Move;
  {
    QSignalBlocker blockTurn(m_turnMode);
    QSignalBlocker blockMove(m_moveMode);
    (m_mode == Mode::Turn ? m_turnMode : m_moveMode)->setChecked(true);
  }
  if (!sameLook) applyLook();
  refreshButtons();
  refresh();
}

void SelectedJointPanel::setTurnBlocked(const QString& why)
{
  if (why == m_turnBlocked && m_reasonsSet) return;
  m_reasonsSet = true;
  m_turnBlocked = why;
  refreshButtons();
}

void SelectedJointPanel::setMoveBlocked(const QString& why)
{
  if (why == m_moveBlocked && m_reasonsSet) return;
  m_reasonsSet = true;
  m_moveBlocked = why;
  refreshButtons();
}

void SelectedJointPanel::setLimitsOn(bool on)
{
  m_limitsOn = on;
  {
    QSignalBlocker block(m_limitsSwitch);
    m_limitsSwitch->setChecked(on);
  }
  refresh();
}

void SelectedJointPanel::setMode(Mode mode)
{
  if (mode == Mode::Move && !m_joint.canMove) mode = Mode::Turn;
  if (mode == Mode::Turn && !m_joint.canTurn) mode = Mode::Move;
  {
    QSignalBlocker blockTurn(m_turnMode);
    QSignalBlocker blockMove(m_moveMode);
    (mode == Mode::Turn ? m_turnMode : m_moveMode)->setChecked(true);
  }
  if (mode == m_mode) return;
  m_mode = mode;
  refreshButtons();
  refresh();
  emit modeChanged(mode);
}

QString SelectedJointPanel::dragCaptionText() const
{
  const bool pull = m_pullLimb ? m_pullLimb->isChecked() : UserSettings::pullLimbOnDrag();
  return tr2(pull ? SelectedJointSpec::dragCaptionPull : SelectedJointSpec::dragCaptionTurn) + QLatin1Char(' ') +
         tr2(SelectedJointSpec::dragCaptionShift);
}

bool SelectedJointPanel::pullLimb() const
{
  return m_pullLimb->isChecked();
}

void SelectedJointPanel::setHintsOn(bool on)
{
  m_hintsOn = on;
  m_hint->setVisible(on);
}

bool SelectedJointPanel::moreOpen() const
{
  return m_more->isChecked();
}

void SelectedJointPanel::setMoreOpen(bool open)
{
  m_more->setChecked(open);
}

QString SelectedJointPanel::number(double value)
{
  // whole numbers without decimals, others with up to two ("-12.5")
  const double rounded = std::round(value * 100.0) / 100.0;
  QString text = QString::number(rounded, 'f', 2);
  while (text.contains(QLatin1Char('.')) && (text.endsWith(QLatin1Char('0')) || text.endsWith(QLatin1Char('.'))))
    text.chop(1);
  if (text == QStringLiteral("-0")) text = QStringLiteral("0");
  return text;
}

QStringList SelectedJointPanel::rangeTexts(int axis) const
{
  return rangeTextsFor(axis, m_mode);
}

QStringList SelectedJointPanel::rangeTextsFor(int axis, Mode mode) const
{
  const bool turn = mode == Mode::Turn;
  const QSlider* slider = turn ? m_turn[axis] : m_move[axis];
  const QString unit = turn ? QStringLiteral("°") : QString();
  const QString middle = (!turn || m_limitsOn) ? tr2(SelectedJointSpec::allowedRange) : tr2(SelectedJointSpec::noLimits);
  return {number(SliderScale::toValue(slider->minimum())) + unit, middle, number(SliderScale::toValue(slider->maximum())) + unit};
}

QList<int> SelectedJointPanel::nearAxes() const
{
  QList<int> near;
  // only turning has limits, and only while they are on
  if (m_page != Page::Joint || m_mode != Mode::Turn || !m_limitsOn || !m_joint.canTurn) return near;
  for (int axis = 0; axis < 3; ++axis)
  {
    // in whole slider steps, so the edge is exact
    const qint64 low = m_turn[axis]->minimum();
    const qint64 high = m_turn[axis]->maximum();
    const qint64 value = m_turn[axis]->value();
    const qint64 span = high - low;
    // a whole turn (the pelvis) has no end to be near
    if (span <= 0 || span >= 360 * SliderScale::precision) continue;
    if ((value - low) * 100 < span * SelectedJointSpec::nearPercent || (high - value) * 100 < span * SelectedJointSpec::nearPercent)
      near << axis;
  }
  return near;
}

void SelectedJointPanel::refresh()
{
  if (!m_turnRows) return;
  const bool turn = m_mode == Mode::Turn;
  m_turnRows->setVisible(turn);
  m_moveRows->setVisible(!turn);
  m_section->setText(tr2(turn ? SelectedJointSpec::turnHeading : SelectedJointSpec::moveHeading));
  m_limitsSwitch->setVisible(turn);

  const QList<int> near = nearAxes();
  QStringList names;
  for (int axis = 0; axis < 3; ++axis)
  {
    const SelectedJointSpec::Axis& spec = SelectedJointSpec::axes[axis];
    const bool isNear = near.contains(axis);
    m_axisName[axis]->setText(tr2(spec.turnName) + (isNear ? tr2(SelectedJointSpec::nearSuffix) : QString()));
    if (isNear) names << QStringLiteral("%1 (%2)").arg(QLatin1String(spec.letter), tr2(spec.turnName).toLower());
    const Mode modes[] = {Mode::Turn, Mode::Move};
    for (Mode mode : modes)
    {
      const QStringList texts = rangeTextsFor(axis, mode);
      QLabel** range = mode == Mode::Turn ? m_range[axis] : m_moveRange[axis];
      for (int i = 0; i < 3; ++i) range[i]->setText(texts[i]);
    }
  }
  m_warning->setVisible(!near.isEmpty());
  m_warningTitle->setText(tr2(SelectedJointSpec::nearTitle).arg(names.join(QStringLiteral(", "))));
  // only the names change colour here; a new style sheet on a slider while it is dragged would disturb it
  if (near != m_lastNear)
  {
    m_lastNear = near;
    applyNearLook();
  }
}

void SelectedJointPanel::applyNearLook()
{
  const QColor warning = Theme::color(Theme::Token::Warning);
  for (int axis = 0; axis < 3; ++axis)
    m_axisName[axis]->setStyleSheet(m_lastNear.contains(axis) ? QStringLiteral("color: %1;").arg(warning.name()) : QString());
}

void SelectedJointPanel::setReason(QWidget* widget, bool enabled, const QString& tip, const QString& why)
{
  widget->setEnabled(enabled);
  // a greyed control says why in its tip and in the status line (charter P6, src/reasonline.h)
  widget->setToolTip(enabled ? tip : why);
  widget->setStatusTip(enabled ? QString() : why);
}

void SelectedJointPanel::refreshButtons()
{
  if (!m_reset) return;
  const bool turn = m_mode == Mode::Turn;
  for (int axis = 0; axis < 3; ++axis)
  {
    const SelectedJointSpec::Axis& spec = SelectedJointSpec::axes[axis];
    const QString keys = tr2(SelectedJointSpec::sliderKeys);
    const QString turnTip = QStringLiteral("%1 (%2): %3\n%4").arg(QLatin1String(spec.letter), tr2(spec.ring), tr2(spec.turnName), keys);
    const QString moveTip = QStringLiteral("%1: %2\n%3").arg(QLatin1String(spec.letter), tr2(spec.moveName), keys);
    setReason(m_turn[axis], m_turnBlocked.isEmpty(), turnTip, m_turnBlocked);
    setReason(m_turnValue[axis], m_turnBlocked.isEmpty(), QString(), m_turnBlocked);
    setReason(m_move[axis], m_moveBlocked.isEmpty(), moveTip, m_moveBlocked);
    setReason(m_moveValue[axis], m_moveBlocked.isEmpty(), QString(), m_moveBlocked);
  }
  const QString blocked = turn ? m_turnBlocked : m_moveBlocked;
  if (!blocked.isEmpty())
    setReason(m_reset, false, QString(), blocked);
  else
    setReason(m_reset, m_joint.changed, tr2(SelectedJointSpec::resetTip), tr2(SelectedJointSpec::resetWhy));
  // Pull with keys turns joints: greyed like the sliders, and for the Avatar Position
  const QString pullTip = QStringLiteral("%1 (%2)\n%3").arg(tr2(SelectedJointSpec::pullKeysName), tr2(SelectedJointSpec::pullKeysKey), tr2(SelectedJointSpec::pullKeysTip));
  if (!m_turnBlocked.isEmpty())
    setReason(m_pullKeys, false, QString(), m_turnBlocked);
  else
    setReason(m_pullKeys, m_joint.canTurn, pullTip, tr2(SelectedJointSpec::turnWhy));
  setReason(m_moveMode, m_joint.canMove, QString(), tr2(SelectedJointSpec::moveWhy));
  setReason(m_turnMode, m_joint.canTurn, QString(), tr2(SelectedJointSpec::turnWhy));
}

void SelectedJointPanel::applyLook()
{
  using Theme::Token;
  const QColor text = Theme::color(Token::Text);
  const QColor muted = Theme::color(Token::TextMuted);
  const QColor disabled = Theme::color(Token::TextDisabled);
  const QColor border = Theme::color(Token::Border);
  const QColor surface = Theme::color(Token::Surface);
  const QColor accent = Theme::color(Token::Accent);
  const QColor accentText = Theme::color(Token::AccentText);
  const QColor warning = Theme::color(Token::Warning);
  const int icon = Theme::Size::iconSmall;

  // empty cards: a dashed border, panel radius, the icon in the accent text colour
  for (QFrame* card : m_cards)
    card->setStyleSheet(QStringLiteral("QFrame#%1 { border: %2px dashed %3; border-radius: %4px; }")
                          .arg(card->objectName()).arg(Theme::Size::borderWidth).arg(border.name()).arg(Theme::Size::radiusPanel));
  if (m_nothingIcon)
    m_nothingIcon->setPixmap(LucideIcons::icon(QStringLiteral("rotate-3d"), accentText, disabled).pixmap(20, 20));
  if (m_pickIcon)
    m_pickIcon->setPixmap(LucideIcons::icon(QStringLiteral("mouse-pointer-2"), accentText, disabled).pixmap(20, 20));
  for (QLabel* label : QList<QLabel*>{m_nothingLine, m_pickLine} + m_pickSteps)
    if (label) label->setStyleSheet(QStringLiteral("color: %1;").arg(muted.name()));
  if (m_pickDemo)
  {
    m_pickDemo->setIcon(LucideIcons::icon(QStringLiteral("mouse-pointer-2"), Theme::color(Token::OnAccent), disabled));
    QPalette palette = m_pickDemo->palette();
    palette.setColor(QPalette::Button, accent);
    palette.setColor(QPalette::ButtonText, Theme::color(Token::OnAccent));
    m_pickDemo->setPalette(palette);
  }
  if (!m_name) return;

  // the joint's head: a bar in the accent text colour on the left
  if (QFrame* head = findChild<QFrame*>(QStringLiteral("jointHead")))
    head->setStyleSheet(QStringLiteral("QFrame#jointHead { border-left: 4px solid %1; }").arg(accentText.name()));
  const Token areaTokens[] = {Token::AreaBody, Token::AreaFace, Token::AreaHands, Token::AreaOther};
  const QColor area = Theme::color(areaTokens[(m_joint.area >= 0 && m_joint.area < 4) ? m_joint.area : 3]);
  const QString chip = QStringLiteral("QLabel { border: %1px solid %2; border-radius: %3px; padding: 0px %4px; color: %5; background: %6; }");
  m_area->setStyleSheet(chip.arg(Theme::Size::borderWidth).arg(area.name()).arg(Theme::Size::radiusSmall).arg(Theme::Size::spacingS)
                          .arg(text.name(), Theme::mix(surface, area, 0.14).name()));
  m_keys->setStyleSheet(chip.arg(Theme::Size::borderWidth).arg(border.name()).arg(Theme::Size::radiusSmall).arg(Theme::Size::spacingS)
                          .arg(text.name(), Theme::mix(surface, border, 0.14).name()));
  for (QLabel* label : {m_slName, m_hint, m_dragCaption, m_moreCaption})
    label->setStyleSheet(QStringLiteral("color: %1;").arg(muted.name()));

  // the axis badges: the axis colour as fill, the letter in on_axis (DESIGN_SYSTEM.md, "Axis colours")
  for (int axis = 0; axis < 3; ++axis)
  {
    const QColor colour = Theme::color(axisToken(axis));
    const QString badge = QStringLiteral("QLabel { background: %1; color: %2; border-radius: %3px; padding: 0px %4px; }")
                            .arg(colour.name(), Theme::color(Token::OnAxis).name()).arg(Theme::Size::radiusSmall).arg(Theme::Size::spacingXs);
    m_badge[axis]->setStyleSheet(badge);
    m_moveBadge[axis]->setStyleSheet(badge);
    // the slider's filled part in the axis colour
    // Qt draws a style sheet's slider parts only when the groove and the handle are styled too
    const QString groove = QString::number(Theme::Size::spacingXs);
    const QString handle = QString::number(Theme::Size::iconSmall);
    const QString slider =
      QStringLiteral("QSlider::groove:horizontal { height: %1px; background: %2; border-radius: %3px; }")
        .arg(groove, border.name(), QString::number(Theme::Size::spacingXs / 2)) +
      QStringLiteral("QSlider::sub-page:horizontal { background: %1; border-radius: %2px; }")
        .arg(colour.name(), QString::number(Theme::Size::spacingXs / 2)) +
      QStringLiteral("QSlider::handle:horizontal { background: %1; border: %2px solid %3; width: %4px; margin: -%5px 0px; border-radius: %6px; }")
        .arg(Theme::color(Token::SurfaceRaised).name(), QString::number(Theme::Size::focusRingWidth), colour.name(), handle,
             QString::number((Theme::Size::iconSmall - Theme::Size::spacingXs) / 2), QString::number(Theme::Size::iconSmall / 2)) +
      // keyboard focus: the handle's ring in the focus colour (charter P8)
      QStringLiteral("QSlider::handle:horizontal:focus { border-color: %1; }").arg(Theme::color(Token::FocusRing).name()) +
      QStringLiteral("QSlider::sub-page:horizontal:disabled { background: %1; } QSlider::handle:horizontal:disabled { border-color: %1; }")
        .arg(disabled.name());
    m_turn[axis]->setStyleSheet(slider);
    m_move[axis]->setStyleSheet(slider);
    for (int i = 0; i < 3; ++i)
    {
      m_range[axis][i]->setStyleSheet(QStringLiteral("color: %1;").arg(muted.name()));
      m_moveRange[axis][i]->setStyleSheet(QStringLiteral("color: %1;").arg(muted.name()));
    }
  }

  // the warning card: a warning border and a light warning fill, the text in the text colour
  m_warning->setStyleSheet(QStringLiteral("QFrame#limitWarning { border: %1px solid %2; border-radius: %3px; background: %4; }")
                             .arg(Theme::Size::borderWidth).arg(warning.name()).arg(Theme::Size::radiusControl)
                             .arg(Theme::mix(surface, warning, 0.10).name()));
  m_warningIcon->setPixmap(LucideIcons::icon(QStringLiteral("triangle-alert"), warning, disabled).pixmap(icon, icon));
  m_warningTitle->setStyleSheet(QStringLiteral("color: %1;").arg(text.name()));
  m_warningText->setStyleSheet(QStringLiteral("color: %1;").arg(text.name()));

  m_reset->setIcon(LucideIcons::icon(QStringLiteral("rotate-ccw"), text, disabled));
  m_pullKeys->setIcon(LucideIcons::icon(QStringLiteral("move-3d"), text, disabled));
  m_pullLimb->setIcon(LucideIcons::icon(QStringLiteral("move-3d"), text, disabled));
  m_turnOnly->setIcon(LucideIcons::icon(QStringLiteral("rotate-3d"), text, disabled));
  m_turnMode->setIcon(LucideIcons::icon(QStringLiteral("rotate-3d"), text, disabled));
  m_moveMode->setIcon(LucideIcons::icon(QStringLiteral("move-3d"), text, disabled));
  m_keys->setToolTip(QString());
  m_more->setIcon(LucideIcons::icon(m_more->isChecked() ? QStringLiteral("chevron-down") : QStringLiteral("chevron-right"), text, disabled));
  applyNearLook();
}
