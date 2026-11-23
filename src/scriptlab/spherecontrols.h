/*
 * PinglyMotion Script Lab - sliders for the vision sphere and its RLV line
 *
 * Part 2 of "show @setsphere live in the 3D view" (part 1:
 * environmentoverride.h). The body of the Script Lab panel's "Vision sphere"
 * part: sliders for the inner and outer distance and the darkness, a colour
 * button (Qt's own colour dialog), the finished RLV line in a read-only field
 * with a Copy button, and Clear. Moving a control sets the same
 * EnvironmentOverride::Sphere the script's @setsphere sets (setTrySphere), so
 * the 3D view changes at once while the "Show environment in 3D view" box is
 * ticked and the panel is shown. With the box off every control is greyed,
 * with the reason as its tip.
 *
 * The line is what a script sends with llOwnerSay: @setsphere=n and the
 * values with =force (RLVa takes them with =force only), the colour as
 * setsphere_param. The mode is always the colour fade (0): kept simple, as the
 * project owner asked.
 *
 * Colours and sizes come from the theme tokens (src/theme.h).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef SPHERECONTROLS_H
#define SPHERECONTROLS_H

#include <functional>

#include <QColor>
#include <QPointer>
#include <QWidget>

#include "environmentoverride.h"

class QLabel;
class QPlainTextEdit;
class QPushButton;
class QSlider;

class SphereControls : public QWidget
{
  Q_OBJECT

  public:
    // distances: 0 to MostMetres in steps of half a metre (the slider counts the steps)
    static constexpr int StepsPerMetre = 2;
    static constexpr int MostMetres = 20;

    // asks for a colour; the default opens Qt's own colour dialog with the given parent
    using ColourPicker = std::function<QColor(const QColor& current, QWidget* parent)>;

    explicit SphereControls(QWidget* parent = nullptr);

    // the override the controls drive (not owned)
    void setOverride(EnvironmentOverride* environmentOverride);
    // the controls work only while the 3D-view box is ticked
    void setAvailable(bool available);
    bool available() const { return m_available; }

    // the values, as the controls show them
    double innerMetres() const;
    double outerMetres() const;
    int darknessPercent() const;
    QColor colour() const { return m_colour; }
    void setColour(const QColor& colour);

    // the sphere and the RLV line for the values shown
    EnvironmentOverride::Sphere sphere() const;
    QString rlvLine() const;
    // the line for any sphere of the colour fade (mode 0), as the controls write it
    static QString lineFor(const EnvironmentOverride::Sphere& sphere);
    // a colour part 0 to 255 as the line writes it: 0 to 1 in hundredths
    static double colourPart(int value);

    void setColourPicker(ColourPicker picker) { m_picker = std::move(picker); }

    QSlider* innerSlider() const { return m_inner; }
    QSlider* outerSlider() const { return m_outer; }
    QSlider* darknessSlider() const { return m_darkness; }
    QLabel* innerValue() const { return m_innerValue; }
    QLabel* outerValue() const { return m_outerValue; }
    QLabel* darknessValue() const { return m_darknessValue; }
    QPushButton* colourButton() const { return m_colourButton; }
    QPlainTextEdit* lineField() const { return m_line; }
    QPushButton* copyButton() const { return m_copy; }
    QPushButton* clearButton() const { return m_clear; }
    QLabel* note() const { return m_note; }
    QLabel* reason() const { return m_reason; }

    // the token colours of the dark or light theme
    void applyStyle(bool dark);

  signals:
    // the sliders' sphere was put on the 3D view or taken off it
    void tryingChanged(bool trying);

  private:
    void changedByUser();
    void pickColour();
    void copyLine();
    void clearTry();
    void refresh();

    QPointer<EnvironmentOverride> m_override;
    bool m_available = false;
    bool m_dark = true;
    QColor m_colour{0, 0, 0};
    ColourPicker m_picker;

    QSlider* m_inner;
    QSlider* m_outer;
    QSlider* m_darkness;
    QLabel* m_innerValue;
    QLabel* m_outerValue;
    QLabel* m_darknessValue;
    QPushButton* m_colourButton;
    QPlainTextEdit* m_line;
    QPushButton* m_copy;
    QPushButton* m_clear;
    QLabel* m_note;
    QLabel* m_reason;
};

#endif
