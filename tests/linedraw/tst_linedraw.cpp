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
// then each program linked (the mesh and line programs share a fragment
// shader without gl_PrimitiveID). So the web variant is known to build without a web build.
void TestLineDraw::shadersBuildForDesktopAndWeb()
{
  QVERIFY(SkinRenderer::shaderHeader(false).startsWith("#version 330 core\n"));
  QVERIFY(SkinRenderer::shaderHeader(true).startsWith("#version 300 es\n"));
  for (bool web : {false, true})
  {
    const QVector<SkinRenderer::ShaderSource> sources = SkinRenderer::shaderSources(web);
    QCOMPARE(sources.size(), 5);
    for (const SkinRenderer::ShaderSource& shader : sources)
    {
      QVERIFY2(shader.source.startsWith(SkinRenderer::shaderHeader(web)), qPrintable(shader.name));
      // the header is the only #version line, and no geometry shader is left
      QCOMPARE(shader.source.count("#version"), 1);
      QVERIFY2(!shader.source.contains("EmitVertex"), qPrintable(shader.name));
      // Mesa's llvmpipe 26 draws nothing with gl_PrimitiveID and no geometry
      // shader, so only the body program may pick by it
      if (shader.name == "plain.frag")
        QVERIFY(!shader.source.contains("#define PICK_BY_PRIMITIVE"));
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
                                         {"mesh.vert", "plain.frag"},
                                         {"line.vert", "plain.frag"}};
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

QTEST_MAIN(TestLineDraw)
#include "tst_linedraw.moc"
