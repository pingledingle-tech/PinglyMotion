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

#include <QSurfaceFormat>
#include <QApplication>

#include <QTimer>
#ifndef Q_OS_WIN
#include <csignal>
#endif

#include "applog.h"
#include "pinglymotionwindow.h"
#include "constants.h"
#include "textsize.h"
#include "translation.h"
#include "usersettings.h"

#ifndef Q_OS_WIN
// Ending the program from outside (kill, system shutdown) should end it properly,
// so the remote control key file is removed. A signal handler may only set a flag;
// a timer in the program checks it.
static volatile std::sig_atomic_t endRequested = 0;
static void requestEnd(int) { endRequested = 1; }
#endif

int main( int argc, char ** argv )
{
    // the local error log (issue 239), before anything else can go wrong
    AppLog::install();
#ifndef Q_OS_WIN
    // first thing, before the window exists and the remote control key file is written
    std::signal(SIGTERM, requestEnd);
    std::signal(SIGINT, requestEnd);
#endif

    // the 3D view is drawn with OpenGL 3.3 shaders: ask for a modern (core)
    // context with depth and stencil (the floor shadow uses the stencil)
    QSurfaceFormat format;
#ifdef Q_OS_WASM
    // web build: WebGL 2 is OpenGL ES 3.0 (docs/DEV.md, "The web trial build")
    format.setRenderableType(QSurfaceFormat::OpenGLES);
    format.setVersion(3, 0);
#else
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
#endif
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    QSurfaceFormat::setDefaultFormat(format);

    QApplication application(argc, argv);
    qInfo("PinglyMotion %s", qUtf8Printable(Constants::buildLabel()));
    // the language of the texts (docs/TRANSLATION.md), before any window makes a text: the
    // setting "language" (default: the computer's language), English when no file matches
    UserSettings::readSettings();
    const QString language = Translation::install(application, UserSettings::language());
    qInfo("Language: %s", language.isEmpty() ? "English" : qUtf8Printable(language));
    // the text size, 100 to 200 percent (src/textsize.h), before any window measures a text
    TextSize::apply(UserSettings::textSizePercent());
    PinglyMotionWindow* pinglyMotionWindow = new PinglyMotionWindow();
    // Queue an event to show the main window
    pinglyMotionWindow->show();
    // Queue an event to execute a method after the main window shows
    pinglyMotionWindow->queueAfterShow();
#ifndef Q_OS_WIN
    QTimer endTimer;
    QObject::connect(&endTimer, &QTimer::timeout, [&application]() { if (endRequested) application.quit(); });
    endTimer.start(300);
#endif
    // Start the event loop
    const int result = application.exec();
    AppLog::markCleanExit();
    return result;
}
