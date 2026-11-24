/*
 * PinglyMotion - the contents of the three panels, built from the designer file
 *
 * The Joints, Selected joint and Timeline panels hold widgets from
 * pinglymotionwindow.ui, moved into dock widgets around the 3D view
 * (docs/archive/PANELS_PLAN.md, job P2). This header builds them and knows
 * how wide they must be at the current text size (docs/UX_DECISIONS.md, 6c):
 * the tall panels as wide as their widest control, the timeline as wide as
 * its rows, on one line or broken into shorter lines. Header-only, so the
 * test in tests/uirender builds the very same panels from the designer file
 * as the program does and checks that nothing is clipped.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#ifndef PANELCONTENT_H
#define PANELCONTENT_H

#include <QComboBox>
#include <QCoreApplication>
#include <QDockWidget>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMainWindow>
#include <QScrollArea>
#include <QScrollBar>
#include <QStackedWidget>
#include <QStyle>
#include <QVBoxLayout>

#include <algorithm>

#include "easingtiles.h"
#include "panellayout.h"
#include "paneltitlebar.h"
#include "selectedjointpanel.h"
#include "selectedproppanel.h"
#include "timelineclip.h"
#include "beginnerview.h"
#include "timelinerows.h"
#include "timelineview.h"
#include "transportlook.h"
#include "transportbar.h"
#include "ui_pinglymotionwindow.h"
#include "wheelguard.h"
#include "wraprow.h"

class MainPanels
{
public:
  QDockWidget* timelineDock=nullptr;
  QDockWidget* jointsDock=nullptr;
  QDockWidget* jointCardDock=nullptr;
  QScrollArea* timelineScroll=nullptr;
  QScrollArea* jointsScroll=nullptr;
  QScrollArea* jointCardScroll=nullptr;
  // the joint page of the Selected joint panel (UX phase 2b)
  SelectedJointPanel* selectedJoint=nullptr;
  // the prop page of the Selected joint panel (Mockup 2, src/selectedproppanel.h)
  SelectedPropPanel* selectedProp=nullptr;
  TransportBar* transport=nullptr;
  TimelineView* timelineView=nullptr;
  // Set key, the main button of the timeline panel (UX phase 3a)
  QPushButton* setKey=nullptr;
  // Paste mirrored, beside Set key, shown in Expert mode (UX phase 3b, the mockup's
  // expert-only button); the window connects it and greys it with its reason
  QPushButton* pasteMirrored=nullptr;
  // the two ways back to what Beginner mode hides (UX-E, src/beginnerview.h), above the
  // Selected joint panel: the Joints list, and Edit curve with the finger grid (Expert mode)
  QWidget* beginnerRow=nullptr;
  QPushButton* showJointsButton=nullptr;
  QPushButton* showMoreButton=nullptr;
  // the loop points, frame and speed row of the timeline panel
  QBoxLayout* rangeRow=nullptr;
  // the "(10%)" labels after the loop points
  QLabel* loopInPercent=nullptr;
  QLabel* loopOutPercent=nullptr;

  // Moves the panel widgets of the designer file into three docks of
  // `window` and puts the 3D view in the middle. The docks are not placed
  // yet (PanelWindow::applyDefaultLayout does that).
  void build(QMainWindow* window,Ui::PinglyMotionWindow* ui)
  {
    // the buttons and the slider that have only a picture or only a position get a name and a tip
    // (charter P3 and P8, job B4), so a screen reader and the mouse hover both say what they do
    const auto name=[](QWidget* widget,const char* source)
    {
      const QString words=text(source);
      widget->setAccessibleName(words);
      if(widget->toolTip().isEmpty()) widget->setToolTip(words);
    };
    name(ui->newBoxPropButton,QT_TRANSLATE_NOOP("PinglyMotionWindow","New box prop"));
    name(ui->newSpherePropButton,QT_TRANSLATE_NOOP("PinglyMotionWindow","New sphere prop"));
    name(ui->newConePropButton,QT_TRANSLATE_NOOP("PinglyMotionWindow","New cone prop"));
    name(ui->newTorusPropButton,QT_TRANSLATE_NOOP("PinglyMotionWindow","New torus prop"));
    name(ui->deletePropButton,QT_TRANSLATE_NOOP("PinglyMotionWindow","Delete the selected prop"));
    name(ui->currentFrameSlider,QT_TRANSLATE_NOOP("PinglyMotionWindow","Current frame"));
    // --- timeline panel: tracks, frame slider, play and key frame buttons,
    // loop in / out, frame count and FPS
    QWidget* timelinePanel=new QWidget;
    QVBoxLayout* timelineLayout=new QVBoxLayout(timelinePanel);
    timelineLayout->setContentsMargins(4,4,4,4);
    // the old layout kept the timeline at a fixed height; in a panel it grows
    ui->timelineView->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    // at least 200 wide; as high as its tracks, easing tiles and hint need at
    // the current text size (a fixed height squeezed the tiles at large text)
    ui->timelineView->setMinimumWidth(200);
    timelineLayout->addWidget(ui->timelineView,1);
    timelineView=ui->timelineView;
    QHBoxLayout* playLayout=new QHBoxLayout;
    playLayout->addWidget(ui->currentFrameSlider,1);
    // Set key: the blue main button of the timeline (UX phase 3a, the mockup's
    // #bKey), drawn by src/transportlook.* in a row of its own
    QWidget* keyBar=new QWidget(timelinePanel);
    keyBar->setObjectName(QStringLiteral("setKeyBar"));
    QHBoxLayout* keyBarLayout=new QHBoxLayout(keyBar);
    keyBarLayout->setContentsMargins(0,0,0,0);
    setKey=ui->keyframeButton;
    setKey->setCheckable(false);
    setKey->setText(TimelineSpec::text(TimelineSpec::setKey));
    setKey->setParent(keyBar);
    keyBarLayout->addWidget(setKey);
    pasteMirrored=new QPushButton(TimelineSpec::text(ClipSpec::pasteMirroredButton),keyBar);
    pasteMirrored->setObjectName(QStringLiteral("pasteMirroredButton"));
    // Expert mode shows it and Beginner mode hides it; the space stays, so Set key and the frame
    // slider never move when the mode changes (charter P12)
    {
      QSizePolicy keepSpace=pasteMirrored->sizePolicy();
      keepSpace.setRetainSizeWhenHidden(true);
      pasteMirrored->setSizePolicy(keepSpace);
    }
    keyBarLayout->addWidget(pasteMirrored);
    TransportLook::apply(keyBar);
    playLayout->addWidget(keyBar);
    timelineLayout->addLayout(playLayout);

    // Start, Stop, Replay from start, Frame back, Frame forward and Loop
    // (docs/UX_DECISIONS.md section 1); the window connects them
    transport=new TransportBar(timelinePanel);
    QHBoxLayout* transportLayout=new QHBoxLayout;
    transportLayout->addWidget(transport);
    transportLayout->addStretch(1);
    timelineLayout->addLayout(transportLayout);

    // the loop points, then the frame and the speed: one line, or two lines
    // when they do not fit on one (docs/UX_DECISIONS.md, 6c)
    QHBoxLayout* loopPart=new QHBoxLayout;
    loopPart->addWidget(ui->loopInLabel);
    loopPart->addWidget(ui->loopInSpinBox);
    loopPart->addWidget(ui->loopInPercentLabel);
    loopPart->addSpacing(12);
    loopPart->addWidget(ui->loopOutLabel);
    loopPart->addWidget(ui->loopOutSpinBox);
    loopPart->addWidget(ui->loopOutPercentLabel);
    loopPart->addStretch(1);
    // the stretch first: the frame and the speed stand on the right
    QHBoxLayout* framePart=new QHBoxLayout;
    framePart->addStretch(1);
    framePart->addWidget(ui->frameLabel);
    framePart->addWidget(ui->currentFrameLabel);
    framePart->addWidget(ui->ofLabel);
    framePart->addWidget(ui->framesSpin);
    framePart->addSpacing(12);
    framePart->addWidget(ui->fpsLabel);
    framePart->addWidget(ui->fpsSpin);
    rangeRow=WrapRow::make({loopPart,framePart});
    loopInPercent=ui->loopInPercentLabel;
    loopOutPercent=ui->loopOutPercentLabel;
    timelineLayout->addLayout(rangeRow);

    timelineDock=new QDockWidget(text(QT_TRANSLATE_NOOP("PinglyMotionWindow","Timeline")),window);
    timelineDock->setObjectName(PanelLayout::TimelineDock);
    // when the window is too low for all of it, it scrolls down, never
    // sideways: fit() keeps it as wide as its rows need
    timelineScroll=new QScrollArea;
    timelineScroll->setWidget(timelinePanel);
    timelineScroll->setWidgetResizable(true);
    timelineScroll->setFrameShape(QFrame::NoFrame);
    timelineScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    timelineDock->setWidget(timelineScroll);
    timelineDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);
    timelineDock->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    timelineDock->setTitleBarWidget(new PanelTitleBar(timelineDock));

    // --- Joints panel: the Avatar / Props switch (the tabs), with the joint
    // tabs, search and list on Avatar, the prop list on Props
    QWidget* jointsPanel=new QWidget;
    QVBoxLayout* jointsLayout=new QVBoxLayout(jointsPanel);
    jointsLayout->setContentsMargins(4,4,4,4);
    jointsLayout->addWidget(ui->avatarPropsTab,1);
    jointsDock=makeTallPanel(window,text(QT_TRANSLATE_NOOP("PinglyMotionWindow","Joints")),PanelLayout::JointsDock,jointsPanel,jointsScroll);

    // --- Selected joint panel: the rotation and position boxes of the joint,
    // or the position, scale and rotation boxes of the prop. The boxes come
    // from the designer file and are moved here from the tabs; the page
    // follows the Avatar / Props switch.
    // The joint page is the Selected joint panel of UX phase 2b
    // (src/selectedjointpanel.h); it lays out the designer file's rotation and
    // position sliders and fields, so the window's code keeps driving them. The
    // old group boxes with their labels are left empty and hidden.
    SelectedJointPanel::Controls controls;
    controls.turn[0]=ui->xRotationSlider;
    controls.turn[1]=ui->yRotationSlider;
    controls.turn[2]=ui->zRotationSlider;
    controls.turnValue[0]=ui->xRotationEdit;
    controls.turnValue[1]=ui->yRotationEdit;
    controls.turnValue[2]=ui->zRotationEdit;
    controls.move[0]=ui->xPositionSlider;
    controls.move[1]=ui->yPositionSlider;
    controls.move[2]=ui->zPositionSlider;
    controls.moveValue[0]=ui->xPositionEdit;
    controls.moveValue[1]=ui->yPositionEdit;
    controls.moveValue[2]=ui->zPositionEdit;
    selectedJoint=new SelectedJointPanel(nullptr,&controls);
    // out of the Joints panel's layout, kept (hidden) for the designer file's own code
    for(QWidget* old : {static_cast<QWidget*>(ui->rotationGroupBox),static_cast<QWidget*>(ui->positionGroupBox)})
    {
      old->hide();
      old->setParent(selectedJoint);
      old->hide();
    }
    QWidget* jointCard=selectedJoint;
    // the prop page: Selected prop with Shape, Place, Turn and Size (Mockup 2); the designer
    // file's old prop boxes are kept, hidden, for its own code
    selectedProp=new SelectedPropPanel;
    for(QWidget* old : {static_cast<QWidget*>(ui->propPositionGroup),static_cast<QWidget*>(ui->propScaleGroup),
                        static_cast<QWidget*>(ui->propRotationGroup)})
    {
      old->hide();
      old->setParent(selectedProp);
      old->hide();
    }
    QWidget* propCard=selectedProp;
    QStackedWidget* cardStack=new QStackedWidget;
    cardStack->setObjectName("jointCardStack");
    cardStack->addWidget(jointCard);
    cardStack->addWidget(propCard);
    cardStack->setCurrentIndex(ui->avatarPropsTab->currentIndex());
    QObject::connect(ui->avatarPropsTab,&QTabWidget::currentChanged,cardStack,&QStackedWidget::setCurrentIndex);
    QWidget* cardPanel=new QWidget;
    QVBoxLayout* cardPanelLayout=new QVBoxLayout(cardPanel);
    cardPanelLayout->setContentsMargins(4,4,4,4);
    // Beginner mode (UX-E): plain buttons that bring back what the mode hides; the window
    // shows and hides them with the mode (BeginnerView::showJointsButtonVisible)
    beginnerRow=new QWidget;
    beginnerRow->setObjectName("beginnerRow");
    QVBoxLayout* beginnerLayout=new QVBoxLayout(beginnerRow);
    beginnerLayout->setContentsMargins(0,0,0,4);
    beginnerLayout->setSpacing(4);
    showJointsButton=new QPushButton(BeginnerView::tr(BeginnerView::showJointsButton),beginnerRow);
    showJointsButton->setObjectName("showJointsButton");
    showJointsButton->setToolTip(BeginnerView::tr(BeginnerView::showJointsTip));
    showMoreButton=new QPushButton(BeginnerView::tr(BeginnerView::showMoreButton),beginnerRow);
    showMoreButton->setObjectName("showMoreButton");
    showMoreButton->setToolTip(BeginnerView::tr(BeginnerView::showMoreTip));
    for(QPushButton* button : {showJointsButton,showMoreButton})
    {
      button->setStatusTip(button->toolTip());
      beginnerLayout->addWidget(button);
    }
    cardPanelLayout->addWidget(beginnerRow);
    cardPanelLayout->addWidget(cardStack,1);
    jointCardDock=makeTallPanel(window,text(QT_TRANSLATE_NOOP("PinglyMotionWindow","Selected joint")),PanelLayout::JointCardDock,cardPanel,jointCardScroll);

    // the Joints panel's tabs lost the old group boxes above: their layouts
    // forget the sizes they kept for them, so the panel is measured as it is now
    for(QLayout* layout : ui->avatarPropsTab->findChildren<QLayout*>())
      layout->invalidate();
    ui->avatarPropsTab->updateGeometry();
    jointsPanel->layout()->invalidate();
    // a drop-down asks for the width of its items only once it is shown (Qt's
    // default); the panel is measured before that, so the figure choice (two
    // fixed items) asks for it at once
    ui->figureCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);

    // --- 3D view in the middle (replaces the old fixed layout)
    QWidget* central=new QWidget;
    QVBoxLayout* centralLayout=new QVBoxLayout(central);
    centralLayout->setContentsMargins(0,0,0,0);
    centralLayout->addWidget(ui->animationView);
    window->setCentralWidget(central);
  }

  // The width a tall panel needs to show its widest control.
  static int fitWidth(QScrollArea* scroll)
  {
    QWidget* panel=scroll->widget();
    panel->ensurePolished();
    const int width=std::max(panel->sizeHint().width(),panel->minimumSizeHint().width());
    return width+scroll->verticalScrollBar()->sizeHint().width();
  }

  // The Joints and Selected joint panels are at least as wide as their
  // contents need, in every layout, and start that wide; the user can drag the
  // line beside them to make them wider (UX fixes 1: in Expert mode the Joints
  // panel could not be made wider, because the wide layout fixed its width).
  // The timeline is as wide as its rows need, as they stand now.
  void fit(PanelLayout::Reflow mode) const
  {
    Q_UNUSED(mode)
    for (QScrollArea* scroll : {jointsScroll,jointCardScroll})
    {
      scroll->setMinimumWidth(fitWidth(scroll));
      scroll->setMaximumWidth(QWIDGETSIZE_MAX);
    }
    fitTimeline();
  }

  // The smallest width of the timeline panel with its rows on one line and
  // with its rows broken into shorter lines, whichever it shows now.
  void timelineWidths(int& oneLine,int& wrapped) const
  {
    QLayout* layout=timelineScroll->widget()->layout();
    oneLine=0;
    wrapped=0;
    for (int i=0;i<layout->count();++i)
    {
      QLayoutItem* item=layout->itemAt(i);
      if (item->isEmpty()) continue;
      int one=item->minimumSize().width();
      int two=one;
      if (item->widget()==timelineView)
      {
        // the tracks take any width; the easing tile row under them does not
        const QMargins m=timelineView->layout()->contentsMargins();
        const int least=timelineView->minimumWidth();
        one=std::max(least,timelineView->easingTiles()->oneLineWidth()+m.left()+m.right());
        two=std::max(least,timelineView->easingTiles()->wrappedWidth()+m.left()+m.right());
      }
      else if (item->layout()==rangeRow)
      {
        one=WrapRow::oneLineWidth(rangeRow);
        two=WrapRow::wrappedWidth(rangeRow);
      }
      else if (item->layout() && item->layout()->indexOf(transport)>=0)
      {
        const QMargins m=item->layout()->contentsMargins();
        one=transport->oneLineWidth()+m.left()+m.right();
        two=transport->wrappedWidth()+m.left()+m.right();
      }
      oneLine=std::max(oneLine,one);
      wrapped=std::max(wrapped,two);
    }
    const QMargins m=layout->contentsMargins();
    const int scrollBar=timelineScroll->verticalScrollBar()->sizeHint().width();
    oneLine+=m.left()+m.right()+scrollBar;
    wrapped+=m.left()+m.right()+scrollBar;
  }

  bool isTimelineWrapped() const
  {
    return WrapRow::isWrapped(rangeRow);
  }

  // The timeline panel's rows on one line, or broken into shorter lines.
  void setTimelineWrapped(bool wrapped) const
  {
    timelineView->easingTiles()->setWrapped(wrapped);
    transport->setWrapped(wrapped);
    WrapRow::setWrapped(rangeRow,wrapped);
    fitTimeline();
  }

  // The widths the layouts need, at the current text size (PanelLayout::chooseReflow).
  PanelLayout::ReflowNeeds needs(const QWidget* window) const
  {
    PanelLayout::ReflowNeeds n;
    // fit() gave each scroll area its fit width as its smallest width; the
    // dock adds its frame
    const int jointsFrame=std::max(0,jointsDock->minimumSizeHint().width()-jointsScroll->minimumWidth());
    const int cardFrame=std::max(0,jointCardDock->minimumSizeHint().width()-jointCardScroll->minimumWidth());
    n.joints=fitWidth(jointsScroll)+jointsFrame;
    n.jointCard=fitWidth(jointCardScroll)+cardFrame;
    timelineWidths(n.timeline,n.timelineWrapped);
    n.separator=window->style()->pixelMetric(QStyle::PM_DockWidgetSeparatorExtent,nullptr,window);
    return n;
  }

private:
  static QString text(const char* source)
  {
    return QCoreApplication::translate("PinglyMotionWindow",source);
  }

  // The designer file gives the percent labels a fixed least width of 60
  // pixels, which the layout trusts over the text: at large text it squeezed
  // them below their text. Their least width is the widest text they show,
  // "(100%)", in their font now.
  // Answers whether the width changed.
  static bool fitPercentLabel(QLabel* label)
  {
    const QMargins m=label->contentsMargins();
    const int text=label->fontMetrics().horizontalAdvance(QStringLiteral("(100%)"))
                   +m.left()+m.right()+2*label->margin()+2*label->frameWidth();
    const int width=std::max(text,label->sizeHint().width());
    if (label->minimumWidth()==width) return false;
    label->setMinimumWidth(width);
    return true;
  }

  void fitTimeline() const
  {
    const bool inChanged=fitPercentLabel(loopInPercent);
    const bool outChanged=fitPercentLabel(loopOutPercent);
    if (inChanged || outChanged)
    {
      // the layouts keep their old least sizes until the next layout pass;
      // the widths below are measured now
      for (QLayout* layout : timelineScroll->widget()->findChildren<QLayout*>())
        layout->invalidate();
    }
    int oneLine=0;
    int wrapped=0;
    timelineWidths(oneLine,wrapped);
    timelineScroll->setMinimumWidth(isTimelineWrapped() ? wrapped : oneLine);
  }

  // A tall panel keeps its width, whatever the window around it does. If the
  // window is too low for all of it, it scrolls. A wheel over a slider or spin
  // box scrolls the panel instead of changing the value (issue #82: the figure
  // moved while the panel was scrolled).
  static QDockWidget* makeTallPanel(QMainWindow* window,const QString& title,const char* objectName,
                                    QWidget* panel,QScrollArea*& scroll)
  {
    QDockWidget* dock=new QDockWidget(title,window);
    dock->setObjectName(QString::fromLatin1(objectName));
    scroll=new QScrollArea;
    scroll->setWidget(panel);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    dock->setWidget(scroll);
    (new WheelGuard(scroll))->guardChildrenOf(panel);
    dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);
    dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    dock->setTitleBarWidget(new PanelTitleBar(dock));
    return dock;
  }
};

#endif
