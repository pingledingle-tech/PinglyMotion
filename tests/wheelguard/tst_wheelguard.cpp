// Mouse wheel over controls in a scrolling panel (issue #82)
#include <QApplication>
#include <QScrollArea>
#include <QScrollBar>
#include <QSlider>
#include <QSpinBox>
#include <QTableWidget>
#include <QTest>
#include <QVBoxLayout>
#include <QWheelEvent>

#include "wheelguard.h"

namespace
{
void wheel(QWidget* w,int notches)
{
  const QPointF pos(w->width()/2.0,w->height()/2.0);
  QWheelEvent e(pos,w->mapToGlobal(pos),QPoint(),QPoint(0,120*notches),Qt::NoButton,Qt::NoModifier,
                Qt::NoScrollPhase,false);
  QApplication::sendEvent(w,&e);
}
}

class TstWheelGuard : public QObject
{
  Q_OBJECT

  QScrollArea* area=nullptr;
  QSlider* slider=nullptr;
  QSpinBox* spin=nullptr;

  void build(bool guarded)
  {
    area=new QScrollArea;
    QWidget* panel=new QWidget;
    QVBoxLayout* layout=new QVBoxLayout(panel);
    // the slider sits at the top, so the pointer is over it while it scrolls
    slider=new QSlider(Qt::Horizontal);
    slider->setRange(-19600,19600);
    slider->setValue(500);
    spin=new QSpinBox;
    spin->setRange(-100,100);
    layout->addWidget(slider);
    layout->addWidget(spin);
    QWidget* filler=new QWidget;
    filler->setMinimumHeight(2000);
    layout->addWidget(filler);
    area->setWidget(panel);
    area->setWidgetResizable(true);
    area->resize(300,300);
    if(guarded) (new WheelGuard(area))->guardChildrenOf(panel);
    area->show();
    QVERIFY(QTest::qWaitForWindowExposed(area));
  }

  private slots:
    void cleanup() { delete area; area=nullptr; }

    // what the program did before: the control takes the wheel
    void unguardedSliderChanges()
    {
      build(false);
      wheel(slider,-3);
      QVERIFY(slider->value()!=500);
    }

    void unfocusedSliderStaysAndPanelScrolls()
    {
      build(true);
      slider->clearFocus();
      QCOMPARE(area->verticalScrollBar()->value(),0);
      wheel(slider,-3);
      QCOMPARE(slider->value(),500);
      QVERIFY(area->verticalScrollBar()->value()>0);
    }

    void unfocusedSpinBoxStaysAndPanelScrolls()
    {
      build(true);
      spin->clearFocus();
      wheel(spin,-3);
      QCOMPARE(spin->value(),0);
      QVERIFY(area->verticalScrollBar()->value()>0);
    }

    void focusedSliderTakesTheWheel()
    {
      build(true);
      slider->setFocus();
      QTRY_VERIFY(slider->hasFocus());
      wheel(slider,3);
      QVERIFY(slider->value()!=500);
      QCOMPARE(area->verticalScrollBar()->value(),0);
    }

    // crash seen in the Windows build of #85: a table (the joint list) inside the
    // panel has scroll bars of its own; guarding them sent the wheel round in a
    // circle (scroll bar -> viewport -> scroll bar ...) until the stack was full
    void nestedTableScrollBarsDoNotLoop()
    {
      area=new QScrollArea;
      QWidget* panel=new QWidget;
      QVBoxLayout* layout=new QVBoxLayout(panel);
      QTableWidget* table=new QTableWidget(100,2);
      table->setMinimumHeight(120);
      table->setMaximumHeight(120);
      layout->addWidget(table);
      QWidget* filler=new QWidget;
      filler->setMinimumHeight(2000);
      layout->addWidget(filler);
      area->setWidget(panel);
      area->setWidgetResizable(true);
      area->resize(300,300);
      (new WheelGuard(area))->guardChildrenOf(panel);
      area->show();
      QVERIFY(QTest::qWaitForWindowExposed(area));
      QScrollBar* bar=table->verticalScrollBar();
      QVERIFY(bar->isVisible());
      // no scroll bar of a nested area is guarded
      QVERIFY(!WheelGuard::isGuardable(bar));
      for(int i=0;i<20;i++)
      {
        wheel(bar,-3);
        wheel(table->viewport(),-3);
        wheel(bar,3);
      }
      // reaching this line means no endless loop
      QVERIFY(true);
    }

    void wheelDoesNotGiveFocus()
    {
      build(true);
      QCOMPARE(slider->focusPolicy(),Qt::StrongFocus);
      QCOMPARE(spin->focusPolicy(),Qt::StrongFocus);
    }
};

QTEST_MAIN(TstWheelGuard)
#include "tst_wheelguard.moc"
