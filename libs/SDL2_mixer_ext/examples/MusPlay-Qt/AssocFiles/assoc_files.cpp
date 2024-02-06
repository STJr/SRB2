#include "assoc_files.h"
#include "ui_assoc_files.h"
#include <QListWidgetItem>
#include <QList>
#include <QPair>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QSettings>
#include <QProcess>
#include <QMessageBox>
#include <QtDebug>

struct tentry
{
    QString ext;
    QString description;
    QString file_desc;
};

AssocFiles::AssocFiles(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AssocFiles)
{
    ui->setupUi(this);
    QList<tentry> formats;
    formats << tentry{"wav",  "WAV - Uncompressed PCM Audio", tr("PCM Audio File", "File Type Name")};
    formats << tentry{"voc",  "VOC - Creative Labs Audio File", tr("Creative Labs Audio File", "File Type Name")};
    formats << tentry{"mp3",  "MP3 - MPEG-1/2/2.5 Layer 3, Lossy data compressed audio", tr("MPEG Layer-3 Audio File", "File Type Name")};
    formats << tentry{"ogg",  "OGG - OGG Vorbis, Lossy data compressed audio", tr("OGG Vorbis File", "File Type Name")};
    formats << tentry{"opus", "OPUS - OGG Opus, Lossy data compressed audio", tr("OPUS Audio File", "File Type Name")};
    formats << tentry{"flac", "FLAC - Free Lossless Audio Codec, Loss-less compressed", tr("FLAC Audio File", "File Type Name")};
    formats << tentry{"mid",  "mid - Music Instrument Digital Interface, commands list", tr("MIDI File", "File Type Name")};
    formats << tentry{"midi", "midi - Music Instrument Digital Interface, commands list", tr("MIDI File", "File Type Name")};
    formats << tentry{"kar",  "kar - Music Instrument Digital Interface, commands list", tr("MIDI Karaoke File", "File Type Name")};
    formats << tentry{"rmi",  "RMI - Music Instrument Digital Interface, commands list", tr("RIFF MIDI File", "File Type Name")};
    formats << tentry{"xmi",  "XMI - AIL eXtended MIDI file", tr("AIL eXtended MIDI File", "File Type Name")};
    formats << tentry{"mus",  "MUS - DMX MIDI Music file", tr("DMX MIDI File", "File Type Name")};
    formats << tentry{"cmf",  "CMF - Creative Music File (mixed MIDI and OPL2 synth)", tr("Creative MIDI+OPL2 File", "File Type Name")};

    formats << tentry{"ay",   "AY - ZX Spectrum/Amstrad CPC", tr("ZX Spectrum/Amstrad Music", "File Type Name")};
    formats << tentry{"gbs",  "GBS - Nintendo Game Boy", tr("Nintendo Game Boy Music", "File Type Name")};
    formats << tentry{"gym",  "GYM - Sega Genesis/Mega Drive", tr("Sega Music", "File Type Name")};
    formats << tentry{"hes",  "HES - NEC TurboGrafx-16/PC Engine", tr("NEC TurboGrafx-16 Music", "File Type Name")};
    formats << tentry{"kss",  "KSS - Z80 systems music", tr("Z80 systems Music file", "File Type Name")};
    formats << tentry{"nsf",  "NSF - Nintendo NES/Famicom", tr("NES/Famicom Music File", "File Type Name")};
    formats << tentry{"nsfe", "NSFE - Nintendo NES/Famicom", tr("NES/Famicom Music File", "File Type Name")};
    formats << tentry{"sap",  "SAP - Atari (POKEY)", tr("Atari POKEY music", "File Type Name")};
    formats << tentry{"spc",  "SPC - Super NES/Famicom Music", tr("Super NES/Famicom Music", "File Type Name")};
    formats << tentry{"vgm",  "VGM - Video Game Music", tr("Video Game Music", "File Type Name")};
    formats << tentry{"vgz",  "VGZ - Video Game Music (GZ Compressed)", tr("Video Game Music (GZ-compressed)", "File Type Name")};

    formats << tentry{"669",  "669 - Composer 669, Unis 669", tr("Composer 669, Unis 669", "File Type Name")};
    formats << tentry{"amf",  "AMF - ASYLUM Music Format V1.0/DSMI Advanced Module Format", tr("ASYLUM Music Format", "File Type Name")};
    formats << tentry{"apun", "APUN - APlayer", tr("APlayer Music File", "File Type Name")};
    formats << tentry{"dsm",  "DSM - DSIK internal format", tr("DSIK Music File", "File Type Name")};
    formats << tentry{"far",  "FAR - Farandole Composer", tr("Farandole Composer File", "File Type Name")};
    formats << tentry{"gdm",  "GDM - General DigiMusic", tr("General DigiMusic File", "File Type Name")};
    formats << tentry{"it",   "IT - Impulse Tracker", tr("Impulse Tracker File", "File Type Name")};
    formats << tentry{"mptm", "MPTM - Open ModPlug Tracker", tr("Open ModPlug Tracker Music", "File Type Name")};
    formats << tentry{"imf",  "IMF - Imago Orpheus/Id Music File", tr("Imago Orpheus/Id Music File", "File Type Name")};
    formats << tentry{"mod",  "MOD - 15 and 31 instruments", tr("Module Music file", "File Type Name")};
    formats << tentry{"med",  "MED - OctaMED", tr("OctaMED Music File", "File Type Name")};
    formats << tentry{"mtm",  "MTM - MultiTracker Module editor", tr("MultiTracker Module File", "File Type Name")};
    formats << tentry{"okt",  "OKT - Amiga Oktalyzer", tr("Amiga Oktalyzer File", "File Type Name")};
    formats << tentry{"s3m",  "S3M - Scream Tracker 3", tr("Scream Tracker 3 File", "File Type Name")};
    formats << tentry{"stm",  "STM - Scream Tracker", tr("Scream Tracker File", "File Type Name")};
    formats << tentry{"stx",  "STX - Scream Tracker Music Interface Kit", tr("Scream Tracker Music", "File Type Name")};
    formats << tentry{"ult",  "ULT - UltraTracker", tr("UltraTracker Music", "File Type Name")};
    formats << tentry{"uni",  "UNI - MikMod", tr("MikMod Music", "File Type Name")};
    formats << tentry{"xm",   "XM - FastTracker 2", tr("FastTracker 2 Music", "File Type Name")};

    formats << tentry{"pttune",   "PTTUNE - PXTONE Tune", tr("PXTONE Tune", "File Type Name")};
    formats << tentry{"ptcop",   "PTTUNE - PXTONE Collage Project", tr("PXTONE Collage Project", "File Type Name")};

    for(int i=0; i<formats.size(); i++)
    {
        const tentry& e = formats[i];
        QListWidgetItem item(e.description, ui->typeList);
        item.setData(Qt::UserRole, e.ext);
        item.setData(Qt::UserRole + 1, e.file_desc);
        item.setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable|Qt::ItemIsUserCheckable);
        items.append(item);
        ui->typeList->addItem(&items.last());
    }
    ui->typeList->sortItems(Qt::AscendingOrder);
    on_reset_clicked();
}

