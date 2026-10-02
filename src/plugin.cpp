#include <obs-module.h>
#include <obs-frontend-api.h>
#include <util/platform.h>
#include <QApplication>
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontComboBox>
#include <QFormLayout>
#include <QImageReader>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPixmap>
#include <QLinearGradient>
#include <QPointer>
#include <QPushButton>
#include <QSaveFile>
#include <QSpinBox>
#include <QTabWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <mutex>
#include "bible.hpp"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-biblia", "es-ES")
MODULE_EXPORT const char *obs_module_description(void) { return "Panel y fuente nativos para proyectar la Biblia"; }

namespace {
constexpr int Width = 1920, Height = 1080;
std::mutex frameMutex;
QImage sharedFrame;
uint64_t sharedRevision = 0;

QString dataPath(const char *relative)
{
    char *path = obs_module_file(relative);
    QString result = path ? QString::fromUtf8(path) : QString();
    bfree(path); return result;
}
QString userPath()
{
    char *path = obs_module_config_path("versions");
    QString result = path ? QString::fromUtf8(path) : QString();
    bfree(path); return result;
}

class Panel final : public QWidget {
public:
    explicit Panel(QWidget *parent = nullptr) : QWidget(parent)
    {
        auto *layout = new QVBoxLayout(this);
        auto *tabs = new QTabWidget(this);
        auto *search = new QWidget(tabs);
        auto *searchLayout = new QVBoxLayout(search);
        auto *form = new QFormLayout;
        versions = new QComboBox(search);
        reference = new QLineEdit(QStringLiteral("Juan 3:16"), search);
        reference->setPlaceholderText(QStringLiteral("Juan 3:16 / Salmos 23 / Juan 3:16-18"));
        form->addRow(QStringLiteral("Versión de la Biblia"), versions);
        form->addRow(QStringLiteral("Libro, capítulo y versículo"), reference);
        searchLayout->addLayout(form);
        auto *lookup = new QPushButton(QStringLiteral("Buscar pasaje"), search);
        searchLayout->addWidget(lookup);
        text = new QTextEdit(search); text->setReadOnly(true); searchLayout->addWidget(text);
        auto *project = new QPushButton(QStringLiteral("Proyectar"), search);
        auto *clear = new QPushButton(QStringLiteral("Ocultar pasaje"), search);
        searchLayout->addWidget(project); searchLayout->addWidget(clear);
        auto *import = new QPushButton(QStringLiteral("Importar versión JSON…"), search);
        searchLayout->addWidget(import);
        tabs->addTab(search, QStringLiteral("Biblia"));

        auto *appearance = new QWidget(tabs);
        auto *styleForm = new QFormLayout(appearance);
        fonts = new QFontComboBox(appearance); fonts->setCurrentFont(QFont(QStringLiteral("Arial")));
        size = new QSpinBox(appearance); size->setRange(24, 160); size->setValue(64);
        bold = new QCheckBox(QStringLiteral("Negrita"), appearance); bold->setChecked(true);
        italic = new QCheckBox(QStringLiteral("Cursiva"), appearance);
        auto *colorButton = new QPushButton(QStringLiteral("Color del texto…"), appearance);
        backgrounds = new QComboBox(appearance);
        backgrounds->addItems({QStringLiteral("Azul profundo"), QStringLiteral("Amanecer"), QStringLiteral("Bosque"), QStringLiteral("Púrpura"), QStringLiteral("Imagen propia")});
        auto *imageButton = new QPushButton(QStringLiteral("Escoger imagen…"), appearance);
        imageLabel = new QLabel(QStringLiteral("Sin imagen seleccionada"), appearance); imageLabel->setWordWrap(true);
        shade = new QSpinBox(appearance); shade->setRange(0, 90); shade->setSuffix(" %"); shade->setValue(35);
        styleForm->addRow(QStringLiteral("Tipografía"), fonts);
        styleForm->addRow(QStringLiteral("Tamaño de letra (máximo)"), size);
        styleForm->addRow(bold); styleForm->addRow(italic); styleForm->addRow(colorButton);
        styleForm->addRow(QStringLiteral("Fondo incluido"), backgrounds);
        styleForm->addRow(imageButton); styleForm->addRow(imageLabel);
        styleForm->addRow(QStringLiteral("Oscurecer fondo"), shade);
        auto *hint = new QLabel(QStringLiteral("La apariencia actualiza el pasaje proyectado. El texto se ajusta al espacio disponible."), appearance);
        hint->setWordWrap(true); styleForm->addRow(hint);
        tabs->addTab(appearance, QStringLiteral("Apariencia")); layout->addWidget(tabs);
        preview = new QLabel(this); preview->setMinimumSize(240, 135); preview->setAlignment(Qt::AlignCenter);
        layout->addWidget(preview);
        status = new QLabel(this); status->setWordWrap(true); layout->addWidget(status);
        connect(lookup, &QPushButton::clicked, this, [this] { find(); });
        connect(reference, &QLineEdit::returnPressed, this, [this] { find(); });
        connect(project, &QPushButton::clicked, this, [this] {
            if (!find()) return;
            const auto old = current; const auto oldVersion = currentVersion; const bool oldVisible = visible;
            current = candidate; currentVersion = versions->currentText(); visible = true;
            if (!render()) { current = old; currentVersion = oldVersion; visible = oldVisible; }
            else status->setText(QStringLiteral("Proyectando %1 · %2").arg(current.reference, currentVersion));
        });
        connect(clear, &QPushButton::clicked, this, [this] { visible = false; render(); status->setText(QStringLiteral("Pasaje oculto.")); });
        connect(import, &QPushButton::clicked, this, [this] { importVersion(); });
        auto changed = [this] { if (!restoring) render(); };
        connect(fonts, &QFontComboBox::currentFontChanged, this, changed);
        connect(size, qOverload<int>(&QSpinBox::valueChanged), this, changed);
        connect(shade, qOverload<int>(&QSpinBox::valueChanged), this, changed);
        connect(bold, &QCheckBox::toggled, this, changed); connect(italic, &QCheckBox::toggled, this, changed);
        connect(backgrounds, qOverload<int>(&QComboBox::currentIndexChanged), this, changed);
        connect(versions, qOverload<int>(&QComboBox::currentIndexChanged), this, [this] { text->clear(); });
        connect(reference, &QLineEdit::textEdited, this, [this] { text->clear(); });
        connect(colorButton, &QPushButton::clicked, this, [this] {
            const QColor selected = QColorDialog::getColor(color, this, QStringLiteral("Color del texto"));
            if (selected.isValid()) { color = selected; render(); }
        });
        connect(imageButton, &QPushButton::clicked, this, [this] {
            const auto path = QFileDialog::getOpenFileName(this, QStringLiteral("Seleccionar fondo"), {}, QStringLiteral("Imágenes (*.png *.jpg *.jpeg *.bmp)"));
            if (path.isEmpty()) return;
            QImageReader reader(path); reader.setAutoTransform(true);
            const auto dimensions = reader.size();
            if (!dimensions.isValid() || dimensions.width() > 16000 || dimensions.height() > 16000) {
                status->setText(QStringLiteral("Imagen inválida o demasiado grande.")); return;
            }
            reader.setScaledSize(dimensions.scaled(Width, Height, Qt::KeepAspectRatioByExpanding));
            auto image = reader.read();
            if (image.isNull()) { status->setText(QStringLiteral("No se pudo leer la imagen.")); return; }
            customImage = image; imagePath = path; imageLabel->setText(QFileInfo(path).fileName());
            backgrounds->setCurrentIndex(4); render();
        });
        loadVersions(); render();
        status->setText(QStringLiteral("Agrega la fuente «Biblia» a tu escena y busca un pasaje."));
    }

