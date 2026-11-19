/********************************************************************************
** Form generated from reading UI file 'whatsnewdialog.ui'
**
** Created by: Qt User Interface Compiler version 6.4.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_WHATSNEWDIALOG_H
#define UI_WHATSNEWDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QPlainTextEdit>

QT_BEGIN_NAMESPACE

class Ui_WhatsNewDialog
{
public:
    QDialogButtonBox *okButtonBox;
    QPlainTextEdit *plainTextEdit;

    void setupUi(QDialog *WhatsNewDialog)
    {
        if (WhatsNewDialog->objectName().isEmpty())
            WhatsNewDialog->setObjectName("WhatsNewDialog");
        WhatsNewDialog->resize(340, 300);
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(WhatsNewDialog->sizePolicy().hasHeightForWidth());
        WhatsNewDialog->setSizePolicy(sizePolicy);
        WhatsNewDialog->setMinimumSize(QSize(340, 300));
        WhatsNewDialog->setMaximumSize(QSize(340, 300));
        okButtonBox = new QDialogButtonBox(WhatsNewDialog);
        okButtonBox->setObjectName("okButtonBox");
        okButtonBox->setGeometry(QRect(10, 260, 320, 32));
        okButtonBox->setOrientation(Qt::Horizontal);
        okButtonBox->setStandardButtons(QDialogButtonBox::Ok);
        okButtonBox->setCenterButtons(true);
        plainTextEdit = new QPlainTextEdit(WhatsNewDialog);
        plainTextEdit->setObjectName("plainTextEdit");
        plainTextEdit->setGeometry(QRect(10, 10, 320, 250));
        plainTextEdit->setFrameShape(QFrame::NoFrame);
        plainTextEdit->setReadOnly(true);

        retranslateUi(WhatsNewDialog);
        QObject::connect(okButtonBox, &QDialogButtonBox::accepted, WhatsNewDialog, qOverload<>(&QDialog::accept));
        QObject::connect(okButtonBox, &QDialogButtonBox::rejected, WhatsNewDialog, qOverload<>(&QDialog::reject));

        QMetaObject::connectSlotsByName(WhatsNewDialog);
    } // setupUi

    void retranslateUi(QDialog *WhatsNewDialog)
    {
        WhatsNewDialog->setWindowTitle(QCoreApplication::translate("WhatsNewDialog", "What's New", nullptr));
    } // retranslateUi

};

namespace Ui {
    class WhatsNewDialog: public Ui_WhatsNewDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_WHATSNEWDIALOG_H
