/********************************************************************************
** Form generated from reading UI file 'pinglymotionwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.4.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_PINGLYMOTIONWINDOW_H
#define UI_PINGLYMOTIONWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QWidget>
#include "animationview.h"
#include "timelineview.h"

QT_BEGIN_NAMESPACE

class Ui_PinglyMotionWindow
{
public:
    QAction *fileNewAction;
    QAction *fileOpenAction;
    QAction *fileAddAction;
    QAction *fileSaveAction;
    QAction *fileSaveAsAction;
    QAction *fileLoadPropsAction;
    QAction *fileSavePropsAction;
    QAction *fileExitAction;
    QAction *editUndoAction;
    QAction *editRedoAction;
    QAction *editCutAction;
    QAction *editCopyAction;
    QAction *editPasteAction;
    QAction *toolsOptimizeBVHAction;
    QAction *optionsSkeletonAction;
    QAction *optionsJointLimitsAction;
    QAction *optionsLoopAction;
    QAction *optionsProtectFirstFrameAction;
    QAction *optionsShowTimelineAction;
    QAction *optionsConfigurePinglyMotionAction;
    QAction *helpAboutAction;
    QAction *resetCameraAction;
    QAction *fileExportForSecondLifeAction;
    QAction *toolsMirrorAction;
    QAction *helpWhatsNewAction;
    QWidget *centralwidget;
    QGridLayout *gridLayout;
    QGridLayout *gridLayout1;
    AnimationView *animationView;
    TimelineView *timelineView;
    QSlider *currentFrameSlider;
    QPushButton *playButton;
    QPushButton *keyframeButton;
    QTabWidget *avatarPropsTab;
    QWidget *avatarTab;
    QGridLayout *gridLayout2;
    QLabel *animationLabel;
    QComboBox *selectAnimationCombo;
    QLabel *figureLabel;
    QComboBox *figureCombo;
    QLabel *scaleLabel;
    QSpinBox *scaleSpin;
    QWidget *jointPicker;
    QGroupBox *rotationGroupBox;
    QGridLayout *gridLayout3;
    QLabel *yRotationLabel;
    QLineEdit *yRotationEdit;
    QLabel *altLabel;
    QSlider *yRotationSlider;
    QLabel *zRotationLabel;
    QLineEdit *zRotationEdit;
    QLabel *ctrlLabel;
    QSlider *zRotationSlider;
    QLabel *xRotationLabel;
    QLineEdit *xRotationEdit;
    QLabel *shiftLabel;
    QSlider *xRotationSlider;
    QGroupBox *positionGroupBox;
    QGridLayout *gridLayout4;
    QLabel *xPositionLabel;
    QLineEdit *xPositionEdit;
    QSlider *xPositionSlider;
    QLabel *ypositionLabel;
    QLineEdit *yPositionEdit;
    QSlider *yPositionSlider;
    QLabel *zRotationLabel_2;
    QLineEdit *zPositionEdit;
    QSlider *zPositionSlider;
    QGroupBox *easeInOutGroup;
    QGridLayout *gridLayout5;
    QCheckBox *easeInCheck;
    QCheckBox *easeOutCheck;
    QSpacerItem *spacerItem;
    QWidget *propsTab;
    QGridLayout *gridLayout6;
    QHBoxLayout *hboxLayout;
    QPushButton *newBoxPropButton;
    QPushButton *newSpherePropButton;
    QPushButton *newConePropButton;
    QPushButton *newTorusPropButton;
    QLabel *propNameLabel;
    QComboBox *propNameCombo;
    QPushButton *deletePropButton;
    QLabel *attachToLabel;
    QComboBox *attachToComboBox;
    QGroupBox *propPositionGroup;
    QGridLayout *gridLayout7;
    QLabel *propXPosLabel;
    QSpinBox *propXPosSpin;
    QLabel *propYPosLabel;
    QSpinBox *propYPosSpin;
    QLabel *propYPosLabel_2;
    QSpinBox *propZPosSpin;
    QGroupBox *propScaleGroup;
    QGridLayout *gridLayout8;
    QLabel *propXScaleLabel;
    QSpinBox *propXScaleSpin;
    QLabel *propYScaleLabel;
    QSpinBox *propYScaleSpin;
    QLabel *propZScaleLabel;
    QSpinBox *propZScaleSpin;
    QGroupBox *propRotationGroup;
    QGridLayout *gridLayout9;
    QLabel *propXRotLabel;
    QSpinBox *propXRotSpin;
    QLabel *propYRotLabel;
    QSpinBox *propYRotSpin;
    QLabel *propZRotLabel;
    QSpinBox *propZRotSpin;
    QSpacerItem *spacerItem1;
    QGridLayout *gridLayout10;
    QSpacerItem *spacerItem2;
    QLabel *loopInLabel;
    QSpinBox *loopInSpinBox;
    QLabel *loopInPercentLabel;
    QSpacerItem *spacerItem3;
    QLabel *loopOutLabel;
    QSpinBox *loopOutSpinBox;
    QLabel *loopOutPercentLabel;
    QHBoxLayout *hboxLayout1;
    QSpacerItem *spacerItem4;
    QLabel *frameLabel;
    QLabel *currentFrameLabel;
    QLabel *ofLabel;
    QSpinBox *framesSpin;
    QLabel *fpsLabel;
    QSpinBox *fpsSpin;
    QMenuBar *menubar;
    QMenu *menuFile;
    QMenu *menuEdit;
    QMenu *menuOptions;
    QMenu *menuHelp;
    QMenu *menuTools;
    QToolBar *toolBar;

    void setupUi(QMainWindow *PinglyMotionWindow)
    {
        if (PinglyMotionWindow->objectName().isEmpty())
            PinglyMotionWindow->setObjectName("PinglyMotionWindow");
        PinglyMotionWindow->resize(669, 818);
        fileNewAction = new QAction(PinglyMotionWindow);
        fileNewAction->setObjectName("fileNewAction");
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/icons/icons/filenew.png"), QSize(), QIcon::Normal, QIcon::Off);
        fileNewAction->setIcon(icon);
        fileOpenAction = new QAction(PinglyMotionWindow);
        fileOpenAction->setObjectName("fileOpenAction");
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/icons/icons/fileopen.png"), QSize(), QIcon::Normal, QIcon::Off);
        fileOpenAction->setIcon(icon1);
        fileAddAction = new QAction(PinglyMotionWindow);
        fileAddAction->setObjectName("fileAddAction");
        QIcon icon2;
        icon2.addFile(QString::fromUtf8(":/icons/icons/fileadd.png"), QSize(), QIcon::Normal, QIcon::Off);
        fileAddAction->setIcon(icon2);
        fileSaveAction = new QAction(PinglyMotionWindow);
        fileSaveAction->setObjectName("fileSaveAction");
        QIcon icon3;
        icon3.addFile(QString::fromUtf8(":/icons/icons/filesave.png"), QSize(), QIcon::Normal, QIcon::Off);
        fileSaveAction->setIcon(icon3);
        fileSaveAsAction = new QAction(PinglyMotionWindow);
        fileSaveAsAction->setObjectName("fileSaveAsAction");
        QIcon icon4;
        icon4.addFile(QString::fromUtf8(":/icons/icons/filesaveas.png"), QSize(), QIcon::Normal, QIcon::Off);
        fileSaveAsAction->setIcon(icon4);
        fileLoadPropsAction = new QAction(PinglyMotionWindow);
        fileLoadPropsAction->setObjectName("fileLoadPropsAction");
        fileSavePropsAction = new QAction(PinglyMotionWindow);
        fileSavePropsAction->setObjectName("fileSavePropsAction");
        fileExitAction = new QAction(PinglyMotionWindow);
        fileExitAction->setObjectName("fileExitAction");
        editUndoAction = new QAction(PinglyMotionWindow);
        editUndoAction->setObjectName("editUndoAction");
        editUndoAction->setEnabled(false);
        QIcon icon5;
        icon5.addFile(QString::fromUtf8(":/icons/icons/undo.png"), QSize(), QIcon::Normal, QIcon::Off);
        editUndoAction->setIcon(icon5);
        editUndoAction->setVisible(false);
        editRedoAction = new QAction(PinglyMotionWindow);
        editRedoAction->setObjectName("editRedoAction");
        editRedoAction->setEnabled(false);
        QIcon icon6;
        icon6.addFile(QString::fromUtf8(":/icons/icons/redo.png"), QSize(), QIcon::Normal, QIcon::Off);
        editRedoAction->setIcon(icon6);
        editRedoAction->setVisible(false);
        editCutAction = new QAction(PinglyMotionWindow);
        editCutAction->setObjectName("editCutAction");
        QIcon icon7;
        icon7.addFile(QString::fromUtf8(":/icons/icons/editcut.png"), QSize(), QIcon::Normal, QIcon::Off);
        editCutAction->setIcon(icon7);
        editCopyAction = new QAction(PinglyMotionWindow);
        editCopyAction->setObjectName("editCopyAction");
        QIcon icon8;
        icon8.addFile(QString::fromUtf8(":/icons/icons/editcopy.png"), QSize(), QIcon::Normal, QIcon::Off);
        editCopyAction->setIcon(icon8);
        editPasteAction = new QAction(PinglyMotionWindow);
        editPasteAction->setObjectName("editPasteAction");
        QIcon icon9;
        icon9.addFile(QString::fromUtf8(":/icons/icons/editpaste.png"), QSize(), QIcon::Normal, QIcon::Off);
        editPasteAction->setIcon(icon9);
        toolsOptimizeBVHAction = new QAction(PinglyMotionWindow);
        toolsOptimizeBVHAction->setObjectName("toolsOptimizeBVHAction");
        optionsSkeletonAction = new QAction(PinglyMotionWindow);
        optionsSkeletonAction->setObjectName("optionsSkeletonAction");
        optionsSkeletonAction->setCheckable(true);
        optionsJointLimitsAction = new QAction(PinglyMotionWindow);
        optionsJointLimitsAction->setObjectName("optionsJointLimitsAction");
        optionsJointLimitsAction->setCheckable(true);
        optionsLoopAction = new QAction(PinglyMotionWindow);
        optionsLoopAction->setObjectName("optionsLoopAction");
        optionsLoopAction->setCheckable(true);
        optionsProtectFirstFrameAction = new QAction(PinglyMotionWindow);
        optionsProtectFirstFrameAction->setObjectName("optionsProtectFirstFrameAction");
        optionsProtectFirstFrameAction->setCheckable(true);
        optionsShowTimelineAction = new QAction(PinglyMotionWindow);
        optionsShowTimelineAction->setObjectName("optionsShowTimelineAction");
        optionsShowTimelineAction->setCheckable(true);
        optionsConfigurePinglyMotionAction = new QAction(PinglyMotionWindow);
        optionsConfigurePinglyMotionAction->setObjectName("optionsConfigurePinglyMotionAction");
        helpAboutAction = new QAction(PinglyMotionWindow);
        helpAboutAction->setObjectName("helpAboutAction");
        resetCameraAction = new QAction(PinglyMotionWindow);
        resetCameraAction->setObjectName("resetCameraAction");
        QIcon icon10;
        icon10.addFile(QString::fromUtf8(":/icons/icons/resetcamera.png"), QSize(), QIcon::Normal, QIcon::Off);
        resetCameraAction->setIcon(icon10);
        fileExportForSecondLifeAction = new QAction(PinglyMotionWindow);
        fileExportForSecondLifeAction->setObjectName("fileExportForSecondLifeAction");
        toolsMirrorAction = new QAction(PinglyMotionWindow);
        toolsMirrorAction->setObjectName("toolsMirrorAction");
        helpWhatsNewAction = new QAction(PinglyMotionWindow);
        helpWhatsNewAction->setObjectName("helpWhatsNewAction");
        centralwidget = new QWidget(PinglyMotionWindow);
        centralwidget->setObjectName("centralwidget");
        gridLayout = new QGridLayout(centralwidget);
        gridLayout->setObjectName("gridLayout");
        gridLayout1 = new QGridLayout();
        gridLayout1->setObjectName("gridLayout1");
        animationView = new AnimationView(centralwidget);
        animationView->setObjectName("animationView");
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(animationView->sizePolicy().hasHeightForWidth());
        animationView->setSizePolicy(sizePolicy);

        gridLayout1->addWidget(animationView, 0, 0, 1, 3);

        timelineView = new TimelineView(centralwidget);
        timelineView->setObjectName("timelineView");
        QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(timelineView->sizePolicy().hasHeightForWidth());
        timelineView->setSizePolicy(sizePolicy1);
        timelineView->setMinimumSize(QSize(360, 258));

        gridLayout1->addWidget(timelineView, 1, 0, 1, 3);

        currentFrameSlider = new QSlider(centralwidget);
        currentFrameSlider->setObjectName("currentFrameSlider");
        currentFrameSlider->setOrientation(Qt::Horizontal);

        gridLayout1->addWidget(currentFrameSlider, 2, 0, 1, 1);

        playButton = new QPushButton(centralwidget);
        playButton->setObjectName("playButton");
        QIcon icon11;
        icon11.addFile(QString::fromUtf8(":/icons/icons/loop.png"), QSize(), QIcon::Normal, QIcon::Off);
        playButton->setIcon(icon11);

        gridLayout1->addWidget(playButton, 2, 1, 1, 1);

        keyframeButton = new QPushButton(centralwidget);
        keyframeButton->setObjectName("keyframeButton");
        QIcon icon12;
        icon12.addFile(QString::fromUtf8(":/icons/icons/key.png"), QSize(), QIcon::Normal, QIcon::Off);
        keyframeButton->setIcon(icon12);
        keyframeButton->setCheckable(true);

        gridLayout1->addWidget(keyframeButton, 2, 2, 1, 1);


        gridLayout->addLayout(gridLayout1, 0, 0, 2, 1);

        avatarPropsTab = new QTabWidget(centralwidget);
        avatarPropsTab->setObjectName("avatarPropsTab");
        QSizePolicy sizePolicy2(QSizePolicy::Fixed, QSizePolicy::Expanding);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(avatarPropsTab->sizePolicy().hasHeightForWidth());
        avatarPropsTab->setSizePolicy(sizePolicy2);
        avatarTab = new QWidget();
        avatarTab->setObjectName("avatarTab");
        gridLayout2 = new QGridLayout(avatarTab);
        gridLayout2->setObjectName("gridLayout2");
        animationLabel = new QLabel(avatarTab);
        animationLabel->setObjectName("animationLabel");
        QSizePolicy sizePolicy3(QSizePolicy::Fixed, QSizePolicy::Preferred);
        sizePolicy3.setHorizontalStretch(0);
        sizePolicy3.setVerticalStretch(0);
        sizePolicy3.setHeightForWidth(animationLabel->sizePolicy().hasHeightForWidth());
        animationLabel->setSizePolicy(sizePolicy3);

        gridLayout2->addWidget(animationLabel, 0, 0, 1, 1);

        selectAnimationCombo = new QComboBox(avatarTab);
        selectAnimationCombo->setObjectName("selectAnimationCombo");
        QSizePolicy sizePolicy4(QSizePolicy::Minimum, QSizePolicy::Fixed);
        sizePolicy4.setHorizontalStretch(0);
        sizePolicy4.setVerticalStretch(0);
        sizePolicy4.setHeightForWidth(selectAnimationCombo->sizePolicy().hasHeightForWidth());
        selectAnimationCombo->setSizePolicy(sizePolicy4);

        gridLayout2->addWidget(selectAnimationCombo, 0, 1, 1, 3);

        figureLabel = new QLabel(avatarTab);
        figureLabel->setObjectName("figureLabel");
        sizePolicy3.setHeightForWidth(figureLabel->sizePolicy().hasHeightForWidth());
        figureLabel->setSizePolicy(sizePolicy3);

        gridLayout2->addWidget(figureLabel, 1, 0, 1, 1);

        figureCombo = new QComboBox(avatarTab);
        figureCombo->addItem(QString());
        figureCombo->addItem(QString());
        figureCombo->setObjectName("figureCombo");
        sizePolicy4.setHeightForWidth(figureCombo->sizePolicy().hasHeightForWidth());
        figureCombo->setSizePolicy(sizePolicy4);

        gridLayout2->addWidget(figureCombo, 1, 1, 1, 1);

        scaleLabel = new QLabel(avatarTab);
        scaleLabel->setObjectName("scaleLabel");
        sizePolicy3.setHeightForWidth(scaleLabel->sizePolicy().hasHeightForWidth());
        scaleLabel->setSizePolicy(sizePolicy3);

        gridLayout2->addWidget(scaleLabel, 1, 2, 1, 1);

        scaleSpin = new QSpinBox(avatarTab);
        scaleSpin->setObjectName("scaleSpin");
        QSizePolicy sizePolicy5(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy5.setHorizontalStretch(0);
        sizePolicy5.setVerticalStretch(0);
        sizePolicy5.setHeightForWidth(scaleSpin->sizePolicy().hasHeightForWidth());
        scaleSpin->setSizePolicy(sizePolicy5);
        scaleSpin->setMinimum(30);
        scaleSpin->setMaximum(200);
        scaleSpin->setValue(100);

        gridLayout2->addWidget(scaleSpin, 1, 3, 1, 1);

        jointPicker = new QWidget(avatarTab);
        jointPicker->setObjectName("jointPicker");
        QSizePolicy sizePolicy6(QSizePolicy::Preferred, QSizePolicy::Expanding);
        sizePolicy6.setHorizontalStretch(0);
        sizePolicy6.setVerticalStretch(1);
        sizePolicy6.setHeightForWidth(jointPicker->sizePolicy().hasHeightForWidth());
        jointPicker->setSizePolicy(sizePolicy6);

        gridLayout2->addWidget(jointPicker, 2, 0, 1, 4);

        rotationGroupBox = new QGroupBox(avatarTab);
        rotationGroupBox->setObjectName("rotationGroupBox");
        gridLayout3 = new QGridLayout(rotationGroupBox);
        gridLayout3->setObjectName("gridLayout3");
        yRotationLabel = new QLabel(rotationGroupBox);
        yRotationLabel->setObjectName("yRotationLabel");
        sizePolicy3.setHeightForWidth(yRotationLabel->sizePolicy().hasHeightForWidth());
        yRotationLabel->setSizePolicy(sizePolicy3);

        gridLayout3->addWidget(yRotationLabel, 2, 0, 1, 1);

        yRotationEdit = new QLineEdit(rotationGroupBox);
        yRotationEdit->setObjectName("yRotationEdit");
        QSizePolicy sizePolicy7(QSizePolicy::Preferred, QSizePolicy::Fixed);
        sizePolicy7.setHorizontalStretch(0);
        sizePolicy7.setVerticalStretch(0);
        sizePolicy7.setHeightForWidth(yRotationEdit->sizePolicy().hasHeightForWidth());
        yRotationEdit->setSizePolicy(sizePolicy7);

        gridLayout3->addWidget(yRotationEdit, 2, 1, 1, 1);

        altLabel = new QLabel(rotationGroupBox);
        altLabel->setObjectName("altLabel");
        sizePolicy3.setHeightForWidth(altLabel->sizePolicy().hasHeightForWidth());
        altLabel->setSizePolicy(sizePolicy3);
        altLabel->setMinimumSize(QSize(40, 0));
        altLabel->setStyleSheet(QString::fromUtf8("color: #20af00;\n"
""));
        altLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout3->addWidget(altLabel, 2, 2, 1, 1);

        yRotationSlider = new QSlider(rotationGroupBox);
        yRotationSlider->setObjectName("yRotationSlider");
        sizePolicy7.setHeightForWidth(yRotationSlider->sizePolicy().hasHeightForWidth());
        yRotationSlider->setSizePolicy(sizePolicy7);
        yRotationSlider->setOrientation(Qt::Horizontal);

        gridLayout3->addWidget(yRotationSlider, 3, 0, 1, 3);

        zRotationLabel = new QLabel(rotationGroupBox);
        zRotationLabel->setObjectName("zRotationLabel");
        sizePolicy3.setHeightForWidth(zRotationLabel->sizePolicy().hasHeightForWidth());
        zRotationLabel->setSizePolicy(sizePolicy3);

        gridLayout3->addWidget(zRotationLabel, 4, 0, 1, 1);

        zRotationEdit = new QLineEdit(rotationGroupBox);
        zRotationEdit->setObjectName("zRotationEdit");
        sizePolicy7.setHeightForWidth(zRotationEdit->sizePolicy().hasHeightForWidth());
        zRotationEdit->setSizePolicy(sizePolicy7);

        gridLayout3->addWidget(zRotationEdit, 4, 1, 1, 1);

        ctrlLabel = new QLabel(rotationGroupBox);
        ctrlLabel->setObjectName("ctrlLabel");
        sizePolicy3.setHeightForWidth(ctrlLabel->sizePolicy().hasHeightForWidth());
        ctrlLabel->setSizePolicy(sizePolicy3);
        ctrlLabel->setMinimumSize(QSize(40, 0));
        ctrlLabel->setStyleSheet(QString::fromUtf8("color: #3c78ff;\n"
""));
        ctrlLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout3->addWidget(ctrlLabel, 4, 2, 1, 1);

        zRotationSlider = new QSlider(rotationGroupBox);
        zRotationSlider->setObjectName("zRotationSlider");
        sizePolicy7.setHeightForWidth(zRotationSlider->sizePolicy().hasHeightForWidth());
        zRotationSlider->setSizePolicy(sizePolicy7);
        zRotationSlider->setOrientation(Qt::Horizontal);

        gridLayout3->addWidget(zRotationSlider, 5, 0, 1, 3);

        xRotationLabel = new QLabel(rotationGroupBox);
        xRotationLabel->setObjectName("xRotationLabel");
        sizePolicy3.setHeightForWidth(xRotationLabel->sizePolicy().hasHeightForWidth());
        xRotationLabel->setSizePolicy(sizePolicy3);

        gridLayout3->addWidget(xRotationLabel, 0, 0, 1, 1);

        xRotationEdit = new QLineEdit(rotationGroupBox);
        xRotationEdit->setObjectName("xRotationEdit");
        sizePolicy7.setHeightForWidth(xRotationEdit->sizePolicy().hasHeightForWidth());
        xRotationEdit->setSizePolicy(sizePolicy7);

        gridLayout3->addWidget(xRotationEdit, 0, 1, 1, 1);

        shiftLabel = new QLabel(rotationGroupBox);
        shiftLabel->setObjectName("shiftLabel");
        sizePolicy3.setHeightForWidth(shiftLabel->sizePolicy().hasHeightForWidth());
        shiftLabel->setSizePolicy(sizePolicy3);
        shiftLabel->setMinimumSize(QSize(40, 0));
        shiftLabel->setStyleSheet(QString::fromUtf8("color: #e05050;\n"
""));
        shiftLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout3->addWidget(shiftLabel, 0, 2, 1, 1);

        xRotationSlider = new QSlider(rotationGroupBox);
        xRotationSlider->setObjectName("xRotationSlider");
        sizePolicy7.setHeightForWidth(xRotationSlider->sizePolicy().hasHeightForWidth());
        xRotationSlider->setSizePolicy(sizePolicy7);
        xRotationSlider->setOrientation(Qt::Horizontal);

        gridLayout3->addWidget(xRotationSlider, 1, 0, 1, 3);


        gridLayout2->addWidget(rotationGroupBox, 3, 0, 1, 4);

        positionGroupBox = new QGroupBox(avatarTab);
        positionGroupBox->setObjectName("positionGroupBox");
        gridLayout4 = new QGridLayout(positionGroupBox);
        gridLayout4->setObjectName("gridLayout4");
        xPositionLabel = new QLabel(positionGroupBox);
        xPositionLabel->setObjectName("xPositionLabel");
        sizePolicy3.setHeightForWidth(xPositionLabel->sizePolicy().hasHeightForWidth());
        xPositionLabel->setSizePolicy(sizePolicy3);

        gridLayout4->addWidget(xPositionLabel, 0, 0, 1, 1);

        xPositionEdit = new QLineEdit(positionGroupBox);
        xPositionEdit->setObjectName("xPositionEdit");
        sizePolicy7.setHeightForWidth(xPositionEdit->sizePolicy().hasHeightForWidth());
        xPositionEdit->setSizePolicy(sizePolicy7);

        gridLayout4->addWidget(xPositionEdit, 0, 1, 1, 1);

        xPositionSlider = new QSlider(positionGroupBox);
        xPositionSlider->setObjectName("xPositionSlider");
        sizePolicy7.setHeightForWidth(xPositionSlider->sizePolicy().hasHeightForWidth());
        xPositionSlider->setSizePolicy(sizePolicy7);
        xPositionSlider->setMinimum(-19600);
        xPositionSlider->setMaximum(19600);
        xPositionSlider->setOrientation(Qt::Horizontal);

        gridLayout4->addWidget(xPositionSlider, 1, 0, 1, 2);

        ypositionLabel = new QLabel(positionGroupBox);
        ypositionLabel->setObjectName("ypositionLabel");
        sizePolicy3.setHeightForWidth(ypositionLabel->sizePolicy().hasHeightForWidth());
        ypositionLabel->setSizePolicy(sizePolicy3);

        gridLayout4->addWidget(ypositionLabel, 2, 0, 1, 1);

        yPositionEdit = new QLineEdit(positionGroupBox);
        yPositionEdit->setObjectName("yPositionEdit");
        sizePolicy7.setHeightForWidth(yPositionEdit->sizePolicy().hasHeightForWidth());
        yPositionEdit->setSizePolicy(sizePolicy7);

        gridLayout4->addWidget(yPositionEdit, 2, 1, 1, 1);

        yPositionSlider = new QSlider(positionGroupBox);
        yPositionSlider->setObjectName("yPositionSlider");
        sizePolicy7.setHeightForWidth(yPositionSlider->sizePolicy().hasHeightForWidth());
        yPositionSlider->setSizePolicy(sizePolicy7);
        yPositionSlider->setMinimum(-19600);
        yPositionSlider->setMaximum(19600);
        yPositionSlider->setOrientation(Qt::Horizontal);

        gridLayout4->addWidget(yPositionSlider, 3, 0, 1, 2);

        zRotationLabel_2 = new QLabel(positionGroupBox);
        zRotationLabel_2->setObjectName("zRotationLabel_2");
        sizePolicy3.setHeightForWidth(zRotationLabel_2->sizePolicy().hasHeightForWidth());
        zRotationLabel_2->setSizePolicy(sizePolicy3);

        gridLayout4->addWidget(zRotationLabel_2, 4, 0, 1, 1);

        zPositionEdit = new QLineEdit(positionGroupBox);
        zPositionEdit->setObjectName("zPositionEdit");
        sizePolicy7.setHeightForWidth(zPositionEdit->sizePolicy().hasHeightForWidth());
        zPositionEdit->setSizePolicy(sizePolicy7);

        gridLayout4->addWidget(zPositionEdit, 4, 1, 1, 1);

        zPositionSlider = new QSlider(positionGroupBox);
        zPositionSlider->setObjectName("zPositionSlider");
        sizePolicy7.setHeightForWidth(zPositionSlider->sizePolicy().hasHeightForWidth());
        zPositionSlider->setSizePolicy(sizePolicy7);
        zPositionSlider->setMinimum(-19600);
        zPositionSlider->setMaximum(19600);
        zPositionSlider->setOrientation(Qt::Horizontal);

        gridLayout4->addWidget(zPositionSlider, 5, 0, 1, 2);


        gridLayout2->addWidget(positionGroupBox, 4, 0, 1, 4);

        easeInOutGroup = new QGroupBox(avatarTab);
        easeInOutGroup->setObjectName("easeInOutGroup");
        gridLayout5 = new QGridLayout(easeInOutGroup);
        gridLayout5->setObjectName("gridLayout5");
        easeInCheck = new QCheckBox(easeInOutGroup);
        easeInCheck->setObjectName("easeInCheck");
        sizePolicy7.setHeightForWidth(easeInCheck->sizePolicy().hasHeightForWidth());
        easeInCheck->setSizePolicy(sizePolicy7);

        gridLayout5->addWidget(easeInCheck, 0, 0, 1, 1);

        easeOutCheck = new QCheckBox(easeInOutGroup);
        easeOutCheck->setObjectName("easeOutCheck");
        sizePolicy7.setHeightForWidth(easeOutCheck->sizePolicy().hasHeightForWidth());
        easeOutCheck->setSizePolicy(sizePolicy7);

        gridLayout5->addWidget(easeOutCheck, 0, 1, 1, 1);


        gridLayout2->addWidget(easeInOutGroup, 5, 0, 1, 4);

        spacerItem = new QSpacerItem(20, 31, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout2->addItem(spacerItem, 6, 1, 1, 2);

        avatarPropsTab->addTab(avatarTab, QString());
        propsTab = new QWidget();
        propsTab->setObjectName("propsTab");
        gridLayout6 = new QGridLayout(propsTab);
        gridLayout6->setObjectName("gridLayout6");
        hboxLayout = new QHBoxLayout();
        hboxLayout->setObjectName("hboxLayout");
        newBoxPropButton = new QPushButton(propsTab);
        newBoxPropButton->setObjectName("newBoxPropButton");
        QIcon icon13;
        icon13.addFile(QString::fromUtf8(":/icons/icons/box.png"), QSize(), QIcon::Normal, QIcon::Off);
        newBoxPropButton->setIcon(icon13);

        hboxLayout->addWidget(newBoxPropButton);

        newSpherePropButton = new QPushButton(propsTab);
        newSpherePropButton->setObjectName("newSpherePropButton");
        QIcon icon14;
        icon14.addFile(QString::fromUtf8(":/icons/icons/sphere.png"), QSize(), QIcon::Normal, QIcon::Off);
        newSpherePropButton->setIcon(icon14);

        hboxLayout->addWidget(newSpherePropButton);

        newConePropButton = new QPushButton(propsTab);
        newConePropButton->setObjectName("newConePropButton");
        QIcon icon15;
        icon15.addFile(QString::fromUtf8(":/icons/icons/cone.png"), QSize(), QIcon::Normal, QIcon::Off);
        newConePropButton->setIcon(icon15);

        hboxLayout->addWidget(newConePropButton);

        newTorusPropButton = new QPushButton(propsTab);
        newTorusPropButton->setObjectName("newTorusPropButton");
        QIcon icon16;
        icon16.addFile(QString::fromUtf8(":/icons/icons/torus.png"), QSize(), QIcon::Normal, QIcon::Off);
        newTorusPropButton->setIcon(icon16);

        hboxLayout->addWidget(newTorusPropButton);


        gridLayout6->addLayout(hboxLayout, 0, 0, 1, 3);

        propNameLabel = new QLabel(propsTab);
        propNameLabel->setObjectName("propNameLabel");
        sizePolicy3.setHeightForWidth(propNameLabel->sizePolicy().hasHeightForWidth());
        propNameLabel->setSizePolicy(sizePolicy3);

        gridLayout6->addWidget(propNameLabel, 1, 0, 1, 1);

        propNameCombo = new QComboBox(propsTab);
        propNameCombo->setObjectName("propNameCombo");
        sizePolicy7.setHeightForWidth(propNameCombo->sizePolicy().hasHeightForWidth());
        propNameCombo->setSizePolicy(sizePolicy7);

        gridLayout6->addWidget(propNameCombo, 1, 1, 1, 1);

        deletePropButton = new QPushButton(propsTab);
        deletePropButton->setObjectName("deletePropButton");
        sizePolicy5.setHeightForWidth(deletePropButton->sizePolicy().hasHeightForWidth());
        deletePropButton->setSizePolicy(sizePolicy5);
        QIcon icon17;
        icon17.addFile(QString::fromUtf8(":/icons/icons/delete.png"), QSize(), QIcon::Normal, QIcon::Off);
        deletePropButton->setIcon(icon17);

        gridLayout6->addWidget(deletePropButton, 1, 2, 1, 1);

        attachToLabel = new QLabel(propsTab);
        attachToLabel->setObjectName("attachToLabel");
        sizePolicy3.setHeightForWidth(attachToLabel->sizePolicy().hasHeightForWidth());
        attachToLabel->setSizePolicy(sizePolicy3);

        gridLayout6->addWidget(attachToLabel, 2, 0, 1, 1);

        attachToComboBox = new QComboBox(propsTab);
        attachToComboBox->addItem(QString());
        attachToComboBox->addItem(QString());
        attachToComboBox->addItem(QString());
        attachToComboBox->addItem(QString());
        attachToComboBox->addItem(QString());
        attachToComboBox->addItem(QString());
        attachToComboBox->addItem(QString());
        attachToComboBox->addItem(QString());
        attachToComboBox->addItem(QString());
        attachToComboBox->addItem(QString());
        attachToComboBox->addItem(QString());
        attachToComboBox->addItem(QString());
        attachToComboBox->addItem(QString());
        attachToComboBox->addItem(QString());
        attachToComboBox->addItem(QString());
        attachToComboBox->addItem(QString());
        attachToComboBox->addItem(QString());
        attachToComboBox->addItem(QString());
        attachToComboBox->addItem(QString());
        attachToComboBox->addItem(QString());
        attachToComboBox->addItem(QString());
        attachToComboBox->addItem(QString());
        attachToComboBox->addItem(QString());
        attachToComboBox->addItem(QString());
        attachToComboBox->setObjectName("attachToComboBox");

        gridLayout6->addWidget(attachToComboBox, 2, 1, 1, 2);

        propPositionGroup = new QGroupBox(propsTab);
        propPositionGroup->setObjectName("propPositionGroup");
        gridLayout7 = new QGridLayout(propPositionGroup);
        gridLayout7->setObjectName("gridLayout7");
        propXPosLabel = new QLabel(propPositionGroup);
        propXPosLabel->setObjectName("propXPosLabel");
        sizePolicy3.setHeightForWidth(propXPosLabel->sizePolicy().hasHeightForWidth());
        propXPosLabel->setSizePolicy(sizePolicy3);

        gridLayout7->addWidget(propXPosLabel, 0, 0, 1, 1);

        propXPosSpin = new QSpinBox(propPositionGroup);
        propXPosSpin->setObjectName("propXPosSpin");
        sizePolicy7.setHeightForWidth(propXPosSpin->sizePolicy().hasHeightForWidth());
        propXPosSpin->setSizePolicy(sizePolicy7);
        propXPosSpin->setMinimum(-9999);
        propXPosSpin->setMaximum(9999);

        gridLayout7->addWidget(propXPosSpin, 0, 1, 1, 1);

        propYPosLabel = new QLabel(propPositionGroup);
        propYPosLabel->setObjectName("propYPosLabel");
        sizePolicy3.setHeightForWidth(propYPosLabel->sizePolicy().hasHeightForWidth());
        propYPosLabel->setSizePolicy(sizePolicy3);

        gridLayout7->addWidget(propYPosLabel, 1, 0, 1, 1);

        propYPosSpin = new QSpinBox(propPositionGroup);
        propYPosSpin->setObjectName("propYPosSpin");
        sizePolicy7.setHeightForWidth(propYPosSpin->sizePolicy().hasHeightForWidth());
        propYPosSpin->setSizePolicy(sizePolicy7);
        propYPosSpin->setMinimum(-9999);
        propYPosSpin->setMaximum(9999);

        gridLayout7->addWidget(propYPosSpin, 1, 1, 1, 1);

        propYPosLabel_2 = new QLabel(propPositionGroup);
        propYPosLabel_2->setObjectName("propYPosLabel_2");
        sizePolicy3.setHeightForWidth(propYPosLabel_2->sizePolicy().hasHeightForWidth());
        propYPosLabel_2->setSizePolicy(sizePolicy3);

        gridLayout7->addWidget(propYPosLabel_2, 2, 0, 1, 1);

        propZPosSpin = new QSpinBox(propPositionGroup);
        propZPosSpin->setObjectName("propZPosSpin");
        sizePolicy7.setHeightForWidth(propZPosSpin->sizePolicy().hasHeightForWidth());
        propZPosSpin->setSizePolicy(sizePolicy7);
        propZPosSpin->setMinimum(-9999);
        propZPosSpin->setMaximum(9999);

        gridLayout7->addWidget(propZPosSpin, 2, 1, 1, 1);


        gridLayout6->addWidget(propPositionGroup, 3, 0, 1, 3);

        propScaleGroup = new QGroupBox(propsTab);
        propScaleGroup->setObjectName("propScaleGroup");
        gridLayout8 = new QGridLayout(propScaleGroup);
        gridLayout8->setObjectName("gridLayout8");
        propXScaleLabel = new QLabel(propScaleGroup);
        propXScaleLabel->setObjectName("propXScaleLabel");
        sizePolicy3.setHeightForWidth(propXScaleLabel->sizePolicy().hasHeightForWidth());
        propXScaleLabel->setSizePolicy(sizePolicy3);

        gridLayout8->addWidget(propXScaleLabel, 0, 0, 1, 1);

        propXScaleSpin = new QSpinBox(propScaleGroup);
        propXScaleSpin->setObjectName("propXScaleSpin");
        sizePolicy7.setHeightForWidth(propXScaleSpin->sizePolicy().hasHeightForWidth());
        propXScaleSpin->setSizePolicy(sizePolicy7);
        propXScaleSpin->setMinimum(-9999);
        propXScaleSpin->setMaximum(9999);

        gridLayout8->addWidget(propXScaleSpin, 0, 1, 1, 1);

        propYScaleLabel = new QLabel(propScaleGroup);
        propYScaleLabel->setObjectName("propYScaleLabel");
        sizePolicy3.setHeightForWidth(propYScaleLabel->sizePolicy().hasHeightForWidth());
        propYScaleLabel->setSizePolicy(sizePolicy3);

        gridLayout8->addWidget(propYScaleLabel, 1, 0, 1, 1);

        propYScaleSpin = new QSpinBox(propScaleGroup);
        propYScaleSpin->setObjectName("propYScaleSpin");
        sizePolicy7.setHeightForWidth(propYScaleSpin->sizePolicy().hasHeightForWidth());
        propYScaleSpin->setSizePolicy(sizePolicy7);
        propYScaleSpin->setMinimum(-9999);
        propYScaleSpin->setMaximum(9999);

        gridLayout8->addWidget(propYScaleSpin, 1, 1, 1, 1);

        propZScaleLabel = new QLabel(propScaleGroup);
        propZScaleLabel->setObjectName("propZScaleLabel");
        sizePolicy3.setHeightForWidth(propZScaleLabel->sizePolicy().hasHeightForWidth());
        propZScaleLabel->setSizePolicy(sizePolicy3);

        gridLayout8->addWidget(propZScaleLabel, 2, 0, 1, 1);

        propZScaleSpin = new QSpinBox(propScaleGroup);
        propZScaleSpin->setObjectName("propZScaleSpin");
        sizePolicy7.setHeightForWidth(propZScaleSpin->sizePolicy().hasHeightForWidth());
        propZScaleSpin->setSizePolicy(sizePolicy7);
        propZScaleSpin->setMinimum(-9999);
        propZScaleSpin->setMaximum(9999);

        gridLayout8->addWidget(propZScaleSpin, 2, 1, 1, 1);


        gridLayout6->addWidget(propScaleGroup, 4, 0, 1, 3);

        propRotationGroup = new QGroupBox(propsTab);
        propRotationGroup->setObjectName("propRotationGroup");
        gridLayout9 = new QGridLayout(propRotationGroup);
        gridLayout9->setObjectName("gridLayout9");
        propXRotLabel = new QLabel(propRotationGroup);
        propXRotLabel->setObjectName("propXRotLabel");
        sizePolicy3.setHeightForWidth(propXRotLabel->sizePolicy().hasHeightForWidth());
        propXRotLabel->setSizePolicy(sizePolicy3);

        gridLayout9->addWidget(propXRotLabel, 0, 0, 1, 1);

        propXRotSpin = new QSpinBox(propRotationGroup);
        propXRotSpin->setObjectName("propXRotSpin");
        sizePolicy7.setHeightForWidth(propXRotSpin->sizePolicy().hasHeightForWidth());
        propXRotSpin->setSizePolicy(sizePolicy7);
        propXRotSpin->setMinimum(-9999);
        propXRotSpin->setMaximum(9999);

        gridLayout9->addWidget(propXRotSpin, 0, 1, 1, 1);

        propYRotLabel = new QLabel(propRotationGroup);
        propYRotLabel->setObjectName("propYRotLabel");
        sizePolicy3.setHeightForWidth(propYRotLabel->sizePolicy().hasHeightForWidth());
        propYRotLabel->setSizePolicy(sizePolicy3);

        gridLayout9->addWidget(propYRotLabel, 1, 0, 1, 1);

        propYRotSpin = new QSpinBox(propRotationGroup);
        propYRotSpin->setObjectName("propYRotSpin");
        sizePolicy7.setHeightForWidth(propYRotSpin->sizePolicy().hasHeightForWidth());
        propYRotSpin->setSizePolicy(sizePolicy7);
        propYRotSpin->setMinimum(-9999);
        propYRotSpin->setMaximum(9999);

        gridLayout9->addWidget(propYRotSpin, 1, 1, 1, 1);

        propZRotLabel = new QLabel(propRotationGroup);
        propZRotLabel->setObjectName("propZRotLabel");
        sizePolicy3.setHeightForWidth(propZRotLabel->sizePolicy().hasHeightForWidth());
        propZRotLabel->setSizePolicy(sizePolicy3);

        gridLayout9->addWidget(propZRotLabel, 2, 0, 1, 1);

        propZRotSpin = new QSpinBox(propRotationGroup);
        propZRotSpin->setObjectName("propZRotSpin");
        sizePolicy7.setHeightForWidth(propZRotSpin->sizePolicy().hasHeightForWidth());
        propZRotSpin->setSizePolicy(sizePolicy7);
        propZRotSpin->setMinimum(-9999);
        propZRotSpin->setMaximum(9999);

        gridLayout9->addWidget(propZRotSpin, 2, 1, 1, 1);


        gridLayout6->addWidget(propRotationGroup, 5, 0, 1, 3);

        spacerItem1 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout6->addItem(spacerItem1, 6, 1, 1, 1);

        avatarPropsTab->addTab(propsTab, QString());

        gridLayout->addWidget(avatarPropsTab, 0, 1, 1, 1);

        gridLayout10 = new QGridLayout();
        gridLayout10->setObjectName("gridLayout10");
        spacerItem2 = new QSpacerItem(0, 20, QSizePolicy::Preferred, QSizePolicy::Minimum);

        gridLayout10->addItem(spacerItem2, 0, 0, 1, 1);

        loopInLabel = new QLabel(centralwidget);
        loopInLabel->setObjectName("loopInLabel");
        sizePolicy3.setHeightForWidth(loopInLabel->sizePolicy().hasHeightForWidth());
        loopInLabel->setSizePolicy(sizePolicy3);

        gridLayout10->addWidget(loopInLabel, 0, 1, 1, 1);

        loopInSpinBox = new QSpinBox(centralwidget);
        loopInSpinBox->setObjectName("loopInSpinBox");
        loopInSpinBox->setMaximum(9999);

        gridLayout10->addWidget(loopInSpinBox, 0, 2, 1, 1);

        loopInPercentLabel = new QLabel(centralwidget);
        loopInPercentLabel->setObjectName("loopInPercentLabel");
        sizePolicy3.setHeightForWidth(loopInPercentLabel->sizePolicy().hasHeightForWidth());
        loopInPercentLabel->setSizePolicy(sizePolicy3);
        loopInPercentLabel->setMinimumSize(QSize(60, 0));
        loopInPercentLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout10->addWidget(loopInPercentLabel, 0, 3, 1, 1);

        spacerItem3 = new QSpacerItem(0, 20, QSizePolicy::Preferred, QSizePolicy::Minimum);

        gridLayout10->addItem(spacerItem3, 1, 0, 1, 1);

        loopOutLabel = new QLabel(centralwidget);
        loopOutLabel->setObjectName("loopOutLabel");
        sizePolicy3.setHeightForWidth(loopOutLabel->sizePolicy().hasHeightForWidth());
        loopOutLabel->setSizePolicy(sizePolicy3);

        gridLayout10->addWidget(loopOutLabel, 1, 1, 1, 1);

        loopOutSpinBox = new QSpinBox(centralwidget);
        loopOutSpinBox->setObjectName("loopOutSpinBox");
        loopOutSpinBox->setMaximum(9999);

        gridLayout10->addWidget(loopOutSpinBox, 1, 2, 1, 1);

        loopOutPercentLabel = new QLabel(centralwidget);
        loopOutPercentLabel->setObjectName("loopOutPercentLabel");
        sizePolicy3.setHeightForWidth(loopOutPercentLabel->sizePolicy().hasHeightForWidth());
        loopOutPercentLabel->setSizePolicy(sizePolicy3);
        loopOutPercentLabel->setMinimumSize(QSize(60, 0));
        loopOutPercentLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout10->addWidget(loopOutPercentLabel, 1, 3, 1, 1);

        hboxLayout1 = new QHBoxLayout();
        hboxLayout1->setObjectName("hboxLayout1");
        spacerItem4 = new QSpacerItem(0, 20, QSizePolicy::Preferred, QSizePolicy::Minimum);

        hboxLayout1->addItem(spacerItem4);

        frameLabel = new QLabel(centralwidget);
        frameLabel->setObjectName("frameLabel");
        sizePolicy3.setHeightForWidth(frameLabel->sizePolicy().hasHeightForWidth());
        frameLabel->setSizePolicy(sizePolicy3);

        hboxLayout1->addWidget(frameLabel);

        currentFrameLabel = new QLabel(centralwidget);
        currentFrameLabel->setObjectName("currentFrameLabel");
        sizePolicy3.setHeightForWidth(currentFrameLabel->sizePolicy().hasHeightForWidth());
        currentFrameLabel->setSizePolicy(sizePolicy3);
        currentFrameLabel->setMinimumSize(QSize(40, 0));
        currentFrameLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        hboxLayout1->addWidget(currentFrameLabel);

        ofLabel = new QLabel(centralwidget);
        ofLabel->setObjectName("ofLabel");
        sizePolicy3.setHeightForWidth(ofLabel->sizePolicy().hasHeightForWidth());
        ofLabel->setSizePolicy(sizePolicy3);

        hboxLayout1->addWidget(ofLabel);

        framesSpin = new QSpinBox(centralwidget);
        framesSpin->setObjectName("framesSpin");
        sizePolicy5.setHeightForWidth(framesSpin->sizePolicy().hasHeightForWidth());
        framesSpin->setSizePolicy(sizePolicy5);
        framesSpin->setMaximum(9999);

        hboxLayout1->addWidget(framesSpin);

        fpsLabel = new QLabel(centralwidget);
        fpsLabel->setObjectName("fpsLabel");
        sizePolicy3.setHeightForWidth(fpsLabel->sizePolicy().hasHeightForWidth());
        fpsLabel->setSizePolicy(sizePolicy3);

        hboxLayout1->addWidget(fpsLabel);

        fpsSpin = new QSpinBox(centralwidget);
        fpsSpin->setObjectName("fpsSpin");
        sizePolicy5.setHeightForWidth(fpsSpin->sizePolicy().hasHeightForWidth());
        fpsSpin->setSizePolicy(sizePolicy5);
        fpsSpin->setMinimum(1);
        fpsSpin->setMaximum(50);

        hboxLayout1->addWidget(fpsSpin);


        gridLayout10->addLayout(hboxLayout1, 2, 0, 1, 4);


        gridLayout->addLayout(gridLayout10, 1, 1, 1, 1);

        PinglyMotionWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(PinglyMotionWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 669, 21));
        menuFile = new QMenu(menubar);
        menuFile->setObjectName("menuFile");
        menuEdit = new QMenu(menubar);
        menuEdit->setObjectName("menuEdit");
        menuOptions = new QMenu(menubar);
        menuOptions->setObjectName("menuOptions");
        menuHelp = new QMenu(menubar);
        menuHelp->setObjectName("menuHelp");
        menuTools = new QMenu(menubar);
        menuTools->setObjectName("menuTools");
        PinglyMotionWindow->setMenuBar(menubar);
        toolBar = new QToolBar(PinglyMotionWindow);
        toolBar->setObjectName("toolBar");
        PinglyMotionWindow->addToolBar(Qt::TopToolBarArea, toolBar);

        menubar->addAction(menuFile->menuAction());
        menubar->addAction(menuEdit->menuAction());
        menubar->addAction(menuTools->menuAction());
        menubar->addAction(menuOptions->menuAction());
        menubar->addAction(menuHelp->menuAction());
        menuFile->addAction(fileNewAction);
        menuFile->addAction(fileOpenAction);
        menuFile->addAction(fileAddAction);
        menuFile->addSeparator();
        menuFile->addAction(fileSaveAction);
        menuFile->addAction(fileSaveAsAction);
        menuFile->addAction(fileExportForSecondLifeAction);
        menuFile->addSeparator();
        menuFile->addAction(fileLoadPropsAction);
        menuFile->addAction(fileSavePropsAction);
        menuFile->addSeparator();
        menuFile->addAction(fileExitAction);
        menuEdit->addAction(editUndoAction);
        menuEdit->addAction(editRedoAction);
        menuEdit->addSeparator();
        menuEdit->addAction(editCutAction);
        menuEdit->addAction(editCopyAction);
        menuEdit->addAction(editPasteAction);
        menuOptions->addAction(optionsSkeletonAction);
        menuOptions->addAction(optionsJointLimitsAction);
        menuOptions->addAction(optionsLoopAction);
        menuOptions->addAction(optionsProtectFirstFrameAction);
        menuOptions->addAction(optionsShowTimelineAction);
        menuOptions->addAction(optionsConfigurePinglyMotionAction);
        menuHelp->addAction(helpWhatsNewAction);
        menuHelp->addAction(helpAboutAction);
        menuTools->addAction(toolsOptimizeBVHAction);
        menuTools->addAction(toolsMirrorAction);
        toolBar->addAction(fileNewAction);
        toolBar->addAction(fileOpenAction);
        toolBar->addAction(fileAddAction);
        toolBar->addSeparator();
        toolBar->addAction(fileSaveAction);
        toolBar->addAction(fileSaveAsAction);
        toolBar->addSeparator();
        toolBar->addAction(editCutAction);
        toolBar->addAction(editCopyAction);
        toolBar->addAction(editPasteAction);
        toolBar->addSeparator();
        toolBar->addAction(editUndoAction);
        toolBar->addAction(editRedoAction);
        toolBar->addSeparator();
        toolBar->addAction(resetCameraAction);

        retranslateUi(PinglyMotionWindow);

        avatarPropsTab->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(PinglyMotionWindow);
    } // setupUi

    void retranslateUi(QMainWindow *PinglyMotionWindow)
    {
        PinglyMotionWindow->setWindowTitle(QCoreApplication::translate("PinglyMotionWindow", "MainWindow", nullptr));
        fileNewAction->setText(QCoreApplication::translate("PinglyMotionWindow", "New", nullptr));
#if QT_CONFIG(shortcut)
        fileNewAction->setShortcut(QCoreApplication::translate("PinglyMotionWindow", "Ctrl+N", nullptr));
#endif // QT_CONFIG(shortcut)
        fileOpenAction->setText(QCoreApplication::translate("PinglyMotionWindow", "Open...", nullptr));
#if QT_CONFIG(shortcut)
        fileOpenAction->setShortcut(QCoreApplication::translate("PinglyMotionWindow", "Ctrl+O", nullptr));
#endif // QT_CONFIG(shortcut)
        fileAddAction->setText(QCoreApplication::translate("PinglyMotionWindow", "Add...", nullptr));
        fileSaveAction->setText(QCoreApplication::translate("PinglyMotionWindow", "Save", nullptr));
#if QT_CONFIG(shortcut)
        fileSaveAction->setShortcut(QCoreApplication::translate("PinglyMotionWindow", "Ctrl+S", nullptr));
#endif // QT_CONFIG(shortcut)
        fileSaveAsAction->setText(QCoreApplication::translate("PinglyMotionWindow", "Save As...", nullptr));
#if QT_CONFIG(shortcut)
        fileSaveAsAction->setShortcut(QCoreApplication::translate("PinglyMotionWindow", "Ctrl+A", nullptr));
#endif // QT_CONFIG(shortcut)
        fileLoadPropsAction->setText(QCoreApplication::translate("PinglyMotionWindow", "Load Props...", nullptr));
        fileSavePropsAction->setText(QCoreApplication::translate("PinglyMotionWindow", "Save Props...", nullptr));
        fileExitAction->setText(QCoreApplication::translate("PinglyMotionWindow", "Exit", nullptr));
#if QT_CONFIG(shortcut)
        fileExitAction->setShortcut(QCoreApplication::translate("PinglyMotionWindow", "Ctrl+Q", nullptr));
#endif // QT_CONFIG(shortcut)
        editUndoAction->setText(QCoreApplication::translate("PinglyMotionWindow", "Undo", nullptr));
#if QT_CONFIG(shortcut)
        editUndoAction->setShortcut(QCoreApplication::translate("PinglyMotionWindow", "Ctrl+Z", nullptr));
#endif // QT_CONFIG(shortcut)
        editRedoAction->setText(QCoreApplication::translate("PinglyMotionWindow", "Redo", nullptr));
#if QT_CONFIG(shortcut)
        editRedoAction->setShortcut(QCoreApplication::translate("PinglyMotionWindow", "Ctrl+Shift+Z", nullptr));
#endif // QT_CONFIG(shortcut)
        editCutAction->setText(QCoreApplication::translate("PinglyMotionWindow", "Cut", nullptr));
#if QT_CONFIG(shortcut)
        editCutAction->setShortcut(QCoreApplication::translate("PinglyMotionWindow", "Ctrl+X", nullptr));
#endif // QT_CONFIG(shortcut)
        editCopyAction->setText(QCoreApplication::translate("PinglyMotionWindow", "Copy", nullptr));
#if QT_CONFIG(shortcut)
        editCopyAction->setShortcut(QCoreApplication::translate("PinglyMotionWindow", "Ctrl+C", nullptr));
#endif // QT_CONFIG(shortcut)
        editPasteAction->setText(QCoreApplication::translate("PinglyMotionWindow", "Paste", nullptr));
#if QT_CONFIG(shortcut)
        editPasteAction->setShortcut(QCoreApplication::translate("PinglyMotionWindow", "Ctrl+V", nullptr));
#endif // QT_CONFIG(shortcut)
        toolsOptimizeBVHAction->setText(QCoreApplication::translate("PinglyMotionWindow", "Optimize BVH", nullptr));
        optionsSkeletonAction->setText(QCoreApplication::translate("PinglyMotionWindow", "Skeleton", nullptr));
        optionsJointLimitsAction->setText(QCoreApplication::translate("PinglyMotionWindow", "Joint Limits", nullptr));
        optionsLoopAction->setText(QCoreApplication::translate("PinglyMotionWindow", "Loop", nullptr));
        optionsProtectFirstFrameAction->setText(QCoreApplication::translate("PinglyMotionWindow", "Protect First Frame", nullptr));
        optionsShowTimelineAction->setText(QCoreApplication::translate("PinglyMotionWindow", "Show Timeline", nullptr));
        optionsConfigurePinglyMotionAction->setText(QCoreApplication::translate("PinglyMotionWindow", "Configure PinglyMotion...", nullptr));
        helpAboutAction->setText(QCoreApplication::translate("PinglyMotionWindow", "About...", nullptr));
        resetCameraAction->setText(QCoreApplication::translate("PinglyMotionWindow", "Reset Camera", nullptr));
#if QT_CONFIG(shortcut)
        resetCameraAction->setShortcut(QCoreApplication::translate("PinglyMotionWindow", "Ctrl+0", nullptr));
#endif // QT_CONFIG(shortcut)
        fileExportForSecondLifeAction->setText(QCoreApplication::translate("PinglyMotionWindow", "Export For Second Life", nullptr));
        toolsMirrorAction->setText(QCoreApplication::translate("PinglyMotionWindow", "Mirror", nullptr));
#if QT_CONFIG(tooltip)
        toolsMirrorAction->setToolTip(QCoreApplication::translate("PinglyMotionWindow", "Mirror Animation or Track", nullptr));
#endif // QT_CONFIG(tooltip)
        helpWhatsNewAction->setText(QCoreApplication::translate("PinglyMotionWindow", "What's New...", nullptr));
        playButton->setText(QString());
        keyframeButton->setText(QString());
        animationLabel->setText(QCoreApplication::translate("PinglyMotionWindow", "Animation:", nullptr));
        figureLabel->setText(QCoreApplication::translate("PinglyMotionWindow", "Figure:", nullptr));
        figureCombo->setItemText(0, QCoreApplication::translate("PinglyMotionWindow", "female", nullptr));
        figureCombo->setItemText(1, QCoreApplication::translate("PinglyMotionWindow", "male", nullptr));

        scaleLabel->setText(QCoreApplication::translate("PinglyMotionWindow", "Size:", nullptr));
        scaleSpin->setSuffix(QCoreApplication::translate("PinglyMotionWindow", "%", nullptr));
        rotationGroupBox->setTitle(QCoreApplication::translate("PinglyMotionWindow", "Rotation", nullptr));
        yRotationLabel->setText(QCoreApplication::translate("PinglyMotionWindow", "Y Rotation:", nullptr));
        altLabel->setText(QCoreApplication::translate("PinglyMotionWindow", "Alt", nullptr));
        zRotationLabel->setText(QCoreApplication::translate("PinglyMotionWindow", "Z Rotation:", nullptr));
        ctrlLabel->setText(QCoreApplication::translate("PinglyMotionWindow", "Ctrl", nullptr));
        xRotationLabel->setText(QCoreApplication::translate("PinglyMotionWindow", "X Rotation:", nullptr));
        shiftLabel->setText(QCoreApplication::translate("PinglyMotionWindow", "Shift", nullptr));
        positionGroupBox->setTitle(QCoreApplication::translate("PinglyMotionWindow", "Position", nullptr));
        xPositionLabel->setText(QCoreApplication::translate("PinglyMotionWindow", "X Position:", nullptr));
        ypositionLabel->setText(QCoreApplication::translate("PinglyMotionWindow", "Y Position:", nullptr));
        zRotationLabel_2->setText(QCoreApplication::translate("PinglyMotionWindow", "Z Position:", nullptr));
        easeInOutGroup->setTitle(QCoreApplication::translate("PinglyMotionWindow", "Ease In / Out", nullptr));
        easeInCheck->setText(QCoreApplication::translate("PinglyMotionWindow", "Ease In", nullptr));
        easeOutCheck->setText(QCoreApplication::translate("PinglyMotionWindow", "Ease Out", nullptr));
        avatarPropsTab->setTabText(avatarPropsTab->indexOf(avatarTab), QCoreApplication::translate("PinglyMotionWindow", "Avatar", nullptr));
        newBoxPropButton->setText(QString());
        newSpherePropButton->setText(QString());
        newConePropButton->setText(QString());
        newTorusPropButton->setText(QString());
        propNameLabel->setText(QCoreApplication::translate("PinglyMotionWindow", "Prop:", nullptr));
        deletePropButton->setText(QString());
        attachToLabel->setText(QCoreApplication::translate("PinglyMotionWindow", "Attach to:", nullptr));
        attachToComboBox->setItemText(0, QCoreApplication::translate("PinglyMotionWindow", "* not attached *", nullptr));
        attachToComboBox->setItemText(1, QCoreApplication::translate("PinglyMotionWindow", "hip", nullptr));
        attachToComboBox->setItemText(2, QCoreApplication::translate("PinglyMotionWindow", "abdomen", nullptr));
        attachToComboBox->setItemText(3, QCoreApplication::translate("PinglyMotionWindow", "chest", nullptr));
        attachToComboBox->setItemText(4, QCoreApplication::translate("PinglyMotionWindow", "neck", nullptr));
        attachToComboBox->setItemText(5, QCoreApplication::translate("PinglyMotionWindow", "head", nullptr));
        attachToComboBox->setItemText(6, QCoreApplication::translate("PinglyMotionWindow", "-", nullptr));
        attachToComboBox->setItemText(7, QCoreApplication::translate("PinglyMotionWindow", "lCollar", nullptr));
        attachToComboBox->setItemText(8, QCoreApplication::translate("PinglyMotionWindow", "lShldr", nullptr));
        attachToComboBox->setItemText(9, QCoreApplication::translate("PinglyMotionWindow", "lForeArm", nullptr));
        attachToComboBox->setItemText(10, QCoreApplication::translate("PinglyMotionWindow", "lHand", nullptr));
        attachToComboBox->setItemText(11, QCoreApplication::translate("PinglyMotionWindow", "-", nullptr));
        attachToComboBox->setItemText(12, QCoreApplication::translate("PinglyMotionWindow", "rCollar", nullptr));
        attachToComboBox->setItemText(13, QCoreApplication::translate("PinglyMotionWindow", "rShldr", nullptr));
        attachToComboBox->setItemText(14, QCoreApplication::translate("PinglyMotionWindow", "rForeArm", nullptr));
        attachToComboBox->setItemText(15, QCoreApplication::translate("PinglyMotionWindow", "rHand", nullptr));
        attachToComboBox->setItemText(16, QCoreApplication::translate("PinglyMotionWindow", "-", nullptr));
        attachToComboBox->setItemText(17, QCoreApplication::translate("PinglyMotionWindow", "lThigh", nullptr));
        attachToComboBox->setItemText(18, QCoreApplication::translate("PinglyMotionWindow", "lShin", nullptr));
        attachToComboBox->setItemText(19, QCoreApplication::translate("PinglyMotionWindow", "lFoot", nullptr));
        attachToComboBox->setItemText(20, QCoreApplication::translate("PinglyMotionWindow", "-", nullptr));
        attachToComboBox->setItemText(21, QCoreApplication::translate("PinglyMotionWindow", "rThigh", nullptr));
        attachToComboBox->setItemText(22, QCoreApplication::translate("PinglyMotionWindow", "rShin", nullptr));
        attachToComboBox->setItemText(23, QCoreApplication::translate("PinglyMotionWindow", "rFoot", nullptr));

        propPositionGroup->setTitle(QCoreApplication::translate("PinglyMotionWindow", "Position", nullptr));
        propXPosLabel->setText(QCoreApplication::translate("PinglyMotionWindow", "X Position:", nullptr));
        propYPosLabel->setText(QCoreApplication::translate("PinglyMotionWindow", "Y Position:", nullptr));
        propYPosLabel_2->setText(QCoreApplication::translate("PinglyMotionWindow", "Z Position:", nullptr));
        propScaleGroup->setTitle(QCoreApplication::translate("PinglyMotionWindow", "Scale", nullptr));
        propXScaleLabel->setText(QCoreApplication::translate("PinglyMotionWindow", "X Scale:", nullptr));
        propYScaleLabel->setText(QCoreApplication::translate("PinglyMotionWindow", "Y Scale:", nullptr));
        propZScaleLabel->setText(QCoreApplication::translate("PinglyMotionWindow", "Z Scale:", nullptr));
        propRotationGroup->setTitle(QCoreApplication::translate("PinglyMotionWindow", "Rotation", nullptr));
        propXRotLabel->setText(QCoreApplication::translate("PinglyMotionWindow", "X Rotation:", nullptr));
        propYRotLabel->setText(QCoreApplication::translate("PinglyMotionWindow", "Y Rotation:", nullptr));
        propZRotLabel->setText(QCoreApplication::translate("PinglyMotionWindow", "Z Rotation:", nullptr));
        avatarPropsTab->setTabText(avatarPropsTab->indexOf(propsTab), QCoreApplication::translate("PinglyMotionWindow", "Props", nullptr));
        loopInLabel->setText(QCoreApplication::translate("PinglyMotionWindow", "Loop In:", nullptr));
        loopInPercentLabel->setText(QCoreApplication::translate("PinglyMotionWindow", "(10%)", nullptr));
        loopOutLabel->setText(QCoreApplication::translate("PinglyMotionWindow", "Loop Out:", nullptr));
        loopOutPercentLabel->setText(QCoreApplication::translate("PinglyMotionWindow", "(20%)", nullptr));
        frameLabel->setText(QCoreApplication::translate("PinglyMotionWindow", "Frame", nullptr));
        currentFrameLabel->setText(QCoreApplication::translate("PinglyMotionWindow", "0", nullptr));
        ofLabel->setText(QCoreApplication::translate("PinglyMotionWindow", "of", nullptr));
        fpsLabel->setText(QCoreApplication::translate("PinglyMotionWindow", "FPS:", nullptr));
        menuFile->setTitle(QCoreApplication::translate("PinglyMotionWindow", "File", nullptr));
        menuEdit->setTitle(QCoreApplication::translate("PinglyMotionWindow", "Edit", nullptr));
        menuOptions->setTitle(QCoreApplication::translate("PinglyMotionWindow", "Options", nullptr));
        menuHelp->setTitle(QCoreApplication::translate("PinglyMotionWindow", "Help", nullptr));
        menuTools->setTitle(QCoreApplication::translate("PinglyMotionWindow", "Tools", nullptr));
        toolBar->setWindowTitle(QCoreApplication::translate("PinglyMotionWindow", "toolBar", nullptr));
#if QT_CONFIG(tooltip)
        toolBar->setToolTip(QCoreApplication::translate("PinglyMotionWindow", "Reset camera view to default position", nullptr));
#endif // QT_CONFIG(tooltip)
    } // retranslateUi

};

namespace Ui {
    class PinglyMotionWindow: public Ui_PinglyMotionWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_PINGLYMOTIONWINDOW_H
