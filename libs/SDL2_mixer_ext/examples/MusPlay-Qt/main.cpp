#include "SDL.h"
#ifdef USE_SDL_MIXER_X
#   include "SDL_mixer_ext.h"
#else
#   include "SDL_mixer.h"
#endif

#if (SDL_MIXER_MAJOR_VERSION > 2) || (SDL_MIXER_MAJOR_VERSION == 2 && SDL_MIXER_MINOR_VERSION >= 1)
#   define SDL_MIXER_GE21
#endif

#include <locale.h>
#include <QApplication>
#include <QtDebug>
#include <QMessageBox>
#include <QDir>
#include <QFileInfo>
#include "SingleApplication/singleapplication.h"
#include "SingleApplication/pge_application.h"
#include "MainWindow/musplayer_qt.h"
#include "Player/mus_player.h"
#include "version.h"

static void error(QString msg)
{
    QMessageBox::warning(nullptr, "SDL error", msg, QMessageBox::Ok);
}

extern "C"
int main(int argc, char *argv[])
{
    QApplication::addLibraryPath(".");
    QApplication::addLibraryPath(QFileInfo(QString::fromUtf8(argv[0])).dir().path());
    QApplication::addLibraryPath(QFileInfo(QString::fromLocal8Bit(argv[0])).dir().path());

    QApplication::setOrganizationName(V_COMPANY);
    QApplication::setOrganizationDomain(V_PGE_URL);
    QApplication::setApplicationName("PGE Music Player");

    PGE_Application a(argc, argv);
    // https://doc.qt.io/qt-5/qcoreapplication.html#locale-settings
    setlocale(LC_NUMERIC, "C");

    QStringList args;
#ifdef _WIN32
    for(int i = 0; i < argc; i++)
        args.push_back(QString::fromUtf8(argv[i]));
#else
    args = a.arguments();
#endif

    SingleApplication *as = new SingleApplication(args);
    if(!as->shouldContinue())
    {
        QTextStream(stdout) << "SDL2 Mixer X Player already runned!\n";
        delete as;
        return 0;
    }
#ifdef Q_OS_LINUX
    a.setStyle("GTK");
#endif

#if defined(SDL_MIXER_GE21)
    QString timidityPath(a.applicationDirPath() + "/timidity/");
    if(QDir(timidityPath).exists())
    {
        qDebug() << "Timidity path is" << timidityPath;
        QByteArray tp = timidityPath.toUtf8();
        Mix_SetTimidityCfg(tp.data());
    }
#endif

    PGE_MusicPlayer::loadAudioSettings();

    QString mixErr;
    if(!PGE_MusicPlayer::openAudio(mixErr))
        error(mixErr);

#if defined(SDL_MIXER_GE21)
    //Disallow auto-resetting MIDI properties (to allow manipulation with MIDI settings by functions)
    Mix_SetLockMIDIArgs(1);
#endif

    MusPlayer_Qt w;
    //Set acception of external file openings
    w.connect(as, SIGNAL(openFile(QString)), &w, SLOT(openMusicByArg(QString)));
#ifdef __APPLE__
    w.connect(&a, SIGNAL(openFileRequested(QString)), &w, SLOT(openMusicByArg(QString)));
    a.setConnected();
#endif

    w.show();
    if(args.size() > 1)
        w.openMusicByArg(args[1]);
#   ifdef __APPLE__
    {
        QStringList argx = a.getOpenFileChain();
        if(!argx.isEmpty())
            w.openMusicByArg(argx[0]);
    }
#   endif
    int result = a.exec();
    delete as;

    PGE_MusicPlayer::closeAudio();

    SDL_Quit();

    return result;
}
