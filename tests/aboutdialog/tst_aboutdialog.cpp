// Credits text and size of Help > About (issue 106).
#include <QtTest>
#include <QApplication>
#include <QFont>
#include <QScrollBar>
#include <QTextBrowser>
#include <QDialogButtonBox>

#include "aboutcredits.h"
#include "aboutdialog.h"

class TstAboutDialog : public QObject
{
  Q_OBJECT
  private slots:
    void textHasEveryEntry();
    void htmlHasEveryEntryAndLinks();
    void browserShowsAllText_data();
    void browserShowsAllText();
    void dialogIsResizableAndKeepsOk();
};

void TstAboutDialog::textHasEveryEntry()
{
  const QString text=AboutCredits::plainText();
  QVERIFY(!AboutCredits::requiredEntries().isEmpty());
  for (const QString& entry : AboutCredits::requiredEntries())
    QVERIFY2(text.contains(entry), qPrintable("missing: "+entry));
}

void TstAboutDialog::htmlHasEveryEntryAndLinks()
{
  QTextDocument doc;
  doc.setHtml(AboutCredits::html());
  QCOMPARE(doc.toPlainText().simplified(),AboutCredits::plainText().simplified());
  QVERIFY(AboutCredits::html().contains("href=\"https://github.com/RuthAndRoth/Ruth2\""));
  QVERIFY(AboutCredits::html().contains("href=\"https://github.com/RuthAndRoth/Roth2\""));
}

void TstAboutDialog::browserShowsAllText_data()
{
  QTest::addColumn<int>("pointSize");
  // 100%, 125% and 150% Windows font scaling on a 9 point base font
  QTest::newRow("100%") << 9;
  QTest::newRow("125%") << 11;
  QTest::newRow("150%") << 14;
}

void TstAboutDialog::browserShowsAllText()
{
  QFont font=QApplication::font();
  font.setPointSize(QTest::currentDataTag()==QByteArray("100%") ? 9 :
                    QTest::currentDataTag()==QByteArray("125%") ? 11 : 14);
  QApplication::setFont(font);

  AboutDialog dialog;
  dialog.show();
  QTextBrowser* browser=dialog.findChild<QTextBrowser*>("creditBrowser");
  QVERIFY(browser);
  QVERIFY(browser->isVisible());
  QVERIFY(browser->height()>=60);
  QCOMPARE(browser->toPlainText().simplified(),AboutCredits::plainText().simplified());

  // either all text fits, or the scroll bar reaches the very end
  QScrollBar* bar=browser->verticalScrollBar();
  const int contentHeight=int(browser->document()->size().height());
  if (bar->maximum()==0)
    QVERIFY(contentHeight<=browser->viewport()->height());
  else
  {
    QVERIFY(bar->isVisible());
    bar->setValue(bar->maximum());
    QVERIFY(bar->value()+bar->pageStep()>=contentHeight-2);
  }
}

void TstAboutDialog::dialogIsResizableAndKeepsOk()
{
  AboutDialog dialog;
  dialog.show();
  QVERIFY(dialog.minimumSize()!=dialog.maximumSize());
  QVERIFY(dialog.maximumSize().width()>=1000);

  QDialogButtonBox* buttons=dialog.findChild<QDialogButtonBox*>("buttonBox");
  QVERIFY(buttons);
  const QList<QSize> sizes={QSize(10,10),QSize(400,500),QSize(700,300)};
  for (const QSize& size : sizes)
  {
    dialog.resize(size);   // the layout never goes below its minimum size
    QCoreApplication::processEvents();
    QVERIFY(dialog.width()>=dialog.minimumSizeHint().width());
    QVERIFY(dialog.height()>=dialog.minimumSizeHint().height());
    QVERIFY(dialog.rect().contains(buttons->geometry()));
    QVERIFY(buttons->isVisible());
  }
}

QTEST_MAIN(TstAboutDialog)
#include "tst_aboutdialog.moc"
