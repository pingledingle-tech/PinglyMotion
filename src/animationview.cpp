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
#include <QOpenGLContext>
#include <limits>
#include <vector>
#include <QOpenGLFunctions>
#include <QOpenGLTextureBlitter>
#include <QTimer>
#include <QMessageBox>
#include <climits>
#include <QKeyEvent>
#include <QLabel>
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
#include "jointrings.h"
#include "viewprojection.h"
#include "timelinerows.h"
#include "skinrenderer.h"

#include "usersettings.h"
#include "scenestate.h"
#include "textsize.h"
#include "theme.h"
#include "screenshotsheet.h"
#include "scriptlab/cameraoverride.h"
#include "scriptlab/environmentoverride.h"
#include "scriptlab/viewmodesoverride.h"
#include "scriptlab/effectspreview.h"
#include "viewcolours.h"
#include "viewtoolspec.h"

AnimationView::AnimationView(QWidget* parent, const char* /* name */)
 : QOpenGLWidget(parent),
   m_cubeMeshModel(nullptr),
   m_sphereMeshModel(nullptr),
   m_coneMeshModel(nullptr)
{
  figureFiles << Constants::maleBvhPath() << Constants::femaleBvhPath();

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
  // we do not support (e.g. the SL example bvh files); the animations use the
  // scene's reader (Scene::bvh), this one only reads the reference skeletons
  BVH reader;
  for(int i=0;i<Animation::NUM_FIGURES;i++)
  {
    QString model(figureFiles[i]);
    qCDebug(lcView,"Reading reference model '%s'",model.toLatin1().constData());
    joints[i]=reader.animRead(model,limFile);
  }

  const AvatarSkin::Result skinResult=m_avatarSkin.load(Constants::pinglyMotionDataPath());
  if(!skinResult.ok())
    qWarning("AnimationView::AnimationView(): no avatar body: %s",skinResult.error.toLatin1().constData());

  // the reference skeleton decides how many tracks the timeline has
  Constants::setNumParts(BVH::countNodes(joints[0])+1);
  m_partNames=partNames();
  JointGroups::setTracks(m_partNames);

  modifier=Qt::NoModifier;
  setMouseTracking(true);
  setFocusPolicy(Qt::StrongFocus);

  // the label next to the pointer while a joint is pulled (the mockup's dragtip)
  m_pullLabel=new QLabel(this);
  m_pullLabel->setObjectName(QStringLiteral("pullLabel"));
  m_pullLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
  m_pullLabel->setTextFormat(Qt::RichText);
  m_pullLabel->hide();
  // the letters on the rotation rings
  for(int axis=0;axis<3;axis++)
  {
    m_ringLabel[axis]=new QLabel(QString(QChar('X'+axis)),this);
    m_ringLabel[axis]->setObjectName(QStringLiteral("ringLabel%1").arg(QChar('X'+axis)));
    m_ringLabel[axis]->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_ringLabel[axis]->setAlignment(Qt::AlignCenter);
    m_ringLabel[axis]->hide();
  }
}

