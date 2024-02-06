#ifndef PLAYLISTMODEL_H
#define PLAYLISTMODEL_H

#include <QAbstractListModel>
#include <QString>
#include <QList>
#include <QSet>

struct PlayListEntry
{
    QString name;
    QString fullPath;

    int  gme_trackNum = 0;
    int  midi_device = 0;
    int  adl_bankNo = 58;
    int  adl_volumeModel = 0;
    bool adl_tremolo = true;
    bool adl_vibrato = true;
    bool adl_adlibDrums = false;
    bool adl_modulation = false;
    bool adl_cmfVolumes = false;
};

class PlayListModel : public QAbstractListModel
{
    Q_OBJECT

    QList<PlayListEntry>    m_list;
    int                     m_current = 0;
    QSet<int>               m_playedTracks;
public:
    explicit PlayListModel(QObject *parent = 0);

    PlayListEntry &nextEntry();
    PlayListEntry &currentEntry();
    void insertEntry(const PlayListEntry &e);
    void removeEntry(int index = -1);
    void clear();

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

private:
};

#endif // PLAYLISTMODEL_H
