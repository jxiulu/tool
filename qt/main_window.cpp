// main window imp

#include "main_window.hpp"
#include <QFileDialog>
#include <QMessageBox>
#include <QStatusBar>

main_window::main_window(QWidget *parent) : QMainWindow(parent) {
    init_ui();
    init_menu_bar();

    setWindowTitle("SetteiMain v0.0.1");
    resize(1200, 800);
}

main_window::~main_window() {}

void main_window::init_ui() {
    central_widget = new QWidget(this);
    setCentralWidget(central_widget);

    QVBoxLayout *main_layout = new QVBoxLayout(central_widget);

    status_label = new QLabel("No project selected.", this);
    main_layout->addWidget(status_label);

    // horizontal layout main area
    QHBoxLayout *content_layout = new QHBoxLayout();

    // left panel
    cut_list = new QListWidget(this);
    cut_list->setMinimumWidth(300);
    connect(cut_list, &QListWidget::itemClicked, this,
            &main_window::on_cut_select);
    content_layout->addWidget(cut_list);

    // right panel (cut details)
    // placeholder
    QWidget *details_panel = new QWidget(this);
    QVBoxLayout *details_layout = new QVBoxLayout(details_panel);
    details_layout->addWidget(new QLabel("Cut details", this));
    content_layout->addWidget(details_panel);

    main_layout->addLayout(content_layout);

    statusBar()->showMessage("hello");
}

void main_window::init_menu_bar() {
    auto *menu_bar = new QMenuBar(this);
    setMenuBar(menu_bar);

    // file menu
    QMenu *file_menu = menu_bar->addMenu("&File");

    QAction *file_open = file_menu->addAction("&Open Project...");
    connect(file_open, &QAction::triggered, this,
            &main_window::on_open_project);

    QAction *file_new = file_menu->addAction("&New Project...");
    connect(file_new, &QAction::triggered, this, &main_window::on_new_project);

    file_menu->addSeparator();

    QAction *exit_action = file_menu->addAction("E&xit");
    connect(exit_action, &QAction::triggered, this, &QWidget::close);

    // edit
    QMenu *edit_menu = menu_bar->addMenu("&Edit");
    edit_menu->addAction("Preferences...");

    // view
    QMenu *view_menu = menu_bar->addMenu("&View");
    view_menu->addAction("Refresh");

    // help
    QMenu *help_menu = menu_bar->addMenu("&Help");
    help_menu->addAction("About");
}

void main_window::on_open_project() {
    QString dir = QFileDialog::getExistingDirectory(
        this, "Open Project Directory", QString(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (dir.isEmpty()) {
        return;
    }

    status_label->setText("Loaded project directory:" + dir);
    statusBar()->showMessage("Project loaded: " + dir, 3000);

    // placeholder
    cut_list->clear();
    cut_list->addItem("poop");
    cut_list->addItem("doop");
    cut_list->addItem("sloop");
}

void main_window::on_new_project() {
    QMessageBox::information(this, "New Project", "67");
}

void main_window::on_cut_select(QListWidgetItem *item) {
    if (!item)
        return;
    statusBar()->showMessage("Selected: " + item->text(), 2000);
    // todo
}
