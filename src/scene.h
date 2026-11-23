/***************************************************************************
 *   Copyright (C) 2006 by Zi Ree   *
 *   Zi Ree @ SecondLife   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef SCENE_H
#define SCENE_H

#include <QObject>

#include <memory>

#include "animationmanager.h"
#include "floor.h"
#include "prop.h"

class Animation;
class BVH;
class Camera;
class PropManager;

class Scene : public QObject
{
  Q_OBJECT

  public:
    Scene(int nextPropId, QObject* parent = nullptr);
    ~Scene();

    // Sets an animation "active"
    void selectAnimation(int index);

    // this is for setting a single-pose animation. It will clear all other
    // current animations, and become the only active one
    void setAnimation(Animation* animation);

    // this is for adding subsequent animations after the first call to
    // setAnimation
    void addAnimation(Animation* animation);

    // takes one animation out of the scene without touching the others (two figures,
    // docs/UX_DECISIONS.md 6m); the caller owns it after this and deletes it.
    // false when it was not in the scene
    bool removeAnimation(Animation* animation);

    // This function clears the animations
    void clear();

    // These functions are re-implemented here so that every animation's
    // frame data can be changed at once
    void setFrame(int frame);
    void stepForward();
    void setFPS(int fps);

    // getAnimation returns the *current* animation
    Animation* getAnimation();
    Animation* getAnimation(unsigned int index);
    int getIndexOfAnimation(Animation* animation);
    int getCountOfAnimations();

    void drawFloor();

    // the reader the scene's animations load, save and free their keys with;
    // it lives as long as the scene, so longer than every animation in it
    BVH* bvh();

    // the camera and the props of the scene
    Camera* camera();
    PropManager* propManager();

    const Prop* addProp(Prop::PropType type, QSharedPointer<Mesh> mesh,
                        int attach = 0,
                        double xPosition = 10, double yPosition = 40, double zPosition = 10,
                        double xScale = 10, double yScale = 10, double zScale = 10,
                        double xRotation = 0, double yRotation = 0, double zRotation = 0);
    Prop* getPropByName(const QString& lookName);
    Prop* getPropById(unsigned int id);
    void deleteProp(Prop* prop);
    void clearProps();

  signals:
    void repaint();
    void animationSelected(Animation* animation);

  public slots:
    void protectFrame(bool on);

    void storeCameraPosition(int num);
    void restoreCameraPosition(int num);

  private:
    // declared first, so it is freed after the animations
    std::unique_ptr<BVH> m_bvh;
    AnimationManager m_animationManager;

    Floor m_floor;
    PropManager* m_propManager;

    Camera* m_camera;
};

#endif
