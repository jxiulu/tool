// main window imp

#include "config.hpp"
#include "google.hpp"
#include "main_window.hpp"
#include "materials.hpp"
#include <QFileDialog>
#include <QMessageBox>
#include <QStatusBar>
#include <filesystem>

namespace fs = std::filesystem;

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    init_ui();
    init_menu_bar();
    init_config();

    setWindowTitle("SetteiMain v0.0.1");
    resize(1200, 800);
}

MainWindow::~MainWindow() {}

void MainWindow::init_ui() {
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
            &MainWindow::on_cut_select);
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

void MainWindow::init_menu_bar() {
    auto *menu_bar = new QMenuBar(this);
    setMenuBar(menu_bar);

    // file menu
    QMenu *file_menu = menu_bar->addMenu("&File");

    QAction *file_open = file_menu->addAction("&Open Project...");
    connect(file_open, &QAction::triggered, this,
            &MainWindow::on_open_project);

    QAction *file_new = file_menu->addAction("&New Project...");
    connect(file_new, &QAction::triggered, this, &MainWindow::on_new_project);

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

void MainWindow::init_config() {
    auto cfgpath = setman::find_config();
    std::string apikey;

    if (cfgpath.has_value()) {
        auto cfgresult = setman::load_config(*cfgpath);

        if (!cfgresult.has_value()) {
            QMessageBox::warning(
                this, "Config Error",
                QString("Failed to load config: %1")
                    .arg(QString::fromStdString(cfgresult.error().message())));
        }

        auto &cfg = cfgresult.value();

        auto key = cfg.find("gemini_api_key");
        if (key.has_value()) {
            apikey = *key;
            statusBar()->showMessage(
                QString("Loaded config from %1")
                    .arg(QString::fromStdString(cfgpath->string())),
                5000);
        }
    }

    // fallback to env variable
    if (apikey.empty()) {
        QString envkey = qgetenv("GEMINI_API_KEY");
        if (!envkey.isEmpty()) {
            apikey = envkey.toStdString();
            statusBar()->showMessage("Using API Key from environment variable",
                                     5000);
        }
    }

    if (!apikey.empty()) {
        ocr_client_ = setman::ai::new_google_client(apikey);
    } else {
        QMessageBox::warning(this, "API Key Missing",
                             "No key found in env or in config file");
    }
}

void MainWindow::on_open_project() {
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

void MainWindow::on_new_project() {
    QMessageBox::information(this, "New Project", "67");
}

void MainWindow::on_cut_select(QListWidgetItem *item) {
    if (!item)
        return;
    statusBar()->showMessage("Selected: " + item->text(), 2000);
    // todo
}

void MainWindow::load_keyframes(const QString &directory) {
    cut_list->clear();

    fs::path dir(directory.toStdString());
    if (!fs::exists(dir) || !fs::is_directory(dir)) {
        return;
    }

    for (const auto &entry : fs::recursive_directory_iterator(dir)) {
        if (!fs::is_regular_file(entry)) {
            continue;
        }

        auto isimg = setman::materials::isimg(entry.path());
        if (isimg.has_value() && isimg.value()) {
            QString filepath = QString::fromStdString(entry.path().string());
            QString filename =
                QString::fromStdString(entry.path().filename().string());

            auto *item = new QListWidgetItem(filename, cut_list);
            item->setData(Qt::UserRole, filepath); // store full path
            cut_list->addItem(item);
        }
    }

    statusBar()->showMessage(
        QString("Found %1 images in path.").arg(cut_list->count()), 3000);
}

void MainWindow::on_ocr() {
    if (current_img_path_.isEmpty() || !ocr_client_) {
        QMessageBox::warning(
            this, "Error",
            "No image selected or Gemini client not initialized");
        return;
    }

    ocr_button->setEnabled(false);
    ocr_content->setText("Processing");
    statusBar()->showMessage("Sending to Gemini");

    fs::path imgpath(current_img_path_.toStdString());
    auto result = setman::materials::img_tob64(imgpath);

    if (!result.has_value()) {
        QString errmsg = QString::fromStdString(result.error().message());
        ocr_content->setText("Error: " + errmsg);
        ocr_button->setEnabled(true);
        statusBar()->showMessage("Error converting image", 3000);
        return;
    }

    auto ext = setman::materials::file_ext(imgpath);
    std::string mime_type = "image/jpeg"; // default
    if (ext.has_value()) {
        if (*ext == "png")
            mime_type = "image/png";
        else if (*ext == "webp")
            mime_type = "image/webp";
        // etc.
    }

    setman::ai::GoogleRequest req;
    req.set_model("gemini-3.0-pro");
    req.add_inline_image(result.value(), mime_type);
    req.add_text("Please extract all visible text in this image. Provide this "
                 "text in a structured format");

    auto response = ocr_client_->send(req);

    if (!response.valid()) {
        ocr_content->setText("Error: " +
                             QString::fromStdString(response.error()));
        statusBar()->showMessage("Gemini request failed", 3000);
    } else {
        const auto &content = response.content();
        if (!content.empty()) {
            QString text = QString::fromStdString(
                std::string(content[0].data(), content[0].size()));
            ocr_content->setText(text);

            statusBar()->showMessage("Response received!", 3000);
        } else {
            ocr_content->setText("No content in response");
            statusBar()->showMessage("Empty response", 3000);
        }
    }

    ocr_button->setEnabled(true);
}
