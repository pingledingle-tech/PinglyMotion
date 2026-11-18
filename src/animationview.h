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
 ***************************************************************************

 * Copyright (C) 2006 by Vinay Pulim.
 * ported to Qt by Zi Ree
 * features added by Darkside Eldrich
 * All rights reserved.             */

/**
  @author Vinay Pulim
  @author Zi Ree <Zi Ree @ SecondLife>
  @author Darkside Eldrich
*/

#ifndef ANIMATIONVIEW_H
#define ANIMATIONVIEW_H

#include <QOpenGLWidget>
#include <QMatrix4x4>
#include <QVector3D>

#include <QHash>
#include <memory>
#include <QMatrix4x4>
#include "avatarskin.h"
#include "skinrenderer.h"
#include "camera.h"
#include "prop.h"
#include "bvh.h"

class QMouseEvent;
class QOpenGLFramebufferObject;
class QOpenGLShaderProgram;
class Animation;
class PropManager;
class Scene;

class AnimationView : public QOpenGLWidget
{
  Q_OBJECT

  public:
    QStringList partNames() const;
    static bool isBentoBone(const QString& name);
    static bool isFaceBone(const QString& name);
    AnimationView(QWidget* parent=0, const char* name=0, Animation* anim=0);
    ~AnimationView();

    // TODO Temporary method while logic is moving around
    Scene* initializeScene(QObject* parent);

    // exports the BVH class handler (ugly, need to find a better way)
    BVH* getBVH() const;

    void selectPart(BVHNode* node);
    void selectProp();
    BVHNode* getSelectedPart();
    unsigned int getSelectedPartIndex();

    // The picture the view shows, drawn at its own size whatever the window size is.
    QImage renderImage(int imageWidth, int imageHeight);
    Camera* camera() const;
    // const QString getPartName(int index);

    // TODO Temporary method to set mesh models for removing GLUT
    void setMeshModels(QSharedPointer<Mesh> cubeMesh, QSharedPointer<Mesh> sphereMesh, QSharedPointer<Mesh> coneMesh);

  signals:
    void partClicked(BVHNode* node, QVector3D rotation, RotationLimits rotLimit, QVector3D position);
    void partClicked(int part);
    void propClicked(Prop* prop);

    void partDragged(BVHNode* node,double changeX,double changeY,double changeZ);

    void propDragged(Prop* prop,double changeX,double changeY,double changeZ);
    void propRotated(Prop* prop,double changeX,double changeY,double changeZ);
    void propScaled(Prop* prop,double changeX,double changeY,double changeZ);

    void backgroundClicked();
    void animationSelected(Animation* animation);

    void storeCameraPosition(int num);
    void restoreCameraPosition(int num);

  public slots:
    void resetCamera();
    // true: the Max Bento avatar (fingers, face); false: the Linden system
    // avatar. Falls back to the system avatar if Max cannot be loaded;
    // returns whether the wanted body is shown
    // loads the body; if that fails the system avatar is loaded instead and false is returned
    bool setBody(AvatarSkin::Body wanted);
    // Options > Light...: each 0-100, 50 is the standard look
    void setBodyLight(int brightness, int shine, int warmth, bool floorShadow);
    // false on the old OpenGL path, where the light cannot be changed
    bool bodyLightAvailable() const { return m_skinRenderer.isReady(); }
    // call after changing UserSettings::hiddenJointGroups() or hideJointsInLists()
    void jointGroupsChanged();
    void selectPart(int part);

  protected slots:
    void draw();

  protected:
    enum
    {
      MODE_COLLECT,   // only note where each joint is, for the avatar mesh
      MODE_PARTS,
      MODE_SKELETON,
      MODE_ROT_AXES
    };

