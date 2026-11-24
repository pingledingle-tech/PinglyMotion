/*
 * PinglyMotion - draws the skinned avatar body with OpenGL 3.3 shaders
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef SKINRENDERER_H
#define SKINRENDERER_H

#include <QGenericMatrix>
#include <QMatrix4x4>
#include <QOpenGLBuffer>
#include <QOpenGLExtraFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QVector>
#include <QVector3D>
#include <QVector4D>

#include <map>
#include <memory>
#include <vector>

#include "scenestate.h"

class AvatarSkin;
class Mesh;

// Draws the 3D view with OpenGL 3.3 shaders (issue #61): the avatar body,
// meshes (balls, props, handles), lines, rings, floor and the floor shadow.
// The rest pose of every mesh part sits in vertex buffers on the graphics
// card and is only sent again when it changes (AvatarSkin::restRevision()).
// Each frame only the joint matrices and joint colours are sent; the shader
// bends the mesh (skinning) and lights it with studio lights.
class SkinRenderer
{
  public:
    using Fog = SceneState::Fog;

    // Call with the view's context current. Returns false when the context
    // is older than OpenGL 3.3 or a shader does not build; status() says why.
    bool init();
    // frees everything on the graphics card; call with the context current
    void cleanup();
    bool isReady() const { return m_ready; }

    // Options > Light...: each 0 to 1, 0.5 is the standard look.
    struct Look
    {
      float brightness = 0.5f;
      float shine = 0.5f;
      float warmth = 0.5f;
      bool shadow = true;   // shadow of the figure on the floor
    };
    void setLook(const Look& look) { m_look = look; }
    // short text for the log, e.g. "OpenGL 4.5 shaders" or the reason for the old path
    QString status() const { return m_status; }

    // positions/normals: AvatarSkin::skinMatrices(), eye space;
    // jointColors and pickNames: one per entry of AvatarSkin::jointNames().
    // With picking on, every triangle is drawn in the colour that encodes
    // the pick name of the joint it mostly belongs to (Part::triangleJoint),
    // see encodePickName(); lights and fog are not used then.
    void draw(const AvatarSkin& skin, const QVector<QMatrix4x4>& positions,
              const QVector<QMatrix3x3>& normals, const QVector<QVector4D>& jointColors,
              const QVector<unsigned int>& pickNames, bool picking,
              const QMatrix4x4& projection, const Fog& fog);

    // Draws a simple mesh (Bento ball, prop, handle) with the same light as
    // the body. modelview/projection: the current OpenGL matrices; lit false
    // gives the plain colour (lighting was off for it in the old path).
    void drawMesh(const Mesh& mesh, const QVector4D& color, const QMatrix4x4& modelview,
                  const QMatrix4x4& projection, const Fog& fog, bool lit);

    // Draws lines or triangles given point by point (rings, skeleton lines,
    // floor). normals empty: plain colours, no light. colors: one per point.
    // Lines are drawn as thin strips facing the screen, lineWidth pixels wide,
    // two triangles each (no geometry shader, so WebGL 2 can draw them too).
    void drawGeometry(GLenum mode, const QVector<QVector3D>& positions,
                      const QVector<QVector3D>& normals, const QVector<QVector4D>& colors,
                      const QMatrix4x4& modelview, const QMatrix4x4& projection, const Fog& fog,
                      float lineWidth = 1.0f);

    // Draws the body flattened onto the floor (y = 0 in the world) along a
    // light from above, as a soft dark shadow. view: the camera matrix
    // without any model transformation. Call after the floor is drawn.
    void drawShadow(const AvatarSkin& skin, const QVector<QMatrix4x4>& positions,
                    const QVector<QMatrix3x3>& normals, const QMatrix4x4& view,
                    const QMatrix4x4& projection, const Fog& fog);

    // pick names travel as colours: red is the lowest byte, then green, then blue
    static QVector4D encodePickName(unsigned int name);
    static unsigned int decodePickName(const unsigned char* rgba);
    static constexpr unsigned int MAX_PICK_NAME = 0xFFFFFF;

    // The first lines of every shader: "#version 330 core" on the desktop,
    // "#version 300 es" with precisions for WebGL 2 (the later web version).
    static QByteArray shaderHeader(bool web);
    // every shader with its header, for checks (tests/linedraw): name with a
    // .vert or .frag ending, and the source
    struct ShaderSource
    {
      QString name;
      QByteArray source;
    };
    static QVector<ShaderSource> shaderSources(bool web);

  private:
    struct PartBuffers
    {
      std::unique_ptr<QOpenGLVertexArrayObject> vao;
      std::unique_ptr<QOpenGLBuffer> floats;    // rest position, rest normal, weights, colour blend
      std::unique_ptr<QOpenGLBuffer> ints;      // influence joints, colour joints A and B
      std::unique_ptr<QOpenGLBuffer> indices;
      int indexCount = 0;
      bool eye = false;
      // Part::triangleJoint, read by the picking shader with gl_PrimitiveID
      GLuint triangleBuffer = 0;
      GLuint triangleTexture = 0;
    };

    struct MeshBuffers
    {
      std::unique_ptr<QOpenGLVertexArrayObject> vao;
      std::unique_ptr<QOpenGLBuffer> vertices;   // position 3, normal 3
      int count = 0;
      int revision = -1;
    };

    bool uploadRest(const AvatarSkin& skin);
    void releaseRest();
    void setLookUniforms(QOpenGLShaderProgram* program, const Fog& fog) const;
    void drawLines(const QVector<QVector3D>& positions, const QVector<QVector3D>& normals,
                   const QVector<QVector4D>& colors, const QMatrix4x4& modelview,
                   const QMatrix4x4& projection, const Fog& fog, float lineWidth);

    bool m_ready = false;
    QString m_status;
    Look m_look;
    bool m_shadowPass = false;
    QMatrix4x4 m_shadowView;
    // from above, a little from the front right, so the shadow falls behind
    QVector3D m_shadowDirection = QVector3D(-0.30f, -1.0f, -0.35f).normalized();
    float m_shadowStrength = 0.38f;
    QOpenGLExtraFunctions m_gl;
    void (QOPENGLF_APIENTRYP m_texBuffer)(GLenum, GLenum, GLuint) = nullptr;
    // made again by every init(): Qt makes a new context when the view is
    // moved to another window, and the old objects belong to the old one
    std::unique_ptr<QOpenGLShaderProgram> m_program;
    std::unique_ptr<QOpenGLShaderProgram> m_meshProgram;
    std::unique_ptr<QOpenGLShaderProgram> m_lineProgram;   // lines as strips of two triangles
    std::map<const Mesh*, MeshBuffers> m_meshes;
    // for drawGeometry(), filled anew for every call
    std::unique_ptr<QOpenGLVertexArrayObject> m_geometryVao;
    std::unique_ptr<QOpenGLBuffer> m_geometryBuffer;   // position 3, normal 3, colour 4
    QVector<float> m_geometryData;
    std::unique_ptr<QOpenGLVertexArrayObject> m_lineVao;
    std::unique_ptr<QOpenGLBuffer> m_lineBuffer;        // four corners per line, see drawLines()
    std::unique_ptr<QOpenGLBuffer> m_lineIndexBuffer;   // six indices per line
    int m_lineIndexCount = 0;                            // indices in m_lineIndexBuffer
    QVector<float> m_lineData;
    std::vector<PartBuffers> m_parts;
    const AvatarSkin* m_restSkin = nullptr;
    int m_restRevision = -1;
    // joint matrices and colours, read by the shader as a buffer texture
    GLuint m_jointBuffer = 0;
    GLuint m_jointTexture = 0;
    QVector<float> m_jointData;
};

#endif
