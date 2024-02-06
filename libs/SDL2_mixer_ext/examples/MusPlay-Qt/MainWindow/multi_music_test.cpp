#include <QFileDialog>
#include <QVBoxLayout>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>

#include "multi_music_test.h"
#include "multi_music_item.h"
#include "ui_multi_music_test.h"

#include "qfile_dialogs_default_options.hpp"


MultiMusicTest::MultiMusicTest(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MultiMusicTest)
{
    ui->setupUi(this);
}

MultiMusicTest::~MultiMusicTest()
{
    delete ui;
}

void MultiMusicTest::openMusicByArg(QString musPath)
{
    auto *it = new MultiMusicItem(musPath, this);
    m_items.push_back(it);
    QObject::connect(it,
                     &MultiMusicItem::wannaClose,
                     this,
                     &MultiMusicTest::wannaClose);
    auto *l = qobject_cast<QVBoxLayout*>(ui->musicsList->widget()->layout());
    l->insertWidget(l->count() - 1, it);
}

void MultiMusicTest::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void MultiMusicTest::closeEvent(QCloseEvent *)
{
    for(auto *it : m_items)
    {
        auto *l = qobject_cast<QVBoxLayout*>(ui->musicsList->widget()->layout());
        l->removeWidget(it);
        it->on_stop_clicked();
        it->deleteLater();
    }
    m_items.clear();
}

void MultiMusicTest::dropEvent(QDropEvent *e)
{
    this->raise();
    this->setFocus(Qt::ActiveWindowFocusReason);

    for(const QUrl &url : e->mimeData()->urls())
    {
        const QString &fileName = url.toLocalFile();
        openMusicByArg(fileName);
    }
}

void MultiMusicTest::dragEnterEvent(QDragEnterEvent *e)
{
    if(e->mimeData()->hasUrls())
        e->acceptProposedAction();
}

void MultiMusicTest::on_open_clicked()
{
    QString file = QFileDialog::getOpenFileName(this,
                                                tr("Open music file"),
                                                (m_lastMusic.isEmpty() ?
                                                QApplication::applicationDirPath() :
                                                m_lastMusic),
                                                "All (*.*)", nullptr,
                                                c_fileDialogOptions);

    if(file.isEmpty())
        return;

    m_lastMusic = file;
    openMusicByArg(file);
}

void MultiMusicTest::wannaClose(MultiMusicItem *item)
{
    auto *l = qobject_cast<QVBoxLayout*>(ui->musicsList->widget()->layout());
    l->removeWidget(item);
    m_items.removeAll(item);
    item->on_stop_clicked();
    item->deleteLater();
}


void MultiMusicTest::on_playAll_clicked()
{
    for(auto *it : m_items)
        it->on_playpause_clicked();
}


void MultiMusicTest::on_stopAll_clicked()
{
    for(auto *it : m_items)
    {
        it->on_stop_clicked();
    }
}

void MultiMusicTest::on_generalVolume_sliderMoved(int position)
{
    Mix_VolumeMusicGeneral(position);
}