    void paintGL() override;
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);
    virtual void mouseDoubleClickEvent(QMouseEvent* event);
    virtual void wheelEvent(QWheelEvent* event);
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void keyReleaseEvent(QKeyEvent* event);

    QSize m_renderSize;   // set while renderImage() draws

    void drawAnimations();
    void drawFigure(Animation* anim, unsigned int index);
    void drawPart(Animation* anim, unsigned int index, int frame, BVHNode *motion,
                  BVHNode *joints, int mode);
    void drawSkin();
    void drawProps();
    void drawProp(const Prop* prop) const;
    void drawDragHandles(const Prop* prop) const;

    BVH* bvh;

    bool leftMouseButton;
    char modifier;

    QPoint clickPos;           // holds the mouse click position for dragging
    QPoint returnPos;          // holds the mouse position to return to after dragging

    QStringList figureFiles;   // holds the names of the BVH files for male/female skeleton models

    double changeX, changeY, changeZ;
    BVHNode* joints[Animation::NUM_FIGURES];

    unsigned int selectName;
    unsigned int partHighlighted;
    QMatrix4x4 m_viewMatrix;
    QVector3D m_selectedPosition;
    bool m_haveSelectedPosition=false;
    void zoomToSelectedPart();
    QStringList m_partNames;
    QString selectionName(unsigned int selection) const;
    bool faceFocus() const;
    unsigned int partSelected;
    unsigned int mirrorSelected;
    unsigned int propDragging;  // holds the drag handle id

    int dragX, dragY;           // holds the last mouse drag offset
    int oldDragX, oldDragY;     // holds the mouse position before the last drag

    int drawMode;
    bool xSelect, ySelect, zSelect;

    void setProjection();
    void setModelView();
    void clearSelected();
    int pickPart(int x, int y);
    // picking by colour: every pickable thing is drawn into an offscreen
    // buffer in a colour that encodes its name (OpenGL 3.3 path)
    int pickPartByColor(int x, int y);
    // true while drawing for either kind of picking
    bool isPicking() const;
    // Draws lines or triangles with the shader, using the current OpenGL
    // matrices, fog and lighting. Returns false when the caller must draw
    // the old way (picking, old OpenGL path, classic light).
    bool drawWithShader(GLenum mode, const QVector<QVector3D>& positions,
                        const QVector<QVector3D>& normals, const QVector<QVector4D>& colors) const;
    bool drawWithShader(GLenum mode, const QVector<QVector3D>& positions, const QVector4D& color) const;
    // fog as it is set in OpenGL now, for the shaders
    SkinRenderer::Fog currentFog() const;
    // gives what is drawn next a pick name (its pick colour)
    void pickName(unsigned int name) const;
    void drawCircle(int axis, float radius, bool strong);
    QPointF ringPointOnScreen(int axis, float angle, float* depth = nullptr) const;
    void startRingDrag(int axis, const QPoint& mouse);

  private:
    enum class OpenGlIds
    {
      // defines where we start counting opengl ids for parts with multiple animations
      // first animation counts 0-ANIMATION_INCREMENT-1, next ANIMATION_INCREMENT++
      AnimationIncrement = 1000,

      ObjectStart = 100000,
      DragHandlStart = 200000,
      DragHandleX = 200000,
      DragHandleY = 200001,
      DragHandleZ = 200002,
      ScaleHandleX = 200003,
      ScaleHandleY = 200004,
      ScaleHandleZ = 200005,
      RotateHandleX = 200006,
      RotateHandleY = 200007,
      RotateHandleZ = 200008,
      // the three rotation rings around the selected part
      RotateRingX = 300000,
      RotateRingY = 300001,
      RotateRingZ = 300002
    };

    enum class ModifierKeys
    {
      Shift = 1,
      Ctrl = 2,
      Alt = 4
    };

    // TODO Temporary methods while logic is moved around
    PropManager* propManager() const;

    // TODO Temporary pointer to scene while logic is being moved around
    Scene* m_scene;

    // TODO Temporary mesh models to remove GLUT
    QScopedPointer<MeshModel> m_cubeMeshModel;
    QScopedPointer<MeshModel> m_sphereMeshModel;
    QScopedPointer<MeshModel> m_coneMeshModel;

    // the avatar body and what the last walk over the skeleton found per joint
    struct SkinJointState
    {
      QMatrix4x4 matrix;    // joint space -> eye space
      float color[4];
      int selectName;
    };
    // rotation rings of the selected part: where they are on screen, and
    // the ring being dragged (-1: none)
    QMatrix4x4 m_ringMatrix[3];     // ring space -> clip space
    bool m_ringVisible[3]={false,false,false};
    float m_ringRadius=10.0f;
    int m_ringDragging=-1;
    QPointF m_ringTangent;          // screen pixels the mouse moves per degree
    QPoint m_ringLastPos;

    AvatarSkin m_avatarSkin;
    // draws the body with shaders; when it is not ready the old path is used
    // mutable: the const drawing functions draw through it too
    mutable SkinRenderer m_skinRenderer;
    // bodies drawn this frame, for their shadows on the floor (drawn after the floor)
    struct ShadowCaster
    {
      QVector<QMatrix4x4> positions;
      QVector<QMatrix3x3> normals;
      bool male=false;
    };
    QVector<ShadowCaster> m_shadowCasters;
    void drawShadows();
    // picking by colour: the colour of what is drawn next (pickName())
    mutable QVector4D m_pickColor;
    std::unique_ptr<QOpenGLFramebufferObject> m_pickBuffer;
    bool m_colorPicking = false;
    // the "OpenGL 3.3 needed" message is shown only once
    bool m_glMessageShown = false;
    // scale of the joint above the one being drawn (male skeleton moves children out)
    QVector3D m_parentScale{1,1,1};
    // selection name of the nearest joint above that is shown, while drawing
    unsigned int m_shownAncestor=0;
    QHash<QString,SkinJointState> m_skinJointStates;
};

#endif