    QJsonObject save() const
    {
        return {{"version", versions->currentData().toString()}, {"reference", reference->text()},
                {"font", fonts->currentFont().family()}, {"size", size->value()}, {"bold", bold->isChecked()},
                {"italic", italic->isChecked()}, {"color", color.name()}, {"background", backgrounds->currentIndex()},
                {"image", imagePath}, {"shade", shade->value()}, {"visible", visible},
                {"projectedText", current.text}, {"projectedReference", current.reference}, {"projectedVersion", currentVersion}};
    }
    void restore(const QJsonObject &o)
    {
        // Do not retain the previous collection's live passage if restoration fails.
        visible = false; render();
        restoring = true;
        int index = versions->findData(o.value("version").toString()); versions->setCurrentIndex(index < 0 ? 0 : index);
        reference->setText(o.value("reference").toString(QStringLiteral("Juan 3:16")));
        fonts->setCurrentFont(QFont(o.value("font").toString(QStringLiteral("Arial"))));
        size->setValue(o.value("size").toInt(64)); bold->setChecked(o.value("bold").toBool(true));
        italic->setChecked(o.value("italic").toBool(false)); color = QColor(o.value("color").toString("#ffffff"));
        if (!color.isValid()) color = Qt::white;
        backgrounds->setCurrentIndex(qBound(0, o.value("background").toInt(), 4));
        shade->setValue(o.value("shade").toInt(35)); imagePath = o.value("image").toString();
        customImage = QImage();
        if (!imagePath.isEmpty()) {
            QImageReader reader(imagePath); reader.setAutoTransform(true);
            auto dimensions = reader.size();
            if (dimensions.isValid() && dimensions.width() <= 16000 && dimensions.height() <= 16000) {
                reader.setScaledSize(dimensions.scaled(Width, Height, Qt::KeepAspectRatioByExpanding)); customImage = reader.read();
            }
        }
        imageLabel->setText(customImage.isNull() ? QStringLiteral("Sin imagen disponible") : QFileInfo(imagePath).fileName());
        current = {o.value("projectedReference").toString(), o.value("projectedText").toString()};
        currentVersion = o.value("projectedVersion").toString(); visible = o.value("visible").toBool(false);
        restoring = false; text->clear(); status->clear(); render();
    }
private:
    QComboBox *versions, *backgrounds;
    QLineEdit *reference;
    QFontComboBox *fonts;
    QSpinBox *size, *shade;
    QCheckBox *bold, *italic;
    QTextEdit *text;
    QLabel *preview, *status, *imageLabel;
    QVector<Bible> bibles;
    Passage candidate, current;
    QString currentVersion, imagePath;
    QColor color = Qt::white;
    QImage customImage;
    bool visible = false, restoring = false;