void AssocFiles::on_reset_clicked()
{
    QHash<QString, bool> defaultFormats;

    defaultFormats["kar"] = true;

    defaultFormats["xmi"] = true;
    defaultFormats["mus"] = true;
    defaultFormats["cmf"] = true;

    defaultFormats["voc"] = true;
    defaultFormats["ay"]  = true;
    defaultFormats["gbs"] = true;
    defaultFormats["gym"] = true;
    defaultFormats["hes"] = true;
    defaultFormats["kss"] = true;
    defaultFormats["nsf"] = true;
    defaultFormats["nsfe"]= true;
    defaultFormats["sap"] = true;
    defaultFormats["spc"] = true;
    defaultFormats["vgm"] = true;
    defaultFormats["vgz"] = true;

    defaultFormats["669"] = true;
    defaultFormats["amf"] = true;
    defaultFormats["apun"]= true;
    defaultFormats["dsm"] = true;
    defaultFormats["far"] = true;
    defaultFormats["gdm"] = true;
    defaultFormats["it"]  = true;
    defaultFormats["mptm"]= true;
    defaultFormats["imf"] = true;
    defaultFormats["mod"] = true;
    defaultFormats["med"] = true;
    defaultFormats["mtm"] = true;
    defaultFormats["okt"] = true;
    defaultFormats["s3m"] = true;
    defaultFormats["stm"] = true;
    defaultFormats["stx"] = true;
    defaultFormats["ult"] = true;
    defaultFormats["uni"] = true;
    defaultFormats["xm"]  = true;

    defaultFormats["pttune"]  = true;
    defaultFormats["ptcop"]  = true;

    for(int i=0; i<items.size(); i++)
    {
        items[i].setCheckState(
                    defaultFormats.contains(items[i].data(Qt::UserRole).toString()) ?
                        Qt::Checked : Qt::Unchecked );

    }
}

void AssocFiles::on_selectAll_clicked()
{
    for(int i=0; i<items.size(); i++)
    {
        items[i].setCheckState( Qt::Checked );
    }
}

