/*
 * PinglyMotion - test of the lines of the 3D view (SkinRenderer::drawGeometry
 * with GL_LINES): bones, rings, axes and grid, drawn offscreen and compared
 * with a reference picture made with the old geometry shader; a bone drawn in
 * its pick colour gives its pick name back; every shader builds as GLSL 3.30
 * (desktop) and as GLSL ES 3.00 (WebGL 2, the later web version).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "skinrenderer.h"

#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtMath>
#include <QtTest>

namespace
{
const int WIDTH = 320;
const int HEIGHT = 240;

// On a CI machine the saved picture cannot be opened, so a failing test
// writes it into the log as base64 PNG (decode with base64 -d).
void dumpPicture(const char* name, const QImage& image)
{
  if (!qEnvironmentVariableIsSet("CI")) return;
  QByteArray png;
  QBuffer buffer(&png);
  buffer.open(QIODevice::WriteOnly);
  image.save(&buffer, "PNG");
  const QByteArray text = png.toBase64();
  qInfo("PICTURE %s BEGIN", name);
  for (int i = 0; i < text.size(); i += 4000) qInfo("%s", text.mid(i, 4000).constData());
  qInfo("PICTURE %s END", name);
}
}

class TestLineDraw : public QObject
{
  Q_OBJECT

  private slots:
    void initTestCase();
    void cleanupTestCase();
    void linesLookAsBefore();
    void pickColourLineGivesName();
    void lineBehindCameraDrawsNothing();
    void shadersBuildForDesktopAndWeb();
    void diagnose();

  private:
    bool m_haveGl = false;
    QString m_glReason;
    QOffscreenSurface m_surface;
    QOpenGLContext m_context;
    std::unique_ptr<QOpenGLFramebufferObject> m_fbo;
    std::unique_ptr<SkinRenderer> m_renderer;

    void begin(const QVector4D& background);
    QImage end();
    QMatrix4x4 projection() const;
};

void TestLineDraw::initTestCase()
{
  // the same format as the program asks for (src/main.cpp)
  QSurfaceFormat format;
  format.setVersion(3, 3);
  format.setProfile(QSurfaceFormat::CoreProfile);
  format.setDepthBufferSize(24);
  format.setStencilBufferSize(8);
  m_surface.setFormat(format);
  m_surface.create();
  m_context.setFormat(format);
  if (!m_surface.isValid() || !m_context.create() || !m_context.makeCurrent(&m_surface))
  {
    m_glReason = "no OpenGL context on this platform";
    return;
  }
  m_renderer = std::make_unique<SkinRenderer>();
  if (!m_renderer->init())
  {
    m_glReason = "no OpenGL 3.3 shaders: " + m_renderer->status();
    m_renderer.reset();
    return;
  }
  QOpenGLFramebufferObjectFormat fboFormat;
  fboFormat.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
  m_fbo = std::make_unique<QOpenGLFramebufferObject>(WIDTH, HEIGHT, fboFormat);
  if (!m_fbo->isValid())
  {
    m_glReason = "no framebuffer object";
    return;
  }
  m_haveGl = true;
  qInfo("OpenGL: %s, %s", reinterpret_cast<const char*>(m_context.functions()->glGetString(GL_RENDERER)),
        qPrintable(m_renderer->status()));
}

void TestLineDraw::cleanupTestCase()
{
  if (m_context.isValid() && m_context.makeCurrent(&m_surface))
  {
    if (m_renderer) m_renderer->cleanup();
    m_fbo.reset();
    m_context.doneCurrent();
  }
}

QMatrix4x4 TestLineDraw::projection() const
{
  QMatrix4x4 p;
  p.perspective(45.0f, float(WIDTH) / HEIGHT, 0.1f, 100.0f);
  return p;
}

void TestLineDraw::begin(const QVector4D& background)
{
  m_context.makeCurrent(&m_surface);
  m_fbo->bind();
  QOpenGLFunctions* gl = m_context.functions();
  gl->glViewport(0, 0, WIDTH, HEIGHT);
  gl->glClearColor(background.x(), background.y(), background.z(), background.w());
  gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  gl->glEnable(GL_DEPTH_TEST);
  gl->glDepthFunc(GL_LEQUAL);
  gl->glDisable(GL_CULL_FACE);
}

QImage TestLineDraw::end()
{
  m_context.functions()->glFinish();
  QImage image = m_fbo->toImage().convertToFormat(QImage::Format_RGBA8888);
  m_fbo->release();
  return image;
}

// A scene with the kinds of lines the 3D view draws: a fogged floor grid,
// bones of several widths, a ring, the three axes, lit lines with normals,
// a half-transparent line and a line of zero length.
void TestLineDraw::linesLookAsBefore()
{
  if (!m_haveGl) QSKIP(qPrintable(m_glReason));
  begin(QVector4D(0.15f, 0.16f, 0.18f, 1.0f));
  QOpenGLFunctions* gl = m_context.functions();
  gl->glEnable(GL_BLEND);
  gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  const QMatrix4x4 p = projection();
  QMatrix4x4 camera;
  camera.translate(0.0f, -0.3f, -6.0f);
  camera.rotate(20.0f, 1.0f, 0.0f, 0.0f);
  camera.rotate(-30.0f, 0.0f, 1.0f, 0.0f);
  SkinRenderer::Fog noFog;

  // floor grid with fog, 1 pixel
  SkinRenderer::Fog fog;
  fog.on = true;
  fog.density = 0.12f;
  fog.color = QVector3D(0.15f, 0.16f, 0.18f);
  QVector<QVector3D> grid;
  for (int i = -10; i <= 10; i++)
  {
    grid << QVector3D(i, -1.0f, -10.0f) << QVector3D(i, -1.0f, 10.0f);
    grid << QVector3D(-10.0f, -1.0f, i) << QVector3D(10.0f, -1.0f, i);
  }
  m_renderer->drawGeometry(GL_LINES, grid, {}, QVector<QVector4D>(grid.size(), QVector4D(0.5f, 0.5f, 0.55f, 1.0f)),
                           camera, p, fog, 1.0f);

  // bones: a little skeleton, each line its own width and colour per end
  const QVector<QVector3D> bones = {
    {0.0f, -0.2f, 0.0f}, {0.0f, 0.8f, 0.0f},
    {0.0f, 0.8f, 0.0f}, {0.0f, 1.3f, 0.1f},
    {0.0f, 0.7f, 0.0f}, {-0.8f, 0.4f, 0.2f},
    {-0.8f, 0.4f, 0.2f}, {-1.3f, -0.1f, 0.5f},
    {0.0f, 0.7f, 0.0f}, {0.8f, 0.5f, -0.1f},
    {0.8f, 0.5f, -0.1f}, {1.5f, 0.6f, -0.4f},
    {0.0f, -0.2f, 0.0f}, {-0.3f, -1.0f, 0.1f},
    {0.0f, -0.2f, 0.0f}, {0.35f, -1.0f, -0.2f}};
  const float widths[] = {2.0f, 3.0f, 5.0f};
  for (int i = 0; i < bones.size(); i += 2)
  {
    const QVector<QVector3D> bone = {bones[i], bones[i + 1]};
    const QVector<QVector4D> colours = {QVector4D(0.95f, 0.75f, 0.2f, 1.0f),
                                        QVector4D(0.3f, 0.6f, 1.0f, 1.0f)};
    m_renderer->drawGeometry(GL_LINES, bone, {}, colours, camera, p, noFog, widths[(i / 2) % 3]);
  }

  // a joint ring, 2 pixels
  QVector<QVector3D> ring;
  const int segments = 32;
  for (int i = 0; i < segments; i++)
  {
    const float a0 = 2.0f * float(M_PI) * i / segments;
    const float a1 = 2.0f * float(M_PI) * (i + 1) / segments;
    ring << QVector3D(0.35f * qCos(a0), 0.8f, 0.35f * qSin(a0)) << QVector3D(0.35f * qCos(a1), 0.8f, 0.35f * qSin(a1));
  }
  m_renderer->drawGeometry(GL_LINES, ring, {}, QVector<QVector4D>(ring.size(), QVector4D(0.2f, 0.9f, 0.5f, 1.0f)),
                           camera, p, noFog, 2.0f);

  // the three axes, 3 pixels
  const QVector<QVector3D> axes = {{-1.5f, -1.0f, -1.5f}, {-0.5f, -1.0f, -1.5f},
                                   {-1.5f, -1.0f, -1.5f}, {-1.5f, 0.0f, -1.5f},
                                   {-1.5f, -1.0f, -1.5f}, {-1.5f, -1.0f, -0.5f}};
  const QVector<QVector4D> axisColours = {{1, 0, 0, 1}, {1, 0, 0, 1}, {0, 1, 0, 1}, {0, 1, 0, 1}, {0, 0, 1, 1}, {0, 0, 1, 1}};
  m_renderer->drawGeometry(GL_LINES, axes, {}, axisColours, camera, p, noFog, 3.0f);

  // lit lines (normals given), 4 pixels
  const QVector<QVector3D> lit = {{-2.0f, 1.5f, -1.0f}, {-1.0f, 1.8f, -1.5f},
                                  {-1.0f, 1.8f, -1.5f}, {0.0f, 1.6f, -2.0f}};
  const QVector<QVector3D> litNormals = {{0, 0, 1}, {0.5f, 0.5f, 0.7f}, {0.5f, 0.5f, 0.7f}, {-0.6f, 0.2f, 0.7f}};
  m_renderer->drawGeometry(GL_LINES, lit, litNormals, QVector<QVector4D>(lit.size(), QVector4D(0.8f, 0.4f, 0.7f, 1.0f)),
                           camera, p, noFog, 4.0f);

  // a half-transparent line over the others, and one of zero length
  const QVector<QVector3D> glass = {{-2.5f, 0.2f, 0.0f}, {2.5f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}};
  m_renderer->drawGeometry(GL_LINES, glass, {}, QVector<QVector4D>(glass.size(), QVector4D(1.0f, 1.0f, 1.0f, 0.5f)),
                           camera, p, noFog, 6.0f);

  const QImage image = end();
  gl->glDisable(GL_BLEND);

  const QString referencePath = QStringLiteral(PINGLYMOTION_LINEDRAW_DIR "/lines_reference.png");
  const QByteArray writeTo = qgetenv("PINGLYMOTION_LINEDRAW_WRITE");
  if (!writeTo.isEmpty())
  {
    QVERIFY(image.save(QString::fromLocal8Bit(writeTo)));
    QSKIP("picture written, nothing compared");
  }
  QImage reference(referencePath);
  QVERIFY2(!reference.isNull(), qPrintable("no reference picture " + referencePath));
  reference = reference.convertToFormat(QImage::Format_RGBA8888);
  QCOMPARE(image.size(), reference.size());

  // a small tolerance: another software or hardware driver may round an edge
  // pixel differently, but a moved, thicker, thinner or missing line fails
  int different = 0;
  int drawn = 0;
  const QRgb background = reference.pixel(0, 0);
  for (int y = 0; y < HEIGHT; y++)
    for (int x = 0; x < WIDTH; x++)
    {
      const QRgb a = image.pixel(x, y);
      const QRgb b = reference.pixel(x, y);
      if (b != background) drawn++;
      const int diff = qMax(qMax(qAbs(qRed(a) - qRed(b)), qAbs(qGreen(a) - qGreen(b))), qAbs(qBlue(a) - qBlue(b)));
      if (diff > 24) different++;
    }
  const double share = double(different) / (WIDTH * HEIGHT);
  if (share > 0.004)
  {
    const QString out = QDir::temp().filePath("linedraw_actual.png");
    image.save(out);
    dumpPicture("linesLookAsBefore", image);
    QFAIL(qPrintable(QString("%1 of %2 pixels differ from the reference (picture saved to %3)")
                       .arg(different).arg(WIDTH * HEIGHT).arg(out)));
  }
  // the reference really has lines in it
  QVERIFY2(drawn > 2000, qPrintable(QString("only %1 drawn pixels in the reference").arg(drawn)));
}

// Picking draws every bone in a colour that encodes its pick name
// (AnimationView::pickPartByColor); the colour must arrive unchanged,
// as wide as the line, and nowhere else.
void TestLineDraw::pickColourLineGivesName()
{
  if (!m_haveGl) QSKIP(qPrintable(m_glReason));
  begin(QVector4D(0, 0, 0, 0));
  const unsigned int name = 0x0304D2;   // all three bytes in use
  QMatrix4x4 camera;
  camera.translate(0.0f, 0.0f, -5.0f);
  const QVector<QVector3D> bone = {{-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
  m_renderer->drawGeometry(GL_LINES, bone, {}, QVector<QVector4D>(2, SkinRenderer::encodePickName(name)),
                           camera, projection(), SkinRenderer::Fog(), 6.0f);
  const QImage image = end();
  const int cx = WIDTH / 2;
  const int cy = HEIGHT / 2;
  auto nameAt = [&](int x, int y) {
    const QRgb c = image.pixel(x, y);
    const unsigned char rgba[4] = {uchar(qRed(c)), uchar(qGreen(c)), uchar(qBlue(c)), uchar(qAlpha(c))};
    return SkinRenderer::decodePickName(rgba);
  };
  if (nameAt(cx, cy) != name)
  {
    const QRgb c = image.pixel(cx, cy);
    qInfo("centre pixel %d %d %d %d", qRed(c), qGreen(c), qBlue(c), qAlpha(c));
    dumpPicture("pickColourLineGivesName", image);
  }
  QCOMPARE(nameAt(cx, cy), name);
  QCOMPARE(nameAt(cx, cy - 2), name);
  QCOMPARE(nameAt(cx, cy + 1), name);
  QCOMPARE(nameAt(cx + 20, cy), name);
  // 6 pixels wide: 10 pixels above or below is empty
  QCOMPARE(nameAt(cx, cy - 10), 0u);
  QCOMPARE(nameAt(cx, cy + 10), 0u);
  int rows = 0;
  for (int y = 0; y < HEIGHT; y++)
    if (nameAt(cx, y) == name) rows++;
  QVERIFY2(rows >= 5 && rows <= 7, qPrintable(QString("line is %1 pixels wide").arg(rows)));
}

// A line with an end behind the camera is left out, as before
void TestLineDraw::lineBehindCameraDrawsNothing()
{
  if (!m_haveGl) QSKIP(qPrintable(m_glReason));
  begin(QVector4D(0, 0, 0, 0));
  const QVector<QVector3D> line = {{0.2f, 0.1f, -3.0f}, {0.2f, 0.1f, 2.0f}};
  m_renderer->drawGeometry(GL_LINES, line, {}, QVector<QVector4D>(2, QVector4D(1, 1, 1, 1)),
                           QMatrix4x4(), projection(), SkinRenderer::Fog(), 4.0f);
  const QImage image = end();
  for (int y = 0; y < HEIGHT; y++)
    for (int x = 0; x < WIDTH; x++)
      if (qRed(image.pixel(x, y)) != 0) QFAIL(qPrintable(QString("pixel %1,%2 drawn").arg(x).arg(y)));
}

// Every shader with its desktop header and with its web header goes through
// glslangValidator (the Khronos reference compiler): each stage on its own,
// then each program linked (the body, mesh and line programs share one
// fragment shader). So the web variant is known to build without a web build.
void TestLineDraw::shadersBuildForDesktopAndWeb()
{
  QVERIFY(SkinRenderer::shaderHeader(false).startsWith("#version 330 core\n"));
  QVERIFY(SkinRenderer::shaderHeader(true).startsWith("#version 300 es\n"));
  for (bool web : {false, true})
  {
    const QVector<SkinRenderer::ShaderSource> sources = SkinRenderer::shaderSources(web);
    QCOMPARE(sources.size(), 4);
    for (const SkinRenderer::ShaderSource& shader : sources)
    {
      QVERIFY2(shader.source.startsWith(SkinRenderer::shaderHeader(web)), qPrintable(shader.name));
      // the header is the only #version line, and no geometry shader is left
      QCOMPARE(shader.source.count("#version"), 1);
      QVERIFY2(!shader.source.contains("EmitVertex"), qPrintable(shader.name));
    }
  }

  const QString validator = QStandardPaths::findExecutable("glslangValidator");
  if (validator.isEmpty())
  {
#ifdef Q_OS_LINUX
    // the Linux builds install it (glslang-tools), so a missing one is a mistake there
    if (qEnvironmentVariableIsSet("CI")) QFAIL("glslangValidator is missing; install glslang-tools");
#endif
    QSKIP("glslangValidator is not installed");
  }
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  for (bool web : {false, true})
  {
    const QString variant = web ? "web" : "desktop";
    QMap<QString, QString> files;
    for (const SkinRenderer::ShaderSource& shader : SkinRenderer::shaderSources(web))
    {
      QDir().mkpath(dir.filePath(variant));
      const QString path = dir.filePath(variant + "/" + shader.name);
      QFile file(path);
      QVERIFY(file.open(QIODevice::WriteOnly));
      file.write(shader.source);
      file.close();
      files.insert(shader.name, path);
    }
    const QList<QStringList> programs = {{"body.vert", "body.frag"},
                                         {"mesh.vert", "body.frag"},
                                         {"line.vert", "body.frag"}};
    for (const QStringList& program : programs)
    {
      QStringList arguments = {"-l"};
      for (const QString& name : program) arguments << files.value(name);
      QProcess process;
      process.start(validator, arguments);
      QVERIFY(process.waitForFinished(30000));
      const QString output = QString::fromLocal8Bit(process.readAllStandardOutput() + process.readAllStandardError());
      QVERIFY2(process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0,
               qPrintable(variant + " " + program.join(" + ") + ":\n" + output));
    }
  }
}

// TEMPORARY: find out why CI's Mesa draws nothing
static const char* oldVert = R"(#version 330 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec4 vertexColor;
uniform mat4 modelview;
uniform mat4 projection;
uniform mat3 normalMatrix;
out vec4 lineColor;
out vec3 lineEyePosition;
out vec3 lineEyeNormal;
void main()
{
  vec4 eye = modelview * vec4(position, 1.0);
  lineEyePosition = eye.xyz;
  lineEyeNormal = normalMatrix * normal;
  lineColor = vertexColor;
  gl_Position = projection * eye;
}
)";
static const char* oldGeom = R"(#version 330 core
layout(lines) in;
layout(triangle_strip, max_vertices = 4) out;
in vec4 lineColor[];
in vec3 lineEyePosition[];
in vec3 lineEyeNormal[];
uniform vec2 viewportSize;
uniform float lineWidth;
out vec4 baseColor;
out vec3 eyePosition;
out vec3 eyeNormal;
out float eyeDepth;
void corner(int i, vec2 offset)
{
  baseColor = lineColor[i];
  eyePosition = lineEyePosition[i];
  eyeNormal = lineEyeNormal[i];
  eyeDepth = abs(lineEyePosition[i].z);
  vec4 p = gl_in[i].gl_Position;
  gl_Position = vec4(p.xy + offset * p.w, p.zw);
  EmitVertex();
}
void main()
{
  vec4 a = gl_in[0].gl_Position;
  vec4 b = gl_in[1].gl_Position;
  if (a.w <= 0.0 || b.w <= 0.0) return;
  vec2 screenA = a.xy / a.w * viewportSize;
  vec2 screenB = b.xy / b.w * viewportSize;
  vec2 along = screenB - screenA;
  if (dot(along, along) < 1e-8) along = vec2(1.0, 0.0);
  vec2 across = normalize(vec2(-along.y, along.x));
  vec2 offset = across * lineWidth / viewportSize;
  corner(0, offset);
  corner(0, -offset);
  corner(1, offset);
  corner(1, -offset);
  EndPrimitive();
}
)";

void TestLineDraw::diagnose()
{
  if (!m_haveGl) QSKIP(qPrintable(m_glReason));
  QOpenGLExtraFunctions* gl = m_context.extraFunctions();
  QString frag;
  QString lineVert;
  for (const auto& sh : SkinRenderer::shaderSources(false))
  {
    if (sh.name == "body.frag") frag = QString::fromLatin1(sh.source);
    if (sh.name == "line.vert") lineVert = QString::fromLatin1(sh.source);
  }
  QMatrix4x4 camera;
  camera.translate(0.0f, 0.0f, -5.0f);
  const QVector4D colour(0.2f, 0.6f, 1.0f, 1.0f);
  auto centre = [&](const char* what) {
    const QImage image = end();
    const QRgb c = image.pixel(WIDTH / 2, HEIGHT / 2);
    int drawn = 0;
    for (int y = 0; y < HEIGHT; y++)
      for (int x = 0; x < WIDTH; x++)
        if (qAlpha(image.pixel(x, y))) drawn++;
    qInfo("DIAG %s: centre %d %d %d %d, drawn %d, glError 0x%x", what, qRed(c), qGreen(c), qBlue(c), qAlpha(c), drawn,
          m_context.functions()->glGetError());
  };
  auto setCommon = [&](QOpenGLShaderProgram& p) {
    p.bind();
    p.setUniformValue("modelview", camera);
    p.setUniformValue("projection", projection());
    p.setUniformValue("normalMatrix", camera.normalMatrix());
    p.setUniformValue("unlit", true);
    p.setUniformValue("picking", false);
    p.setUniformValue("shadow", false);
    p.setUniformValue("fogOn", false);
    p.setUniformValue("exposure", 1.0f);
    p.setUniformValue("tint", QVector3D(1, 1, 1));
    p.setUniformValue("viewportSize", QVector2D(WIDTH, HEIGHT));
    p.setUniformValue("lineWidth", 6.0f);
  };

  // 1: the renderer
  begin(QVector4D(0, 0, 0, 0));
  m_renderer->drawGeometry(GL_LINES, {{-1, 0, 0}, {1, 0, 0}}, {}, QVector<QVector4D>(2, colour), camera, projection(),
                           SkinRenderer::Fog(), 6.0f);
  centre("renderer lines");
  begin(QVector4D(0, 0, 0, 0));
  m_renderer->drawGeometry(GL_TRIANGLES, {{-1, -0.5f, 0}, {1, -0.5f, 0}, {0, 0.5f, 0}}, {}, QVector<QVector4D>(3, colour),
                           camera, projection(), SkinRenderer::Fog(), 1.0f);
  centre("renderer triangles");

  // 2: the old geometry shader, raw GL
  {
    QOpenGLShaderProgram p;
    const bool ok = p.addShaderFromSourceCode(QOpenGLShader::Vertex, oldVert)
                    && p.addShaderFromSourceCode(QOpenGLShader::Geometry, oldGeom)
                    && p.addShaderFromSourceCode(QOpenGLShader::Fragment, frag) && p.link();
    qInfo("DIAG old program ok %d %s", ok, qPrintable(p.log()));
    const float data[] = {-1, 0, 0, 0, 0, 1, 0.2f, 0.6f, 1, 1, 1, 0, 0, 0, 0, 1, 0.2f, 0.6f, 1, 1};
    GLuint vao = 0, vbo = 0;
    gl->glGenVertexArrays(1, &vao);
    gl->glBindVertexArray(vao);
    gl->glGenBuffers(1, &vbo);
    gl->glBindBuffer(GL_ARRAY_BUFFER, vbo);
    gl->glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
    for (int i = 0; i < 3; i++) gl->glEnableVertexAttribArray(i);
    gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 40, reinterpret_cast<void*>(0));
    gl->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 40, reinterpret_cast<void*>(12));
    gl->glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 40, reinterpret_cast<void*>(24));
    begin(QVector4D(0, 0, 0, 0));
    setCommon(p);
    gl->glBindVertexArray(vao);
    gl->glDrawArrays(GL_LINES, 0, 2);
    gl->glBindVertexArray(0);
    p.release();
    centre("old geometry shader");
    gl->glDeleteBuffers(1, &vbo);
    gl->glDeleteVertexArrays(1, &vao);
  }

  // 3: the new line shader, raw GL, with glDrawArrays and with an index buffer
  {
    QOpenGLShaderProgram p;
    const bool ok = p.addShaderFromSourceCode(QOpenGLShader::Vertex, lineVert)
                    && p.addShaderFromSourceCode(QOpenGLShader::Fragment, frag) && p.link();
    qInfo("DIAG new program ok %d %s", ok, qPrintable(p.log()));
    QVector<float> d;
    const float A[3] = {-1, 0, 0}, B[3] = {1, 0, 0};
    auto corner = [&](const float* self, const float* other, float side) {
      d << self[0] << self[1] << self[2] << 0 << 0 << 1 << 0.2f << 0.6f << 1 << 1 << other[0] << other[1] << other[2] << side;
    };
    corner(A, B, 1); corner(A, B, -1); corner(B, A, -1); corner(B, A, 1);
    QVector<float> flat;
    const int order[6] = {0, 1, 2, 2, 1, 3};
    for (int i : order) for (int k = 0; k < 14; k++) flat << d[i * 14 + k];
    GLuint vao = 0, vbo = 0, ibo = 0;
    gl->glGenVertexArrays(1, &vao);
    gl->glBindVertexArray(vao);
    gl->glGenBuffers(1, &vbo);
    gl->glBindBuffer(GL_ARRAY_BUFFER, vbo);
    for (int i = 0; i < 5; i++) gl->glEnableVertexAttribArray(i);
    auto pointers = [&]() {
      gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 56, reinterpret_cast<void*>(0));
      gl->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 56, reinterpret_cast<void*>(12));
      gl->glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 56, reinterpret_cast<void*>(24));
      gl->glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 56, reinterpret_cast<void*>(40));
      gl->glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, 56, reinterpret_cast<void*>(52));
    };
    gl->glBufferData(GL_ARRAY_BUFFER, flat.size() * 4, flat.constData(), GL_STATIC_DRAW);
    pointers();
    begin(QVector4D(0, 0, 0, 0));
    setCommon(p);
    gl->glBindVertexArray(vao);
    gl->glDrawArrays(GL_TRIANGLES, 0, 6);
    p.release();
    centre("new shader, glDrawArrays");

    gl->glBindVertexArray(vao);
    gl->glBindBuffer(GL_ARRAY_BUFFER, vbo);
    gl->glBufferData(GL_ARRAY_BUFFER, d.size() * 4, d.constData(), GL_STATIC_DRAW);
    const GLuint idx[6] = {0, 1, 2, 2, 1, 3};
    gl->glGenBuffers(1, &ibo);
    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    gl->glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
    begin(QVector4D(0, 0, 0, 0));
    setCommon(p);
    gl->glBindVertexArray(vao);
    gl->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    gl->glBindVertexArray(0);
    p.release();
    centre("new shader, glDrawElements");
    gl->glDeleteBuffers(1, &vbo);
    gl->glDeleteBuffers(1, &ibo);
    gl->glDeleteVertexArrays(1, &vao);
  }
}

QTEST_MAIN(TestLineDraw)
#include "tst_linedraw.moc"
