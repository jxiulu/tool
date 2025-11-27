// main app window

#pragma once

#include <QAction>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

class main_window : public QMainWindow {
    Q_OBJECT

  public:
    explicit main_window(QWidget *parent = nullptr);
    ~main_window();

  private slots:
    void on_open_project();
    void on_new_project();
    void on_cut_select(QListWidgetItem *item);

  private:
    void init_ui();
    void init_menu_bar();

    QListWidget *cut_list;
    QLabel *status_label;
    QWidget *central_widget;
};
