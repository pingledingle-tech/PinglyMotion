/*
 * PinglyMotion - light and dark look for the whole program
 * Licensed under the GNU General Public License version 2 or later.
 */
#include <QApplication>
#include <QPalette>
#include <QStyle>
#include <QStyleFactory>

#include "theme.h"

void Theme::apply(bool dark)
{
  // Fusion looks the same on Windows, macOS and Linux and follows the palette
  QApplication::setStyle(QStyleFactory::create("Fusion"));

  if (!dark)
  {
    QApplication::setPalette(QApplication::style()->standardPalette());
    return;
  }

  const QColor window(45, 45, 48);
  const QColor base(30, 30, 32);
  const QColor alternate(53, 53, 56);
  const QColor text(225, 225, 225);
  const QColor disabled(120, 120, 120);
  const QColor accent(74, 144, 226);

  QPalette p;
  p.setColor(QPalette::Window, window);
  p.setColor(QPalette::WindowText, text);
  p.setColor(QPalette::Base, base);
  p.setColor(QPalette::AlternateBase, alternate);
  p.setColor(QPalette::ToolTipBase, alternate);
  p.setColor(QPalette::ToolTipText, text);
  p.setColor(QPalette::Text, text);
  p.setColor(QPalette::Button, alternate);
  p.setColor(QPalette::ButtonText, text);
  p.setColor(QPalette::BrightText, Qt::red);
  p.setColor(QPalette::Link, accent);
  p.setColor(QPalette::Highlight, accent);
  p.setColor(QPalette::HighlightedText, Qt::white);
  p.setColor(QPalette::Light, QColor(70, 70, 74));
  p.setColor(QPalette::Midlight, QColor(60, 60, 64));
  p.setColor(QPalette::Mid, QColor(40, 40, 42));
  p.setColor(QPalette::Dark, QColor(22, 22, 24));
  p.setColor(QPalette::Shadow, Qt::black);

  p.setColor(QPalette::Disabled, QPalette::WindowText, disabled);
  p.setColor(QPalette::Disabled, QPalette::Text, disabled);
  p.setColor(QPalette::Disabled, QPalette::ButtonText, disabled);
  p.setColor(QPalette::Disabled, QPalette::Highlight, QColor(80, 80, 80));
  p.setColor(QPalette::Disabled, QPalette::HighlightedText, disabled);

  QApplication::setPalette(p);
}
