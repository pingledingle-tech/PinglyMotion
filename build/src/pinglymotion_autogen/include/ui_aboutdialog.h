/********************************************************************************
** Form generated from reading UI file 'aboutdialog.ui'
**
** Created by: Qt User Interface Compiler version 6.4.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_ABOUTDIALOG_H
#define UI_ABOUTDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>

QT_BEGIN_NAMESPACE

class Ui_AboutDialog
{
public:
    QLabel *titleLabel;
    QLabel *versionLabel;
    QGroupBox *updatesGroupBox;
    QPushButton *checkUpdatesButton;
    QLabel *lastUpdateCheckLabel;
    QCheckBox *autoUpdatesCheckBox;
    QLabel *dateTimeLabel;
    QLabel *creditLabel;
    QLabel *websiteLabel;

    void setupUi(QDialog *AboutDialog)
    {
        if (AboutDialog->objectName().isEmpty())
            AboutDialog->setObjectName("AboutDialog");
        AboutDialog->resize(320, 316);
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(AboutDialog->sizePolicy().hasHeightForWidth());
        AboutDialog->setSizePolicy(sizePolicy);
        AboutDialog->setMinimumSize(QSize(320, 316));
        AboutDialog->setMaximumSize(QSize(320, 316));
        titleLabel = new QLabel(AboutDialog);
        titleLabel->setObjectName("titleLabel");
        titleLabel->setGeometry(QRect(10, 10, 301, 31));
        QFont font;
        font.setPointSize(16);
        font.setBold(true);
        titleLabel->setFont(font);
        titleLabel->setAlignment(Qt::AlignCenter);
        versionLabel = new QLabel(AboutDialog);
        versionLabel->setObjectName("versionLabel");
        versionLabel->setGeometry(QRect(10, 40, 301, 21));
        QFont font1;
        font1.setPointSize(10);
        font1.setUnderline(false);
        versionLabel->setFont(font1);
        versionLabel->setAlignment(Qt::AlignCenter);
        updatesGroupBox = new QGroupBox(AboutDialog);
        updatesGroupBox->setObjectName("updatesGroupBox");
        updatesGroupBox->setGeometry(QRect(10, 120, 301, 121));
        QFont font2;
        font2.setBold(true);
        updatesGroupBox->setFont(font2);
        checkUpdatesButton = new QPushButton(updatesGroupBox);
        checkUpdatesButton->setObjectName("checkUpdatesButton");
        checkUpdatesButton->setGeometry(QRect(70, 40, 151, 41));
        QFont font3;
        font3.setPointSize(10);
        font3.setBold(false);
        checkUpdatesButton->setFont(font3);
        lastUpdateCheckLabel = new QLabel(updatesGroupBox);
        lastUpdateCheckLabel->setObjectName("lastUpdateCheckLabel");
        lastUpdateCheckLabel->setGeometry(QRect(10, 20, 281, 21));
        QFont font4;
        font4.setBold(false);
        lastUpdateCheckLabel->setFont(font4);
        lastUpdateCheckLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
        autoUpdatesCheckBox = new QCheckBox(updatesGroupBox);
        autoUpdatesCheckBox->setObjectName("autoUpdatesCheckBox");
        autoUpdatesCheckBox->setGeometry(QRect(10, 100, 281, 17));
        autoUpdatesCheckBox->setFont(font4);
        autoUpdatesCheckBox->setChecked(true);
        dateTimeLabel = new QLabel(AboutDialog);
        dateTimeLabel->setObjectName("dateTimeLabel");
        dateTimeLabel->setGeometry(QRect(10, 60, 301, 20));
        QFont font5;
        font5.setPointSize(10);
        dateTimeLabel->setFont(font5);
        dateTimeLabel->setAlignment(Qt::AlignCenter);
        creditLabel = new QLabel(AboutDialog);
        creditLabel->setObjectName("creditLabel");
        creditLabel->setGeometry(QRect(10, 246, 301, 64));
        QFont font6;
        font6.setPointSize(8);
        creditLabel->setFont(font6);
        creditLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);
        creditLabel->setWordWrap(true);
        websiteLabel = new QLabel(AboutDialog);
        websiteLabel->setObjectName("websiteLabel");
        websiteLabel->setGeometry(QRect(10, 90, 301, 20));
        websiteLabel->setFont(font5);
        websiteLabel->setAlignment(Qt::AlignCenter);
        websiteLabel->setOpenExternalLinks(true);

        retranslateUi(AboutDialog);

        QMetaObject::connectSlotsByName(AboutDialog);
    } // setupUi

    void retranslateUi(QDialog *AboutDialog)
    {
        AboutDialog->setWindowTitle(QCoreApplication::translate("AboutDialog", "About PinglyMotion", nullptr));
        titleLabel->setText(QCoreApplication::translate("AboutDialog", "PinglyMotion", nullptr));
        versionLabel->setText(QString());
        updatesGroupBox->setTitle(QCoreApplication::translate("AboutDialog", "Check for Updates", nullptr));
        checkUpdatesButton->setText(QCoreApplication::translate("AboutDialog", "Check Now", nullptr));
        lastUpdateCheckLabel->setText(QString());
        autoUpdatesCheckBox->setText(QCoreApplication::translate("AboutDialog", "Automatically Check for Updates", nullptr));
        dateTimeLabel->setText(QString());
        creditLabel->setText(QCoreApplication::translate("AboutDialog", "Avatar body: Second Life viewer artwork, Copyright (C) Linden Research, Inc., licensed under the Creative Commons Attribution-Share Alike 3.0 License. See data/character/LICENSE.txt. Bento body: \"Maxine\" from the Max Avatar Project by Ada Radius and the RuthAndRoth community, licensed under the Creative Commons Attribution 4.0 License. See data/character/max/LICENSE.txt", nullptr));
        websiteLabel->setText(QCoreApplication::translate("AboutDialog", "<html><head/><body><p>Website: <a href=\"https://github.com/pingledingle-tech/PinglyMotion\"><span style=\" text-decoration: underline; color:#0000ff;\">PinglyMotion on GitHub</span></a> \342\200\223 based on QAvimator</p></body></html>", nullptr));
    } // retranslateUi

};

namespace Ui {
    class AboutDialog: public Ui_AboutDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_ABOUTDIALOG_H
