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
#include "limbpull.h"
#include "chainlines.h"

#include <functional>

class QMouseEvent;
class QOpenGLFramebufferObject;
class QOpenGLShaderProgram;
class QOpenGLTextureBlitter;
class Animation;
class CameraOverride;
class EnvironmentOverride;
class ViewModesOverride;
class EffectsPreview;
class PropManager;
class Scene;
class QLabel;

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
    // Script Lab chains and ribbons (job A4, part 2): straight lines between two props by name,
    // drawn while the list is not empty (the Script Lab panel gives it while its props are shown)
    void setChainLines(const QList<ChainLines::Line>& lines);
    QList<ChainLines::Line> chainLines() const { return m_chainLines; }
    // Script Lab camera limits (job 7b): while the override is active the camera keeps to its
    // constraint; when it stops, the camera from before is put back exactly. Not owned.
    void setCameraOverride(CameraOverride* cameraOverride);
    // Script Lab sky and draw distance (job 7c): the background and fog while its box is ticked
    void setEnvironmentOverride(EnvironmentOverride* environmentOverride);
    // Script Lab view modes (job 7c): hidden figure, plain floor, overlay while its box is ticked
    void setViewModesOverride(ViewModesOverride* viewModesOverride);

    void selectPart(BVHNode* node);
    void selectProp();
    BVHNode* getSelectedPart();
    unsigned int getSelectedPartIndex();

    // The picture the view shows, drawn at its own size whatever the window size is.
    QImage renderImage(int imageWidth, int imageHeight);
    Camera* camera() const;
    // the mouse and key habit the camera follows (UX phase 4a); the window sets it from
    // UserSettings::cameraHabit() at start and whenever Setup or Camera and keys changes it
    void setCameraHabit(CameraHabit habit) { m_cameraHabit = habit; }
    CameraHabit cameraHabit() const { return m_cameraHabit; }
    // the view tool of the left mouse button (UX phase 4b); the window sets it from
    // UserSettings::viewTool() and when the toolbar or the keys V, O, M, Z choose one
    void setViewTool(ViewTool tool);
    ViewTool viewTool() const { return m_viewTool; }
    // the view toolbar's Show menu: the rotation rings around the picked joint and the floor
    void setShowRings(bool on);
    bool showRings() const { return m_showRings; }
    void setShowFloor(bool on);
    bool showFloor() const { return m_showFloor; }
    // View > Joint dots (UX job U2): the dots on the joints; picking still finds them when off
    void setShowJointDots(bool on);
    bool showJointDots() const { return m_showJointDots; }
    // View > Click and bend on the figure (UX job U2): off, a click or drag on a joint or a
    // rotation ring does nothing but say so in the status line (src/viewmenuspec.h)
    void setClickAndBend(bool on);
    bool clickAndBend() const { return m_clickAndBend; }
    // what the hint line says about what is under the mouse: a rotation ring, or the view tool
    // when it is not Pick (src/viewtoolspec.h); empty for nothing
    QString hoverHint() const { return m_hoverHint; }

    // the shapes the view draws for its handles (cube, sphere and cone)
    // Dragging a joint (UX phase 4b part 2, docs/UX_DECISIONS.md 6a): the window says why a pull
    // cannot start now ("Stop playing first.", the locked first frame; empty: it can), and gives
    // the joints' plain names ("Right Hand") for the label next to the pointer
    void setPullGuard(const std::function<QString()>& guard) { m_pullGuard = guard; }
    void setJointNamer(const std::function<QString(const QString&)>& namer) { m_jointNamer = namer; }
    // true while a joint is being pulled or turned by a drag or the keys
    bool pulling() const { return m_pull.active; }
    // what the label next to the pointer says during a pull (its two or three lines), for the tests
    QStringList pullLabelLines() const;
    // the turns a pull shows now, before it is released (empty when no pull runs)
    QHash<const BVHNode*,QVector3D> pullPreview() const { return m_pullPreview; }

    void setMeshModels(QSharedPointer<Mesh> cubeMesh, QSharedPointer<Mesh> sphereMesh, QSharedPointer<Mesh> coneMesh);

    // Two figures in one scene (docs/UX_DECISIONS.md 6m): each figure stands at its own place
    // sideways (scene units; the window keeps it, so removing one never moves the other); the
    // figure that is not active is drawn in text_disabled with no joint dots and no rings
    void setFigureOffset(const Animation* anim, float x);
    float figureOffset(const Animation* anim) const;
    // a click on another figure asks this first; false keeps everything as it is (Beginner mode)
    void setFigureGuard(const std::function<bool(int)>& guard) { m_figureGuard = guard; }
    // where a figure's name tag goes (a little over its head, widget pixels), from the last picture
    bool figureTagPoint(int index, QPoint* point) const;

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
    // shows the panels (docs/archive/PANELS_PLAN.md, job P4)
    void tabPressed();

    // V, O, M or Z was pressed in the 3D view: the window chooses that view tool
    void viewToolRequested(ViewTool tool);
    // hoverHint() changed
    void hoverHintChanged(const QString& hint);
    // A pull ended with a release (or Enter): the window sets these turns as keys at the current
    // frame, nearest joint first. pulled: the joint that was dragged; turnOnly: only it turned.
    void pullFinished(BVHNode* pulled, const QList<QPair<BVHNode*,QVector3D>>& turns, bool turnOnly);
    // a pull was cancelled with Esc: nothing changed
    void pullCancelled();
    // the 3D view has a sentence for the status line ("Pull with keys: Pick a joint first. ...")
    void statusMessage(const QString& text);
    // the view was drawn on screen (the camera may have moved): the view toolbar follows it
    void viewDrawn();

  public slots:
    void resetCamera();
    // Shift+F: the camera looks at the whole figure from just far enough to see all of
    // it (the joints where they were last drawn), keeping its turn; false when nothing
    // was drawn yet
    bool frameWholeFigure();
    // F, and the view toolbar's Picked joint: the camera turns around the picked joint; false
    // when no joint is picked (or it was not drawn yet)
    bool focusOnPickedJoint();
    // set_camera with a joint (phase 2c-a): where the joint of the first animation is drawn now,
    // the point focusOnPickedJoint() looks at; draws a small picture first so the position
    // follows the current frame and pose; false when the joint is not drawn
    bool drawnJointPosition(const QString& joint, QVector3D* position);
    // Pull with keys (the Selected joint panel's button, Enter or G in the 3D view): picks the
    // joint up; the arrow keys move it, Enter sets the key, Esc cancels. turnOnly: the other way
    // than the drag choice (Shift + Enter)
    void startKeyPull(bool otherWay=false);
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
      MODE_SKELETON,  // the bone lines
      MODE_ROT_AXES,
      MODE_RINGS      // the joint rings (src/jointrings.h)
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
    void leaveEvent(QEvent* event) override;

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
    void updateRingChanged(Animation* anim);
    void drawJointRing(Animation* anim, const BVHNode* joint);
    // by track: the joint has changed, so its ring has its area colour
    QVector<bool> m_ringChanged;
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
      Joint,     // pressed on a joint: a click picks it, a drag pulls it
      Pull,      // pulling a joint (or turning only it) with the mouse
      Prop       // dragging a prop handle
    };
    MouseDrag m_mouseDrag=MouseDrag::None;
    Qt::MouseButton m_dragButton=Qt::NoButton;
    CameraDrag m_cameraDrag=CameraDrag::None;
    CameraHabit m_cameraHabit=CameraHabit::PinglyMotion;
    ViewTool m_viewTool=ViewTool::Pick;
    bool m_showRings=true;
    bool m_showFloor=true;
    bool m_showJointDots=true;
    bool m_clickAndBend=true;
    // the press was on a joint or ring while Click and bend is off
    bool m_pressBlocked=false;
    QString m_hoverHint;
    // the hint for what is highlighted now (a ring) and the view tool
    void updateHoverHint();
    void setHoverHint(const QString& hint);
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
    // the projection of the last picture on screen (for pulling a joint between pictures)
    QMatrix4x4 m_lastProjection;
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
    // PanelLayout::freeArea(), docs/archive/PANELS_PLAN.md, job P5)
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

    // two figures (6m): the places, the guard, and the figure being drawn is not the active one
    QHash<const Animation*,float> m_figureOffsets;
    std::function<bool(int)> m_figureGuard;
    bool m_otherFigure=false;
    QHash<int,QVector3D> m_figureTops;

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
    QPointer<EnvironmentOverride> m_environmentOverride;
    QPointer<ViewModesOverride> m_viewModesOverride;
    // draws the Script Lab overlay (setoverlay) over the whole view
    void drawOverlay();
    // the Script Lab vision sphere (setsphere): reads the drawn picture and its depth back, works the
    // effect on it by each pixel's distance from the avatar (or the camera) and draws it back. Only
    // while the environment override's sphere is active; otherwise the picture is not touched.
    void drawSphere();
    // where the vision sphere is centred for the avatar: the hip of the figure drawn last
    QVector3D m_sphereOrigin;
    std::unique_ptr<QOpenGLTextureBlitter> m_sphereBlitter;
    unsigned int m_sphereTexture=0;
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

    // dragging a joint (UX phase 4b part 2)
    struct PullState
    {
      bool active=false;
      bool viaKeys=false;
      LimbPull::Pull pull;
      unsigned int selectName=0;  // the pick name of the pulled joint
      QPointF start;              // where the tip was on screen when the pull started (widget pixels)
      QPointF target;             // where the tip should go now (widget pixels)
      float depth=0;              // the tip's depth when it started (clip space z / w)
      QPoint pressPos;            // the mouse position when the button went down
      bool shiftAtPress=false;
    };
    PullState m_pull;
    QHash<const BVHNode*,QVector3D> m_pullPreview;
    // where each joint's frame was drawn, before its own turn (world coordinates)
    QHash<const BVHNode*,QMatrix4x4> m_jointFrames;
    std::function<QString()> m_pullGuard;
    std::function<QString(const QString&)> m_jointNamer;
    QLabel* m_pullLabel=nullptr;
    // starts a pull of this pick name; false (with the reason in the status line) when it cannot
    bool startPull(unsigned int selected,bool turnOnly,bool viaKeys);
    void movePullTo(const QPointF& target);
    void finishPull();
    void cancelPull();
    void endPull();
    QString jointName(const BVHNode* node) const;
    void showPullLabel();
    // the dashed line from where the joint was to the pointer, and the target ring (screen space)
    void drawPullLine();
    QPointF toScreen(const QVector3D& world,float* depth=nullptr) const;
    QVector3D fromScreen(const QPointF& point,float depth) const;

    // the letters on the rotation rings (X, Y, Z on their axis colour), placed where the mockup puts them
    QLabel* m_ringLabel[3]={nullptr,nullptr,nullptr};
    QPointF m_ringLabelPos[3];
    bool m_ringLabelShown[3]={false,false,false};
    void placeRingLabels();
    // job A4: the chain lines, and the word tags beside them (placed after the picture, as the ring letters)
    void drawChainLines();
    void placeChainLabels();
    QList<ChainLines::Line> m_chainLines;
    mutable QHash<QString,QVector3D> m_propEye;   // each prop's centre in eye space, as last drawn
    QList<QPointF> m_chainMiddles;                // where each line's word goes (-1,-1: not on screen)
    QList<QLabel*> m_chainLabels;
};

#endif
