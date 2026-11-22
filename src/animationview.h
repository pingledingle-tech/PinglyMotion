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
#include <QPointer>
#include <QVector3D>

#include <QHash>
#include <QList>
#include <QRect>
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
class CameraOverride;
class EffectsPreview;
class PropManager;
class Scene;

class AnimationView : public QOpenGLWidget
{
  Q_OBJECT

  public:
    QStringList partNames() const;
    static bool isBentoBone(const QString& name);
    static bool isFaceBone(const QString& name);
    AnimationView(QWidget* parent=0, const char* name=0);
    ~AnimationView();

    // The scene (animations, props, camera) belongs to the window; the view only draws it.
    // The first id the scene may give to objects: the view uses the ids below it for its own parts.
    static int firstObjectId();
    void setScene(Scene* scene);
    // Script Lab effects (issue 293): while the preview has an animation it is drawn instead of the
    // scene's animations (and cannot be clicked); its markers are drawn on the figure. Not owned.
    void setEffectsPreview(EffectsPreview* preview);
    // Script Lab camera limits (job 7b): while the override is active the camera keeps to its
    // constraint; when it stops, the camera from before is put back exactly. Not owned.
    void setCameraOverride(CameraOverride* cameraOverride);

    void selectPart(BVHNode* node);
    void selectProp();
    BVHNode* getSelectedPart();
    unsigned int getSelectedPartIndex();

    // The picture the view shows, drawn at its own size whatever the window size is.
    QImage renderImage(int imageWidth, int imageHeight);
    Camera* camera() const;

    // the shapes the view draws for its handles (cube, sphere and cone)
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

    // Tab was pressed while the 3D view had the keys: the window hides or
    // shows the panels (docs/PANELS_PLAN.md, job P4)
    void tabPressed();

  public slots:
    void resetCamera();
    // loads the body (Ruth2 or Max); if that fails the other one is loaded
    // instead and false is returned
    bool setBody(AvatarSkin::Body wanted);
    // the body that is shown now
    AvatarSkin::Body body() const { return m_avatarSkin.body(); }
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
    // takes Tab before QWidget uses it to move the keyboard focus
    bool event(QEvent* event) override;
    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);
    virtual void mouseDoubleClickEvent(QMouseEvent* event);
    virtual void wheelEvent(QWheelEvent* event);
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void keyReleaseEvent(QKeyEvent* event);

    QSize m_renderSize;   // set while renderImage() draws
    // true while renderImage() draws a picture without the Bento markers
    // (screenshot option "markers": false, src/screenshotsheet.h)
    bool m_hideMarkers=false;
    // the view's height (widget pixels) the last time setProjection() ran
    // with the live widget size; -1 before the first time (P5, see
    // keepFigureSizeOnHeightChange())
    int m_lastViewHeight=-1;

    void drawAnimations();
    void drawFigure(Animation* anim, unsigned int index);
    // the Script Lab markers (worn objects, locks) at the joints of the figure drawn last
    void drawEffectMarkers();
    void drawPart(Animation* anim, unsigned int index, int frame, BVHNode *motion,
                  BVHNode *joints, int mode);
    void drawSkin();
    void drawProps();
    void drawProp(const Prop* prop) const;
    void drawDragHandles(const Prop* prop) const;

    // keys held down, for the prop handles (Shift: scale, Ctrl: rotate)
    Qt::KeyboardModifiers modifier;

    // what the mouse is doing between press and release
    enum class MouseDrag
    {
      None,
      Click,     // left button on a joint, prop or empty space: a click, or orbit once it moves
      Camera,    // moving the camera (m_cameraDrag says how)
      Ring,      // turning a joint with a rotation ring
      Prop       // dragging a prop handle
    };
    MouseDrag m_mouseDrag=MouseDrag::None;
    Qt::MouseButton m_dragButton=Qt::NoButton;
    CameraDrag m_cameraDrag=CameraDrag::None;
    QPoint m_pressPos;               // where the button went down (widget pixels)
    unsigned int m_pressSelected=0;  // what was under the mouse then
    unsigned int m_lastClicked=0;    // what the last click selected
    // a left click on this: select a joint or prop, or clear the selection
    void clickSelect(unsigned int selected);
    // moves the camera for one mouse step (pixels)
    void moveCamera(CameraDrag how, int dx, int dy);
    // where each joint was in the world when it was last drawn
    QHash<unsigned int,QVector3D> m_jointPositions;
    // turn the camera around this joint; distance < 0 keeps the distance
    bool focusOnJoint(unsigned int selected, float distance);

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

    void setProjection();
    // the free (uncovered) area of the view, in widget-local pixels: the
    // view itself cut by any floating panel that lies over it (P1's
    // PanelLayout::freeArea(), docs/PANELS_PLAN.md, job P5)
    QList<QRect> coveringPanelRects() const;
    // moves the camera's pivot nearer or further so the figure keeps about
    // the same size on screen when the view's height changes (a panel is
    // docked or undocked); skipped the first time and while renderImage()
    // draws at a size of its own
    void keepFigureSizeOnHeightChange(int newHeight);
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
    // drawFigure() index of the Script Lab preview figure: its pick names are far from the scene's
    static constexpr unsigned int EffectFigureIndex=99;

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

    // the props of the scene
    PropManager* propManager() const;

    // the scene that is drawn; owned by the window
    Scene* m_scene=nullptr;
    // the Script Lab effects preview (issue 293); owned by the window
    QPointer<EffectsPreview> m_effects;
    // the Script Lab camera override (job 7b); owned by the window
    QPointer<CameraOverride> m_cameraOverride;
    // puts the override's constraint on the camera, or takes it off
    void applyCameraOverride();

    // the handle shapes
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
