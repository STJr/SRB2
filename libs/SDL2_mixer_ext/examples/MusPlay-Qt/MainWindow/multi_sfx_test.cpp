#include <QFileDialog>
#include <QVBoxLayout>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QSettings>
#include <QMessageBox>
#include <ctime>

#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
#include <QRandomGenerator>
#define HAS_QRANDOM_GENERATOR
#define XX_QRAND QRandomGenerator::global()->generate
#else
#define XX_QRAND qrand
#endif

#include "multi_sfx_test.h"
#include "multi_sfx_item.h"
#include "ui_multi_sfx_test.h"

#include "qfile_dialogs_default_options.hpp"


static MultiSfxTester* m_testerSelf = nullptr;

MultiSfxTester::MultiSfxTester(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::MultiSfxTester)
{
    Q_ASSERT(!m_testerSelf); // No multiple instances allowed!
    ui->setupUi(this);
    m_testerSelf = this;
    Mix_ChannelFinished(&sfxFinished);
    QObject::connect(&m_randomPlayTrigger,
                     &QTimer::timeout,
                     this,
                     &MultiSfxTester::playRandom);
#ifndef HAS_QRANDOM_GENERATOR
    qsrand((uint)std::time(nullptr));
#endif
}

MultiSfxTester::~MultiSfxTester()
{
    Mix_ChannelFinished(nullptr);
    delete ui;
    m_testerSelf = nullptr;
}

MultiSfxItem *MultiSfxTester::openSfxByArg(QString musPath)
{
    auto* it = new MultiSfxItem(musPath, this);
    m_items.push_back(it);
    QObject::connect(it,
                     &MultiSfxItem::wannaClose,
                     this,
                     &MultiSfxTester::wannaClose);
    auto* l = qobject_cast<QVBoxLayout*>(ui->sfxList->widget()->layout());
    l->insertWidget(l->count() - 1, it);
    return it;
}

void MultiSfxTester::reloadAll()
{
    for(auto& i : m_items)
        i->reOpenSfx();
}

void MultiSfxTester::changeEvent(QEvent* e)
{
    QDialog::changeEvent(e);

    switch(e->type())
    {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;

    default:
        break;
    }
}

void MultiSfxTester::closeEvent(QCloseEvent*)
{
    closeAll();
    ui->randomPlaying->setChecked(false);
    on_randomPlaying_clicked(false);
}

void MultiSfxTester::dropEvent(QDropEvent* e)
{
    this->raise();
    this->setFocus(Qt::ActiveWindowFocusReason);

    for(const QUrl& url : e->mimeData()->urls())
    {
        const QString& fileName = url.toLocalFile();
        openSfxByArg(fileName);
    }
}

void MultiSfxTester::dragEnterEvent(QDragEnterEvent* e)
{
    if(e->mimeData()->hasUrls())
        e->acceptProposedAction();
}

void MultiSfxTester::anySfxFinished(int numChannel)
{
    for(auto& i : m_items)
        i->sfxStoppedHook(numChannel);
}

void MultiSfxTester::on_openSfx_clicked()
{
    QString file = QFileDialog::getOpenFileName(this,
                                                tr("Open SFX file"),
                                                (m_lastSfx.isEmpty() ?
                                                QApplication::applicationDirPath() :
                                                m_lastSfx),
                                                "All (*.*)", nullptr,
                                                c_fileDialogOptions);

    if(file.isEmpty())
        return;

    m_lastSfx = file;
    openSfxByArg(file);
}

void MultiSfxTester::on_randomPlaying_clicked(bool checked)
{
    if(checked)
    {
        m_randomPlayTrigger.start(200);
        ui->randomPlaying->setText(tr("Stop random"));
    }
    else
    {
        m_randomPlayTrigger.stop();
        ui->randomPlaying->setText(tr("Play random"));
    }
}

void MultiSfxTester::playRandom()
{
    m_randomPlayTrigger.setInterval(100 + (XX_QRAND() % ui->maxRandomDelay->value()));

    if(m_items.isEmpty())
        return;

    int len = m_items.size();
    int select = XX_QRAND() % len;
    m_items[select]->on_play_clicked();
}

void MultiSfxTester::wannaClose(MultiSfxItem* item)
{
    auto* l = qobject_cast<QVBoxLayout*>(ui->sfxList->widget()->layout());
    l->removeWidget(item);
    m_items.removeAll(item);
    item->on_stop_clicked();
    item->deleteLater();
}

void MultiSfxTester::on_saveList_clicked()
{
    int ret = QMessageBox::question(this,
                                    tr("Save list?"),
                                    tr("You are trying to save the SFX list for future use. "
                                       "If you saved the SFX list previously, it will be overriden. Do you want to save the SFX list?"),
                                    QMessageBox::Yes|QMessageBox::No);

    if(ret != QMessageBox::Yes)
        return;

    QSettings setup;
    setup.beginGroup("sfx-test-list");
    setup.beginWriteArray("sfx", m_items.size());

    int i = 0;
    for(auto* it : m_items)
    {
        setup.setArrayIndex(i++);
        setup.setValue("path", it->path());
        setup.setValue("channel", it->channel());
        setup.setValue("fade-delay", it->fadeDelay());
        setup.setValue("init-volume", it->initVolume());
    }

    setup.endArray();
    setup.endGroup();
    setup.sync();
    QMessageBox::information(this,
                             tr("SFX list has been saved"),
                             tr("The SFX list has been saved (%1 entries)").arg(m_items.size()),
                             QMessageBox::Ok);
}

void MultiSfxTester::on_loadList_clicked()
{
    closeAll();

    QSettings setup;
    setup.beginGroup("sfx-test-list");
    int size = setup.beginReadArray("sfx");

    for(int i = 0; i < size; ++i)
    {
        setup.setArrayIndex(i);
        auto val = setup.value("path").toString();
        auto ch = setup.value("channel", -1).toInt();
        auto fade = setup.value("fade-delay", 1000).toInt();
        auto ivol = setup.value("init-volume", 128).toInt();

        if(val.isEmpty())
            continue;

        auto *m = openSfxByArg(val);
        if(m)
        {
            m->setChannel(ch);
            m->setFadeDelay(fade);
            m->setInitVolume(ivol);
        }
    }

    setup.endArray();
    setup.endGroup();
}

void MultiSfxTester::closeAll()
{
    for(auto* it : m_items)
    {
        auto* l = qobject_cast<QVBoxLayout*>(ui->sfxList->widget()->layout());
        l->removeWidget(it);
        it->on_stop_clicked();
        it->deleteLater();
    }

    m_items.clear();
}

void MultiSfxTester::sfxFinished(int numChannel)
{
    if(m_testerSelf)
        QMetaObject::invokeMethod(m_testerSelf,
                                  "anySfxFinished",
                                  Qt::QueuedConnection,
                                  Q_ARG(int, numChannel));
}
