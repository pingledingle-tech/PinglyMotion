/*
 * PinglyMotion - test stand-in for the 3D view
 *
 * The designer file pinglymotionwindow.ui places an AnimationView (OpenGL)
 * in the middle of the window. The panel test (tst_mainpanels.cpp) builds
 * the real panels from that file without OpenGL, so the 3D view is a plain
 * widget here. Tests only.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#ifndef ANIMATIONVIEW_H
#define ANIMATIONVIEW_H

#include <QWidget>

class AnimationView : public QWidget
{
  public:
    explicit AnimationView(QWidget* parent = nullptr) : QWidget(parent) {}
};

#endif
