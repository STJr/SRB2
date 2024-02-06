#pragma once

#if !defined(ASSOC_FILES_H)
#define ASSOC_FILES_H

#include <QDialog>
#include <QListWidgetItem>
#include <QList>

namespace Ui {
class AssocFiles;
}

class AssocFiles : public QDialog
{
    Q_OBJECT

public:
    explicit AssocFiles(QWidget *parent = 0);
    ~AssocFiles();

private slots:
    void on_AssocFiles_accepted();
    void on_selectAll_clicked();
    void on_clear_clicked();
    void on_reset_clicked();

private:
    QList<QListWidgetItem> items;
    Ui::AssocFiles *ui;
};

#endif // ASSOC_FILES_H
