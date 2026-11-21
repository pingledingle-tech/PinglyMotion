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

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <QApplication.h>
#else
#include <GL/gl.h>
#endif

#include <QDockWidget>
#include <QMatrix4x4>
#include <QOpenGLFramebufferObject>
#include <QOpenGLShaderProgram>
#include <QTimer>
#include <QMessageBox>
#include <climits>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QScopedPointer>
#include <QToolTip>

#include "animation.h"
#include "logging.h"
#include "animationview.h"
#include "bvh.h"
#include "constants.h"
#include "jointgroups.h"
#include "panellayout.h"
#include "propmanager.h"
#include "scene.h"
#include "floor.h"
#include "skeletonwalk.h"
#include "skinrenderer.h"

#include "usersettings.h"
#include "scenestate.h"
#include "screenshotsheet.h"

AnimationView::AnimationView(QWidget* parent, const char* /* name */, Animation* anim)
 : QOpenGLWidget(parent),
   m_cubeMeshModel(nullptr),
   m_sphereMeshModel(nullptr),
   m_coneMeshModel(nullptr)
{
  figureFiles << Constants::maleBvhPath() << Constants::femaleBvhPath();

  bvh=new BVH();
  if(!bvh)
  {
    qCWarning(lcView,"AnimationView(): BVH initialisation failed.");
    return;
  }

  // init
  partHighlighted=0;
  propDragging=0;
  partSelected=0;
  mirrorSelected=0;
  dragX=0;
  dragY=0;
  changeX=0;
  changeY=0;
  changeZ=0;

  QString limFile = Constants::limitsFilePath();
  qCDebug(lcView,"AnimationView(): using limits file '%s'",limFile.toLatin1().constData());

  // read SL reference models to restore joint positions, in case another model has joints
  // we do not support (e.g. the SL example bvh files)
  for(int i=0;i<Animation::NUM_FIGURES;i++)
  {
    QString model(figureFiles[i]);
    qCDebug(lcView,"Reading reference model '%s'",model.toLatin1().constData());
    joints[i]=bvh->animRead(model,limFile);
  }

  QString skinError;
  if(!m_avatarSkin.load(Constants::pinglyMotionDataPath(),&skinError))
    qWarning("AnimationView::AnimationView(): no avatar body: %s",skinError.toLatin1().constData());

  // the reference skeleton decides how many tracks the timeline has
  Constants::setNumParts(BVH::countNodes(joints[0])+1);
  m_partNames=partNames();
  JointGroups::setTracks(m_partNames);

  modifier=Qt::NoModifier;
  if(anim) m_scene->setAnimation(anim);
  setMouseTracking(true);
  setFocusPolicy(Qt::StrongFocus);
}

AnimationView::~AnimationView()
{
  // the body's buffers live in this view's context
  Mesh::setDrawHook(nullptr);
  Floor::setDrawHook(nullptr);
  makeCurrent();
  m_skinRenderer.cleanup();
  m_pickBuffer.reset();
  doneCurrent();
}

// Bento bones are named like mHandIndex1Left; classic joints like lHand
bool AnimationView::isBentoBone(const QString& name)
{
  return name.length()>1 && name[0]=='m' && name[1].isUpper();
}

// bones of the face area, shown only while the head or a face bone is selected
bool AnimationView::isFaceBone(const QString& name)
{
  return name.startsWith("mFace") || name.startsWith("mEye") || name=="mSkull";
}

// joint name for an OpenGL selection id (0 or a prop id give an empty name)
QString AnimationView::selectionName(unsigned int selection) const
{
  if(selection==0 || selection>=static_cast<unsigned int>(OpenGlIds::ObjectStart)) return QString();
  return m_partNames.value(selection % static_cast<unsigned int>(OpenGlIds::AnimationIncrement));
}

bool AnimationView::faceFocus() const
{
  const QString name=selectionName(partSelected);
  return name=="head" || isFaceBone(name);
}

// names of all timeline tracks: "position", then every node of the skeleton
// in the order they are drawn ("Site" for End Sites)
QStringList AnimationView::partNames() const
{
  QStringList names;
  names << "position";
  QList<BVHNode*> nodes;
  SkeletonWalk::collect(joints[0],nodes,true);
  for(BVHNode* node : nodes) names << node->name();
  return names;
}

Scene* AnimationView::initializeScene(QObject* parent)
{
  return m_scene = new Scene(static_cast<int>(OpenGlIds::ObjectStart), parent);
}

BVH* AnimationView::getBVH() const
{
  return bvh;
}

void AnimationView::drawProp(const Prop* prop) const
{
  Prop::State state=Prop::Normal;
  if(propManager()->getSelectedPropId() == prop->id())
    state=Prop::Selected;
  else if(partHighlighted==prop->id()) state=Prop::Highlighted;
  if(isPicking()) pickName(prop->id());
  prop->draw(state);
  if(propManager()->getSelectedPropId() == prop->id())
    drawDragHandles(prop);
}

void AnimationView::drawProps()
{
  for(unsigned int index=0;index<(unsigned int) propManager()->count();index++)
  {
    Prop* prop=propManager()->at(index);
    if(!prop->isAttached()) drawProp(prop);
  }
}

// a floating panel that lies over the view, in widget-local pixels, found by
// looking at the dock widgets of the top-level window (not by a method on
// it, so this view needs no change there, and other panel jobs can run
// beside this one without touching the same file; docs/PANELS_PLAN.md, P5)
QList<QRect> AnimationView::coveringPanelRects() const
{
  QList<QRect> panels;
  const QWidget* top=window();
  if(!top) return panels;
  const QRect viewRect(QPoint(0,0),size());
  const QList<QDockWidget*> docks=top->findChildren<QDockWidget*>();
  for(const QDockWidget* dock : docks)
  {
    if(!dock->isFloating() || dock->isHidden()) continue;
    const QRect local(mapFromGlobal(dock->mapToGlobal(QPoint(0,0))),dock->frameGeometry().size());
    if(local.intersects(viewRect)) panels << local;
  }
  return panels;
}

// keeps the figure about the same size on screen when the view's height
// changes: the vertical field of view is fixed, so a shorter (or taller)
// view would otherwise shrink (or grow) the figure (docs/PANELS_PLAN.md, P5,
// section 5 "Camera centring")
void AnimationView::keepFigureSizeOnHeightChange(int newHeight)
{
  if(newHeight>0 && m_lastViewHeight>0 && newHeight!=m_lastViewHeight)
    camera()->zoom(float(newHeight)/float(m_lastViewHeight));
  if(newHeight>0) m_lastViewHeight=newHeight;
}

void AnimationView::setProjection()
{
  const QSize size=m_renderSize.isValid() ? m_renderSize : QSize(width(),height());
  const QRect viewRect(QPoint(0,0),size);
  QPointF center=viewRect.center();
  // renderImage() draws at a size of its own (exporting a picture): neither
  // the panels nor the live widget's height apply to that image
  if(!m_renderSize.isValid())
  {
    keepFigureSizeOnHeightChange(size.height());
    const QList<QRect> panels=coveringPanelRects();
    if(!panels.isEmpty())
      center=PanelLayout::freeArea(viewRect,panels).center();
  }
  const QMatrix4x4 perspective=shiftedPerspective(60.0f,QRectF(viewRect),center,1.0f,2000.0f);
  SceneState::setProjection(perspective);
}

void AnimationView::paintGL()
{
  // QOpenGLWidget draws into its own buffer, sized in device pixels
  // (more than the widget size on high-DPI screens)
  const qreal ratio=devicePixelRatioF();
  if(m_renderSize.isValid())
    glViewport(0,0,m_renderSize.width(),m_renderSize.height());
  else
    glViewport(0,0,(GLint)(width()*ratio),(GLint)(height()*ratio));
  setProjection();
  SceneState::loadIdentity();

  draw();
}

