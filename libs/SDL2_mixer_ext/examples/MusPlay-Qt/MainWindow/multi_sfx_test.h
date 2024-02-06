#ifndef MULTI_SFX_TEST_H
#define MULTI_SFX_TEST_H

#include <QDialog>
#include <QTimer>
#include <QVector>

class MultiSfxItem;

namespace Ui
{
class MultiSfxTester;
}

class MultiSfxTester : public QDialog
{
    Q_OBJECT

    QVector<MultiSfxItem*> m_items;
    QString m_lastSfx;
    QTimer  m_randomPlayTrigger;

public:
    explicit MultiSfxTester(QWidget* parent = nullptr);
    ~MultiSfxTester();

    MultiSfxItem *openSfxByArg(QString musPath);
    void reloadAll();

protected:
    void changeEvent(QEvent* e);
    void closeEvent(QCloseEvent* e);
    /*!
     * \brief Drop event, triggers when some object was dropped to main window
     * \param e Event parameters
     */
    void dropEvent(QDropEvent* e);

    /*!
     * \brief Drop enter event, triggers when mouse came to window with holden objects
     * \param e Event parameters
     */
    void dragEnterEvent(QDragEnterEvent* e);

private slots:
    void anySfxFinished(int numChannel);
    void on_openSfx_clicked();
    void on_randomPlaying_clicked(bool checked);
    void playRandom();

    void wannaClose(MultiSfxItem* item);

    void on_saveList_clicked();
    void on_loadList_clicked();

private:
    void closeAll();
    static void sfxFinished(int numChannel);
    Ui::MultiSfxTester* ui;
};

#endif // MULTI_SFX_TEST_H