void AssocFiles::on_clear_clicked()
{
    for(int i=0; i<items.size(); i++)
    {
        items[i].setCheckState( Qt::Unchecked );
    }
}

AssocFiles::~AssocFiles()
{
    delete ui;
}

#if defined(__linux__)
static bool xCopyFile(const QString &src, const QString &target)
{
    QFile tmp;
    tmp.setFileName(target);
    if(tmp.exists())
        tmp.remove();
    bool ret = QFile::copy(src, target);
    if(!ret)
        qWarning() << "Failed to copy file" << src << "into" << target;
    else
        qDebug() << "File " << src << "was successfully copiled into" << target;
    return ret;
}

static bool xRunCommand(QString command, const QStringList &args)
{
    QProcess xdg;
    xdg.setProgram(command);
    xdg.setArguments(args);

    xdg.start();
    xdg.waitForFinished();

    QString printed = xdg.readAll();

    if(!printed.isEmpty())
        qDebug() << printed;

    bool ret = (xdg.exitCode() == 0);
    if(!ret)
        qWarning() << "Command" << command << "with arguments" << args << "finished with failure";
    else
        qDebug() << "Command" << command << "with arguments" << args << "successfully finished";

    return ret;
}

static bool xInstallIconResource(QString context, int iconSize, QString iconName, QString mimeName)
{
    QStringList args;
    args << "install";

    args << "--context";
    args << context;

    if(context == "apps")
        args << "--novendor";

    args << "--size";
    args << QString::number(iconSize);

    args << QDir::home().absolutePath() + "/.local/share/icons/" + iconName.arg(iconSize);
    args << mimeName;

    return xRunCommand("xdg-icon-resource", args);
}

static bool xIconSize(int iconSize)
{
    bool success = true;
    const QString home = QDir::home().absolutePath();
    if(success) success = xCopyFile(QString(":/cat_musplay/cat_musplay_%1x%1.png")
                                    .arg(iconSize),
                                    QString("%1/.local/share/icons/MoondustMusplay-%2.png")
                                    .arg(home).arg(iconSize));
    if(success) success = xCopyFile(QString(":/file_musplay/file_musplay_%1x%1.png")
                                    .arg(iconSize),
                                    QString("%1/.local/share/icons/moondust-music-file-%2.png")
                                    .arg(home).arg(iconSize));

    if(success) success = xInstallIconResource("mimetypes",
                                               iconSize,
                                               "moondust-music-file-%1.png",
                                               "x-application-moondust-music");

    if(success) success = xInstallIconResource("apps", iconSize, "MoondustMusplay-%1.png", "MoondustMusplay");

    return success;
}
#endif