QImage AnimationView::renderImage(int imageWidth,int imageHeight)
{
  makeCurrent();
  QOpenGLFramebufferObjectFormat format;
  format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
  QOpenGLFramebufferObject buffer(imageWidth,imageHeight,format);

  // a control command may ask for this one picture from another side, without
  // the Bento markers, and for where the joints are (src/screenshotsheet.h);
  // the camera is put back exactly afterwards, so the window does not change
  ScreenshotSheet::Request* request=ScreenshotSheet::current();
  const CameraPosition savedCamera=camera()->state();
  if(request && request->overrideCamera)
    camera()->orbit(request->height,request->angle,request->distance);
  m_hideMarkers=request && request->hideMarkers;

  m_renderSize=QSize(imageWidth,imageHeight);
  buffer.bind();
  paintGL();
  buffer.release();

  if(request && request->wantMarks)
  {
    // the joints of the first animation, where they were drawn
    QList<QPair<QString,QVector3D>> world;
    for(auto it=m_jointPositions.constBegin();it!=m_jointPositions.constEnd();++it)
    {
      if(it.key()>=static_cast<unsigned int>(OpenGlIds::AnimationIncrement)) continue;
      const QString name=selectionName(it.key());
      if(!name.isEmpty()) world.append(qMakePair(name,it.value()));
    }
    request->marks=ScreenshotSheet::project(world,SceneState::projection()*m_viewMatrix,m_renderSize);
  }
  if(request) request->drawn=true;

  m_renderSize=QSize();
  m_hideMarkers=false;
  camera()->setState(savedCamera);

  const QImage image=buffer.toImage();
  doneCurrent();
  // the window shows its own picture again
  update();
  return image;
}

void AnimationView::resizeGL(int,int)
{
  // viewport and projection are set at the start of every paintGL()
}

void AnimationView::initializeGL()
{
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  setProjection();

  if(m_skinRenderer.init())
    qInfo("3D view drawn with %s",qPrintable(m_skinRenderer.status()));
  else
  {
    // PinglyMotion draws everything with OpenGL 3.3 shaders (Second Life itself
    // needs OpenGL 3.2 or newer). Say so once, after the window is up.
    qWarning("3D view cannot be drawn: %s",qPrintable(m_skinRenderer.status()));
    if(!m_glMessageShown)
    {
      m_glMessageShown=true;
      const QString reason=m_skinRenderer.status();
      QTimer::singleShot(0,this,[this,reason]()
      {
        QMessageBox::critical(this,tr("OpenGL 3.3 needed"),
          tr("PinglyMotion needs a graphics card and driver with OpenGL 3.3 or newer "
             "to show the 3D view.\n\nPlease update the graphics driver.\n\n(%1)").arg(reason));
      });
    }
  }

  // Bento balls, props and handles: drawn by the shader with the same light
  // as the body; while picking in their pick colour
  Mesh::setDrawHook([this](const Mesh& mesh)
  {
    if(!m_skinRenderer.isReady()) return true;
    if(m_colorPicking)
      m_skinRenderer.drawMesh(mesh,m_pickColor,SceneState::modelview(),SceneState::projection(),
                              currentFog(),false);
    else
      m_skinRenderer.drawMesh(mesh,SceneState::color(),SceneState::modelview(),SceneState::projection(),
                              currentFog(),SceneState::lighting());
    return true;
  });
  Floor::setDrawHook([this](const QVector<QVector3D>& positions,const QVector<QVector3D>& normals,
                            const QVector<QVector4D>& colors)
  {
    return drawWithShader(GL_TRIANGLES,positions,normals,colors);
  });
  m_pickBuffer.reset();
}