AnimationView::~AnimationView()
{
  // the body's buffers live in this view's context
  Mesh::setDrawHook(nullptr);
  Floor::setDrawHook(nullptr);
  makeCurrent();
  m_skinRenderer.cleanup();
  m_pickBuffer.reset();
  if(m_sphereBlitter) m_sphereBlitter->destroy();
  m_sphereBlitter.reset();
  if(m_sphereTexture && QOpenGLContext::currentContext())
    QOpenGLContext::currentContext()->functions()->glDeleteTextures(1,&m_sphereTexture);
  m_sphereTexture=0;
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

int AnimationView::firstObjectId()
{
  return static_cast<int>(OpenGlIds::ObjectStart);
}

void AnimationView::setScene(Scene* scene)
{
  m_scene = scene;
}

void AnimationView::setEffectsPreview(EffectsPreview* preview)
{
  if(m_effects) disconnect(m_effects,nullptr,this,nullptr);
  m_effects=preview;
  if(preview) connect(preview,&EffectsPreview::changed,this,[this]() { update(); });
  update();
}

void AnimationView::setCameraOverride(CameraOverride* cameraOverride)
{
  if(m_cameraOverride) disconnect(m_cameraOverride,nullptr,this,nullptr);
  m_cameraOverride=cameraOverride;
  if(cameraOverride) connect(cameraOverride,&CameraOverride::changed,this,&AnimationView::applyCameraOverride);
  applyCameraOverride();
}

void AnimationView::setEnvironmentOverride(EnvironmentOverride* environmentOverride)
{
  if(m_environmentOverride) disconnect(m_environmentOverride,nullptr,this,nullptr);
  m_environmentOverride=environmentOverride;
  if(environmentOverride) connect(environmentOverride,&EnvironmentOverride::changed,this,[this]() { update(); });
  update();
}

void AnimationView::setViewModesOverride(ViewModesOverride* viewModesOverride)
{
  if(m_viewModesOverride) disconnect(m_viewModesOverride,nullptr,this,nullptr);
  m_viewModesOverride=viewModesOverride;
  if(viewModesOverride) connect(viewModesOverride,&ViewModesOverride::changed,this,[this]() { update(); });
  update();
}

void AnimationView::drawOverlay()
{
  if(!m_viewModesOverride || !m_viewModesOverride->overlay() || isPicking() || !m_skinRenderer.isReady()) return;
  // two triangles over the whole view in clip space, no depth, no fog, blended over the picture
  const QVector<QVector3D> quad{QVector3D(-1,-1,0),QVector3D(1,-1,0),QVector3D(1,1,0),
                                QVector3D(-1,-1,0),QVector3D(1,1,0),QVector3D(-1,1,0)};
  const QVector4D colour=m_viewModesOverride->modes().overlayColour();
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  m_skinRenderer.drawGeometry(GL_TRIANGLES,quad,QVector<QVector3D>(),QVector<QVector4D>(quad.size(),colour),
                              QMatrix4x4(),QMatrix4x4(),SceneState::Fog(),1.0f);
  glEnable(GL_DEPTH_TEST);
}

void AnimationView::drawSphere()
{
  if(!m_environmentOverride || !m_environmentOverride->sphereActive() || isPicking() || !m_skinRenderer.isReady()) return;
  QOpenGLContext* context=QOpenGLContext::currentContext();
  if(!context) return;
  QOpenGLFunctions* f=context->functions();
  GLint viewport[4]={0,0,0,0};
  f->glGetIntegerv(GL_VIEWPORT,viewport);
  const int w=viewport[2],h=viewport[3];
  if(w<=0 || h<=0) return;

  // the picture and its depth, rows from the bottom up as OpenGL keeps them
  QImage image(w,h,QImage::Format_RGBA8888);
  std::vector<float> depth(size_t(w)*size_t(h));
  f->glPixelStorei(GL_PACK_ALIGNMENT,1);
  f->glReadPixels(viewport[0],viewport[1],w,h,GL_RGBA,GL_UNSIGNED_BYTE,image.bits());
  f->glReadPixels(viewport[0],viewport[1],w,h,GL_DEPTH_COMPONENT,GL_FLOAT,depth.data());

  // each pixel's distance from the sphere's centre, in metres (world units are inches)
  const EnvironmentOverride::Sphere sphere=m_environmentOverride->sphere();
  const QMatrix4x4 toWorld=(SceneState::projection()*m_viewMatrix).inverted();
  const QVector3D origin=sphere.origin==EnvironmentOverride::Sphere::Camera ? m_viewMatrix.inverted().map(QVector3D(0,0,0))
                                                                           : m_sphereOrigin;
  std::vector<float> metres(depth.size());
  for(int y=0;y<h;y++)
  {
    const float ny=2.0f*(float(y)+0.5f)/float(h)-1.0f;
    for(int x=0;x<w;x++)
    {
      const size_t i=size_t(y)*size_t(w)+size_t(x);
      if(depth[i]>=1.0f)
      {
        // nothing drawn here: as far away as the sky
        metres[i]=std::numeric_limits<float>::infinity();
        continue;
      }
      const QVector4D p=toWorld.map(QVector4D(2.0f*(float(x)+0.5f)/float(w)-1.0f,ny,2.0f*depth[i]-1.0f,1.0f));
      metres[i]=p.w()!=0.0f ? (p.toVector3DAffine()-origin).length()/EnvironmentOverride::InchesPerMetre
                            : std::numeric_limits<float>::infinity();
    }
  }
  m_environmentOverride->processPicture(image,metres);

  // and back over the whole view, as it is: no depth test, no blending
  if(!m_sphereBlitter)
  {
    m_sphereBlitter=std::make_unique<QOpenGLTextureBlitter>();
    if(!m_sphereBlitter->create())
    {
      qWarning("3D view: the vision sphere cannot be drawn back");
      return;
    }
  }
  if(!m_sphereTexture) f->glGenTextures(1,&m_sphereTexture);
  f->glBindTexture(GL_TEXTURE_2D,m_sphereTexture);
  f->glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
  f->glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
  f->glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
  f->glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
  f->glPixelStorei(GL_UNPACK_ALIGNMENT,1);
  f->glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,image.constBits());
  f->glBindTexture(GL_TEXTURE_2D,0);

  const GLboolean depthWasOn=f->glIsEnabled(GL_DEPTH_TEST);
  const GLboolean blendWasOn=f->glIsEnabled(GL_BLEND);
  f->glDisable(GL_DEPTH_TEST);
  f->glDisable(GL_BLEND);
  m_sphereBlitter->bind();
  m_sphereBlitter->blit(m_sphereTexture,QOpenGLTextureBlitter::targetTransform(QRectF(0,0,w,h),QRect(0,0,w,h)),
                        QOpenGLTextureBlitter::OriginBottomLeft);
  m_sphereBlitter->release();
  if(depthWasOn) f->glEnable(GL_DEPTH_TEST);
  if(blendWasOn) f->glEnable(GL_BLEND);
}