    void loadVersions()
    {
        const auto previous = versions->currentData().toString();
        versions->clear(); bibles.clear();
        for (const auto &folder : {dataPath("bibles"), userPath()}) {
            if (folder.isEmpty()) continue;
            QDir directory(folder);
            for (const auto &file : directory.entryList({"*.json"}, QDir::Files, QDir::Name)) {
                QFile input(directory.filePath(file)); if (input.size() > 32 * 1024 * 1024 || !input.open(QIODevice::ReadOnly)) continue;
                Bible bible; QString error; if (!bible.load(input.readAll(), error)) continue;
                bool exists = false; for (const auto &loaded : bibles) if (loaded.id == bible.id) exists = true;
                if (exists) continue;
                bibles.append(bible); versions->addItem(bible.name, bible.id);
            }
        }
        int index = versions->findData(previous); if (index >= 0) versions->setCurrentIndex(index);
    }
    bool find()
    {
        int index = versions->currentIndex(); QString error;
        if (index < 0 || index >= bibles.size()) { status->setText(QStringLiteral("Importa una versión bíblica JSON.")); return false; }
        if (!bibles[index].lookup(reference->text(), candidate, error)) {
            text->clear(); status->setText(error); return false;
        }
        text->setPlainText(candidate.reference + " · " + bibles[index].name + "\n\n" + candidate.text);
        status->setText(QStringLiteral("Pasaje encontrado. Pulsa Proyectar para mostrarlo.")); return true;
    }
    void importVersion()
    {
        auto path = QFileDialog::getOpenFileName(this, QStringLiteral("Importar Biblia"), {}, "JSON (*.json)");
        if (path.isEmpty()) return;
        QFile file(path);
        if (file.size() > 32 * 1024 * 1024 || !file.open(QIODevice::ReadOnly)) { status->setText(QStringLiteral("No se pudo abrir el archivo (máximo 32 MB).")); return; }
        const auto bytes = file.readAll(); Bible bible; QString error;
        if (!bible.load(bytes, error)) { status->setText(error); return; }
        for (const auto &loaded : bibles) if (loaded.id == bible.id) {
            status->setText(QStringLiteral("Ya existe una versión con ese identificador.")); return;
        }
        auto directory = userPath();
        if (directory.isEmpty() || !QDir().mkpath(directory)) { status->setText(QStringLiteral("No se pudo crear la carpeta de versiones.")); return; }
        QSaveFile output(QDir(directory).filePath(QString::fromLatin1(QCryptographicHash::hash(bible.id.toUtf8(), QCryptographicHash::Sha256).toHex()) + ".json"));
        if (!output.open(QIODevice::WriteOnly) || output.write(bytes) != bytes.size() || !output.commit()) {
            status->setText(QStringLiteral("No se pudo guardar la versión.")); return;
        }
        loadVersions(); versions->setCurrentIndex(versions->findData(bible.id)); status->setText(QStringLiteral("Versión importada: ") + bible.name);
    }
    bool render()
    {
        QImage image(Width, Height, QImage::Format_RGBA8888); image.fill(Qt::transparent);
        if (visible && !current.text.isEmpty()) {
            if (backgrounds->currentIndex() == 4 && customImage.isNull()) {
                status->setText(QStringLiteral("Selecciona una imagen de fondo válida.")); return false;
            }
            QPainter painter(&image); painter.setRenderHint(QPainter::Antialiasing); painter.setRenderHint(QPainter::TextAntialiasing);
            if (backgrounds->currentIndex() == 4) {
                const auto scaled = customImage.scaled(Width, Height, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
                painter.drawImage(QPoint((Width-scaled.width())/2, (Height-scaled.height())/2), scaled);
            } else {
                static const char *starts[] = {"#07172f", "#3e2145", "#082c25", "#241244"};
                static const char *ends[] = {"#245b85", "#c57c46", "#467c58", "#795898"};
                const int index = qBound(0, backgrounds->currentIndex(), 3);
                QLinearGradient gradient(0, 0, Width, Height); gradient.setColorAt(0, QColor(starts[index])); gradient.setColorAt(1, QColor(ends[index]));
                painter.fillRect(image.rect(), gradient);
                painter.setPen(Qt::NoPen); painter.setBrush(QColor(255,255,255,12));
                painter.drawEllipse(QPoint(1600,200), 500,500); painter.drawEllipse(QPoint(150,1000), 600,600);
            }
            painter.fillRect(image.rect(), QColor(0,0,0,shade->value()*255/100));
            QFont font = fonts->currentFont(); font.setBold(bold->isChecked()); font.setItalic(italic->isChecked());
            const QRect area(130, 120, Width-260, Height-350);
            const int flags = Qt::AlignCenter | Qt::TextWordWrap;
            int pixel = size->value(); QRect bounds;
            do {
                font.setPixelSize(pixel); painter.setFont(font);
                bounds = painter.boundingRect(area, flags, current.text);
                if (bounds.height() <= area.height() && bounds.width() <= area.width()) break;
            } while (--pixel >= 18);
            if (pixel < 18) { status->setText(QStringLiteral("El pasaje es demasiado largo. Proyecta un rango más corto.")); return false; }
            painter.setPen(QColor(0,0,0,180)); painter.drawText(area.translated(3,3), flags, current.text);
            painter.setPen(color); painter.drawText(area, flags, current.text);
            font.setPixelSize(36); font.setBold(true); painter.setFont(font);
            painter.drawText(QRect(130, Height-185, Width-260, 100), flags, current.reference + " · " + currentVersion);
        }
        preview->setPixmap(QPixmap::fromImage(image.scaled(320,180,Qt::KeepAspectRatio,Qt::SmoothTransformation)));
        std::lock_guard<std::mutex> lock(frameMutex); sharedFrame = image; ++sharedRevision; return true;
    }
};
QPointer<Panel> panel;

struct Source { gs_texture_t *texture = nullptr; uint64_t revision = 0; };
const char *sourceName(void *) { return "Biblia"; }
void *createSource(obs_data_t *, obs_source_t *) { return new Source; }
void destroySource(void *data)
{
    auto *source = static_cast<Source *>(data); obs_enter_graphics();
    gs_texture_destroy(source->texture); obs_leave_graphics(); delete source;
}
void renderSource(void *data, gs_effect_t *)
{
    auto *source = static_cast<Source *>(data); QImage image; uint64_t revision;
    {
        std::lock_guard<std::mutex> lock(frameMutex); revision = sharedRevision;
        if (revision != source->revision) image = sharedFrame;
    }
    if (!image.isNull()) {
        const uint8_t *pixels = image.constBits();
        if (!source->texture) source->texture = gs_texture_create(Width, Height, GS_RGBA, 1, &pixels, GS_DYNAMIC);
        else gs_texture_set_image(source->texture, pixels, uint32_t(image.bytesPerLine()), false);
        if (source->texture) source->revision = revision;
    }
    if (source->texture) obs_source_draw(source->texture, 0, 0, Width, Height, false);
}
uint32_t getWidth(void *) { return Width; }
uint32_t getHeight(void *) { return Height; }

void collectionState(obs_data_t *data, bool saving, void *)
{
    if (!panel) return;
    if (saving) {
        const auto bytes = QJsonDocument(panel->save()).toJson(QJsonDocument::Compact);
        obs_data_set_string(data, "obs-biblia-state", bytes.constData());
    } else {
        const auto object = QJsonDocument::fromJson(QByteArray(obs_data_get_string(data, "obs-biblia-state"))).object();
        panel->restore(object);
    }
}
}

bool obs_module_load(void)
{
    obs_source_info info = {}; info.id = "obs_biblia_source"; info.type = OBS_SOURCE_TYPE_INPUT;
    info.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CAP_DONT_SHOW_PROPERTIES;
    info.get_name = sourceName; info.create = createSource; info.destroy = destroySource;
    info.get_width = getWidth; info.get_height = getHeight; info.video_render = renderSource;
    obs_register_source(&info);
    return true;
}
void obs_module_post_load(void)
{
    panel = new Panel;
    if (!obs_frontend_add_dock_by_id("obs-biblia-panel", "Biblia", panel.data())) {
        delete panel.data(); panel = nullptr; blog(LOG_ERROR, "[obs-biblia] No se pudo registrar el panel"); return;
    }
    obs_frontend_add_save_callback(collectionState, nullptr);
}
void obs_module_unload(void)
{
    obs_frontend_remove_save_callback(collectionState, nullptr);
    if (panel) obs_frontend_remove_dock("obs-biblia-panel");
    panel = nullptr;
}
