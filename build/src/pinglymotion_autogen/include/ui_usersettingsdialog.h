/********************************************************************************
** Form generated from reading UI file 'usersettingsdialog.ui'
**
** Created by: Qt User Interface Compiler version 6.4.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_USERSETTINGSDIALOG_H
#define UI_USERSETTINGSDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_UserSettingsDialog
{
public:
    QGridLayout *gridLayout;
    QPushButton *okButton;
    QPushButton *cancelButton;
    QPushButton *applyButton;
    QTabWidget *tabWidget4;
    QWidget *TabPage;
    QVBoxLayout *vboxLayout;
    QGroupBox *keyframeTypeGroupBox;
    QVBoxLayout *vboxLayout1;
    QCheckBox *easeInCheckbox;
    QCheckBox *easeOutCheckbox;
    QWidget *tab;
    QGridLayout *gridLayout1;
    QSpacerItem *spacerItem;
    QHBoxLayout *hboxLayout;
    QVBoxLayout *vboxLayout2;
    QCheckBox *useFogCheckbox;
    QHBoxLayout *hboxLayout1;
    QLabel *floorTranslucencyLabel;
    QSpinBox *floorTranslucencySpin;
    QSpacerItem *spacerItem1;
    QSpacerItem *spacerItem2;

    void setupUi(QDialog *UserSettingsDialog)
    {
        if (UserSettingsDialog->objectName().isEmpty())
            UserSettingsDialog->setObjectName("UserSettingsDialog");
        UserSettingsDialog->setWindowModality(Qt::WindowModal);
        UserSettingsDialog->resize(340, 190);
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(UserSettingsDialog->sizePolicy().hasHeightForWidth());
        UserSettingsDialog->setSizePolicy(sizePolicy);
        UserSettingsDialog->setMinimumSize(QSize(340, 190));
        UserSettingsDialog->setMaximumSize(QSize(340, 190));
        gridLayout = new QGridLayout(UserSettingsDialog);
        gridLayout->setSpacing(6);
        gridLayout->setContentsMargins(11, 11, 11, 11);
        gridLayout->setObjectName("gridLayout");
        okButton = new QPushButton(UserSettingsDialog);
        okButton->setObjectName("okButton");

        gridLayout->addWidget(okButton, 1, 1, 1, 1);

        cancelButton = new QPushButton(UserSettingsDialog);
        cancelButton->setObjectName("cancelButton");

        gridLayout->addWidget(cancelButton, 1, 3, 1, 1);

        applyButton = new QPushButton(UserSettingsDialog);
        applyButton->setObjectName("applyButton");

        gridLayout->addWidget(applyButton, 1, 2, 1, 1);

        tabWidget4 = new QTabWidget(UserSettingsDialog);
        tabWidget4->setObjectName("tabWidget4");
        TabPage = new QWidget();
        TabPage->setObjectName("TabPage");
        vboxLayout = new QVBoxLayout(TabPage);
        vboxLayout->setSpacing(6);
        vboxLayout->setContentsMargins(11, 11, 11, 11);
        vboxLayout->setObjectName("vboxLayout");
        keyframeTypeGroupBox = new QGroupBox(TabPage);
        keyframeTypeGroupBox->setObjectName("keyframeTypeGroupBox");
        vboxLayout1 = new QVBoxLayout(keyframeTypeGroupBox);
        vboxLayout1->setSpacing(6);
        vboxLayout1->setContentsMargins(11, 11, 11, 11);
        vboxLayout1->setObjectName("vboxLayout1");
        easeInCheckbox = new QCheckBox(keyframeTypeGroupBox);
        easeInCheckbox->setObjectName("easeInCheckbox");

        vboxLayout1->addWidget(easeInCheckbox);

        easeOutCheckbox = new QCheckBox(keyframeTypeGroupBox);
        easeOutCheckbox->setObjectName("easeOutCheckbox");

        vboxLayout1->addWidget(easeOutCheckbox);


        vboxLayout->addWidget(keyframeTypeGroupBox);

        tabWidget4->addTab(TabPage, QString());
        tab = new QWidget();
        tab->setObjectName("tab");
        gridLayout1 = new QGridLayout(tab);
        gridLayout1->setSpacing(6);
        gridLayout1->setContentsMargins(11, 11, 11, 11);
        gridLayout1->setObjectName("gridLayout1");
        spacerItem = new QSpacerItem(20, 50, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout1->addItem(spacerItem, 1, 0, 1, 1);

        hboxLayout = new QHBoxLayout();
        hboxLayout->setSpacing(6);
        hboxLayout->setObjectName("hboxLayout");
        vboxLayout2 = new QVBoxLayout();
        vboxLayout2->setSpacing(6);
        vboxLayout2->setObjectName("vboxLayout2");
        useFogCheckbox = new QCheckBox(tab);
        useFogCheckbox->setObjectName("useFogCheckbox");

        vboxLayout2->addWidget(useFogCheckbox);

        hboxLayout1 = new QHBoxLayout();
        hboxLayout1->setSpacing(6);
        hboxLayout1->setObjectName("hboxLayout1");
        floorTranslucencyLabel = new QLabel(tab);
        floorTranslucencyLabel->setObjectName("floorTranslucencyLabel");
        floorTranslucencyLabel->setWordWrap(false);

        hboxLayout1->addWidget(floorTranslucencyLabel);

        floorTranslucencySpin = new QSpinBox(tab);
        floorTranslucencySpin->setObjectName("floorTranslucencySpin");
        floorTranslucencySpin->setMaximum(100);

        hboxLayout1->addWidget(floorTranslucencySpin);


        vboxLayout2->addLayout(hboxLayout1);


        hboxLayout->addLayout(vboxLayout2);

        spacerItem1 = new QSpacerItem(406, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout->addItem(spacerItem1);


        gridLayout1->addLayout(hboxLayout, 0, 0, 1, 1);

        tabWidget4->addTab(tab, QString());

        gridLayout->addWidget(tabWidget4, 0, 0, 1, 4);

        spacerItem2 = new QSpacerItem(61, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(spacerItem2, 1, 0, 1, 1);


        retranslateUi(UserSettingsDialog);

        tabWidget4->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(UserSettingsDialog);
    } // setupUi

    void retranslateUi(QDialog *UserSettingsDialog)
    {
        UserSettingsDialog->setWindowTitle(QCoreApplication::translate("UserSettingsDialog", "Configure PinglyMotion", nullptr));
        okButton->setText(QCoreApplication::translate("UserSettingsDialog", "&OK", nullptr));
#if QT_CONFIG(shortcut)
        okButton->setShortcut(QCoreApplication::translate("UserSettingsDialog", "Alt+O", nullptr));
#endif // QT_CONFIG(shortcut)
        cancelButton->setText(QCoreApplication::translate("UserSettingsDialog", "&Cancel", nullptr));
#if QT_CONFIG(shortcut)
        cancelButton->setShortcut(QCoreApplication::translate("UserSettingsDialog", "Alt+C", nullptr));
#endif // QT_CONFIG(shortcut)
        applyButton->setText(QCoreApplication::translate("UserSettingsDialog", "&Apply", nullptr));
#if QT_CONFIG(shortcut)
        applyButton->setShortcut(QCoreApplication::translate("UserSettingsDialog", "Alt+A", nullptr));
#endif // QT_CONFIG(shortcut)
        keyframeTypeGroupBox->setTitle(QCoreApplication::translate("UserSettingsDialog", "Default Keyframe Type", nullptr));
        easeInCheckbox->setText(QCoreApplication::translate("UserSettingsDialog", "Ease In", nullptr));
        easeOutCheckbox->setText(QCoreApplication::translate("UserSettingsDialog", "Ease Out", nullptr));
        tabWidget4->setTabText(tabWidget4->indexOf(TabPage), QCoreApplication::translate("UserSettingsDialog", "General", nullptr));
        useFogCheckbox->setText(QCoreApplication::translate("UserSettingsDialog", "Use Fog", nullptr));
        floorTranslucencyLabel->setText(QCoreApplication::translate("UserSettingsDialog", "Floor Translucency:", nullptr));
        floorTranslucencySpin->setSuffix(QCoreApplication::translate("UserSettingsDialog", "%", nullptr));
        tabWidget4->setTabText(tabWidget4->indexOf(tab), QCoreApplication::translate("UserSettingsDialog", "OpenGL", nullptr));
    } // retranslateUi

};

namespace Ui {
    class UserSettingsDialog: public Ui_UserSettingsDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_USERSETTINGSDIALOG_H
