/*
 * PinglyMotion Script Lab - sliders for the vision sphere and its RLV line
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <cmath>

#include <QClipboard>
#include <QColorDialog>
#include <QGridLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QPainter>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QResizeEvent>
#include <QTextBlock>
#include <QTextDocument>
#include <QTextLayout>
#include <QSignalBlocker>
#include <QSlider>
#include <QVBoxLayout>

#include "theme.h"

#include "spherecontrols.h"

namespace
{
  QString number(double value)
  {
    return QLocale::c().toString(value, 'g', 6);
  }

  // the read-only line field: as tall as its wrapped text at its width (height for width), so the whole
  // line always shows and the layout knows the height before it places the field
  class LineField : public QPlainTextEdit
  {
    public:
      explicit LineField(QWidget* parent) : QPlainTextEdit(parent)
      {
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        QSizePolicy policy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        policy.setHeightForWidth(true);
        setSizePolicy(policy);
        connect(this, &QPlainTextEdit::textChanged, this, [this]() { updateGeometry(); });
      }

      bool hasHeightForWidth() const override { return true; }

      int heightForWidth(int width) const override
      {
        const QMargins m = contentsMargins();
        const qreal margin = document()->documentMargin();
        // the text's own room, as QPlainTextEdit lays it out (a little narrower to be safe)
        const qreal room = qMax(qreal(8), width - m.left() - m.right() - 2 * margin - 2);
        QTextLayout layout(toPlainText(), font());
        QTextOption option;
        option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        layout.setTextOption(option);
        int lines = 0;
        layout.beginLayout();
        for (QTextLine line = layout.createLine(); line.isValid(); line = layout.createLine())
        {
          line.setLineWidth(room);
          ++lines;
        }
        layout.endLayout();
        lines = qMax(2, lines);
        return lines * QFontMetrics(font()).lineSpacing() + int(std::ceil(2 * margin)) + m.top() + m.bottom() + 2;
      }

      QSize sizeHint() const override
      {
        const int w = QFontMetrics(font()).horizontalAdvance(QLatin1Char('x')) * 30;
        return QSize(w, heightForWidth(width() > 0 ? width() : w));
      }

      QSize minimumSizeHint() const override
      {
        const int w = QFontMetrics(font()).horizontalAdvance(QLatin1Char('x')) * 12;
        return QSize(w, heightForWidth(width() > 0 ? width() : w));
      }

    protected:
      void resizeEvent(QResizeEvent* event) override
      {
        QPlainTextEdit::resizeEvent(event);
        if (event->oldSize().width() != event->size().width()) updateGeometry();
      }
  };

  QSlider* makeSlider(int most, int value, QWidget* parent)
  {
    QSlider* slider = new QSlider(Qt::Horizontal, parent);
    slider->setRange(0, most);
    slider->setValue(value);
    slider->setSingleStep(1);
    slider->setPageStep(SphereControls::StepsPerMetre);
    slider->setMinimumHeight(Theme::Size::controlCompact);
    slider->setFocusPolicy(Qt::StrongFocus);
    return slider;
  }
}

SphereControls::SphereControls(QWidget* parent) :
  QWidget(parent)
{
  setObjectName("scriptLabSphereControls");
  m_picker = [](const QColor& current, QWidget* parent)
  {
    return QColorDialog::getColor(current, parent, tr("Vision sphere colour"));
  };

  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(Theme::Size::spacingS);

  m_reason = new QLabel(tr("Tick Show environment in 3D view to try the sphere."), this);
  m_reason->setObjectName("scriptLabSphereReason");
  m_reason->setWordWrap(true);
  layout->addWidget(m_reason);

  QGridLayout* grid = new QGridLayout;
  grid->setHorizontalSpacing(Theme::Size::spacingS);
  grid->setVerticalSpacing(Theme::Size::spacingS);
  const int most = MostMetres * StepsPerMetre;
  m_inner = makeSlider(most, 1 * StepsPerMetre, this);
  m_inner->setObjectName("scriptLabSphereInner");
  m_outer = makeSlider(most, 4 * StepsPerMetre, this);
  m_outer->setObjectName("scriptLabSphereOuter");
  m_darkness = makeSlider(100, 100, this);
  m_darkness->setObjectName("scriptLabSphereDarkness");
  m_darkness->setPageStep(10);
  m_innerValue = new QLabel(this);
  m_innerValue->setObjectName("scriptLabSphereInnerValue");
  m_outerValue = new QLabel(this);
  m_outerValue->setObjectName("scriptLabSphereOuterValue");
  m_darknessValue = new QLabel(this);
  m_darknessValue->setObjectName("scriptLabSphereDarknessValue");
  // the value column is as wide as its widest text, so the sliders do not move while dragging
  const int valueWidth = qMax(m_innerValue->fontMetrics().horizontalAdvance(tr("%1 m").arg(QStringLiteral("20.5"))),
                              m_darknessValue->fontMetrics().horizontalAdvance(tr("%1%").arg(100)));
  for (QLabel* value : {m_innerValue, m_outerValue, m_darknessValue})
  {
    value->setMinimumWidth(valueWidth);
    value->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  }
  QLabel* innerLabel = new QLabel(tr("Inner distance"), this);
  QLabel* outerLabel = new QLabel(tr("Outer distance"), this);
  QLabel* darknessLabel = new QLabel(tr("Darkness"), this);
  QLabel* colourLabel = new QLabel(tr("Colour"), this);
  innerLabel->setBuddy(m_inner);
  outerLabel->setBuddy(m_outer);
  darknessLabel->setBuddy(m_darkness);
  m_inner->setAccessibleName(tr("Inner distance"));
  m_outer->setAccessibleName(tr("Outer distance"));
  m_darkness->setAccessibleName(tr("Darkness"));

  m_colourButton = new QPushButton(this);
  m_colourButton->setObjectName("scriptLabSphereColour");
  m_colourButton->setMinimumHeight(Theme::Size::controlDefault);
  m_colourButton->setAccessibleName(tr("Colour"));
  colourLabel->setBuddy(m_colourButton);
  QHBoxLayout* colourRow = new QHBoxLayout;
  colourRow->addWidget(m_colourButton);
  colourRow->addStretch(1);

  grid->addWidget(innerLabel, 0, 0);
  grid->addWidget(m_inner, 0, 1);
  grid->addWidget(m_innerValue, 0, 2);
  grid->addWidget(outerLabel, 1, 0);
  grid->addWidget(m_outer, 1, 1);
  grid->addWidget(m_outerValue, 1, 2);
  grid->addWidget(darknessLabel, 2, 0);
  grid->addWidget(m_darkness, 2, 1);
  grid->addWidget(m_darknessValue, 2, 2);
  grid->addWidget(colourLabel, 3, 0);
  grid->addLayout(colourRow, 3, 1, 1, 2);
  grid->setColumnStretch(1, 1);
  layout->addLayout(grid);

  // the finished line: read only, wrapped so all of it shows in a narrow panel
  m_line = new LineField(this);
  m_line->setObjectName("scriptLabSphereLine");
  m_line->setReadOnly(true);
  m_line->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
  m_line->setLineWrapMode(QPlainTextEdit::WidgetWidth);
  m_line->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
  m_line->setTabChangesFocus(true);
  m_line->setAccessibleName(tr("RLV line"));
  layout->addWidget(m_line);

  m_copy = new QPushButton(tr("Copy"), this);
  m_copy->setObjectName("scriptLabSphereCopy");
  m_copy->setMinimumHeight(Theme::Size::controlDefault);
  m_clear = new QPushButton(tr("Clear"), this);
  m_clear->setObjectName("scriptLabSphereClear");
  m_clear->setMinimumHeight(Theme::Size::controlDefault);
  QHBoxLayout* buttons = new QHBoxLayout;
  buttons->setSpacing(Theme::Size::spacingS);
  buttons->addWidget(m_copy);
  buttons->addStretch(1);
  buttons->addWidget(m_clear);
  layout->addLayout(buttons);

  m_note = new QLabel(tr("RLVa only. Paste the line into llOwnerSay in your script."), this);
  m_note->setObjectName("scriptLabSphereNote");
  m_note->setWordWrap(true);
  layout->addWidget(m_note);

  connect(m_inner, &QSlider::valueChanged, this, [this](int value)
  {
    if (value > m_outer->value())
    {
      const QSignalBlocker block(m_outer);
      m_outer->setValue(value);
    }
    changedByUser();
  });
  connect(m_outer, &QSlider::valueChanged, this, [this](int value)
  {
    if (value < m_inner->value())
    {
      const QSignalBlocker block(m_inner);
      m_inner->setValue(value);
    }
    changedByUser();
  });
  connect(m_darkness, &QSlider::valueChanged, this, [this](int) { changedByUser(); });
  connect(m_colourButton, &QPushButton::clicked, this, &SphereControls::pickColour);
  connect(m_copy, &QPushButton::clicked, this, &SphereControls::copyLine);
  connect(m_clear, &QPushButton::clicked, this, &SphereControls::clearTry);

  setTabOrder(m_inner, m_outer);
  setTabOrder(m_outer, m_darkness);
  setTabOrder(m_darkness, m_colourButton);
  setTabOrder(m_colourButton, m_line);
  setTabOrder(m_line, m_copy);
  setTabOrder(m_copy, m_clear);

  applyStyle(true);
  refresh();
}

void SphereControls::setOverride(EnvironmentOverride* environmentOverride)
{
  if (m_override) disconnect(m_override, nullptr, this, nullptr);
  m_override = environmentOverride;
  if (environmentOverride) connect(environmentOverride, &EnvironmentOverride::changed, this, &SphereControls::refresh);
  refresh();
}

void SphereControls::setAvailable(bool available)
{
  if (m_available == available) return;
  m_available = available;
  refresh();
}

double SphereControls::innerMetres() const
{
  return m_inner->value() / double(StepsPerMetre);
}

double SphereControls::outerMetres() const
{
  return m_outer->value() / double(StepsPerMetre);
}

int SphereControls::darknessPercent() const
{
  return m_darkness->value();
}

double SphereControls::colourPart(int value)
{
  return std::round(qBound(0, value, 255) * 100.0 / 255.0) / 100.0;
}

EnvironmentOverride::Sphere SphereControls::sphere() const
{
  EnvironmentOverride::Sphere s;
  s.mode = EnvironmentOverride::Sphere::ColourFade;
  s.distMin = innerMetres();
  s.distMax = outerMetres();
  s.valueMin = 0;
  s.valueMax = darknessPercent() / 100.0;
  s.params = QVector4D(float(colourPart(m_colour.red())), float(colourPart(m_colour.green())), float(colourPart(m_colour.blue())), 0.0f);
  return s;
}

QString SphereControls::lineFor(const EnvironmentOverride::Sphere& sphere)
{
  // as a script sends it: the restriction, then each value with =force
  return QStringLiteral("@setsphere=n,setsphere_mode:0=force,setsphere_distmin:%1=force,setsphere_distmax:%2=force,"
                        "setsphere_valuemin:%3=force,setsphere_valuemax:%4=force,setsphere_param:%5/%6/%7=force")
    .arg(number(sphere.distMin), number(sphere.distMax), number(sphere.valueMin), number(sphere.valueMax),
         number(double(sphere.params.x())), number(double(sphere.params.y())), number(double(sphere.params.z())));
}

QString SphereControls::rlvLine() const
{
  return lineFor(sphere());
}

void SphereControls::setColour(const QColor& colour)
{
  if (!colour.isValid()) return;
  const QColor c = colour.toRgb();
  if (QColor(c.red(), c.green(), c.blue()) == m_colour) return;
  m_colour = QColor(c.red(), c.green(), c.blue());
  changedByUser();
}

void SphereControls::changedByUser()
{
  m_copy->setText(tr("Copy"));
  if (m_available && m_override)
  {
    const bool was = m_override->hasTrySphere();
    m_override->setTrySphere(sphere());
    if (!was) emit tryingChanged(true);
  }
  refresh();
}

void SphereControls::pickColour()
{
  if (!m_picker) return;
  // Qt's own colour dialog, with the panel as its parent
  const QColor picked = m_picker(m_colour, parentWidget() ? parentWidget() : this);
  if (picked.isValid()) setColour(picked);
}

void SphereControls::copyLine()
{
  if (QClipboard* clipboard = QGuiApplication::clipboard()) clipboard->setText(rlvLine());
  m_copy->setText(tr("Copied"));   // the button says it worked (P6) until a value changes
}

void SphereControls::clearTry()
{
  if (!m_override || !m_override->hasTrySphere()) return;
  m_override->clearTrySphere();
  emit tryingChanged(false);
  refresh();
}

void SphereControls::refresh()
{
  m_innerValue->setText(tr("%1 m").arg(number(innerMetres())));
  m_outerValue->setText(tr("%1 m").arg(number(outerMetres())));
  m_darknessValue->setText(tr("%1%").arg(darknessPercent()));
  const EnvironmentOverride::Sphere s = sphere();
  const QString colourText = EnvironmentOverride::colourText(s.params.toVector3D());
  m_colourButton->setText(tr("%1...").arg(colourText));
  m_colourButton->setAccessibleDescription(tr("Colour %1. Opens the colour picker.").arg(colourText));
  const QString line = rlvLine();
  if (m_line->toPlainText() != line) m_line->setPlainText(line);

  const bool on = m_available;
  const bool trying = m_override && m_override->hasTrySphere();
  m_reason->setVisible(!on);
  for (QWidget* w : std::initializer_list<QWidget*>{m_inner, m_outer, m_darkness, m_colourButton, m_line, m_copy})
    w->setEnabled(on);
  m_clear->setEnabled(on && trying);
  const QString reason = m_reason->text();
  if (on)
  {
    m_inner->setToolTip(tr("Inside this distance the view stays clear (Left and Right arrows: half a metre)"));
    m_outer->setToolTip(tr("At this distance the darkness is full (Left and Right arrows: half a metre)"));
    m_darkness->setToolTip(tr("How dark it gets at the outer distance"));
    m_colourButton->setToolTip(tr("Pick the colour the view fades to"));
    m_line->setToolTip(tr("The line for your script, made from the values above"));
    m_copy->setToolTip(tr("Copy the line to the clipboard"));
    m_clear->setToolTip(trying ? tr("Take the sliders' sphere off the 3D view. A sphere the script holds shows again.")
                               : tr("Move a slider first: then its sphere shows in the 3D view."));
  }
  else
  {
    for (QWidget* w : std::initializer_list<QWidget*>{m_inner, m_outer, m_darkness, m_colourButton, m_line, m_copy, m_clear})
      w->setToolTip(reason);
  }
  // the swatch on the colour button
  QPixmap swatch(Theme::Size::iconSmall, Theme::Size::iconSmall);
  swatch.fill(Qt::transparent);
  {
    QPainter painter(&swatch);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(Theme::color(Theme::Token::Border, m_dark), Theme::Size::borderWidth));
    painter.setBrush(on ? m_colour : Theme::mix(m_colour, Theme::color(Theme::Token::Surface, m_dark), 0.5));
    painter.drawRoundedRect(QRectF(0.5, 0.5, swatch.width() - 1.0, swatch.height() - 1.0), Theme::Size::radiusSmall, Theme::Size::radiusSmall);
  }
  m_colourButton->setIcon(QIcon(swatch));
}

void SphereControls::applyStyle(bool dark)
{
  m_dark = dark;
  // normal buttons (DESIGN_SYSTEM.md, "Shape" and "States")
  const QString normal = QStringLiteral(
    "QPushButton { background: %1; color: %2; border: 1px solid %3; border-radius: %4px; padding: 0 %5px; }"
    "QPushButton:disabled { color: %6; }"
    "QPushButton:focus { border: 2px solid %7; }")
    .arg(Theme::color(Theme::Token::SurfaceRaised, dark).name().toUpper(), Theme::color(Theme::Token::Text, dark).name().toUpper(),
         Theme::color(Theme::Token::Border, dark).name().toUpper())
    .arg(Theme::Size::radiusControl).arg(Theme::Size::spacingM)
    .arg(Theme::color(Theme::Token::TextDisabled, dark).name().toUpper(), Theme::color(Theme::Token::FocusRing, dark).name().toUpper());
  for (QPushButton* b : {m_colourButton, m_copy, m_clear}) b->setStyleSheet(normal);
  // the sliders' groove fill is the accent (DESIGN_SYSTEM.md: accent is for sliders)
  for (QSlider* s : {m_inner, m_outer, m_darkness})
  {
    QPalette p = s->palette();
    p.setColor(QPalette::Highlight, Theme::color(Theme::Token::Accent, dark));
    s->setPalette(p);
  }
  for (QLabel* label : {m_note, m_reason})
  {
    QPalette p = label->palette();
    p.setColor(QPalette::WindowText, Theme::color(Theme::Token::TextMuted, dark));
    label->setPalette(p);
  }
  refresh();
}
