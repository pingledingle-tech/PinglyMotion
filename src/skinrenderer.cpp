/*
 * Pingmator - draws the skinned avatar body with OpenGL 3.3 shaders
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "skinrenderer.h"
#include "avatarskin.h"
#include "mesh.h"

#include <QOpenGLContext>
#include <QSurfaceFormat>
#include <QtGlobal>
#include <QtMath>
#include <QVector2D>

#ifndef GL_TEXTURE_BUFFER
#define GL_TEXTURE_BUFFER 0x8C2A
#endif
#ifndef GL_RGBA32F
#define GL_RGBA32F 0x8814
#endif
#ifndef GL_R32I
#define GL_R32I 0x8235
#endif

namespace
{
// texels (vec4) per joint in the buffer texture:
// 0-3 position matrix columns, 4-6 normal matrix columns, 7 colour, 8 pick name
const int TEXELS_PER_JOINT = 9;
// floats per vertex in the float buffer: rest position 3, rest normal 3, weights 4, blend 1
const int FLOATS_PER_VERTEX = 11;
// ints per vertex in the int buffer: influence joints 4, colour joints 2
const int INTS_PER_VERTEX = 6;

const char* vertexShader = R"(#version 330 core
layout(location = 0) in vec3 restPosition;
layout(location = 1) in vec3 restNormal;
layout(location = 2) in ivec4 joints;      // up to four joints per vertex
layout(location = 3) in vec4 weights;
layout(location = 4) in ivec2 colorJoints; // the two strongest joints, for the colour
layout(location = 5) in float colorBlend;  // how far the colour leans to the second

uniform samplerBuffer jointData;
uniform bool eye;
uniform mat4 projection;
uniform bool shadow;             // flatten onto the floor
uniform mat4 view;               // camera, for the shadow
uniform mat4 viewInverse;
uniform vec3 shadowDirection;    // light direction in the world, pointing down

out vec4 baseColor;     // lit per pixel in the fragment shader
out vec3 eyePosition;
out vec3 eyeNormal;
out float eyeDepth;

const int TEXELS = 9;

void main()
{
  // skinning, the same sums as AvatarSkin::skin()
  vec3 position = vec3(0.0);
  vec3 direction = vec3(0.0);
  for (int k = 0; k < 4; k++)
  {
    float w = weights[k];
    if (w <= 0.0) continue;
    int base = joints[k] * TEXELS;
    mat4 m = mat4(texelFetch(jointData, base), texelFetch(jointData, base + 1),
                  texelFetch(jointData, base + 2), texelFetch(jointData, base + 3));
    mat3 n = mat3(texelFetch(jointData, base + 4).xyz, texelFetch(jointData, base + 5).xyz,
                  texelFetch(jointData, base + 6).xyz);
    position += (m * vec4(restPosition, 1.0)).xyz * w;
    direction += (n * restNormal) * w;
  }

  if (shadow)
  {
    // along the light down to the floor, a little above it
    vec3 world = (viewInverse * vec4(position, 1.0)).xyz;
    world -= shadowDirection * (world.y / shadowDirection.y);
    world.y = 0.05;
    position = (view * vec4(world, 1.0)).xyz;
  }

  // each vertex gets the colour of its joints, so a selected part shows
  vec4 color = mix(texelFetch(jointData, colorJoints.x * TEXELS + 7),
                   texelFetch(jointData, colorJoints.y * TEXELS + 7), colorBlend);
  // eyeballs are lighter than skin
  if (eye) color.rgb = 0.5 + color.rgb * 0.5;

  baseColor = color;
  eyePosition = position;
  eyeNormal = direction;
  eyeDepth = abs(position.z);
  gl_Position = projection * vec4(position, 1.0);
}
)";

const char* fragmentShader = R"(#version 330 core
in vec4 baseColor;
in vec3 eyePosition;
in vec3 eyeNormal;
in float eyeDepth;

uniform bool fogOn;
uniform float fogDensity;
uniform vec3 fogColor;
uniform bool picking;
uniform samplerBuffer jointData;
uniform isamplerBuffer triangleJoints;
uniform bool unlit;     // plain colour, no light
uniform bool shadow;
uniform float shadowStrength;
uniform float exposure;          // Light... brightness: 0.5 to 2
uniform float specularStrength;  // Light... shine
uniform float skinShininess;
uniform vec3 tint;               // Light... warmth, (1, 1, 1) is neutral

out vec4 fragColor;

// Studio lights that move with the camera, in eye space (x right, y up,
// z towards the viewer). Lighting is done in linear colour and turned back
// to screen colour (gamma 2.2) at the end.
const vec3 keyDirection = vec3(0.45, 0.65, 0.62);    // front, above, right
const vec3 keyColor = vec3(1.45, 1.38, 1.28);
const vec3 fillDirection = vec3(-0.75, 0.15, 0.64);  // front left, lower
const vec3 fillColor = vec3(0.36, 0.35, 0.38);
const vec3 skyColor = vec3(0.30, 0.30, 0.32);        // light from above
const vec3 groundColor = vec3(0.09, 0.08, 0.07);     // light bounced from below
const float wrap = 0.35;            // soft edge between light and shadow, like skin
const float rimStrength = 0.18;     // thin light along the outline

vec3 shade(vec3 albedo, vec3 n, vec3 v)
{
  vec3 key = normalize(keyDirection);
  vec3 fill = normalize(fillDirection);
  float keyDiffuse = max((dot(n, key) + wrap) / (1.0 + wrap), 0.0);
  float fillDiffuse = max((dot(n, fill) + wrap) / (1.0 + wrap), 0.0);
  vec3 ambient = mix(groundColor, skyColor, 0.5 + 0.5 * n.y);
  vec3 light = (ambient + keyDiffuse * keyColor + fillDiffuse * fillColor) * tint;
  float specular = 0.0;
  if (dot(n, key) > 0.0)
    specular = pow(max(dot(n, normalize(key + v)), 0.0), skinShininess) * specularStrength;
  float rim = pow(1.0 - max(dot(n, v), 0.0), 3.0) * rimStrength;
  return (albedo * light + specular * keyColor * tint + rim * skyColor) * exposure;
}

const int TEXELS = 9;

void main()
{
  if (shadow)
  {
    float a = shadowStrength;
    if (fogOn) a *= clamp(exp(-fogDensity * eyeDepth), 0.0, 1.0);
    fragColor = vec4(0.0, 0.0, 0.0, a);
    return;
  }
  if (picking)
  {
    // the pick name of the joint this triangle mostly belongs to, as a colour
    int joint = texelFetch(triangleJoints, gl_PrimitiveID).r;
    int name = int(texelFetch(jointData, joint * TEXELS + 8).r + 0.5);
    fragColor = vec4(float(name & 255), float((name >> 8) & 255), float((name >> 16) & 255), 255.0) / 255.0;
    return;
  }
  vec3 rgb;
  if (unlit)
    rgb = baseColor.rgb;
  else
  {
    vec3 n = normalize(eyeNormal);
    vec3 v = normalize(-eyePosition);
    // a normal pointing away (seen from behind) is turned to the viewer
    if (dot(n, v) < 0.0) n = -n;
    vec3 albedo = pow(clamp(baseColor.rgb, 0.0, 1.0), vec3(2.2));
    rgb = pow(clamp(shade(albedo, n, v), 0.0, 1.0), vec3(1.0 / 2.2));
  }
  if (fogOn)
  {
    // GL_EXP fog
    float f = clamp(exp(-fogDensity * eyeDepth), 0.0, 1.0);
    rgb = mix(fogColor, rgb, f);
  }
  fragColor = vec4(rgb, baseColor.a);
}
)";
}

// Bento balls, props and handles: same outputs as the body's vertex shader,
// so they share its fragment shader and light
const char* meshVertexShader = R"(#version 330 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec4 vertexColor;

uniform mat4 modelview;
uniform mat4 projection;
uniform mat3 normalMatrix;
uniform vec4 color;
uniform bool perVertexColor;   // drawGeometry(): a colour for every point

out vec4 baseColor;
out vec3 eyePosition;
out vec3 eyeNormal;
out float eyeDepth;

void main()
{
  vec4 eye = modelview * vec4(position, 1.0);
  eyePosition = eye.xyz;
  eyeNormal = normalMatrix * normal;
  baseColor = perVertexColor ? vertexColor : color;
  eyeDepth = abs(eye.z);
  gl_Position = projection * eye;
}
)";

// Lines: the same as the mesh shader, then a geometry shader turns every
// line into a strip of lineWidth pixels facing the screen (modern OpenGL
// does not promise wide lines)
const char* lineVertexShader = R"(#version 330 core
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

const char* lineGeometryShader = R"(#version 330 core
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
  if (a.w <= 0.0 || b.w <= 0.0) return;   // behind the camera
  vec2 screenA = a.xy / a.w * viewportSize;
  vec2 screenB = b.xy / b.w * viewportSize;
  vec2 along = screenB - screenA;
  if (dot(along, along) < 1e-8) along = vec2(1.0, 0.0);
  vec2 across = normalize(vec2(-along.y, along.x));
  // half the width, in normalised device units
  vec2 offset = across * lineWidth / viewportSize;
  corner(0, offset);
  corner(0, -offset);
  corner(1, offset);
  corner(1, -offset);
  EndPrimitive();
}
)";

bool SkinRenderer::init()
{
  m_ready = false;
  // anything from an earlier context belongs to that context; Qt frees it
  // with the context
  m_program.reset();
  m_meshProgram.reset();
  m_lineProgram.reset();
  m_meshes.clear();
  m_geometryVao.reset();
  m_geometryBuffer.reset();
  m_parts.clear();
  m_restSkin = nullptr;
  m_restRevision = -1;
  m_jointBuffer = 0;
  m_jointTexture = 0;

  QOpenGLContext* context = QOpenGLContext::currentContext();
  if (!context)
  {
    m_status = "no OpenGL context";
    return false;
  }
  const QSurfaceFormat format = context->format();
  if (context->isOpenGLES() || format.version() < qMakePair(3, 3))
  {
    m_status = QString("the graphics driver gives %1 %2.%3")
                 .arg(context->isOpenGLES() ? "OpenGL ES" : "OpenGL")
                 .arg(format.majorVersion()).arg(format.minorVersion());
    return false;
  }

  m_gl.initializeOpenGLFunctions();
  m_texBuffer = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum, GLenum, GLuint)>(
    context->getProcAddress("glTexBuffer"));
  if (!m_texBuffer)
  {
    m_status = "the graphics driver has no glTexBuffer";
    return false;
  }

  m_program = std::make_unique<QOpenGLShaderProgram>();
  if (!m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShader)
      || !m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShader)
      || !m_program->link())
  {
    m_status = "the body shader did not build: " + m_program->log().trimmed();
    m_program.reset();
    return false;
  }

  m_meshProgram = std::make_unique<QOpenGLShaderProgram>();
  if (!m_meshProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, meshVertexShader)
      || !m_meshProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShader)
      || !m_meshProgram->link())
  {
    m_status = "the mesh shader did not build: " + m_meshProgram->log().trimmed();
    m_meshProgram.reset();
    m_program.reset();
    return false;
  }
  m_meshProgram->bind();
  m_meshProgram->setUniformValue("picking", false);
  m_meshProgram->setUniformValue("shadow", false);
  m_meshProgram->release();

  m_lineProgram = std::make_unique<QOpenGLShaderProgram>();
  if (!m_lineProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, lineVertexShader)
      || !m_lineProgram->addShaderFromSourceCode(QOpenGLShader::Geometry, lineGeometryShader)
      || !m_lineProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShader)
      || !m_lineProgram->link())
  {
    m_status = "the line shader did not build: " + m_lineProgram->log().trimmed();
    m_lineProgram.reset();
    m_meshProgram.reset();
    m_program.reset();
    return false;
  }
  m_lineProgram->bind();
  m_lineProgram->setUniformValue("picking", false);
  m_lineProgram->setUniformValue("shadow", false);
  m_lineProgram->release();

  m_gl.glGenBuffers(1, &m_jointBuffer);
  m_gl.glGenTextures(1, &m_jointTexture);
  if (!m_jointBuffer || !m_jointTexture)
  {
    m_status = "no buffer texture";
    return false;
  }

  m_program->bind();
  m_program->setUniformValue("jointData", 0);        // texture unit 0
  m_program->setUniformValue("triangleJoints", 1);   // texture unit 1
  m_program->release();

  m_status = QString("OpenGL %1.%2 %3 shaders").arg(format.majorVersion()).arg(format.minorVersion())
               .arg(format.profile() == QSurfaceFormat::CoreProfile ? "core" : "compatibility");
  m_ready = true;
  return true;
}

void SkinRenderer::cleanup()
{
  if (!QOpenGLContext::currentContext()) return;
  releaseRest();
  if (m_jointTexture) m_gl.glDeleteTextures(1, &m_jointTexture);
  if (m_jointBuffer) m_gl.glDeleteBuffers(1, &m_jointBuffer);
  m_jointTexture = 0;
  m_jointBuffer = 0;
  m_meshes.clear();
  m_geometryVao.reset();
  m_geometryBuffer.reset();
  m_lineProgram.reset();
  m_meshProgram.reset();
  m_program.reset();
  m_ready = false;
}

void SkinRenderer::drawGeometry(GLenum mode, const QVector<QVector3D>& positions,
                                const QVector<QVector3D>& normals, const QVector<QVector4D>& colors,
                                const QMatrix4x4& modelview, const QMatrix4x4& projection, const Fog& fog,
                                float lineWidth)
{
  if (!m_ready || !m_meshProgram || !m_lineProgram) return;
  const int count = positions.size();
  const bool lit = !normals.isEmpty();
  if (count == 0 || colors.size() != count || (lit && normals.size() != count)) return;

  if (!m_geometryVao)
  {
    m_geometryVao = std::make_unique<QOpenGLVertexArrayObject>();
    m_geometryBuffer = std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer);
    if (!m_geometryVao->create() || !m_geometryBuffer->create())
    {
      m_geometryVao.reset();
      m_geometryBuffer.reset();
      return;
    }
    m_geometryBuffer->setUsagePattern(QOpenGLBuffer::StreamDraw);
    m_geometryVao->bind();
    m_geometryBuffer->bind();
    const GLsizei stride = 10 * sizeof(float);
    m_gl.glEnableVertexAttribArray(0);
    m_gl.glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(0));
    m_gl.glEnableVertexAttribArray(1);
    m_gl.glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(3 * sizeof(float)));
    m_gl.glEnableVertexAttribArray(2);
    m_gl.glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(6 * sizeof(float)));
    m_geometryVao->release();
    m_geometryBuffer->release();
  }

  m_geometryData.resize(count * 10);
  for (int i = 0; i < count; i++)
  {
    float* d = m_geometryData.data() + i * 10;
    d[0] = positions[i].x(); d[1] = positions[i].y(); d[2] = positions[i].z();
    const QVector3D n = lit ? normals[i] : QVector3D(0, 0, 1);
    d[3] = n.x(); d[4] = n.y(); d[5] = n.z();
    d[6] = colors[i].x(); d[7] = colors[i].y(); d[8] = colors[i].z(); d[9] = colors[i].w();
  }
  m_geometryBuffer->bind();
  m_geometryBuffer->allocate(m_geometryData.constData(), m_geometryData.size() * int(sizeof(float)));
  m_geometryBuffer->release();

  const bool lines = (mode == GL_LINES);
  QOpenGLShaderProgram* program = lines ? m_lineProgram.get() : m_meshProgram.get();
  program->bind();
  program->setUniformValue("modelview", modelview);
  program->setUniformValue("projection", projection);
  program->setUniformValue("normalMatrix", modelview.normalMatrix());
  program->setUniformValue("unlit", !lit);
  if (lines)
  {
    GLint viewport[4];
    m_gl.glGetIntegerv(GL_VIEWPORT, viewport);
    program->setUniformValue("viewportSize", QVector2D(qMax(1, viewport[2]), qMax(1, viewport[3])));
    program->setUniformValue("lineWidth", qMax(1.0f, lineWidth));
  }
  else
    program->setUniformValue("perVertexColor", true);
  setLookUniforms(program, fog);
  m_geometryVao->bind();
  m_gl.glDrawArrays(mode, 0, count);
  m_geometryVao->release();
  if (!lines) program->setUniformValue("perVertexColor", false);
  program->release();
}

void SkinRenderer::drawShadow(const AvatarSkin& skin, const QVector<QMatrix4x4>& positions,
                              const QVector<QMatrix3x3>& normals, const QMatrix4x4& view,
                              const QMatrix4x4& projection, const Fog& fog)
{
  if (!m_ready || !m_look.shadow) return;
  const int jointCount = skin.jointNames().size();
  const QVector<QVector4D> colors(jointCount);
  const QVector<unsigned int> names(jointCount, 0);

  // A soft edge: the shadow is drawn several times from light directions
  // spread over a small disc, each time a little dark. Where all of them
  // cover the floor it is as dark as one strong shadow; towards the edge
  // fewer cover it. Parts high above the floor get a wider, softer edge.
  const int passes = 12;
  const float strength = 0.38f;
  const float passStrength = 1.0f - qPow(1.0f - strength, 1.0f / passes);
  const QVector3D baseDirection(-0.30f, -1.0f, -0.35f);
  const float spread = 0.022f;

  // The stencil makes every pixel darker only once, even where parts of the
  // body overlap in the shadow. Nothing is written to the depth buffer.
  m_gl.glEnable(GL_STENCIL_TEST);
  m_gl.glClearStencil(0);
  m_gl.glStencilFunc(GL_EQUAL, 0, 0xFF);
  m_gl.glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
  m_gl.glDepthMask(GL_FALSE);
  m_gl.glEnable(GL_BLEND);
  // darken the colour, but keep the picture's alpha (screenshots have one)
  m_gl.glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);

  m_shadowPass = true;
  m_shadowView = view;
  m_shadowStrength = passStrength;
  for (int pass = 0; pass < passes; pass++)
  {
    // the stencil keeps one pass from darkening a pixel twice
    m_gl.glClear(GL_STENCIL_BUFFER_BIT);
    // a sunflower pattern: evenly spread over the disc
    const float angle = float(pass) * 2.39996f;
    const float radius = spread * qSqrt((pass + 0.5f) / passes);
    m_shadowDirection = (baseDirection.normalized()
                         + QVector3D(qCos(angle) * radius, 0.0f, qSin(angle) * radius)).normalized();
    draw(skin, positions, normals, colors, names, false, projection, fog);
  }
  m_shadowPass = false;

  m_gl.glDepthMask(GL_TRUE);
  m_gl.glDisable(GL_STENCIL_TEST);
  m_gl.glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void SkinRenderer::setLookUniforms(QOpenGLShaderProgram* program, const Fog& fog) const
{
  program->setUniformValue("fogOn", fog.on);
  program->setUniformValue("fogDensity", fog.density);
  program->setUniformValue("fogColor", fog.color);
  // Light...: 0.5 gives the standard look (exposure 1, highlight 0.16 at 20, neutral)
  program->setUniformValue("exposure", float(qPow(2.0, (m_look.brightness - 0.5) * 2.0)));
  program->setUniformValue("specularStrength", 0.32f * m_look.shine);
  program->setUniformValue("skinShininess", 8.0f + 24.0f * m_look.shine);
  const float warm = m_look.warmth * 2.0f - 1.0f;
  program->setUniformValue("tint", QVector3D(1.0f + 0.15f * warm, 1.0f, 1.0f - 0.18f * warm));
}

void SkinRenderer::drawMesh(const Mesh& mesh, const QVector4D& color, const QMatrix4x4& modelview,
                            const QMatrix4x4& projection, const Fog& fog, bool lit)
{
  if (!m_ready || !m_meshProgram) return;
  const QVector<float>& vertices = mesh.drawVertices();
  const QVector<float>& normals = mesh.drawNormals();
  const int count = vertices.size() / 3;
  if (count == 0 || normals.size() != vertices.size()) return;

  MeshBuffers& buffers = m_meshes[&mesh];
  if (!buffers.vao || buffers.revision != mesh.revision())
  {
    QVector<float> data(count * 6);
    for (int i = 0; i < count; i++)
    {
      for (int k = 0; k < 3; k++)
      {
        data[i * 6 + k] = vertices[i * 3 + k];
        data[i * 6 + 3 + k] = normals[i * 3 + k];
      }
    }
    buffers.vao = std::make_unique<QOpenGLVertexArrayObject>();
    buffers.vertices = std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer);
    if (!buffers.vao->create() || !buffers.vertices->create())
    {
      m_meshes.erase(&mesh);
      return;
    }
    buffers.vao->bind();
    buffers.vertices->bind();
    buffers.vertices->allocate(data.constData(), data.size() * int(sizeof(float)));
    m_gl.glEnableVertexAttribArray(0);
    m_gl.glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(0));
    m_gl.glEnableVertexAttribArray(1);
    m_gl.glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
    buffers.vao->release();
    buffers.vertices->release();
    buffers.count = count;
    buffers.revision = mesh.revision();
  }

  m_meshProgram->bind();
  m_meshProgram->setUniformValue("modelview", modelview);
  m_meshProgram->setUniformValue("projection", projection);
  m_meshProgram->setUniformValue("normalMatrix", modelview.normalMatrix());
  m_meshProgram->setUniformValue("color", color);
  m_meshProgram->setUniformValue("perVertexColor", false);
  m_meshProgram->setUniformValue("unlit", !lit);
  setLookUniforms(m_meshProgram.get(), fog);
  buffers.vao->bind();
  m_gl.glDrawArrays(GL_TRIANGLES, 0, buffers.count);
  buffers.vao->release();
  m_meshProgram->release();
}

QVector4D SkinRenderer::encodePickName(unsigned int name)
{
  name = qMin(name, MAX_PICK_NAME);
  return QVector4D((name & 255) / 255.0f, ((name >> 8) & 255) / 255.0f, ((name >> 16) & 255) / 255.0f, 1.0f);
}

unsigned int SkinRenderer::decodePickName(const unsigned char* rgba)
{
  return rgba[0] | (rgba[1] << 8) | (rgba[2] << 16);
}

void SkinRenderer::releaseRest()
{
  if (QOpenGLContext::currentContext())
  {
    for (PartBuffers& part : m_parts)
    {
      if (part.triangleTexture) m_gl.glDeleteTextures(1, &part.triangleTexture);
      if (part.triangleBuffer) m_gl.glDeleteBuffers(1, &part.triangleBuffer);
    }
  }
  m_parts.clear();   // the other buffers free themselves in the current context
  m_restSkin = nullptr;
  m_restRevision = -1;
}

bool SkinRenderer::uploadRest(const AvatarSkin& skin)
{
  releaseRest();
  const int jointCount = skin.jointNames().size();

  for (const AvatarSkin::Part& part : skin.parts())
  {
    const int count = part.restVertices.size();
    const int influences = AvatarSkin::INFLUENCES;
    if (count == 0 || part.indices.isEmpty()
        || part.restNormals.size() != count
        || part.influenceJoints.size() != count * influences
        || part.influenceWeights.size() != count * influences
        || part.jointA.size() != count || part.jointB.size() != count || part.blend.size() != count)
      continue;

    QVector<float> floats(count * FLOATS_PER_VERTEX);
    QVector<qint32> ints(count * INTS_PER_VERTEX);
    for (int i = 0; i < count; i++)
    {
      float* f = floats.data() + i * FLOATS_PER_VERTEX;
      const QVector3D& p = part.restVertices[i];
      const QVector3D& n = part.restNormals[i];
      f[0] = p.x(); f[1] = p.y(); f[2] = p.z();
      f[3] = n.x(); f[4] = n.y(); f[5] = n.z();
      qint32* k = ints.data() + i * INTS_PER_VERTEX;
      for (int j = 0; j < influences; j++)
      {
        const int joint = part.influenceJoints[i * influences + j];
        const float weight = part.influenceWeights[i * influences + j];
        // the loaders check every index; this keeps the shader inside the buffer anyway
        const bool valid = joint >= 0 && joint < jointCount;
        k[j] = valid ? joint : 0;
        f[6 + j] = valid ? weight : 0.0f;
      }
      k[4] = qBound(0, part.jointA[i], jointCount - 1);
      k[5] = qBound(0, part.jointB[i], jointCount - 1);
      f[10] = part.blend[i];
    }

    PartBuffers buffers;
    buffers.vao = std::make_unique<QOpenGLVertexArrayObject>();
    buffers.floats = std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer);
    buffers.ints = std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer);
    buffers.indices = std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::IndexBuffer);
    if (!buffers.vao->create() || !buffers.floats->create() || !buffers.ints->create() || !buffers.indices->create())
      return false;

    buffers.vao->bind();
    buffers.floats->bind();
    buffers.floats->allocate(floats.constData(), floats.size() * int(sizeof(float)));
    const GLsizei floatStride = FLOATS_PER_VERTEX * sizeof(float);
    m_gl.glEnableVertexAttribArray(0);
    m_gl.glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, floatStride, reinterpret_cast<void*>(0));
    m_gl.glEnableVertexAttribArray(1);
    m_gl.glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, floatStride, reinterpret_cast<void*>(3 * sizeof(float)));
    m_gl.glEnableVertexAttribArray(3);
    m_gl.glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, floatStride, reinterpret_cast<void*>(6 * sizeof(float)));
    m_gl.glEnableVertexAttribArray(5);
    m_gl.glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, floatStride, reinterpret_cast<void*>(10 * sizeof(float)));

    buffers.ints->bind();
    buffers.ints->allocate(ints.constData(), ints.size() * int(sizeof(qint32)));
    const GLsizei intStride = INTS_PER_VERTEX * sizeof(qint32);
    m_gl.glEnableVertexAttribArray(2);
    m_gl.glVertexAttribIPointer(2, 4, GL_INT, intStride, reinterpret_cast<void*>(0));
    m_gl.glEnableVertexAttribArray(4);
    m_gl.glVertexAttribIPointer(4, 2, GL_INT, intStride, reinterpret_cast<void*>(4 * sizeof(qint32)));

    buffers.indices->bind();   // recorded in the vertex array object
    buffers.indices->allocate(part.indices.constData(), part.indices.size() * int(sizeof(quint16)));
    buffers.vao->release();
    buffers.ints->release();

    // which joint each triangle mostly belongs to, for picking
    QVector<qint32> triangles(part.indices.size() / 3, 0);
    for (int t = 0; t < triangles.size() && t < part.triangleJoint.size(); t++)
      triangles[t] = qBound(0, part.triangleJoint[t], jointCount - 1);
    m_gl.glGenBuffers(1, &buffers.triangleBuffer);
    m_gl.glGenTextures(1, &buffers.triangleTexture);
    if (!buffers.triangleBuffer || !buffers.triangleTexture) return false;
    m_gl.glBindBuffer(GL_TEXTURE_BUFFER, buffers.triangleBuffer);
    m_gl.glBufferData(GL_TEXTURE_BUFFER, triangles.size() * sizeof(qint32), triangles.constData(), GL_STATIC_DRAW);
    m_gl.glBindBuffer(GL_TEXTURE_BUFFER, 0);

    buffers.indexCount = part.indices.size();
    buffers.eye = part.eye;
    m_parts.push_back(std::move(buffers));
  }

  m_restSkin = &skin;
  m_restRevision = skin.restRevision();
  return true;
}

void SkinRenderer::draw(const AvatarSkin& skin, const QVector<QMatrix4x4>& positions,
                        const QVector<QMatrix3x3>& normals, const QVector<QVector4D>& jointColors,
                        const QVector<unsigned int>& pickNames, bool picking,
                        const QMatrix4x4& projection, const Fog& fog)
{
  if (!m_ready) return;
  const int jointCount = skin.jointNames().size();
  if (jointCount == 0 || positions.size() != jointCount || normals.size() != jointCount
      || jointColors.size() != jointCount || pickNames.size() != jointCount)
    return;

  if (m_restSkin != &skin || m_restRevision != skin.restRevision())
  {
    if (!uploadRest(skin))
    {
      releaseRest();
      return;
    }
  }

  // joint matrices and colours for this frame
  m_jointData.resize(jointCount * TEXELS_PER_JOINT * 4);
  for (int j = 0; j < jointCount; j++)
  {
    float* t = m_jointData.data() + j * TEXELS_PER_JOINT * 4;
    const float* m = positions[j].constData();   // column major
    for (int i = 0; i < 16; i++) t[i] = m[i];
    const float* n = normals[j].constData();     // column major, 3 x 3
    for (int column = 0; column < 3; column++)
    {
      t[16 + column * 4 + 0] = n[column * 3 + 0];
      t[16 + column * 4 + 1] = n[column * 3 + 1];
      t[16 + column * 4 + 2] = n[column * 3 + 2];
      t[16 + column * 4 + 3] = 0.0f;
    }
    t[28] = jointColors[j].x();
    t[29] = jointColors[j].y();
    t[30] = jointColors[j].z();
    t[31] = jointColors[j].w();
    // whole numbers up to 2^24 are exact in a float
    t[32] = float(qMin(pickNames[j], MAX_PICK_NAME));
    t[33] = t[34] = t[35] = 0.0f;
  }
  m_gl.glBindBuffer(GL_TEXTURE_BUFFER, m_jointBuffer);
  m_gl.glBufferData(GL_TEXTURE_BUFFER, m_jointData.size() * sizeof(float), m_jointData.constData(), GL_STREAM_DRAW);
  m_gl.glBindBuffer(GL_TEXTURE_BUFFER, 0);
  m_gl.glActiveTexture(GL_TEXTURE0);
  m_gl.glBindTexture(GL_TEXTURE_BUFFER, m_jointTexture);
  m_texBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, m_jointBuffer);

  m_program->bind();
  m_program->setUniformValue("projection", projection);
  m_program->setUniformValue("picking", picking);
  m_program->setUniformValue("unlit", false);
  m_program->setUniformValue("shadow", m_shadowPass);
  if (m_shadowPass)
  {
    m_program->setUniformValue("view", m_shadowView);
    m_program->setUniformValue("viewInverse", m_shadowView.inverted());
    m_program->setUniformValue("shadowDirection", m_shadowDirection);
    m_program->setUniformValue("shadowStrength", m_shadowStrength);
  }
  setLookUniforms(m_program.get(), fog);
  for (PartBuffers& part : m_parts)
  {
    m_program->setUniformValue("eye", part.eye);
    if (picking)
    {
      m_gl.glActiveTexture(GL_TEXTURE1);
      m_gl.glBindTexture(GL_TEXTURE_BUFFER, part.triangleTexture);
      m_texBuffer(GL_TEXTURE_BUFFER, GL_R32I, part.triangleBuffer);
    }
    part.vao->bind();
    m_gl.glDrawElements(GL_TRIANGLES, part.indexCount, GL_UNSIGNED_SHORT, nullptr);
    part.vao->release();
  }
  m_program->release();
  if (picking)
  {
    m_gl.glActiveTexture(GL_TEXTURE1);
    m_gl.glBindTexture(GL_TEXTURE_BUFFER, 0);
    m_gl.glActiveTexture(GL_TEXTURE0);
  }
  m_gl.glBindTexture(GL_TEXTURE_BUFFER, 0);
}
