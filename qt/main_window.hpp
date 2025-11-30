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
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

namespace apis::google {
class Client;
}

class main_window : public QMainWindow {
    Q_OBJECT

  public:
    explicit main_window(QWidget *parent = nullptr);
    ~main_window();

  private slots:
    void on_open_project();
    void on_new_project();
    void on_cut_select(QListWidgetItem *item);
    void on_ocr();

  private:
    void init_ui();
    void init_menu_bar();
    void init_config();

    void load_keyframes(const QString &directory);

    QListWidget *cut_list;
    QLabel *status_label;
    QWidget *central_widget;

    QPushButton *ocr_button;
    QTextEdit *ocr_content;
    QLabel *image_preview;

    std::unique_ptr<apis::google::Client> ocr_client_;
    QString current_img_path_;
};