void AnimationView::applyCameraOverride()
{
  if(!m_scene) return;
  if(m_cameraOverride && m_cameraOverride->active())
    camera()->setConstraint(m_cameraOverride->constraint());
  else
    camera()->clearConstraint();
  update();
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
// beside this one without touching the same file; docs/archive/PANELS_PLAN.md, P5)
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
// view would otherwise shrink (or grow) the figure (docs/archive/PANELS_PLAN.md, P5,
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
  // 60 degrees, or what the Script Lab camera limits set (job 7b)
  const QMatrix4x4 perspective=shiftedPerspective(camera()->fieldOfView(),QRectF(viewRect),center,1.0f,2000.0f);
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
  // only the picture on screen, not one renderImage() draws from another side
  if(!m_renderSize.isValid())
  {
    // the ring letters are widgets over the picture: moved after it is drawn
    QMetaObject::invokeMethod(this,&AnimationView::placeRingLabels,Qt::QueuedConnection);
    emit viewDrawn();
  }
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

  // meshes (props, handles, markers): drawn by the shader with the same light
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
    // Script Lab setcam_textures (job 7c): the checkerboard in one plain colour
    if(m_viewModesOverride && m_viewModesOverride->plainFloor())
    {
      QVector<QVector4D> plain(colors.size());
      for(int i=0;i<colors.size();i++) plain[i]=ViewModesOverride::plainFloorColour(colors.at(i));
      return drawWithShader(GL_TRIANGLES,positions,normals,plain);
    }
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
    m_jointFrames.clear();
    m_ringLabelShown[0]=m_ringLabelShown[1]=m_ringLabelShown[2]=false;
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
  EnvironmentOverride::Scene look;
  look.background=QVector3D(clearValue,clearValue,clearValue);
  look.fogOn=UserSettings::fog();
  look.fogDensity=0.005f;
  look.fogColour=look.background;
  // Script Lab sky and draw distance (job 7c): only while its box is ticked; worked out for every picture,
  // so the view's own look comes back exactly when it is switched off
  if(m_environmentOverride) look=m_environmentOverride->scene(look);
  SceneState::Fog fog;
  fog.on=look.fogOn;
  fog.density=look.fogDensity;
  fog.color=look.fogColour;
  SceneState::setFog(fog);

  glClearColor(look.background.x(),look.background.y(),look.background.z(),0.3f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  SceneState::setLighting(false);
  glEnable(GL_DEPTH_TEST);

  camera()->setModelView();
  {
    // view matrix without any model transformation, to find world positions
    m_viewMatrix=SceneState::modelview();
    if(!m_renderSize.isValid() && !isPicking()) m_lastProjection=SceneState::projection();
  }
  m_shadowCasters.clear();
  drawAnimations();
  // the view toolbar's Show > Floor grid (UX phase 4b); the shadows lie on the floor
  if(m_showFloor)
  {
    m_scene->drawFloor();
    drawShadows();
  }
  drawProps();
  drawPullLine();
  drawSphere();
  drawOverlay();
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
  // Script Lab showself (job 7c): the figure, its shadow and its picking are left out
  if(m_viewModesOverride && m_viewModesOverride->hideFigure()) return;
  // Script Lab effects (issue 293): the preview figure takes the place of the user's animations,
  // which stay as they are; it is never picked, so a click cannot select or change anything in it
  Animation* effect=m_effects ? m_effects->animation() : nullptr;
  // the vision sphere's centre: the hip of the preview or the current animation, as it was drawn
  const auto sphereOrigin=[this](Animation* anim,unsigned int index)
  {
    const unsigned int base=index*static_cast<unsigned int>(OpenGlIds::AnimationIncrement);
    if(m_jointPositions.contains(base+1)) m_sphereOrigin=m_jointPositions.value(base+1);
    else if(m_jointPositions.contains(base+2)) m_sphereOrigin=m_jointPositions.value(base+2);
    else m_sphereOrigin=anim->getPosition();
  };
  if(effect)
  {
    if(isPicking()) return;
    // an index far above the scene's, so no joint of the preview counts as selected
    drawFigure(effect,EffectFigureIndex);
    sphereOrigin(effect,EffectFigureIndex);
    drawEffectMarkers();
    return;
  }

  Animation* current=m_scene->getAnimation();
  for(unsigned int index=0;index< (unsigned int) m_scene->getCountOfAnimations();index++)
  {
    drawFigure(m_scene->getAnimation(index),index);
    if(m_scene->getAnimation(index)==current)
    {
      drawEffectMarkers();
      if(!isPicking()) sphereOrigin(current,index);
    }
  }
}

void AnimationView::drawEffectMarkers()
{
  if(!m_effects || m_effects->markers().isEmpty() || isPicking() || !m_cubeMeshModel || !m_sphereMeshModel) return;
  // colour tokens accent and danger_fill in the current theme (src/viewcolours.h); the shape
  // tells them apart too: a box for an object, a ball for a lock
  const QVector4D objectColour=ViewColours::vector(ViewColours::objectMarker());
  const QVector4D lockColour=ViewColours::vector(ViewColours::lockMarker());
  QHash<QString,int> used;   // markers already drawn at a joint, so the next one sits beside them
  // on top of the body, so a marker inside the body can be seen
  const GLboolean depthWasOn=glIsEnabled(GL_DEPTH_TEST);
  glDisable(GL_DEPTH_TEST);
  SceneState::setLighting(true);
  for(const EffectsPreview::Marker& marker : m_effects->markers())
  {
    const SkinJointState* state=nullptr;
    QString joint;
    for(const QString& name : marker.joints)
    {
      auto found=m_skinJointStates.constFind(name);
      if(found!=m_skinJointStates.constEnd())
      {
        state=&found.value();
        joint=name;
        break;
      }
    }
    if(!state) continue;
    const int slot=used[joint]++;
    SceneState::pushMatrix();
    SceneState::modelview()=state->matrix;
    SceneState::translate(2.6f*slot,0,0);
    if(marker.kind==EffectsPreview::Marker::Lock)
    {
      SceneState::setColor(lockColour.x(),lockColour.y(),lockColour.z(),lockColour.w());
      SceneState::scale(1.2f,1.2f,1.2f);
      m_sphereMeshModel->draw();
    }
    else
    {
      SceneState::setColor(objectColour.x(),objectColour.y(),objectColour.z(),objectColour.w());
      SceneState::scale(2.4f,2.4f,2.4f);
      m_cubeMeshModel->draw();
    }
    SceneState::popMatrix();
  }
  if(depthWasOn) glEnable(GL_DEPTH_TEST);
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
  // 1. the ring of a Bento joint anywhere in the area (they have no body
  //    part of their own; among them the one nearest the mouse)
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

// Mouse and keys: see docs/UX_DECISIONS.md (6c, UX phase 4a) and
// Camera::gestureFor(): the camera habit decides which buttons move the camera.
// A left click selects; in the PinglyMotion habit a left drag on empty space (or
// on the body) turns the camera; rings turn joints.
void AnimationView::mouseMoveEvent(QMouseEvent* event)
{
  // pressed on a joint: once the mouse moves 5 pixels, the drag pulls it (UX phase 4b part 2)
  if(m_mouseDrag==MouseDrag::Joint)
  {
    if((event->pos()-m_pull.pressPos).manhattanLength()<5) return;
    const unsigned int selected=m_pressSelected;
    // pressing an unpicked joint picks it and pulls it in one gesture
    if(selected!=partSelected) clickSelect(selected);
    // the drag choice in the Selected joint panel; Shift when pressing does the other one
    const bool turnOnly=UserSettings::pullLimbOnDrag()==m_pull.shiftAtPress;
    if(!startPull(selected,turnOnly,false))
    {
      // it cannot be pulled now (the reason is in the status line): the rest of the drag does nothing
      m_mouseDrag=MouseDrag::Pull;
      return;
    }
    m_mouseDrag=MouseDrag::Pull;
  }
  if(m_mouseDrag==MouseDrag::Pull)
  {
    if(m_pull.active && !m_pull.viaKeys)
    {
      movePullTo(m_pull.start+QPointF(event->pos()-m_pull.pressPos));
      showPullLabel();
    }
    return;
  }

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
    updateHoverHint();
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
    // in the Maya, Blender and Second Life habits a left drag only picks
    if(m_cameraDrag==CameraDrag::None) return;
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
  const float perPixel=2.0f*std::tan(camera()->fieldOfView()*0.5f*M_PI/180.0f)*camera()->distance()/qMax(1,height());

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
  // one drag at a time; other buttons are ignored until it ends, and so is the mouse while a
  // joint is pulled with the keys
  if(m_mouseDrag!=MouseDrag::None || m_pull.active) return;

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

  const CameraGesture gesture=Camera::gestureFor(m_cameraHabit,m_viewTool,event->button(),keys);
  m_cameraDrag=gesture.drag;

  // a camera-only gesture of the habit (right-drag, middle-drag, Alt + drag ...)
  if(!gesture.picks)
  {
    if(m_cameraDrag==CameraDrag::None) return;
    // Alt + click on a joint: turn around that joint from here (Second Life habit)
    if(event->button()==Qt::LeftButton && Camera::focusesOnAltClick(m_cameraHabit))
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

  // a joint: a click picks it, a drag pulls the limb (UX phase 4b part 2, 6a)
  if(selected && selected<static_cast<unsigned int>(OpenGlIds::ObjectStart) &&
     m_scene->getAnimation(selected/static_cast<int>(OpenGlIds::AnimationIncrement)))
  {
    m_mouseDrag=MouseDrag::Joint;
    m_pressSelected=selected;
    m_pull.pressPos=event->pos();
    m_pull.shiftAtPress=keys & Qt::ShiftModifier;
    return;
  }

  // a prop or empty space: select on release, unless the mouse
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

  if(ended==MouseDrag::Click || ended==MouseDrag::Joint)
    clickSelect(m_pressSelected);
  else if(ended==MouseDrag::Pull)
  {
    // release sets the key; after Esc the pull is already gone
    if(m_pull.active && !m_pull.viaKeys) finishPull();
  }
  else if(ended==MouseDrag::Ring)
  {
    m_ringDragging=-1;
    update();
  }
}

void AnimationView::mouseDoubleClickEvent(QMouseEvent* event)
{
  // a camera gesture of the habit (Alt in Second Life: the press already focused)
  if(!Camera::gestureFor(m_cameraHabit,m_viewTool,event->button(),event->modifiers()).picks)
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
  else if(Camera::focusesOnDoubleClick(m_cameraHabit))
  {
    // double-click: look at the joint and turn around it (PinglyMotion habit)
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

// Tab hides and shows the panels (docs/archive/PANELS_PLAN.md, job P4). Qt's own
// handling in QWidget::event would move the keyboard focus to the next
// field instead, so Tab is taken here first. Only a plain Tab: Shift+Tab
// and the others go on as before. Tab in a panel's field or in the search
// box never comes here, because the 3D view does not have the keys then.
bool AnimationView::event(QEvent* event)
{
  if(event->type()==QEvent::KeyPress)
  {
    QKeyEvent* key=static_cast<QKeyEvent*>(event);
    // while a joint is pulled every key goes to the pull (keyPressEvent())
    if(m_pull.active && (key->key()==Qt::Key_Tab || key->key()==Qt::Key_Backtab))
    {
      keyPressEvent(key);
      return true;
    }
    if(key->key()==Qt::Key_Tab && key->modifiers()==Qt::NoModifier)
    {
      key->accept();
      emit tabPressed();
      return true;
    }
  }
  // while a joint is pulled no menu key may act: the key comes to keyPressEvent(), which says
  // "Finish or cancel the pull first: Esc cancels."
  if(event->type()==QEvent::ShortcutOverride && m_pull.active)
  {
    event->accept();
    return true;
  }
  return QOpenGLWidget::event(event);
}

void AnimationView::keyPressEvent(QKeyEvent* event)
{
  int num=-1;
  float height,angle;

  // pulling a joint (UX phase 4b part 2): Esc cancels; with the keys the arrow keys move it and
  // Enter sets the key; every other key waits until the pull ends (the mockup's keydown handler)
  if(m_pull.active)
  {
    const int key=event->key();
    event->accept();
    if(key==Qt::Key_Escape)
    {
      cancelPull();
      return;
    }
    if(m_pull.viaKeys)
    {
      const int step=(event->modifiers() & Qt::ShiftModifier) ? ViewToolSpec::keyStepSmall : ViewToolSpec::keyStep;
      QPointF move;
      if(key==Qt::Key_Left) move=QPointF(-step,0);
      else if(key==Qt::Key_Right) move=QPointF(step,0);
      else if(key==Qt::Key_Up) move=QPointF(0,-step);
      else if(key==Qt::Key_Down) move=QPointF(0,step);
      if(!move.isNull())
      {
        movePullTo(m_pull.target+move);
        showPullLabel();
        return;
      }
      if(key==Qt::Key_Return || key==Qt::Key_Enter || key==Qt::Key_Space)
      {
        finishPull();
        return;
      }
    }
    if(key!=Qt::Key_Shift && key!=Qt::Key_Control && key!=Qt::Key_Alt && key!=Qt::Key_Meta &&
       (event->modifiers() & (Qt::ControlModifier | Qt::MetaModifier)))
      emit statusMessage(ViewToolSpec::tr(ViewToolSpec::finishFirst));
    return;
  }
  // Enter or G picks the picked joint up (Shift + Enter: the other way than the drag choice)
  if((event->key()==Qt::Key_Return || event->key()==Qt::Key_Enter || event->key()==Qt::Key_G) &&
     !(event->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier)))
  {
    event->accept();
    startKeyPull(event->key()!=Qt::Key_G && (event->modifiers() & Qt::ShiftModifier));
    return;
  }

  // Shift / Ctrl show the scale / rotate handles of a selected prop
  if(modifier!=event->modifiers())
  {
    modifier=event->modifiers();
    update();
  }

  ViewTool tool;
  if(Camera::toolForKey(event->key(),event->modifiers(),&tool))
  {
    // V, O, M, Z: the tool of the left mouse button (the window chooses it, UX phase 4b)
    emit viewToolRequested(tool);
    return;
  }

  float up,around;
  if(Camera::turnForKey(event->key(),event->modifiers(),&up,&around))
  {
    // Alt + arrow keys: look around in steps
    camera()->rotate(up,around);
    update();
    return;
  }
  if(Camera::viewForKey(event->key(),event->modifiers(),&height,&angle))
  {
    // number pad (or 1, 3, 7 in the 3D view): front / side / top
    camera()->setView(height,angle);
    update();
    return;
  }
  if(Camera::isHomeKey(event->key(),event->modifiers()))
  {
    // number pad 0: home view
    resetCamera();
    return;
  }
  if(Camera::isWholeFigureKey(event->key(),event->modifiers()))
  {
    frameWholeFigure();
    return;
  }
  if(Camera::isFocusKey(m_cameraHabit,event->key(),event->modifiers()))
  {
    // F, or number pad . in the Blender habit
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

void AnimationView::leaveEvent(QEvent* event)
{
  setHoverHint(QString());
  QOpenGLWidget::leaveEvent(event);
}

void AnimationView::setViewTool(ViewTool tool)
{
  m_viewTool=tool;
  if(underMouse()) updateHoverHint();
}

void AnimationView::setShowRings(bool on)
{
  m_showRings=on;
  update();
}

void AnimationView::setShowFloor(bool on)
{
  m_showFloor=on;
  update();
}

void AnimationView::updateHoverHint()
{
  if(m_viewTool!=ViewTool::Pick)
  {
    setHoverHint(ViewToolSpec::toolHoverHint(m_viewTool));
    return;
  }
  if(m_pull.active) return;
  const int ring=static_cast<int>(partHighlighted)-static_cast<int>(OpenGlIds::RotateRingX);
  if(ring>=0 && ring<=2 && partSelected && m_showRings)
  {
    setHoverHint(ViewToolSpec::ringHoverHint(ring));
    return;
  }
  // over a joint: a click picks it, a drag pulls it (the mockup's HINT-02)
  const QString name=selectionName(partHighlighted);
  setHoverHint(!name.isEmpty() && name!=QLatin1String("Site") ? ViewToolSpec::tr(ViewToolSpec::jointHover) : QString());
}

void AnimationView::setHoverHint(const QString& hint)
{
  if(hint==m_hoverHint) return;
  m_hoverHint=hint;
  emit hoverHintChanged(hint);
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

  const AvatarSkin::Result result=m_avatarSkin.load(Constants::pinglyMotionDataPath(),wanted);
  const bool ok=result.ok();
  if(!ok)
  {
    qWarning("AnimationView::setBody(): %s",result.error.toLatin1().constData());
    // fall back to the other body, so that something is shown if one file is damaged
    const AvatarSkin::Body other=wanted==AvatarSkin::RuthBody ? AvatarSkin::MaxBody : AvatarSkin::RuthBody;
    const AvatarSkin::Result fallback=m_avatarSkin.load(Constants::pinglyMotionDataPath(),other);
    if(!fallback.ok())
      qWarning("AnimationView::setBody(): no avatar body: %s",fallback.error.toLatin1().constData());
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
    // the bone lines between the joints, always on, over the body
    selectName = index * static_cast<int>(OpenGlIds::AnimationIncrement);
    glDisable(GL_DEPTH_TEST);
    drawPart(anim,index,anim->getFrame(),anim->getMotion(),joints[figType],MODE_SKELETON);
    // the joint rings, with depth test: the body hides the joints behind it
    // (src/jointrings.h); while picking they carry their joint's name
    if(!isPicking()) updateRingChanged(anim);
    selectName = index * static_cast<int>(OpenGlIds::AnimationIncrement);
    glEnable(GL_DEPTH_TEST);
    drawPart(anim,index,anim->getFrame(),anim->getMotion(),joints[figType],MODE_RINGS);
    glDisable(GL_DEPTH_TEST);

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
    // the line from the parent joint to this one (src/jointrings.h): always
    // drawn, not to the hip from the floor, not to joints of hidden groups
    // and to face joints only while the face is in focus
    if(mode==MODE_SKELETON && !selecting &&
       JointRings::hasBone(jointRoot->type==BVH_ROOT,motion->type==BVH_END,JointGroups::shown(motion->name()),
                           isFaceBone(motion->name()),faceFocus(),m_hideMarkers))
    {
      // the bones of a pulled limb and the bone to the selected joint in accent_text
      const bool pulledBone=m_pull.active && (m_pullPreview.contains(motion) || motion==m_pull.pull.pulled);
      const bool selectedBone=(partSelected==selectName);
      const qreal ratio=m_renderSize.isValid() ? 1.0 : devicePixelRatioF();
      SceneState::setLineWidth(float(JointRings::boneWidth(selectedBone,pulledBone)*ratio));
      const bool lighting=SceneState::lighting();
      SceneState::setLighting(false);
      drawWithShader(GL_LINES,{-offset,QVector3D(0,0,0)},
                     ViewColours::vector(JointRings::boneColour(pulledBone || selectedBone,Theme::isDark())));
      SceneState::setLighting(lighting);
      SceneState::setLineWidth(1);
    }
    if(mode==MODE_RINGS &&
       JointRings::hasRing(motion->type==BVH_END,JointGroups::shown(motion->name()),
                           isFaceBone(motion->name()),faceFocus(),m_hideMarkers))
      drawJointRing(anim,motion);

    // where this joint's frame is before its own turn, for pulling a joint (UX phase 4b part 2)
    if(mode==MODE_PARTS && !selecting)
      m_jointFrames.insert(motion,m_viewMatrix.inverted()*SceneState::modelview());
    // a pull shows its turns before it is released
    const auto previewed=m_pullPreview.constFind(motion);
    QVector3D rotation = previewed!=m_pullPreview.constEnd() ? previewed.value() : motion->frameData(frame).rotation();
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
      if(mode==MODE_ROT_AXES && partSelected==selectName && m_showRings)
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
      else if(m_pull.active && m_pullPreview.contains(motion))
      {
        // the pulled limb is highlighted (accent_text, as the mockup's pulled bones)
        const QVector4D pulled=ViewColours::vector(Theme::color(Theme::Token::AccentText));
        SceneState::setColor(pulled.x(),pulled.y(),pulled.z(),1);
      }
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
      // Bento bones have no body mesh: they are clicked by their ring (MODE_RINGS)

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

// which joints of this animation have changed (TimelineRows::jointChanged),
// by track, for the colour of their rings; the hip also counts as changed
// when the Avatar Position moved
void AnimationView::updateRingChanged(Animation* anim)
{
  m_ringChanged.fill(false,m_partNames.count());
  for(int track=0;track<m_partNames.count();track++)
    m_ringChanged[track]=TimelineRows::jointChanged(anim,track);
  if(m_ringChanged.count()>1 && m_ringChanged.at(0)) m_ringChanged[1]=true;
}

// the ring of the joint being walked (selectName), at the origin of the
// current matrix: a flat ring facing the camera, of a fixed size on the screen,
// moved toward the camera by the body's depth there (src/jointrings.h)
void AnimationView::drawJointRing(Animation* anim,const BVHNode* joint)
{
  const unsigned int track=selectName % static_cast<unsigned int>(OpenGlIds::AnimationIncrement);
  JointRings::State state;
  state.selected=(partSelected==selectName);
  state.hovered=(partHighlighted==selectName);
  state.changed=track<static_cast<unsigned int>(m_ringChanged.count()) && m_ringChanged.at(int(track));
  state.area=TimelineRows::areaOf(joint->name());

  const qreal ratio=m_renderSize.isValid() ? 1.0 : devicePixelRatioF();
  const QSize viewport=m_renderSize.isValid() ? m_renderSize
                                              : QSize(qMax(1,int(width()*ratio)),qMax(1,int(height()*ratio)));
  const QMatrix4x4 projection=SceneState::projection();
  const QVector3D eye=SceneState::modelview().map(QVector3D(0,0,0));
  const QVector3D centre=JointRings::lifted(projection,eye,JointRings::lift(joint->name())*anim->getAvatarScale());
  const QVector2D pixel=JointRings::pixelSize(projection,centre,viewport);
  const float outer=float(JointRings::radius*ratio);

  SceneState::pushMatrix();
  SceneState::loadIdentity();
  const bool lighting=SceneState::lighting();
  SceneState::setLighting(false);
  if(isPicking())
  {
    // the whole disc can be clicked, also inside a hollow ring
    pickName(selectName);
    drawWithShader(GL_TRIANGLES,JointRings::ringTriangles(centre,pixel,outer,0.0f),QVector4D());
  }
  else
  {
    const bool dark=Theme::isDark();
    const float inner=outer-float(JointRings::edgeWidth(state)*ratio);
    const QColor fill=JointRings::fillColour(state,dark);
    if(fill.isValid())
      drawWithShader(GL_TRIANGLES,JointRings::ringTriangles(centre,pixel,inner,0.0f),ViewColours::vector(fill));
    drawWithShader(GL_TRIANGLES,JointRings::ringTriangles(centre,pixel,outer,inner),
                   ViewColours::vector(JointRings::edgeColour(state,dark)));
  }
  SceneState::setLighting(lighting);
  SceneState::popMatrix();
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

  QColor xRGB=ViewColours::axis(0);
  QColor yRGB=ViewColours::axis(1);
  QColor zRGB=ViewColours::axis(2);

  int xLineWidth=1;
  int yLineWidth=1;
  int zLineWidth=1;

  switch(partHighlighted)
  {
    case static_cast<int>(OpenGlIds::ScaleHandleX):
    case static_cast<int>(OpenGlIds::RotateHandleX):
    case static_cast<int>(OpenGlIds::DragHandleX):
      xRGB=Theme::mix(xRGB,Theme::color(Theme::Token::Text),0.2);
      xLineWidth=3;
      break;
    case static_cast<int>(OpenGlIds::ScaleHandleY):
    case static_cast<int>(OpenGlIds::RotateHandleY):
    case static_cast<int>(OpenGlIds::DragHandleY):
      yRGB=Theme::mix(yRGB,Theme::color(Theme::Token::Text),0.2);
      yLineWidth=3;
      break;
    case static_cast<int>(OpenGlIds::ScaleHandleZ):
    case static_cast<int>(OpenGlIds::RotateHandleZ):
    case static_cast<int>(OpenGlIds::DragHandleZ):
      zRGB=Theme::mix(zRGB,Theme::color(Theme::Token::Text),0.2);
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

  const QVector4D ringColor=ViewColours::vector(ViewColours::ring(axis,strong));
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
  // the back half of the ring (further away than the joint) is drawn faint, as in the mockup
  // (opacity 0.35, or 0.6 while hovered or dragged): mixed with the background
  QVector<QVector4D> colours;
  colours.reserve(segments*sides*6);
  const QVector4D backColour=picking ? ringColor :
    ViewColours::vector(Theme::mix(ViewColours::ring(axis,strong),Theme::color(Theme::Token::Background),strong ? 0.4 : 0.65));
  float centreDepth=0;
  if(!picking)
  {
    const QVector4D centreClip=m_ringMatrix[axis]*QVector4D(0,0,0,1);
    centreDepth=centreClip.w()!=0.0f ? centreClip.z()/centreClip.w() : centreClip.z();
  }
  for(int i=0;i<segments;i++)
  {
    bool back=false;
    if(!picking)
    {
      float depth=0;
      ringPointOnScreen(axis,2*M_PI*(i+0.5f)/segments,&depth);
      back=depth>centreDepth;
    }
    for(int j=0;j<sides;j++)
    {
      corner(i,j); corner(i+1,j); corner(i+1,j+1);
      corner(i,j); corner(i+1,j+1); corner(i,j+1);
    }
    for(int k=0;k<sides*6;k++) colours << (back ? backColour : ringColor);
  }
  drawWithShader(GL_TRIANGLES,positions,normals,colours);

  // where its letter goes: the ring point furthest along the mockup's direction for this
  // axis on the screen, front half first (renderRings(): DIRS and the bonus of 40)
  if(!picking)
  {
    static const float dirs[3][2]={{1.0f,-0.35f},{-1.0f,-0.35f},{0.25f,1.0f}};
    const QPointF centre=ViewProjection::toWidget(m_ringMatrix[axis]*QVector4D(0,0,0,1),width(),height());
    float bestValue=-1e9f;
    for(int i=0;i<=48;i++)
    {
      float depth=0;
      const QPointF point=ringPointOnScreen(axis,2*M_PI*i/48,&depth);
      const float value=(point.x()-centre.x())*dirs[axis][0]+(point.y()-centre.y())*dirs[axis][1]+(depth<=centreDepth ? 40.0f : 0.0f);
      if(value>bestValue)
      {
        bestValue=value;
        m_ringLabelPos[axis]=point;
      }
    }
    m_ringLabelShown[axis]=true;
  }

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
  return ViewProjection::toWidget(m_ringMatrix[axis]*QVector4D(ringPoint(axis,m_ringRadius,angle),1.0f),width(),height(),depth);
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
bool AnimationView::focusOnPickedJoint()
{
  if(!partSelected || partSelected>=static_cast<unsigned int>(OpenGlIds::ObjectStart) ||
     !m_jointPositions.contains(partSelected)) return false;
  zoomToSelectedPart();
  return true;
}

void AnimationView::zoomToSelectedPart()
{
  if(!partSelected) return;
  focusOnJoint(partSelected,Camera::focusDistance(selectionName(partSelected)));
}

bool AnimationView::drawnJointPosition(const QString& joint,QVector3D* position)
{
  // a tiny picture off screen fills m_jointPositions for the pose as it is now; the camera
  // and the picture on screen do not change
  renderImage(64,48);
  for(auto it=m_jointPositions.constBegin();it!=m_jointPositions.constEnd();++it)
  {
    if(it.key()>=static_cast<unsigned int>(OpenGlIds::AnimationIncrement)) continue;
    if(selectionName(it.key())!=joint) continue;
    if(position) *position=it.value();
    return true;
  }
  return false;
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

bool AnimationView::frameWholeFigure()
{
  bool any=false;
  QVector3D low,high;
  for(auto it=m_jointPositions.constBegin();it!=m_jointPositions.constEnd();++it)
  {
    if(it.key()>=static_cast<unsigned int>(OpenGlIds::ObjectStart)) continue;
    const QVector3D& p=it.value();
    if(!any) { low=high=p; any=true; continue; }
    low=QVector3D(qMin(low.x(),p.x()),qMin(low.y(),p.y()),qMin(low.z(),p.z()));
    high=QVector3D(qMax(high.x(),p.x()),qMax(high.y(),p.y()),qMax(high.z(),p.z()));
  }
  if(!any) return false;
  camera()->frameBox(low,high,height()>0 ? float(width())/float(height()) : 1.0f);
  update();
  return true;
}


// ---------- dragging a joint: pull the limb, or turn only that joint (UX phase 4b part 2) ----------

QPointF AnimationView::toScreen(const QVector3D& world,float* depth) const
{
  return ViewProjection::toWidget(m_lastProjection*m_viewMatrix*QVector4D(world,1.0f),width(),height(),depth);
}

QVector3D AnimationView::fromScreen(const QPointF& point,float depth) const
{
  const QVector4D ndc(float(point.x()/qMax(1,width())*2.0-1.0),float(1.0-point.y()/qMax(1,height())*2.0),depth,1.0f);
  const QVector4D world=(m_lastProjection*m_viewMatrix).inverted()*ndc;
  return world.w()!=0.0f ? world.toVector3D()/world.w() : world.toVector3D();
}

QString AnimationView::jointName(const BVHNode* node) const
{
  if(!node) return QString();
  const QString name=m_jointNamer ? m_jointNamer(node->name()) : QString();
  return name.isEmpty() ? node->name() : name;
}

QStringList AnimationView::pullLabelLines() const
{
  if(!m_pull.active) return QStringList();
  return ViewToolSpec::pullLines(jointName(m_pull.pull.pulled),m_pull.pull.chain.turnOnly,m_pull.pull.held,m_pull.viaKeys);
}

void AnimationView::startKeyPull(bool otherWay)
{
  if(m_pull.active || m_mouseDrag!=MouseDrag::None) return;
  const QString why=m_pullGuard ? m_pullGuard() : QString();
  if(!why.isEmpty())
  {
    emit statusMessage(ViewToolSpec::tr(ViewToolSpec::keysBlocked).arg(why));
    return;
  }
  if(!partSelected || partSelected>=static_cast<unsigned int>(OpenGlIds::ObjectStart))
  {
    emit statusMessage(ViewToolSpec::tr(ViewToolSpec::keysNoJoint));
    return;
  }
  setFocus();
  startPull(partSelected,UserSettings::pullLimbOnDrag()==otherWay,true);
}

bool AnimationView::startPull(unsigned int selected,bool turnOnly,bool viaKeys)
{
  const QString why=m_pullGuard ? m_pullGuard() : QString();
  if(!why.isEmpty())
  {
    emit statusMessage(ViewToolSpec::tr(viaKeys ? ViewToolSpec::keysBlocked : ViewToolSpec::pullBlocked).arg(why));
    return false;
  }
  Animation* anim=m_scene->getAnimation(selected/static_cast<int>(OpenGlIds::AnimationIncrement));
  if(!anim) return false;
  BVHNode* node=anim->getNode(selected%static_cast<int>(OpenGlIds::AnimationIncrement));
  const LimbPull::FrameOf frameOf=[this](BVHNode* joint,QMatrix4x4& frame)
  {
    const auto found=m_jointFrames.constFind(joint);
    if(found==m_jointFrames.constEnd()) return false;
    frame=found.value();
    return true;
  };
  const LimbPull::TurnOf turnOf=[anim](BVHNode* joint) { return anim->getRotation(joint); };
  const LimbPull::LimitsOf limitsOf=[anim](BVHNode* joint,QVector3D& minimum,QVector3D& maximum)
  {
    const RotationLimits limits=anim->getRotationLimits(joint);
    minimum=limits.minimum;
    maximum=limits.maximum;
  };
  if(!LimbPull::setUp(m_pull.pull,anim->getMotion(),node,turnOnly,frameOf,turnOf,limitsOf)) return false;

  m_pull.active=true;
  m_pull.viaKeys=viaKeys;
  m_pull.selectName=selected;
  m_pull.start=toScreen(LimbPull::tipPosition(m_pull.pull.chain),&m_pull.depth);
  m_pull.target=m_pull.start;
  m_pullPreview.clear();
  for(const auto& turn : LimbPull::drivenTurns(m_pull.pull)) m_pullPreview.insert(turn.first,turn.second);

  QToolTip::hideText();
  setCursor(QCursor(Qt::ClosedHandCursor));
  showPullLabel();
  update();
  return true;
}

void AnimationView::movePullTo(const QPointF& target)
{
  if(!m_pull.active) return;
  m_pull.target=QPointF(qBound(0.0,target.x(),double(width())),qBound(0.0,target.y(),double(height())));
  LimbPull::solve(m_pull.pull.chain,fromScreen(m_pull.target,m_pull.depth),true);
  for(const auto& turn : LimbPull::drivenTurns(m_pull.pull)) m_pullPreview.insert(turn.first,turn.second);
  update();
}

void AnimationView::finishPull()
{
  if(!m_pull.active) return;
  const LimbPull::Pull pull=m_pull.pull;
  endPull();
  // the window sets the keys; the picture keeps the pull's turns until then
  emit pullFinished(pull.pulled,LimbPull::drivenTurns(pull),pull.chain.turnOnly);
  m_pullPreview.clear();
  update();
}

void AnimationView::cancelPull()
{
  if(!m_pull.active) return;
  endPull();
  m_pullPreview.clear();
  update();
  emit pullCancelled();
}

void AnimationView::endPull()
{
  m_pull.active=false;
  m_pull.viaKeys=false;
  m_pullLabel->hide();
  setCursor(Qt::ArrowCursor);
  setHoverHint(QString());
}

void AnimationView::showPullLabel()
{
  if(!m_pull.active)
  {
    m_pullLabel->hide();
    return;
  }
  const QStringList lines=pullLabelLines();
  // the hint line says the same as the label
  setHoverHint(lines.join(QStringLiteral(" · ")));
  QString html=QStringLiteral("<b>%1</b>").arg(lines.value(0).toHtmlEscaped());
  for(int i=1;i<lines.size();i++) html+=QStringLiteral("<br>%1").arg(lines.at(i).toHtmlEscaped());
  m_pullLabel->setText(html);
  m_pullLabel->setStyleSheet(QStringLiteral("QLabel { background: %1; color: %2; border: 1px solid %3; border-radius: %4px; padding: %5px %6px; }")
                             .arg(Theme::color(Theme::Token::Surface).name(),Theme::color(Theme::Token::Text).name(),
                                  Theme::color(Theme::Token::Border).name())
                             .arg(Theme::Size::radiusControl).arg(Theme::Size::spacingXs).arg(Theme::Size::spacingS));
  m_pullLabel->adjustSize();
  // next to the pointer (mouse) or next to where the joint goes (keys), always inside the view
  const QPointF anchor=m_pull.viaKeys ? m_pull.target+QPointF(24,24) : QPointF(mapFromGlobal(QCursor::pos()))+QPointF(18,18);
  const int x=qBound(8,int(anchor.x()),qMax(8,width()-m_pullLabel->width()-8));
  const int y=qBound(8,int(anchor.y()),qMax(8,height()-m_pullLabel->height()-8));
  m_pullLabel->move(x,y);
  m_pullLabel->show();
  m_pullLabel->raise();
}

void AnimationView::drawPullLine()
{
  if(!m_pull.active || isPicking() || !m_skinRenderer.isReady()) return;
  // in clip space: a dashed line (6 on, 4 off) from where the joint was to where it goes, and a
  // ring of 12 pixels with a dot of 3 at the end, in accent_text (the mockup's pullline, pulltarget)
  auto clip=[this](const QPointF& p) { return QVector3D(float(p.x()/qMax(1,width())*2.0-1.0),float(1.0-p.y()/qMax(1,height())*2.0),0.0f); };
  const QVector4D colour=ViewColours::vector(Theme::color(Theme::Token::AccentText));
  const qreal scale=TextSize::scaled(100)/100.0;
  QVector<QVector3D> lines;
  const QPointF from=m_pull.start, to=m_pull.target;
  const qreal length=std::hypot(to.x()-from.x(),to.y()-from.y());
  const qreal on=6*scale, off=4*scale;
  for(qreal at=0;at<length;at+=on+off)
  {
    const qreal end=qMin(length,at+on);
    lines << clip(from+(to-from)*(at/length)) << clip(from+(to-from)*(end/length));
  }
  const int segments=32;
  for(int i=0;i<segments;i++)
  {
    const double a=2*M_PI*i/segments, b=2*M_PI*(i+1)/segments;
    lines << clip(to+QPointF(std::cos(a),std::sin(a))*12*scale) << clip(to+QPointF(std::cos(b),std::sin(b))*12*scale);
  }
  QVector<QVector3D> dot;
  for(int i=0;i<segments;i++)
  {
    const double a=2*M_PI*i/segments, b=2*M_PI*(i+1)/segments;
    dot << clip(to) << clip(to+QPointF(std::cos(a),std::sin(a))*3*scale) << clip(to+QPointF(std::cos(b),std::sin(b))*3*scale);
  }
  glDisable(GL_DEPTH_TEST);
  m_skinRenderer.drawGeometry(GL_LINES,lines,QVector<QVector3D>(),QVector<QVector4D>(lines.size(),colour),
                              QMatrix4x4(),QMatrix4x4(),SceneState::Fog(),float(2*devicePixelRatioF()));
  m_skinRenderer.drawGeometry(GL_TRIANGLES,dot,QVector<QVector3D>(),QVector<QVector4D>(dot.size(),colour),
                              QMatrix4x4(),QMatrix4x4(),SceneState::Fog(),1.0f);
  glEnable(GL_DEPTH_TEST);
}

void AnimationView::placeRingLabels()
{
  const int side=TextSize::scaled(16);
  for(int axis=0;axis<3;axis++)
  {
    QLabel* label=m_ringLabel[axis];
    const bool shown=m_ringLabelShown[axis] && m_showRings && partSelected;
    if(!shown)
    {
      if(label->isVisible()) label->hide();
      continue;
    }
    const QString style=QStringLiteral("QLabel { background: %1; color: %2; border-radius: %3px; font-weight: 700; }")
                        .arg(ViewColours::axis(axis).name(),Theme::color(Theme::Token::OnAxis).name()).arg(TextSize::scaled(Theme::Size::radiusSmall));
    if(label->styleSheet()!=style) label->setStyleSheet(style);
    const QRect place(int(m_ringLabelPos[axis].x())-side/2,int(m_ringLabelPos[axis].y())-side/2,side,side);
    if(label->geometry()!=place) label->setGeometry(place);
    if(!label->isVisible()) label->show();
  }
}
