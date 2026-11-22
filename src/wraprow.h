/*
 * PinglyMotion - a row of controls that can break into one line per part
 *
 * docs/UX_DECISIONS.md, 6c: when the panels stand in one column the rows of
 * the timeline panel break into shorter lines instead of making the window
 * scroll sideways. A wrap row is a box layout of parts (each a row of its
 * own); unwrapped the parts stand side by side, wrapped one under the other.
 * Header-only, so the tests use the same code as the program.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#ifndef WRAPROW_H
#define WRAPROW_H

#include <QBoxLayout>
#include <QList>

#include <algorithm>

namespace WrapRow
{
  // a new row of `parts`, side by side; the row owns the parts
  inline QBoxLayout* make(const QList<QLayout*>& parts)
  {
    QBoxLayout* row=new QBoxLayout(QBoxLayout::LeftToRight);
    row->setContentsMargins(0,0,0,0);
    for (QLayout* part : parts) row->addLayout(part);
    return row;
  }

  inline bool isWrapped(const QBoxLayout* row)
  {
    return row->direction()==QBoxLayout::TopToBottom;
  }

  // side by side (false) or one part per line (true)
  inline void setWrapped(QBoxLayout* row,bool wrapped)
  {
    const QBoxLayout::Direction direction=wrapped ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight;
    if (row->direction()!=direction) row->setDirection(direction);
  }

  // the smallest width of the row when its parts stand side by side, whether
  // it is wrapped now or not
  inline int oneLineWidth(const QBoxLayout* row)
  {
    int width=0;
    int parts=0;
    for (int i=0;i<row->count();++i)
    {
      QLayoutItem* item=row->itemAt(i);
      if (item->isEmpty()) continue;
      width+=item->minimumSize().width();
      ++parts;
    }
    const QMargins m=row->contentsMargins();
    return width+std::max(0,row->spacing())*std::max(0,parts-1)+m.left()+m.right();
  }

  // the smallest width of the row when it is wrapped: its widest part
  inline int wrappedWidth(const QBoxLayout* row)
  {
    int width=0;
    for (int i=0;i<row->count();++i)
    {
      QLayoutItem* item=row->itemAt(i);
      if (!item->isEmpty()) width=std::max(width,item->minimumSize().width());
    }
    const QMargins m=row->contentsMargins();
    return width+m.left()+m.right();
  }
}

#endif
