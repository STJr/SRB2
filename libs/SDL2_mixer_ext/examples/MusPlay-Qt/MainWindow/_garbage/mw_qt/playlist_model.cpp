#include "playlist_model.h"
#include <ctime>

PlayListModel::PlayListModel(QObject *parent)
    : QAbstractListModel(parent)
{
    srand(std::time(NULL));
}

PlayListEntry &PlayListModel::nextEntry()
{
    int newValue = 0;

    if(m_playedTracks.size() >= m_list.size())
        m_playedTracks.clear();

    do{
        newValue = rand() % m_list.size();
    } while((m_list.size() > 1) && (newValue == m_current) && !m_playedTracks.contains(newValue));
    m_current = newValue;
    m_playedTracks.insert(newValue);
    return currentEntry();
}

PlayListEntry &PlayListModel::currentEntry()
{
    if(m_list.empty())
    {
        PlayListEntry x;
        x.name = "<Empty>";
        m_list.push_back(x);
        return m_list[0];
    }
    if(m_current < 0)
        m_current = 0;
    if(m_current >= m_list.size())
        m_current = m_list.size() - 1;
    return m_list[m_current];
}

void PlayListModel::insertEntry(const PlayListEntry &e)
{
    beginInsertRows(QModelIndex(), m_list.size(), m_list.size());
    m_list.push_back(e);
    endInsertRows();
}

void PlayListModel::removeEntry(int index)
{
    if(index < 0)
        index = m_current;
    beginRemoveRows(QModelIndex(), index, index);
    m_list.removeAt(index);
    m_playedTracks.clear();
    if(m_current >= m_list.size())
        m_current = m_list.size() - 1;
    m_playedTracks.insert(m_current);
    endRemoveRows();
}

void PlayListModel::clear()
{
    beginResetModel();
    m_list.clear();
    m_playedTracks.clear();
    m_current = 0;
    endResetModel();
}

QVariant PlayListModel::headerData(int /*section*/, Qt::Orientation /*orientation*/, int /*role*/) const
{
    // FIXME: Implement me!
    return(QVariant());
}

int PlayListModel::rowCount(const QModelIndex &parent) const
{
    // For list models only the root node (an invalid parent) should return the list's size. For all
    // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
    if (parent.isValid())
        return 0;

    return m_list.size();
}

QVariant PlayListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if(index.row() >= m_list.size())
        return QVariant();

    switch(role)
    {
    case Qt::DisplayRole:
        return m_list[index.row()].name;
    case Qt::EditRole:
        return m_list[index.row()].name;
    case Qt::ToolTipRole:
        return m_list[index.row()].fullPath;
    }

    return QVariant();
}
