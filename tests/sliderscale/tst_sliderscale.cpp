/*
 * PinglyMotion - test of the slider step conversion (#192)
 *
 * The rotation and position sliders work in steps of 0.01. Before #192 the
 * window divided the int slider value by the int precision, so 12.34 degrees
 * became 12, and setting a value truncated (12.34 * 100 gave 1233).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>
#include <QLineEdit>
#include <QSlider>

#include "sliderscale.h"

class TestSliderScale : public QObject
{
  Q_OBJECT

  private:
    // the same steps as PinglyMotionWindow::setSliderValue and getX()/getXPos()
    static void setValue(QSlider& slider, QLineEdit& edit, float value);
    static float getValue(const QSlider& slider);

  private slots:
    void sliderStepToValueKeepsDecimals();
    void valueToSliderStepRounds();
    void rotationRoundTripsExactly();
    void positionRoundTripsExactly();
    void oneSliderStepIsOneHundredth();
    void pageStepIsTenWholeUnits();
    void limitsKeepTheirDecimals();
};

void TestSliderScale::setValue(QSlider& slider, QLineEdit& edit, float value)
{
  slider.setValue(SliderScale::toSlider(value));
  edit.setText(QString::number(value));
}

float TestSliderScale::getValue(const QSlider& slider)
{
  return SliderScale::toValue(slider.value());
}

void TestSliderScale::sliderStepToValueKeepsDecimals()
{
  QCOMPARE(SliderScale::precision, 100);
  QCOMPARE(SliderScale::toValue(1234), 12.34);
  QCOMPARE(SliderScale::toValue(1250), 12.5);
  QCOMPARE(SliderScale::toValue(25), 0.25);
  QCOMPARE(SliderScale::toValue(-1234), -12.34);
  QCOMPARE(SliderScale::toValue(1), 0.01);
  QCOMPARE(SliderScale::toValue(0), 0.0);
  QCOMPARE(SliderScale::toValue(35900), 359.0);
  // QCOMPARE on floating point is fuzzy; these must be bit for bit
  QVERIFY(SliderScale::toValue(1234) == 12.34);
  QVERIFY(SliderScale::toValue(1250) == 12.5);
  QVERIFY(SliderScale::toValue(25) == 0.25);
  QVERIFY(float(SliderScale::toValue(1234)) == 12.34f);
  QVERIFY(float(SliderScale::toValue(29)) == 0.29f);
}

void TestSliderScale::valueToSliderStepRounds()
{
  QCOMPARE(SliderScale::toSlider(12.34), 1234);   // 1233.999... before rounding
  QCOMPARE(SliderScale::toSlider(0.29), 29);      // 28.999... before rounding
  QCOMPARE(SliderScale::toSlider(-0.29), -29);
  QCOMPARE(SliderScale::toSlider(12.5), 1250);
  QCOMPARE(SliderScale::toSlider(0.25), 25);
  QCOMPARE(SliderScale::toSlider(12.34f), 1234);  // the window passes floats
  QCOMPARE(SliderScale::toSlider(0.29f), 29);
  QCOMPARE(SliderScale::toSlider(-359.0), -35900);
}

void TestSliderScale::rotationRoundTripsExactly()
{
  QSlider slider(Qt::Horizontal);
  QLineEdit edit;
  slider.setRange(-359 * SliderScale::precision, 359 * SliderScale::precision);

  setValue(slider, edit, 12.5f);
  QCOMPARE(slider.value(), 1250);
  QCOMPARE(getValue(slider), 12.5f);
  QCOMPARE(edit.text(), QString("12.5"));

  setValue(slider, edit, 12.34f);
  QCOMPARE(slider.value(), 1234);
  QCOMPARE(getValue(slider), 12.34f);
  QVERIFY(getValue(slider) == 12.34f);
  QCOMPARE(edit.text(), QString("12.34"));

  setValue(slider, edit, -45.67f);
  QCOMPARE(slider.value(), -4567);
  QCOMPARE(getValue(slider), -45.67f);
}

void TestSliderScale::positionRoundTripsExactly()
{
  QSlider slider(Qt::Horizontal);
  QLineEdit edit;
  slider.setRange(-10 * SliderScale::precision, 10 * SliderScale::precision);

  setValue(slider, edit, 0.25f);
  QCOMPARE(slider.value(), 25);
  QCOMPARE(getValue(slider), 0.25f);
  QVERIFY(getValue(slider) == 0.25f);
  QCOMPARE(edit.text(), QString("0.25"));

  setValue(slider, edit, 0.29f);
  QCOMPARE(slider.value(), 29);
  QCOMPARE(getValue(slider), 0.29f);
}

void TestSliderScale::oneSliderStepIsOneHundredth()
{
  QSlider slider(Qt::Horizontal);
  QLineEdit edit;
  slider.setRange(-359 * SliderScale::precision, 359 * SliderScale::precision);
  slider.setSingleStep(1);

  setValue(slider, edit, 12.5f);
  slider.triggerAction(QAbstractSlider::SliderSingleStepAdd);
  QCOMPARE(slider.value(), 1251);
  QCOMPARE(getValue(slider), 12.51f);

  slider.triggerAction(QAbstractSlider::SliderSingleStepSub);
  slider.triggerAction(QAbstractSlider::SliderSingleStepSub);
  QCOMPARE(getValue(slider), 12.49f);

  QSlider position(Qt::Horizontal);
  position.setRange(-10 * SliderScale::precision, 10 * SliderScale::precision);
  setValue(position, edit, 0.25f);
  position.triggerAction(QAbstractSlider::SliderSingleStepAdd);
  QCOMPARE(getValue(position), 0.26f);
}

void TestSliderScale::pageStepIsTenWholeUnits()
{
  QSlider slider(Qt::Horizontal);
  QLineEdit edit;
  slider.setRange(-359 * SliderScale::precision, 359 * SliderScale::precision);
  slider.setPageStep(10 * SliderScale::precision);

  setValue(slider, edit, 12.34f);
  slider.triggerAction(QAbstractSlider::SliderPageStepAdd);
  QCOMPARE(getValue(slider), 22.34f);
}

void TestSliderScale::limitsKeepTheirDecimals()
{
  // a joint limit of -12.5 .. 45.75 degrees, as from the .lim file
  QSlider slider(Qt::Horizontal);
  slider.setRange(SliderScale::toSlider(-12.5), SliderScale::toSlider(45.75));
  QCOMPARE(slider.minimum(), -1250);
  QCOMPARE(slider.maximum(), 4575);
  // the clamp in rotationValue() / positionValue()
  QCOMPARE(SliderScale::toValue(slider.minimum()), -12.5);
  QCOMPARE(SliderScale::toValue(slider.maximum()), 45.75);
}

QTEST_MAIN(TestSliderScale)
#include "tst_sliderscale.moc"