void AnimationView::draw()
{
  // picking draws too, but finds no positions: keep the last ones
  if(!isPicking())
  {
    m_haveSelectedPosition=false;
    m_jointPositions.clear();
  }
  m_ringVisible[0]=m_ringVisible[1]=m_ringVisible[2]=false;
  if(!isValid()) initializeGL();

  const GLfloat clearValue=UserSettings::darkTheme() ? 0.20f : 0.5f;
  if(!m_skinRenderer.isReady())
  {
    // no OpenGL 3.3: nothing can be drawn (the user got a message)
    glClearColor(clearValue,clearValue,clearValue,1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    return;
  }

  // fog towards the background colour, the shaders use it (exponential)
  SceneState::Fog fog;
  fog.on=UserSettings::fog();
  fog.density=0.005f;
  fog.color=QVector3D(clearValue,clearValue,clearValue);
  SceneState::setFog(fog);

  glClearColor(clearValue,clearValue,clearValue,0.3f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  SceneState::setLighting(false);
  glEnable(GL_DEPTH_TEST);

  camera()->setModelView();
  {
    // view matrix without any model transformation, to find world positions
    m_viewMatrix=SceneState::modelview();
  }
  m_shadowCasters.clear();
  drawAnimations();
  m_scene->drawFloor();
  drawShadows();
  drawProps();
}

// the shadows of the bodies drawn in this frame, on top of the floor
void AnimationView::drawShadows()
{
  if(m_shadowCasters.isEmpty() || !m_skinRenderer.isReady()) return;
  const QMatrix4x4 projectionMatrix=SceneState::projection();
  const SkinRenderer::Fog fog=currentFog();
  for(const ShadowCaster& caster : m_shadowCasters)
  {
    m_avatarSkin.setMale(caster.male);
    m_skinRenderer.drawShadow(m_avatarSkin,caster.positions,caster.normals,m_viewMatrix,projectionMatrix,fog);
  }
  m_shadowCasters.clear();
}

void AnimationView::drawAnimations()
{
  for(unsigned int index=0;index< (unsigned int) m_scene->getCountOfAnimations();index++)
  {
    drawFigure(m_scene->getAnimation(index),index);
  }
}

bool AnimationView::drawWithShader(GLenum mode,const QVector<QVector3D>& positions,
                                   const QVector<QVector3D>& normals,const QVector<QVector4D>& colors) const
{
  if(!m_skinRenderer.isReady()) return false;
  static const QVector<QVector3D> noNormals;
  if(m_colorPicking)
  {
    // the pick colour, plain
    m_skinRenderer.drawGeometry(mode,positions,noNormals,QVector<QVector4D>(positions.size(),m_pickColor),
                                SceneState::modelview(),SceneState::projection(),currentFog(),
                                SceneState::lineWidth());
    return true;
  }
  // with lighting off: plain colours
  m_skinRenderer.drawGeometry(mode,positions,SceneState::lighting() ? normals : noNormals,colors,
                              SceneState::modelview(),SceneState::projection(),currentFog(),
                              SceneState::lineWidth());
  return true;
}

bool AnimationView::drawWithShader(GLenum mode,const QVector<QVector3D>& positions,const QVector4D& color) const
{
  // lines have no normals, so they are always plain colour
  return drawWithShader(mode,positions,QVector<QVector3D>(),QVector<QVector4D>(positions.size(),color));
}

SkinRenderer::Fog AnimationView::currentFog() const
{
  return SceneState::fog();
}

bool AnimationView::isPicking() const
{
  return m_colorPicking;
}


void AnimationView::pickName(unsigned int name) const
{
  if(m_colorPicking)
    m_pickColor=SkinRenderer::encodePickName(name);
}


int AnimationView::pickPart(int x, int y)
{
  return m_skinRenderer.isReady() ? pickPartByColor(x,y) : 0;
}


int AnimationView::pickPartByColor(int x, int y)
{
  // GL calls outside paintGL() need the widget's context
  makeCurrent();

  // mouse positions are in widget pixels, the GL buffer is in device pixels
  const qreal ratio=devicePixelRatioF();
  const int bufferWidth=qMax(1,(int)(width()*ratio));
  const int bufferHeight=qMax(1,(int)(height()*ratio));
  if(!m_pickBuffer || m_pickBuffer->size()!=QSize(bufferWidth,bufferHeight))
  {
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::Depth);
    format.setInternalTextureFormat(GL_RGBA8);
    m_pickBuffer=std::make_unique<QOpenGLFramebufferObject>(bufferWidth,bufferHeight,format);
  }

  // the same area around the mouse as the old selection picking: 5 pixels
  const int half=qMax(2,(int)(2.5*ratio));
  const int size=2*half+1;
  const int centerX=(int)(x*ratio);
  const int centerY=bufferHeight-1-(int)(y*ratio);
  const int left=qBound(0,centerX-half,bufferWidth-1);
  const int bottom=qBound(0,centerY-half,bufferHeight-1);
  const int areaWidth=qMin(size,bufferWidth-left);
  const int areaHeight=qMin(size,bufferHeight-bottom);

  m_pickBuffer->bind();
  glViewport(0,0,bufferWidth,bufferHeight);
  glEnable(GL_SCISSOR_TEST);
  glScissor(left,bottom,areaWidth,areaHeight);
  glClearColor(0,0,0,0);
  glClearDepth(1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  // nothing may change the pick colours
  glDisable(GL_BLEND);
  glDisable(GL_DITHER);
  const SceneState::Fog savedFog=SceneState::fog();
  SceneState::setFog(SceneState::Fog());
  SceneState::setLighting(false);
  glEnable(GL_DEPTH_TEST);

  setProjection();
  camera()->setModelView();

  m_colorPicking=true;
  drawAnimations();
  drawProps();
  m_colorPicking=false;
  SceneState::setFog(savedFog);

  const qsizetype areaPixels=qsizetype(areaWidth)*areaHeight;
  QVector<unsigned char> colors(areaPixels*4);
  QVector<GLfloat> depths(areaPixels);
  glPixelStorei(GL_PACK_ALIGNMENT,1);
  glReadPixels(left,bottom,areaWidth,areaHeight,GL_RGBA,GL_UNSIGNED_BYTE,colors.data());
  glReadPixels(left,bottom,areaWidth,areaHeight,GL_DEPTH_COMPONENT,GL_FLOAT,depths.data());

  glDisable(GL_SCISSOR_TEST);
  glEnable(GL_DITHER);
  glEnable(GL_BLEND);
  m_pickBuffer->release();

  // Which hit wins, in this order:
  // 1. a Bento ball anywhere in the area (they are drawn on top of the body;
  //    among them the one nearest the mouse)
  // 2. a ring, handle or prop in the area that is in front of what is under
  //    the mouse (the nearest one), so thin rings stay easy to grab
  // 3. what is right under the mouse
  // 4. the nearest thing in the area, so small things can be hit when the
  //    mouse is just beside them
  const int centerIndex=(centerY-bottom)*areaWidth+(centerX-left);
  const bool centerInside=centerX>=left && centerX<left+areaWidth && centerY>=bottom && centerY<bottom+areaHeight;
  const unsigned int centerName=centerInside ? SkinRenderer::decodePickName(colors.constData()+centerIndex*4) : 0;
  const float centerDepth=centerName ? depths[centerIndex] : 2.0f;
  const unsigned int objectStart=static_cast<unsigned int>(OpenGlIds::ObjectStart);

  unsigned int name=0, toolName=0, bentoName=0;
  float nearest=2.0f, toolDepth=2.0f;
  int bentoDistance=INT_MAX;
  for(int row=0;row<areaHeight;row++)
  {
    for(int column=0;column<areaWidth;column++)
    {
      const int i=row*areaWidth+column;
      const unsigned int hit=SkinRenderer::decodePickName(colors.constData()+i*4);
      if(!hit) continue;
      if(depths[i]<nearest)
      {
        nearest=depths[i];
        name=hit;
      }
      if(hit>=objectStart && depths[i]<=centerDepth && depths[i]<toolDepth)
      {
        toolDepth=depths[i];
        toolName=hit;
      }
      const int dx=left+column-centerX;
      const int dy=bottom+row-centerY;
      const int distance=dx*dx+dy*dy;
      if(distance<bentoDistance && isBentoBone(selectionName(hit)))
      {
        bentoDistance=distance;
        bentoName=hit;
      }
    }
  }
  if(centerName) name=centerName;
  if(toolName) name=toolName;
  if(bentoName) name=bentoName;
  doneCurrent();
  return name;
}

// Mouse and keys: see docs/POSING_UX.md ("Mouse and keys") and
// Camera::dragFor(). A left click selects; a left drag on empty space (or on
// the body) turns the camera; rings turn joints; Alt is the Second Life camera.
void AnimationView::mouseMoveEvent(QMouseEvent* event)
{
  if(m_mouseDrag==MouseDrag::Ring && m_ringDragging>=0)
  {
    // turn the joint as far as the mouse moved along the ring
    const QPoint moved=event->pos()-m_ringLastPos;
    m_ringLastPos=event->pos();
    const double lengthSquared=m_ringTangent.x()*m_ringTangent.x()+m_ringTangent.y()*m_ringTangent.y();
    double degrees=(moved.x()*m_ringTangent.x()+moved.y()*m_ringTangent.y())/lengthSquared;
    // Shift: finer control
    if(event->modifiers() & Qt::ShiftModifier) degrees*=0.2;
    emit partDragged(getSelectedPart(),
                     m_ringDragging==0 ? degrees : 0,
                     m_ringDragging==1 ? degrees : 0,
                     m_ringDragging==2 ? degrees : 0);
    update();
    return;
  }

  if(m_mouseDrag==MouseDrag::None || m_mouseDrag==MouseDrag::Ring)
  {
    unsigned int oldPart=partHighlighted;
    partHighlighted=pickPart(event->position().toPoint().x(),event->position().toPoint().y());
    if(oldPart!=partHighlighted)
    {
      update();
      // the name of the joint under the mouse, next to the pointer
      const QString name=selectionName(partHighlighted);
      if(name.isEmpty() || name=="Site")
        QToolTip::hideText();
      else
        QToolTip::showText(mapToGlobal(event->pos()),name,this);
    }
    return;
  }

  // a click becomes a camera drag once the mouse has moved a little
  if(m_mouseDrag==MouseDrag::Click)
  {
    if((event->pos()-m_pressPos).manhattanLength()<4) return;
    m_mouseDrag=MouseDrag::Camera;
    QToolTip::hideText();
    setCursor(QCursor(Qt::BlankCursor));
  }

  QPoint dragPos=QCursor::pos();

  // since MacOSX doesn't like the old "drag and snap back" solution, we're going for a
  // more elaborate solution

  // calculate drag distance from last dragging point
  dragX=dragPos.x()-oldDragX;
  dragY=dragPos.y()-oldDragY;

  // remember the current position as last dragging point
  oldDragX=dragPos.x();
  oldDragY=dragPos.y();

  // if mouse has moved sufficiently far enough from first clicking position ...
  if(abs(clickPos.x()-dragPos.x())>100 || abs(clickPos.y()-dragPos.y())>100)
  {
    // set remembered drag position back to first click position
    oldDragX=clickPos.x();
    oldDragY=clickPos.y();
    // set mouse cursor back to first click position
    QCursor::setPos(clickPos);
  }

  if(m_mouseDrag==MouseDrag::Camera)
  {
    moveCamera(m_cameraDrag,dragX,dragY);
    update();
  }
  else if(m_mouseDrag==MouseDrag::Prop && propDragging)
  {
    float rot=camera()->yRotation();
    Prop* prop=m_scene->getPropById(propManager()->getSelectedPropId());

    if (propDragging == static_cast<int>(OpenGlIds::DragHandleX))
    {
      if(rot>90 && rot<270) dragX=-dragX;
      emit propDragged(prop,(double) dragX/10.0,0,0);
    }
    else if (propDragging == static_cast<int>(OpenGlIds::DragHandleY))
    {
      emit propDragged(prop,0,(double) -dragY/10.0,0);
    }
    else if (propDragging == static_cast<int>(OpenGlIds::DragHandleZ))
    {
      if(rot>90 && rot<270) dragY=-dragY;
      emit propDragged(prop,0,0,(double) dragY/10.0);
    }
    else if (propDragging == static_cast<int>(OpenGlIds::ScaleHandleX))
    {
      if(rot>90 && rot<270) dragX=-dragX;
      emit propScaled(prop,(double) dragX/10.0,0,0);
    }
    else if (propDragging == static_cast<int>(OpenGlIds::ScaleHandleY))
    {
      if(rot>90 && rot<270) dragY=-dragY;
      emit propScaled(prop,0,(double) -dragY/10.0,0);
    }
    else if (propDragging == static_cast<int>(OpenGlIds::ScaleHandleZ))
    {
      emit propScaled(prop,0,0,(double) dragY/10.0);
    }
    else if (propDragging == static_cast<int>(OpenGlIds::RotateHandleX))
    {
      emit propRotated(prop,(double) dragX/5.0,0,0);
    }
    else if (propDragging == static_cast<int>(OpenGlIds::RotateHandleY))
    {
      emit propRotated(prop,0,(double) -dragY/5.0,0);
    }
    else if (propDragging == static_cast<int>(OpenGlIds::RotateHandleZ))
    {
      emit propRotated(prop,0,0,(double) dragY/5.0);
    }
    update();
  }
}

void AnimationView::moveCamera(CameraDrag how,int dx,int dy)
{
  // degrees per pixel when turning
  const float turn=0.5f;
  // zoom: 1% per pixel, down = further away
  const float zoom=std::pow(1.01f,(float) dy);
  // pan: the point the camera turns around follows the mouse
  const float perPixel=2.0f*std::tan(30.0f*M_PI/180.0f)*camera()->distance()/qMax(1,height());

  switch(how)
  {
    case CameraDrag::Orbit:
      camera()->rotate(dy*turn,dx*turn);
      break;
    case CameraDrag::OrbitAndZoom:
      camera()->rotate(0,dx*turn);
      camera()->zoom(zoom);
      break;
    case CameraDrag::Pan:
      camera()->panBy(-dx*perPixel,dy*perPixel);
      break;
    case CameraDrag::Zoom:
      camera()->zoom(zoom);
      break;
    case CameraDrag::None:
      break;
  }
}

void AnimationView::mousePressEvent(QMouseEvent* event)
{
  // one drag at a time; other buttons are ignored until it ends
  if(m_mouseDrag!=MouseDrag::None) return;

  const Qt::KeyboardModifiers keys=event->modifiers();
  modifier=keys;
  m_dragButton=event->button();
  m_pressPos=event->pos();
  m_pressSelected=0;

  // remember mouse position for dragging (screen pixels, see mouseMoveEvent())
  returnPos=QCursor::pos();
  clickPos=QCursor::pos();
  oldDragX=clickPos.x();
  oldDragY=clickPos.y();
  dragX=dragY=0;

  m_cameraDrag=Camera::dragFor(event->button(),keys);
  if(m_cameraDrag==CameraDrag::None) return;

  // right and middle button, and Alt (Second Life): always the camera
  if(event->button()!=Qt::LeftButton || (keys & Qt::AltModifier))
  {
    // Alt + click on a joint: turn around that joint from here (Second Life)
    if(event->button()==Qt::LeftButton)
    {
      const unsigned int selected=pickPart(event->position().toPoint().x(),event->position().toPoint().y());
      if(selected && selected<static_cast<unsigned int>(OpenGlIds::ObjectStart))
        focusOnJoint(selected,-1);
    }
    m_mouseDrag=MouseDrag::Camera;
    QToolTip::hideText();
    setCursor(QCursor(Qt::BlankCursor));
    update();
    return;
  }

  // left button: check out which part or prop has been clicked
  const unsigned int selected=pickPart(event->position().toPoint().x(),event->position().toPoint().y());

  // a rotation ring of the selected part was grabbed: drag it to turn the joint
  const int ring=static_cast<int>(selected)-static_cast<int>(OpenGlIds::RotateRingX);
  if(ring>=0 && ring<=2 && partSelected && m_ringVisible[ring])
  {
    m_mouseDrag=MouseDrag::Ring;
    setCursor(QCursor(Qt::ClosedHandCursor));
    startRingDrag(ring,event->pos());
    update();
    return;
  }

  // drag handle of a prop clicked
  if (selected == static_cast<unsigned int>(OpenGlIds::DragHandleX) ||
      selected == static_cast<unsigned int>(OpenGlIds::DragHandleY) ||
      selected == static_cast<unsigned int>(OpenGlIds::DragHandleZ) ||
      selected == static_cast<unsigned int>(OpenGlIds::ScaleHandleX) ||
      selected == static_cast<unsigned int>(OpenGlIds::ScaleHandleY) ||
      selected == static_cast<unsigned int>(OpenGlIds::ScaleHandleZ) ||
      selected == static_cast<unsigned int>(OpenGlIds::RotateHandleX) ||
      selected == static_cast<unsigned int>(OpenGlIds::RotateHandleY) ||
      selected == static_cast<unsigned int>(OpenGlIds::RotateHandleZ))
  {
    m_mouseDrag=MouseDrag::Prop;
    propDragging=selected;
    changeX = changeY = changeZ = 0;
    // hide mouse cursor to avoid confusion
    setCursor(QCursor(Qt::BlankCursor));
    return;
  }

  // a joint, a prop or empty space: select on release, unless the mouse
  // moves first - then it turns the camera
  m_mouseDrag=MouseDrag::Click;
  m_pressSelected=selected;
}

void AnimationView::clickSelect(unsigned int selected)
{
  m_lastClicked=selected;
  // if another part than the current one has been clicked, switch off mirror mode
  if(selected!=partSelected) m_scene->getAnimation()->setMirrored(false);

  // background clicked, reset all
  if(!selected)
  {
    partSelected=0;
    mirrorSelected=0;
    propManager()->selectProp(0);
    propDragging=0;
    emit backgroundClicked();
  }
  // body part clicked
  else if (selected < static_cast<unsigned int>(OpenGlIds::ObjectStart))
  {
    partSelected=selected;
    m_scene->selectAnimation(selected / static_cast<int>(OpenGlIds::AnimationIncrement));
    propManager()->selectProp(0);
    propDragging=0;

    BVHNode* part=getSelectedPart();
    changeX = changeY = changeZ = 0;
    dragX = dragY = 0;

    emit partClicked(part,
                     QVector3D(m_scene->getAnimation()->getRotation(part)),
                     m_scene->getAnimation()->getRotationLimits(part),
                     QVector3D(m_scene->getAnimation()->getPosition())
                    );
    emit partClicked(partSelected % static_cast<int>(OpenGlIds::AnimationIncrement));
  }
  else if (selected < static_cast<unsigned int>(OpenGlIds::DragHandlStart))
  {
    emit propClicked(m_scene->getPropById(selected));
    propManager()->selectProp(selected);
  }
  update();
}

void AnimationView::mouseReleaseEvent(QMouseEvent* event)
{
  if(event->button()!=m_dragButton || m_mouseDrag==MouseDrag::None) return;

  const MouseDrag ended=m_mouseDrag;
  m_mouseDrag=MouseDrag::None;
  m_dragButton=Qt::NoButton;
  m_cameraDrag=CameraDrag::None;
  propDragging=0;
  // show mouse cursor again
  setCursor(Qt::ArrowCursor);

  if(ended==MouseDrag::Click)
    clickSelect(m_pressSelected);
  else if(ended==MouseDrag::Ring)
  {
    m_ringDragging=-1;
    update();
  }
}

void AnimationView::mouseDoubleClickEvent(QMouseEvent* event)
{
  // Alt: Second Life camera, the press already focused
  if(event->button()!=Qt::LeftButton || (event->modifiers() & Qt::AltModifier))
  {
    mousePressEvent(event);
    return;
  }

  unsigned int selected=pickPart(event->position().toPoint().x(),event->position().toPoint().y());

  // the first click selected a joint, and its rings now lie over it (the
  // ring seen edge-on is a line through the joint): it is still that joint
  const int ring=static_cast<int>(selected)-static_cast<int>(OpenGlIds::RotateRingX);
  if(ring>=0 && ring<=2 && partSelected && m_lastClicked==partSelected)
    selected=partSelected;

  // no double clicks for props, drag handles or rings
  if (!selected || selected >= static_cast<unsigned int>(OpenGlIds::ObjectStart))
  {
    mousePressEvent(event);
    return;
  }

  // FIXME: With multiple avatars, double-clicking
  //  other avatars may require a different
  //  animation than what getAnimation() returns
  if (m_scene->getAnimation()->getNode(selected) == 0) return;

  // the first click of the double click selected the joint already
  if(partSelected!=selected) clickSelect(selected);

  if (event->modifiers() & Qt::ShiftModifier)
  {
    // Shift + double-click: the mirrored joint moves along
    mirrorSelected=getSelectedPart()->getMirrorIndex() + (selected / static_cast<int>(OpenGlIds::AnimationIncrement)) * static_cast<int>(OpenGlIds::AnimationIncrement);
    if (mirrorSelected)
    {
      m_scene->getAnimation()->setMirrored(true);
    }
  }
  else if (event->modifiers() & Qt::ControlModifier)
  {
    // Ctrl + double-click: IK on/off (until the joint card has an IK switch, #88)
    m_scene->getAnimation()->setIK(m_scene->getAnimation()->getNode(selected),
                     !m_scene->getAnimation()->getIK(m_scene->getAnimation()->getNode(selected)));
  }
  else
  {
    // double-click: look at the joint and turn around it
    zoomToSelectedPart();
  }
  update();
}

void AnimationView::wheelEvent(QWheelEvent* event)
{
  // one notch (120) moves 15% closer or further; touchpads send smaller steps
  camera()->zoom(std::pow(0.85f,event->angleDelta().y()/120.0f));
  update();
}

// Tab hides and shows the panels (docs/PANELS_PLAN.md, job P4). Qt's own
// handling in QWidget::event would move the keyboard focus to the next
// field instead, so Tab is taken here first. Only a plain Tab: Shift+Tab
// and the others go on as before. Tab in a panel's field or in the search
// box never comes here, because the 3D view does not have the keys then.
bool AnimationView::event(QEvent* event)
{
  if(event->type()==QEvent::KeyPress)
  {
    QKeyEvent* key=static_cast<QKeyEvent*>(event);
    if(key->key()==Qt::Key_Tab && key->modifiers()==Qt::NoModifier)
    {
      key->accept();
      emit tabPressed();
      return true;
    }
  }
  return QOpenGLWidget::event(event);
}

void AnimationView::keyPressEvent(QKeyEvent* event)
{
  int num=-1;
  float height,angle;

  // Shift / Ctrl show the scale / rotate handles of a selected prop
  if(modifier!=event->modifiers())
  {
    modifier=event->modifiers();
    update();
  }

  if(Camera::viewForKey(event->key(),event->modifiers(),&height,&angle))
  {
    // number pad 1 / 3 / 7: front / side / top (Blender)
    camera()->setView(height,angle);
    update();
    return;
  }
  if(Camera::isFocusKey(event->key(),event->modifiers()))
  {
    // F, or number pad . (Blender)
    zoomToSelectedPart();
    return;
  }

  switch(event->key())
  {
    case Qt::Key_PageUp:
      camera()->zoom(0.9f);
      update();
      break;
    case Qt::Key_PageDown:
      camera()->zoom(1.0f/0.9f);
      update();
      break;
    case Qt::Key_F9:
      num=0;
      break;
    case Qt::Key_F10:
      num=1;
      break;
    case Qt::Key_F11:
      num=2;
      break;
    case Qt::Key_F12:
      num=3;
      break;
    case Qt::Key_Escape:
      resetCamera();
      break;
    default:
      event->ignore();
      return;
  }

  if(num>=0)
  {
    Qt::KeyboardModifiers keyModifiers=event->modifiers();
    if(keyModifiers==Qt::ShiftModifier)
      emit storeCameraPosition(num);
    else if(keyModifiers==Qt::NoModifier)
    {
      emit restoreCameraPosition(num);
      update();
    }
  }
  event->ignore();
}

void AnimationView::keyReleaseEvent(QKeyEvent* event)
{
  if(modifier!=event->modifiers())
  {
    modifier=event->modifiers();
    update();
  }
  event->ignore();
}

void AnimationView::jointGroupsChanged()
{
  JointGroups::update();
  update();
}

void AnimationView::setBodyLight(int brightness, int shine, int warmth, bool floorShadow)
{
  SkinRenderer::Look look;
  look.brightness=qBound(0,brightness,100)/100.0f;
  look.shine=qBound(0,shine,100)/100.0f;
  look.warmth=qBound(0,warmth,100)/100.0f;
  look.shadow=floorShadow;
  m_skinRenderer.setLook(look);
  update();
}

bool AnimationView::setBody(AvatarSkin::Body wanted)
{
  if(m_avatarSkin.isLoaded() && m_avatarSkin.body()==wanted) return true;

  QString error;
  bool ok=m_avatarSkin.load(Constants::pinglyMotionDataPath(),wanted,&error);
  if(!ok)
  {
    qWarning("AnimationView::setBody(): %s",error.toLatin1().constData());
    // fall back to the other body, so that something is shown if one file is damaged
    const AvatarSkin::Body other=wanted==AvatarSkin::RuthBody ? AvatarSkin::MaxBody : AvatarSkin::RuthBody;
    if(!m_avatarSkin.load(Constants::pinglyMotionDataPath(),other,&error))
      qWarning("AnimationView::setBody(): no avatar body: %s",error.toLatin1().constData());
  }
  update();
  return ok;
}

void AnimationView::drawFigure(Animation* anim,unsigned int index)
{
    Animation::FigureType figType=anim->getFigureType();

    // lighting on: the shaders light what is drawn now
    SceneState::setLighting(true);

    // save current drawing matrix
    SceneState::pushMatrix();

    // scale drawing matrix to avatar scale specified
    float scale=anim->getAvatarScale();
    SceneState::scale(scale,scale,scale);

    QVector3D position = anim->getPosition();
    SceneState::translate(position.x(), position.y(), position.z());

    // male or female decides the skeleton too, so set it before walking it
    m_avatarSkin.setMale(anim->getFigureType()==Animation::FIGURE_MALE);

    // visual compensation: the hip of a new animation is at 43.53, the
    // pelvis of the avatar mesh is 42.26 above its soles; the male
    // skeleton has longer shins
    SceneState::translate(0, -1.27f+m_avatarSkin.footDrop(), 0);

    // the body: find out where every joint is, then bend the mesh to fit
    selectName = index * static_cast<int>(OpenGlIds::AnimationIncrement);
    glEnable(GL_DEPTH_TEST);
    m_skinJointStates.clear();
    m_parentScale=QVector3D(1,1,1);
    drawPart(anim,index,anim->getFrame(),anim->getMotion(),joints[figType],MODE_COLLECT);
    drawSkin();

    selectName = index * static_cast<int>(OpenGlIds::AnimationIncrement);
    drawPart(anim,index,anim->getFrame(),anim->getMotion(),joints[figType],MODE_PARTS);
    selectName = index * static_cast<int>(OpenGlIds::AnimationIncrement);
    drawPart(anim,index,anim->getFrame(),anim->getMotion(),joints[figType],MODE_ROT_AXES);
    selectName = index * static_cast<int>(OpenGlIds::AnimationIncrement);
    glDisable(GL_DEPTH_TEST);
    drawPart(anim,index,anim->getFrame(),anim->getMotion(),joints[figType],MODE_SKELETON);

    // restore drawing matrix
    SceneState::popMatrix();
}

// NOTE: joints == motion for now
void AnimationView::drawPart(Animation* anim,unsigned int currentAnimationIndex,int frame,BVHNode* motion,BVHNode* jointRoot,int mode)
{
  const bool selecting=isPicking();
  if(motion && jointRoot)
  {
    selectName++;
    SceneState::pushMatrix();
    // the male skeleton moves jointRoot further from bigger parents
    const QVector3D offset=QVector3D(jointRoot->offset[0],jointRoot->offset[1],jointRoot->offset[2])*m_parentScale
                          +m_avatarSkin.jointShift(jointRoot->name());
    const QVector3D ownScale=m_avatarSkin.jointScale(jointRoot->name());
    SceneState::translate(offset.x(),offset.y(),offset.z());
    if(motion->type==BVH_NO_SL)
    {
      selectName++;
      motion=motion->child(0);
    }
    if(mode==MODE_SKELETON && UserSettings::skeleton() && !selecting)
    {
      SceneState::setLineWidth(1);
      drawWithShader(GL_LINES,{-offset,QVector3D(0,0,0)},QVector4D(0,1,1,1));

      // face bones only get a ball while the head or a face bone is selected,
      // otherwise the many face balls cover the whole head
      if(jointRoot->type!=BVH_ROOT && JointGroups::shown(motion->name()) &&
         (!isFaceBone(motion->name()) || faceFocus()))
      {
        // draw joint spheres in skeleton mode, red for selected parts,
        // blue for hightlighted and green for all others
        if(partSelected==selectName)
          SceneState::setColor(1,0,0,1);
        else if(partHighlighted==selectName)
          SceneState::setColor(0,0,1,1);
        else
          SceneState::setColor(0,1,0,1);

        if(isBentoBone(motion->name()) && !m_hideMarkers)
        {
          SceneState::pushMatrix();
          const float size=isFaceBone(motion->name()) ? 0.2f : 0.4f;
          SceneState::scale(size,size,size);
          m_sphereMeshModel->draw();
          SceneState::popMatrix();
        }
        else
          m_sphereMeshModel->draw();
      }
    }

    QVector3D rotation = motion->frameData(frame).rotation();
    for(int i=0;i<motion->numChannels;i++)
    {
/*
      float value;
      if(motion->ikOn)
        value = motion->frame[frame][i] + motion->ikRot[i];
      else
        value = motion->frame[frame][i];

      switch(motion->channelType[i]) {
        case BVH_XROT: SceneState::rotate(value, 1, 0, 0); break;
        case BVH_YROT: SceneState::rotate(value, 0, 1, 0); break;
        case BVH_ZROT: SceneState::rotate(value, 0, 0, 1); break;
        default: break;
      } */

      QVector3D ikRotation;
      if(motion->ikOn) ikRotation = motion->ikRotation;

      // need to do rotations in the right order
      switch(motion->channelType[i])
      {
        case BVH_XROT: SceneState::rotate(rotation.x() + ikRotation.x(), 1, 0, 0); break;
        case BVH_YROT: SceneState::rotate(rotation.y() + ikRotation.y(), 0, 1, 0); break;
        case BVH_ZROT: SceneState::rotate(rotation.z() + ikRotation.z(), 0, 0, 1); break;
        default: break;
      }

      // rotation rings: also drawn while picking, so they can be grabbed
      if(mode==MODE_ROT_AXES && partSelected==selectName)
      {
        const int ringX=static_cast<int>(OpenGlIds::RotateRingX);
        const int hovered=static_cast<int>(partHighlighted)-ringX;
        switch(motion->channelType[i])
        {
          case BVH_XROT: drawCircle(0,m_ringRadius,hovered==0 || m_ringDragging==0); break;
          case BVH_YROT: drawCircle(1,m_ringRadius,hovered==1 || m_ringDragging==1); break;
          case BVH_ZROT: drawCircle(2,m_ringRadius,hovered==2 || m_ringDragging==2); break;
          default: break;
        }
      }
    }
    // jointRoot of a hidden group take the name of the nearest shown joint
    // above them, so clicking or hovering the body there picks that joint
    const unsigned int shownAncestor=m_shownAncestor;
    const bool jointShown=JointGroups::shown(motion->name());
    if(jointShown) m_shownAncestor=selectName;
    const unsigned int colorName=jointShown ? selectName : m_shownAncestor;

    if(mode==MODE_PARTS || mode==MODE_COLLECT)
    {
      const QMatrix4x4 current=SceneState::modelview();

      // remember where the selected part is in the world, for "zoom to part"
      if(mode==MODE_PARTS && !selecting)
      {
        const QVector3D position=(m_viewMatrix.inverted()*current).map(QVector3D(0,0,0));
        m_jointPositions.insert(selectName,position);
        if(partSelected==selectName)
        {
          m_selectedPosition=position;
          m_haveSelectedPosition=true;
        }
      }
      if(selecting) pickName(selectName);
      if(anim->getMirrored() &&
          (mirrorSelected==colorName || partSelected==colorName)
	)
      {
        SceneState::setColor(1.0,0.635,0.059,1);
      }
      else if(partSelected==colorName)
        SceneState::setColor(0.6,0.3,0.3,1);
      else if(partHighlighted==colorName)
        SceneState::setColor(0.4,0.5,0.3,1);
      else
        SceneState::setColor(0.6,0.5,0.5,1);
      if(anim->getIK(motion))
      {
        const QVector4D color=SceneState::color();
        SceneState::setColor(color.x(),color.y(),color.z()+0.3f,color.w());
      }

      if(mode==MODE_COLLECT)
      {
        // the avatar mesh is drawn in one go after this walk, see drawSkin()
        SkinJointState state;
        state.matrix=current;
        const QVector4D color=SceneState::color();
        state.color[0]=color.x();
        state.color[1]=color.y();
        state.color[2]=color.z();
        state.color[3]=color.w();
        state.selectName=colorName;
        m_skinJointStates.insert(motion->name(),state);
      }
      else
      {
      // Bento bones have no body mesh: draw a small ball that can be clicked.
      // The balls are drawn on top of the body, so bones inside it (face,
      // wings) can be seen and clicked. Face balls only show while the head
      // or a face bone is selected, otherwise they would cover the face.
      const QString& boneName=motion->name();
      if(isBentoBone(boneName) && !m_hideMarkers && jointShown && (!isFaceBone(boneName) || faceFocus()))
      {
        SceneState::pushMatrix();
        const float size=isFaceBone(boneName) ? 0.2f : 0.4f;
        SceneState::scale(size,size,size);
        glDisable(GL_DEPTH_TEST);
        m_sphereMeshModel->draw();
        glEnable(GL_DEPTH_TEST);
        SceneState::popMatrix();
      }

      for(unsigned int index=0;index< (unsigned int) propManager()->count();index++)
      {
        Prop* prop=propManager()->at(index);
        if(prop->isAttached()==selectName) drawProp(prop);
      } // for
      } // MODE_PARTS
    }
    const QVector3D parentScale=m_parentScale;
    m_parentScale=ownScale;
    for(int i=0;i<motion->numChildren();i++)
    {
      drawPart(anim,currentAnimationIndex,frame,motion->child(i),jointRoot->child(i),mode);
    }
    m_parentScale=parentScale;
    m_shownAncestor=shownAncestor;
    SceneState::popMatrix();
  }
}

// Draws the avatar body, bent to fit the joints found by the last
// MODE_COLLECT walk. In picking mode every triangle carries the name of the
// joint it mostly belongs to, so clicking the body selects that joint.
void AnimationView::drawSkin()
{
  if(!m_avatarSkin.isLoaded()) return;

  const QStringList& names=m_avatarSkin.jointNames();
  QVector<QMatrix4x4> matrices;
  QVector<const SkinJointState*> states;
  for(const QString& name : names)
  {
    auto found=m_skinJointStates.constFind(name);
    if(found==m_skinJointStates.constEnd()) return;   // skeleton without this joint
    matrices << found->matrix;
    states << &found.value();
  }
  // the graphics card bends the mesh; same projection and fog as the rest
  // of the scene. Also used for picking by colour.
  if(m_skinRenderer.isReady())
  {
    QVector<QMatrix4x4> positions;
    QVector<QMatrix3x3> normals;
    m_avatarSkin.skinMatrices(matrices,&positions,&normals);
    QVector<QVector4D> colors;
    colors.reserve(states.size());
    QVector<unsigned int> pickNames;
    pickNames.reserve(states.size());
    for(const SkinJointState* state : states)
    {
      colors << QVector4D(state->color[0],state->color[1],state->color[2],state->color[3]);
      pickNames << state->selectName;
    }

    m_skinRenderer.draw(m_avatarSkin,positions,normals,colors,pickNames,m_colorPicking,
                        SceneState::projection(),currentFog());
    // its shadow is drawn after the floor
    if(!m_colorPicking)
    {
      ShadowCaster caster;
      caster.positions=positions;
      caster.normals=normals;
      caster.male=m_avatarSkin.isMale();
      m_shadowCasters << caster;
    }
    return;
  }
}

void AnimationView::drawDragHandles(const Prop* prop) const
{
  // get prop's position
  double x=prop->xPosition();
  double y=prop->yPosition();
  double z=prop->zPosition();
 // get prop's scale
  double xs=prop->xScale() / 2.0;
  double ys=prop->yScale() / 2.0;
  double zs=prop->zScale() / 2.0;

  QColor xRGB("#ff0000");
  QColor yRGB("#00ff00");
  QColor zRGB("#0000ff");

  int xLineWidth=1;
  int yLineWidth=1;
  int zLineWidth=1;

  switch(partHighlighted)
  {
    case static_cast<int>(OpenGlIds::ScaleHandleX):
    case static_cast<int>(OpenGlIds::RotateHandleX):
    case static_cast<int>(OpenGlIds::DragHandleX):
      xRGB=xRGB.lighter(120);
      xLineWidth=3;
      break;
    case static_cast<int>(OpenGlIds::ScaleHandleY):
    case static_cast<int>(OpenGlIds::RotateHandleY):
    case static_cast<int>(OpenGlIds::DragHandleY):
      yRGB=yRGB.lighter(120);
      yLineWidth=3;
      break;
    case static_cast<int>(OpenGlIds::ScaleHandleZ):
    case static_cast<int>(OpenGlIds::RotateHandleZ):
    case static_cast<int>(OpenGlIds::DragHandleZ):
      zRGB=zRGB.lighter(120);
      zLineWidth=3;
      break;
    default:
      break;
  }

  // remember drawing matrix on stack
  SceneState::pushMatrix();
  // set matrix origin to our object's center
  SceneState::translate(x,y,z);

  if (modifier & Qt::ShiftModifier)
  {
    // now draw the scale cubes with proper depth sorting
    glEnable(GL_DEPTH_TEST);

    SceneState::rotate(prop->xRotation(),1,0,0);
    SceneState::rotate(prop->yRotation(),0,1,0);
    SceneState::rotate(prop->zRotation(),0,0,1);

    pickName(static_cast<int>(OpenGlIds::ScaleHandleX));
    SceneState::setColor(xRGB.redF(),xRGB.greenF(),xRGB.blueF(),1);
    SceneState::translate(-xs,0,0);
    m_cubeMeshModel->draw();
    SceneState::translate(xs*2,0,0);
    m_cubeMeshModel->draw();

    pickName(static_cast<int>(OpenGlIds::ScaleHandleY));
    SceneState::setColor(yRGB.redF(),yRGB.greenF(),yRGB.blueF(),1);
    SceneState::translate(-xs,-ys,0);
    m_cubeMeshModel->draw();
    SceneState::translate(0,ys*2,0);
    m_cubeMeshModel->draw();

    pickName(static_cast<int>(OpenGlIds::ScaleHandleZ));
    SceneState::setColor(zRGB.redF(),zRGB.greenF(),zRGB.blueF(),1);
    SceneState::translate(0,-ys,-zs);
    m_cubeMeshModel->draw();
    SceneState::translate(0,0,zs*2);
    m_cubeMeshModel->draw();
  }
  else if (modifier & Qt::ControlModifier)
  {
    // now draw the rotate spheres with proper depth sorting
    glEnable(GL_DEPTH_TEST);

    pickName(static_cast<int>(OpenGlIds::RotateHandleX));
    SceneState::setColor(xRGB.redF(),xRGB.greenF(),xRGB.blueF(),1);
    SceneState::translate(-xs-5,0,0);
    m_sphereMeshModel->draw();
    SceneState::translate(2*(xs+5),0,0);
    m_sphereMeshModel->draw();

    pickName(static_cast<int>(OpenGlIds::RotateHandleY));
    SceneState::setColor(yRGB.redF(),yRGB.greenF(),yRGB.blueF(),1);
    SceneState::translate(-xs-5,-ys-5,0);
    m_sphereMeshModel->draw();
    SceneState::translate(0,2*(ys+5),0);
    m_sphereMeshModel->draw();

    pickName(static_cast<int>(OpenGlIds::RotateHandleZ));
    SceneState::setColor(zRGB.redF(),zRGB.greenF(),zRGB.blueF(),1);
    SceneState::translate(0,-ys-5,-zs-5);
    m_sphereMeshModel->draw();
    SceneState::translate(0,0,2*(zs+5));
    m_sphereMeshModel->draw();
  }
  else
  {
    // first draw the crosshair lines without depth testing, so they are always visible
    glDisable(GL_DEPTH_TEST);

    SceneState::setLineWidth(xLineWidth);
    drawWithShader(GL_LINES,{QVector3D(-xs-5,0,0),QVector3D(xs+5,0,0)},QVector4D(xRGB.redF(),xRGB.greenF(),xRGB.blueF(),1));

    SceneState::setLineWidth(yLineWidth);
    drawWithShader(GL_LINES,{QVector3D(0,-ys-5,0),QVector3D(0,ys+5,0)},QVector4D(yRGB.redF(),yRGB.greenF(),yRGB.blueF(),1));

    SceneState::setLineWidth(zLineWidth);
    drawWithShader(GL_LINES,{QVector3D(0,0,-zs-5),QVector3D(0,0,zs+5)},QVector4D(zRGB.redF(),zRGB.greenF(),zRGB.blueF(),1));

    // now draw the drag handle arrows with proper depth sorting
    glEnable(GL_DEPTH_TEST);

    pickName(static_cast<int>(OpenGlIds::DragHandleX));
    SceneState::setColor(xRGB.redF(),xRGB.greenF(),xRGB.blueF(),1);
    SceneState::translate(-xs-5,0,0);
    SceneState::rotate(270,0,1,0);
    m_coneMeshModel->draw();
    SceneState::rotate(90,0,1,0);
    SceneState::translate(2*(xs+5),0,0);
    SceneState::rotate(90,0,1,0);
    m_coneMeshModel->draw();
    SceneState::rotate(270,0,1,0);

    pickName(static_cast<int>(OpenGlIds::DragHandleY));
    SceneState::setColor(yRGB.redF(),yRGB.greenF(),yRGB.blueF(),1);
    SceneState::translate(-xs-5,-ys-5,0);
    SceneState::rotate(90,1,0,0);
    m_coneMeshModel->draw();
    SceneState::rotate(270,1,0,0);
    SceneState::translate(0,2*(ys+5),0);
    SceneState::rotate(270,1,0,0);
    m_coneMeshModel->draw();
    SceneState::rotate(90,1,0,0);

    pickName(static_cast<int>(OpenGlIds::DragHandleZ));
    SceneState::setColor(zRGB.redF(),zRGB.greenF(),zRGB.blueF(),1);
    SceneState::translate(0,-ys-5,-zs-5);
    SceneState::rotate(180,1,0,0);
    m_coneMeshModel->draw();
    SceneState::rotate(180,1,0,0);
    SceneState::translate(0,0,2*(zs+5));
    m_coneMeshModel->draw();
  }
  // restore drawing matrix
  SceneState::popMatrix();
}

// a point on a rotation ring; angle in radians
static QVector3D ringPoint(int axis,float radius,float angle)
{
  const float c=radius*cos(angle);
  const float s=radius*sin(angle);
  switch(axis)
  {
    case 0: return QVector3D(0,c,s);
    case 1: return QVector3D(c,0,s);
    default: return QVector3D(c,s,0);
  }
}

// Draws one rotation ring (0 = X red, 1 = Y green, 2 = Z blue) as a round
// band, thick enough to grab with the mouse. In picking mode the ring gets
// its own name, so a click on it can be told from a click on the body.
void AnimationView::drawCircle(int axis,float radius,bool strong)
{
  const bool picking=isPicking();

  const int segments=72;
  const int sides=8;
  // a little fatter while picking, so the ring is easy to hit
  const float tube=picking ? 0.5f : (strong ? 0.32f : 0.18f);
  QVector3D axisDirection(axis==0,axis==1,axis==2);

  const float bright=strong ? 1.0f : 0.8f;
  const QVector4D ringColor(axis==0 ? bright : 0,axis==1 ? bright : 0,axis==2 ? bright : 0,1);
  if(picking) pickName(static_cast<int>(OpenGlIds::RotateRingX)+axis);
  else
  {
    // remember where the ring is on screen, for dragging it
    m_ringMatrix[axis]=SceneState::projection()*SceneState::modelview();
    m_ringVisible[axis]=true;
    SceneState::setColor(ringColor.x(),ringColor.y(),ringColor.z(),ringColor.w());
  }

  // the tube as triangles
  QVector<QVector3D> positions,normals;
  positions.reserve(segments*sides*6);
  normals.reserve(segments*sides*6);
  auto corner=[&](int i,int j)
  {
    const float angle=2*M_PI*i/segments;
    const float around=2*M_PI*j/sides;
    const QVector3D outwards=ringPoint(axis,1.0f,angle);
    const QVector3D normal=outwards*cos(around)+axisDirection*sin(around);
    positions << outwards*radius+normal*tube;
    normals << normal;
  };
  for(int i=0;i<segments;i++)
    for(int j=0;j<sides;j++)
    {
      corner(i,j); corner(i+1,j); corner(i+1,j+1);
      corner(i,j); corner(i+1,j+1); corner(i,j+1);
    }
  drawWithShader(GL_TRIANGLES,positions,normals,QVector<QVector4D>(positions.size(),ringColor));

  // the axis itself, as a thin line (not pickable)
  if(!picking)
  {
    SceneState::setLineWidth(strong ? 3 : 1);
    drawWithShader(GL_LINES,{-axisDirection*radius,axisDirection*radius},ringColor);
  }
}

// where a point of a rotation ring is in the widget, in widget pixels
QPointF AnimationView::ringPointOnScreen(int axis,float angle,float* depth) const
{
  const QVector4D clip=m_ringMatrix[axis]*QVector4D(ringPoint(axis,m_ringRadius,angle),1.0f);
  const float w=(clip.w()!=0.0f) ? clip.w() : 1.0f;
  if(depth) *depth=clip.z()/w;
  return QPointF((clip.x()/w*0.5f+0.5f)*width(),(0.5f-clip.y()/w*0.5f)*height());
}

// A ring was clicked: find the spot that was grabbed and which way on the
// screen it moves when the joint turns. Dragging along that direction then
// turns the joint, like pushing the rim of a wheel.
void AnimationView::startRingDrag(int axis,const QPoint& mouse)
{
  m_ringDragging=axis;
  m_ringLastPos=mouse;

  const int samples=360;
  // nearest ring point under the mouse; of near and far side take the near one
  float bestDistance=1e9f;
  for(int i=0;i<samples;i++)
  {
    const QPointF d=ringPointOnScreen(axis,2*M_PI*i/samples)-QPointF(mouse);
    bestDistance=qMin(bestDistance,(float) std::hypot(d.x(),d.y()));
  }
  float grabbed=0;
  float bestDepth=1e9f;
  for(int i=0;i<samples;i++)
  {
    float depth;
    const QPointF d=ringPointOnScreen(axis,2*M_PI*i/samples,&depth)-QPointF(mouse);
    if(std::hypot(d.x(),d.y())<=bestDistance+6.0f && depth<bestDepth)
    {
      bestDepth=depth;
      grabbed=2*M_PI*i/samples;
    }
  }

  // a positive turn moves ring points towards larger angles (X, Z) or smaller ones (Y)
  const float degree=M_PI/180.0f*(axis==1 ? -1.0f : 1.0f);
  QPointF tangent=ringPointOnScreen(axis,grabbed+degree)-ringPointOnScreen(axis,grabbed);
  // seen edge-on the grabbed spot hardly moves on screen: keep it usable
  const float length=std::hypot(tangent.x(),tangent.y());
  const float minimum=0.5f;
  if(length<0.001f) tangent=QPointF(minimum,0);
  else if(length<minimum) tangent*=minimum/length;
  m_ringTangent=tangent;
}

PropManager* AnimationView::propManager() const
{
  return m_scene->propManager();
}

Camera* AnimationView::camera() const
{
  return m_scene->camera();
}

BVHNode* AnimationView::getSelectedPart()
{
  return m_scene->getAnimation()->getNode(partSelected % static_cast<int>(OpenGlIds::AnimationIncrement));
}

unsigned int AnimationView::getSelectedPartIndex()
{
  return partSelected % static_cast<int>(OpenGlIds::AnimationIncrement);
}

void AnimationView::setMeshModels(QSharedPointer<Mesh> cubeMesh, QSharedPointer<Mesh> sphereMesh, QSharedPointer<Mesh> coneMesh)
{
  m_cubeMeshModel.reset(new MeshModel(cubeMesh));
  m_sphereMeshModel.reset(new MeshModel(sphereMesh));
  m_coneMeshModel.reset(new MeshModel(coneMesh));

  // Scale cube to an edge length of 2 instead of 1
  m_cubeMeshModel->scale(2.0f, 2.0f, 2.0f);
  // Scale cone to a height of 3 instead of 2
  m_coneMeshModel->scale(1.0f, 1.0f, 1.5f);
}
void AnimationView::selectPart(int partNum)
{
  BVHNode* node=m_scene->getAnimation()->getNode(partNum);

  if(!node)
  {
    qCWarning(lcView,"selectPart(%d): node==0!",partNum);
    return;
  }

  if(node->type==BVH_END)
  {
    partSelected=0;
    mirrorSelected=0;
    propManager()->selectProp(0);
    propDragging=0;
    emit backgroundClicked();
    update();
  }
  else selectPart(node);
}

void AnimationView::selectPart(BVHNode* node)
{
  if(!node)
  {
    qCWarning(lcView,"selectPart(node): node==0!");
    return;
  }

  // make sure no prop is selected anymore
  propManager()->selectProp(0);
  propDragging=0;

  // find out the index count of the animation we're working with currently
  int animationIndex=m_scene->getIndexOfAnimation(m_scene->getAnimation());

  // get the part index to be selected, including the proper animation increment
  // FIXME: when we are adding support for removing animations we need to remember
  //        the increment for each animation so they don't get confused
  partSelected = m_scene->getAnimation()->getPartIndex(node) + static_cast<int>(OpenGlIds::AnimationIncrement) * animationIndex;
  emit partClicked(node,
                   QVector3D(m_scene->getAnimation()->getRotation(node)),
                   m_scene->getAnimation()->getRotationLimits(node),
                   QVector3D(m_scene->getAnimation()->getPosition())
                  );
  update();
}

void AnimationView::selectProp()
{
  // make sure no part is selected anymore
  partSelected=0;
  mirrorSelected=0;
  update();
}

// F key, number pad . or double-click: move the camera close to the selected
// part (closer for face bones); the camera then turns around it
void AnimationView::zoomToSelectedPart()
{
  if(!partSelected) return;
  const QString name=selectionName(partSelected);
  float distance=60.0f;
  if(name=="head" || isFaceBone(name)) distance=18.0f;
  else if(name.startsWith("mHand") || name=="lHand" || name=="rHand") distance=20.0f;
  focusOnJoint(partSelected,distance);
}

bool AnimationView::focusOnJoint(unsigned int selected,float distance)
{
  // positions from the last time the figure was drawn
  if(!m_jointPositions.contains(selected)) return false;
  const QVector3D position=m_jointPositions.value(selected);
  if(distance<0) distance=camera()->distance();
  camera()->lookAt(position.x(),position.y(),position.z(),distance);
  update();
  return true;
}

void AnimationView::resetCamera()
{
  camera()->reset();
  update();
}

