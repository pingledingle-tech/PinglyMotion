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

#include "scriptlab/labwatcher.h"
#include "scriptlab/rlvstate.h"
#include "scriptlab/lsl/session.h"

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
    // like openFile, but parses these bytes instead of reading the file again
    // (load_reference: the bytes it checked against the library index)
    virtual bool openFileData(const QString& path, const QByteArray& data, QString& error) = 0;
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
    // set_camera with a joint: where the joint is now (current frame), in the 3D view's scene
    // units. The default is the pose maths (PoseDescribe::worldTransforms); the window gives
    // where the 3D view draws it, the point the Picked joint framing (F key) looks at.
    virtual QVector3D jointPosition(BVHNode* joint);
    // the camera at height and angle (degrees), looking at "target" from "distance" instead of
    // at the middle of the figure. The default only turns the camera (setCamera).
    virtual void setCameraFocus(float height, float angle, float distance, const QVector3D& target);
    // loop points as frame numbers (first frame is 1), already checked by the command
    virtual void setLoopPoints(int loopIn, int loopOut);
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

    // open_file, save_file and export_anim only work inside this folder: PinglyMotion in the
    // user's Documents folder, or PINGLYMOTION_CONTROL_FOLDER if set
    void setWorkFolder(const QString& folder) { m_workFolder = folder; }
    static QString defaultWorkFolder();
    // Checks a file name for open_file, save_file and export_anim by its text alone, before any
    // file system call: refuses absolute paths outside the work folder, network names, drive
    // letters, ':', '..' and device names (CON, NUL, COM1 ...). On success "relative" is the name
    // relative to the work folder (issue 244).
    static bool plainNameInWorkFolder(const QString& workFolder, const QString& path, QString& relative);

    // find_reference and load_reference read the reference library (library/index.json, issue 19)
    // and the index that tools/fetch_reference.py writes (downloaded_index.json). load_reference
    // opens only files inside these two folders, never a path from the caller.
    // Defaults: PINGLYMOTION_LIBRARY_FOLDER or library/ next to the program; and
    // PINGLYMOTION_REFERENCE_FOLDER or PinglyMotion-reference in the home folder.
    void setLibraryFolder(const QString& folder) { m_libraryFolder = folder; }
    void setReferenceFolder(const QString& folder) { m_referenceFolder = folder; }
    static QString defaultLibraryFolder();
    static QString defaultReferenceFolder();

    // the Script Lab's RLV state that rlv_command, rlv_state and rlv_reset work on (issue 287)
    RlvState& rlv() { return m_rlv; }
    // the world of the last lsl_run, for lsl_event and the panel's object list (Script Lab step 5)
    lsl::Session& lslSession() { return m_lslSession; }
    // the Script Lab's folder watch that lab_watch, lab_unwatch and lab_result work on (issue 292)
    LabWatcher& lab() { return m_lab; }

  private:
    typedef QJsonObject (ControlApi::*Handler)(const QJsonObject&);

    QJsonObject state(const QJsonObject&);
    QJsonObject joints(const QJsonObject&);
    QJsonObject getPose(const QJsonObject&);
    QJsonObject setPose(const QJsonObject&);
    QJsonObject setPosition(const QJsonObject&);
    QJsonObject deleteKey(const QJsonObject&);
    QJsonObject keys(const QJsonObject&);
    QJsonObject setLoop(const QJsonObject&);
    QJsonObject setFrame(const QJsonObject&);
    QJsonObject setFrameCount(const QJsonObject&);
    QJsonObject setFps(const QJsonObject&);
    QJsonObject play(const QJsonObject&);
    QJsonObject stop(const QJsonObject&);
    QJsonObject newAnimation(const QJsonObject&);
    QJsonObject openFile(const QJsonObject&);
    QJsonObject saveFile(const QJsonObject&);
    QJsonObject screenshot(const QJsonObject&);
    QJsonObject screenshotSheet(const QJsonObject&);
    QJsonObject pngAnswer(const QImage& image);
    QJsonObject setFigure(const QJsonObject&);
    QJsonObject setCamera(const QJsonObject&);
    QJsonObject describePose(const QJsonObject&);
    QJsonObject describeMotion(const QJsonObject&);
    QJsonObject describeBody(const QJsonObject&);
    QJsonObject motionScript(const QJsonObject&);
    QJsonObject loadMotionScript(const QJsonObject&);
    QJsonObject checkAnimation(const QJsonObject&);
    QJsonObject exportAnim(const QJsonObject&);
    QJsonObject findReference(const QJsonObject&);
    QJsonObject loadReference(const QJsonObject&);
    QJsonObject rlvCommand(const QJsonObject&);
    QJsonObject rlvStateAnswer(const QJsonObject&);
    QJsonObject rlvReset(const QJsonObject&);
    QJsonObject labWatch(const QJsonObject&);
    QJsonObject labUnwatch(const QJsonObject&);
    QJsonObject labResult(const QJsonObject&);
    QJsonObject labReplay(const QJsonObject&);

    BVHNode* findJoint(const QString& name, QString& error);
    QStringList jointNames();
    // goes to "frame" when the request has one; false and an error if it is out of range
    bool applyFrame(const QJsonObject& request, QString& error);
    // goes to a frame (0-based) and makes sure the animation is on it
    void goToFrame(int frame);
    // the first frame is the reference pose; PinglyMotion refuses edits there unless the option is off
    bool checkNotProtected(QString& error);
    QJsonObject fail(const QString& message);
    QJsonObject ok();
    // the absolute path of a file in the work folder; empty and an error if it is outside
    QString resolveInWorkFolder(const QString& path, bool mustExist, QString& error);

    ControlHost* m_host;
    QString m_workFolder;
    QString m_libraryFolder;
    QString m_referenceFolder;
    RlvState m_rlv;
    lsl::Session m_lslSession;     // after m_rlv: its world refers to m_rlv
    LabWatcher m_lab;
};

#endif
