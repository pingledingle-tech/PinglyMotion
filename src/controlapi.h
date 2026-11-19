/*
 * PinglyMotion control interface - the commands
 *
 * An open, local interface so that any program (and any AI tool) can pose and
 * play the animation that is open in PinglyMotion. This file holds the commands
 * only; controlserver.cpp carries them over a local HTTP connection.
 *
 * Frame numbers in this interface start at 1, like the frame counter shown in
 * the program. Angles are degrees, as in the rotation fields.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef CONTROLAPI_H
#define CONTROLAPI_H

#include <QImage>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QVector3D>

class Animation;
class BVHNode;

// What the commands need from the program. The window implements it; the
// automatic tests use a stand-in.
class ControlHost
{
  public:
    virtual ~ControlHost() {}
    virtual Animation* animation() = 0;
    // move to a frame (0-based) and let the program follow
    virtual void goToFrame(int frame) = 0;
    virtual void refresh() = 0;
    virtual void selectJoint(const QString& name) = 0;
    // a picture of the 3D view, drawn at this size
    virtual QImage screenshot(int width, int height) = 0;
    virtual bool newAnimation(QString& error) = 0;
    virtual bool openFile(const QString& path, QString& error) = 0;
    virtual bool saveFile(const QString& path, QString& error) = 0;
    virtual void setPlaying(bool on) = 0;
    virtual bool playing() = 0;
    virtual QString fileName() = 0;
    // true when any open animation has changes that are not saved
    virtual bool unsavedChanges() = 0;
    // which figure is shown: false = female, true = male (display only, the BVH is the same)
    virtual bool male() = 0;
    virtual void setMale(bool male) = 0;
    // camera around the middle of the figure: height and angle in degrees, distance in scene units
    virtual void setCamera(float height, float angle, float distance) = 0;
    virtual void resetCamera() = 0;
    // current camera as height, angle, distance
    virtual QVector3D camera() = 0;
};

class ControlApi
{
  public:
    explicit ControlApi(ControlHost* host);

    // Runs one command, e.g. {"command":"set_pose","joints":{...}}.
    // The answer always has "ok": true or false; on false there is an "error".
    QJsonObject execute(const QJsonObject& request);

    // name and description of every command, for help texts and MCP
    static QJsonArray commandList();

    // open_file and save_file only work inside this folder: PinglyMotion in the
    // user's Documents folder, or PINGLYMOTION_CONTROL_FOLDER if set
    void setWorkFolder(const QString& folder) { m_workFolder = folder; }
    static QString defaultWorkFolder();

  private:
    typedef QJsonObject (ControlApi::*Handler)(const QJsonObject&);

    QJsonObject state(const QJsonObject&);
    QJsonObject joints(const QJsonObject&);
    QJsonObject getPose(const QJsonObject&);
    QJsonObject setPose(const QJsonObject&);
    QJsonObject setPosition(const QJsonObject&);
    QJsonObject deleteKey(const QJsonObject&);
    QJsonObject keys(const QJsonObject&);
    QJsonObject setFrame(const QJsonObject&);
    QJsonObject setFrameCount(const QJsonObject&);
    QJsonObject setFps(const QJsonObject&);
    QJsonObject play(const QJsonObject&);
    QJsonObject stop(const QJsonObject&);
    QJsonObject newAnimation(const QJsonObject&);
    QJsonObject openFile(const QJsonObject&);
    QJsonObject saveFile(const QJsonObject&);
    QJsonObject screenshot(const QJsonObject&);
    QJsonObject setFigure(const QJsonObject&);
    QJsonObject setCamera(const QJsonObject&);

    BVHNode* findJoint(const QString& name, QString& error);
    QStringList jointNames();
    // goes to "frame" when the request has one; false and an error if it is out of range
    bool applyFrame(const QJsonObject& request, QString& error);
    // the first frame is the reference pose; PinglyMotion refuses edits there unless the option is off
    bool checkNotProtected(QString& error);
    QJsonObject fail(const QString& message);
    QJsonObject ok();
    // the absolute path of a file in the work folder; empty and an error if it is outside
    QString resolveInWorkFolder(const QString& path, bool mustExist, QString& error);

    ControlHost* m_host;
    QString m_workFolder;
};

#endif
