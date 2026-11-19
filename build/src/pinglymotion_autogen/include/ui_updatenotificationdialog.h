/********************************************************************************
** Form generated from reading UI file 'updatenotificationdialog.ui'
**
** Created by: Qt User Interface Compiler version 6.4.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_UPDATENOTIFICATIONDIALOG_H
#define UI_UPDATENOTIFICATIONDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QLabel>

QT_BEGIN_NAMESPACE

class Ui_UpdateNotificationDialog
{
public:
    QDialogButtonBox *okButtonBox;
    QLabel *updateNotificationLabel;
    QCheckBox *autoUpdatesCheckBox;

    void setupUi(QDialog *UpdateNotificationDialog)
    {
        if (UpdateNotificationDialog->objectName().isEmpty())
            UpdateNotificationDialog->setObjectName("UpdateNotificationDialog");
        UpdateNotificationDialog->resize(290, 120);
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(UpdateNotificationDialog->sizePolicy().hasHeightForWidth());
        UpdateNotificationDialog->setSizePolicy(sizePolicy);
        UpdateNotificationDialog->setMinimumSize(QSize(290, 120));
        UpdateNotificationDialog->setMaximumSize(QSize(290, 120));
        okButtonBox = new QDialogButtonBox(UpdateNotificationDialog);
        okButtonBox->setObjectName("okButtonBox");
        okButtonBox->setGeometry(QRect(10, 90, 271, 32));
        okButtonBox->setOrientation(Qt::Horizontal);
        okButtonBox->setStandardButtons(QDialogButtonBox::Ok);
        okButtonBox->setCenterButtons(true);
        updateNotificationLabel = new QLabel(UpdateNotificationDialog);
        updateNotificationLabel->setObjectName("updateNotificationLabel");
        updateNotificationLabel->setGeometry(QRect(10, 10, 271, 51));
        QFont font;
        font.setPointSize(10);
        updateNotificationLabel->setFont(font);
        updateNotificationLabel->setOpenExternalLinks(true);
        autoUpdatesCheckBox = new QCheckBox(UpdateNotificationDialog);
        autoUpdatesCheckBox->setObjectName("autoUpdatesCheckBox");
        autoUpdatesCheckBox->setGeometry(QRect(10, 70, 271, 20));

        retranslateUi(UpdateNotificationDialog);
        QObject::connect(okButtonBox, &QDialogButtonBox::accepted, UpdateNotificationDialog, qOverload<>(&QDialog::accept));
        QObject::connect(okButtonBox, &QDialogButtonBox::rejected, UpdateNotificationDialog, qOverload<>(&QDialog::reject));

        QMetaObject::connectSlotsByName(UpdateNotificationDialog);
    } // setupUi

    void retranslateUi(QDialog *UpdateNotificationDialog)
    {
        UpdateNotificationDialog->setWindowTitle(QCoreApplication::translate("UpdateNotificationDialog", "Update Checker", nullptr));
        updateNotificationLabel->setText(QCoreApplication::translate("UpdateNotificationDialog", "<html><head/><body><p align=\"center\"><span style=\" font-weight:600;\">An update is available!</span></p><p align=\"center\"><a href=\"https://github.com/pingledingle-tech/PinglyMotion/releases/latest\"><span style=\" text-decoration: underline; color:#0000ff;\">Download the new version from GitHub</span></a></p></body></html>", nullptr));
        autoUpdatesCheckBox->setText(QCoreApplication::translate("UpdateNotificationDialog", "Automatically Check for Updates", nullptr));
    } // retranslateUi

};

namespace Ui {
    class UpdateNotificationDialog: public Ui_UpdateNotificationDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_UPDATENOTIFICATIONDIALOG_H