void AssocFiles::on_AssocFiles_accepted()
{
    bool success = true;

#if defined(_WIN32)
    QSettings registry_hkcu("HKEY_CURRENT_USER", QSettings::NativeFormat);

    success = registry_hkcu.isWritable();

    //Main entry
    registry_hkcu.setValue("Software/Classes/PGEWohlstand.MusicFile/Default", tr("Moondust MusPlay Music File", "File Types"));
    registry_hkcu.setValue("Software/Classes/PGEWohlstand.MusicFile/DefaultIcon/Default", "\"" + QApplication::applicationFilePath().replace("/", "\\") + "\",1");
    registry_hkcu.setValue("Software/Classes/PGEWohlstand.MusicFile/Shell/Open/Command/Default", "\"" + QApplication::applicationFilePath().replace("/", "\\") + "\" \"%1\"");

    for(int i=0; i<items.size(); i++)
    {
        if(items[i].checkState()==Qt::Checked)
        {
            registry_hkcu.setValue(QString("Software/Classes/.%1/Default").arg(items[i].data(Qt::UserRole).toString()), "PGEWohlstand.MusicFile");
        }
    }

#elif defined(__linux__)
    const QString home = QDir::home().absolutePath();

    if(success) success = QDir().mkpath(home + "/.local/share/mime/packages");
    if(success) success = QDir().mkpath(home + "/.local/share/applications");
    if(success) success = QDir().mkpath(home + "/.local/share/icons");

    const QMap<QString, QString> magicTable =
    {
        {
            "mid",
            "        <magic priority=\"60\">\n"
            "            <match type=\"string\" offset=\"0\" value=\"MThd\"/>\n"
            "        </magic>\n"
        },
        {
            "midi",
            "        <magic priority=\"60\">\n"
            "            <match type=\"string\" offset=\"0\" value=\"MThd\"/>\n"
            "        </magic>\n"
        },
        {
            "kar",
            "        <magic priority=\"60\">\n"
            "            <match type=\"string\" offset=\"0\" value=\"MThd\"/>\n"
            "        </magic>\n"
        },
        {
            "rmi",
            "        <magic priority=\"60\">\n"
            "            <match type=\"string\" offset=\"0\" value=\"RIFF\">\n"
            "                <match type=\"string\" offset=\"8\" value=\"MThd\"/>\n"
            "            </match>\n"
            "        </magic>\n"
        },
        {
            "ogg",
            "        <magic priority=\"60\">\n"
            "            <match type=\"string\" offset=\"0\" value=\"OggS\"/>\n"
            "        </magic>\n"
        },
        {
            "flac",
            "        <magic priority=\"60\">\n"
            "            <match type=\"string\" offset=\"0\" value=\"fLaC\"/>\n"
            "        </magic>\n"
        },
        {
            "ptcop",
            "        <magic priority=\"60\">\n"
            "            <match type=\"string\" offset=\"0\" value=\"PTCOLLAGE\"/>\n"
            "        </magic>\n"
        },
        {
            "pttune",
            "        <magic priority=\"60\">\n"
            "            <match type=\"string\" offset=\"0\" value=\"PTTUNE\"/>\n"
            "        </magic>\n"
        }
    };

    QString mimeHead =
            "<?xml version=\"1.0\"?>\n"
            "<mime-info xmlns='http://www.freedesktop.org/standards/shared-mime-info'>\n";
    QString entryTemplate =
            "    <mime-type type=\"application/x-moondust-music-%1\">\n"
            "        <icon name=\"x-application-moondust-music\"/>\n"
            "        <comment>%2</comment>\n%3"
            "    </mime-type>\n";
    QString mimeFoot = "</mime-info>\n";
    QStringList entries;
    QString minesForDesktopFile;

    for(int i = 0; i < items.size(); i++)
    {
        if(items[i].checkState() == Qt::Checked)
        {
            auto &item = items[i];
            QString key = item.data(Qt::UserRole).toString();
            QString comment = item.data(Qt::UserRole + 1).toString();
            QString glob = QString("        <glob pattern=\"*.%1\"/>\n").arg(key);
            QString magic = magicTable.contains(key) ? magicTable[key] : glob;
            entries.append(entryTemplate.arg(key, comment, magic));
            minesForDesktopFile.append(QString("application/x-pge-music-%1;").arg(item.data(Qt::UserRole).toString()));
        }
    }

    QString oldFile = home + "/.local/share/mime/packages/pge-musplay-mimeinfo.xml";
    if(QFile::exists(oldFile))
        QFile::remove(oldFile);

    QString mimeFile = home + "/.local/share/mime/packages/moondust-musplay-mimeinfo.xml";
    QFile outMime(mimeFile);
    outMime.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);
    QTextStream outMimeS(&outMime);
    outMimeS << mimeHead;
    for(const QString &s : entries)
        outMimeS << s;
    outMimeS << mimeFoot;
    outMimeS.flush();
    outMime.close();

    if(success) success = xIconSize(16);
    if(success) success = xIconSize(32);
    if(success) success = xIconSize(48);
    if(success) success = xIconSize(256);

    QFile shortcut(":/PGE Music Player.desktop.template");
    if(success) success = shortcut.open(QFile::ReadOnly | QFile::Text);
    if(success)
    {
        QTextStream shtct(&shortcut);
        QString shortcut_text = shtct.readAll();
        QFile saveAs(home + "/.local/share/applications/sdlmixer_musplay.desktop");

        if(success) success = saveAs.open(QFile::WriteOnly | QFile::Text);
        if(success) QTextStream(&saveAs) << shortcut_text.arg(QApplication::applicationFilePath(),
                                                              QApplication::applicationDirPath() + "/",
                                                              minesForDesktopFile);
    }

    for(int i = 0; i < items.size(); i++)
    {
        if(items[i].checkState() == Qt::Checked)
        {
            auto &item = items[i];
            if(success) success = xRunCommand("xdg-mime",
                                              {
                                                  "default",
                                                  "sdlmixer_musplay.desktop",
                                                  QString("application/x-moondust-music-%1").arg(item.data(Qt::UserRole).toString())
                                              });
        }
    }

    if(success) success = xRunCommand("xdg-mime", {"install", mimeFile});
    if(success) success = xRunCommand("update-desktop-database", {home + "/.local/share/applications"});
    if(success) success = xRunCommand("update-mime-database", {home + "/.local/share/mime"});
#else
    success = false;
#endif

    if(!success)
        QMessageBox::warning(this, tr("Error"), tr("File association error has occured."));
    else
        QMessageBox::information(this, tr("Success"), tr("All files has been associated!"));
}
