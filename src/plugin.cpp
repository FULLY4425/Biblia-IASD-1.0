#include <obs-module.h>
#include <obs-frontend-api.h>
#include <util/platform.h>
#include <graphics/vec4.h>
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
#include <QListWidget>
#include <QHBoxLayout>
#include <QSignalBlocker>
#include <QTimer>
#include <QVBoxLayout>
#include <mutex>
#include "bible.hpp"
#include "text-fit.hpp"
#include "slides.hpp"
#include "font-cache.hpp"
#include <QInputDialog>
#include <QJsonArray>
#include <QScrollArea>
#include <QFontMetrics>
#include <QVariantAnimation>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-biblia", "es-ES")
MODULE_EXPORT const char *obs_module_description(void) { return "Panel y fuente nativos para proyectar la Biblia"; }

namespace {
constexpr int Width = 1920, Height = 1080;
std::mutex frameMutex;
QImage sharedFrame;
uint64_t sharedRevision = 0;
obs_source_t *sharedMedia = nullptr;
QString sharedVideoPath;

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

QString configPath(const char *relative)
{
    char *path=obs_module_config_path(relative);
    QString result=path?QString::fromUtf8(path):QString(); bfree(path); return result;
}

class Panel final : public QWidget {
public:
    explicit Panel(QWidget *parent = nullptr) : QWidget(parent)
    {
        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(8,8,8,8);
        setMinimumWidth(310);
        setStyleSheet(QStringLiteral("QListWidget { background:#101116; border:1px solid #41434d; border-radius:5px; } QListWidget::item { padding:10px 7px; border-bottom:1px solid #292b34; } QListWidget::item:selected { background:#39435b; color:white; border-left:3px solid #a17bdc; } QPushButton { padding:5px; }"));
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
        auto *navigation = new QHBoxLayout;
        const QStringList labels = {QStringLiteral("« Cap."), QStringLiteral("‹ Vers."), QStringLiteral("Vers. ›"), QStringLiteral("Cap. »")};
        for (int n = 0; n < labels.size(); ++n) {
            auto *button = new QPushButton(labels[n], search); navigation->addWidget(button);
            connect(button, &QPushButton::clicked, this, [this,n] { navigate(n < 2 ? -1 : 1, n == 0 || n == 3); });
        }
        searchLayout->addLayout(navigation);
        passageTitle = new QLabel(QStringLiteral("Selecciona un pasaje"), search); searchLayout->addWidget(passageTitle);
        verseList = new QListWidget(search); verseList->setSelectionMode(QAbstractItemView::ExtendedSelection);
        verseList->setWordWrap(true); verseList->setMinimumHeight(150); searchLayout->addWidget(verseList,1);
        autoProject = new QCheckBox(QStringLiteral("Proyectar al seleccionar / navegar"),search);
        autoProject->setToolTip(QStringLiteral("Actívalo para cambiar la salida con un clic. Desactivado: prepara el pasaje y pulsa Proyectar."));
        searchLayout->addWidget(autoProject);
        auto *project = new QPushButton(QStringLiteral("Proyectar"), search);
        auto *clear = new QPushButton(QStringLiteral("Ocultar pasaje"), search);
        searchLayout->addWidget(project); searchLayout->addWidget(clear);
        auto *import = new QPushButton(QStringLiteral("Importar versión JSON…"), search);
        searchLayout->addWidget(import);
        tabs->addTab(search, QStringLiteral("Biblia"));

        auto *appearance = new QWidget(tabs);
        auto *styleForm = new QFormLayout(appearance);
        presentation = new QComboBox(appearance);
        presentation->addItems({QStringLiteral("Franja inferior"),QStringLiteral("Texto centrado")});
        styleForm->addRow(QStringLiteral("Diseño de proyección"), presentation);
        fonts = new QFontComboBox(appearance); fonts->setCurrentFont(QFont(QStringLiteral("Arial")));
        size = new QSpinBox(appearance); size->setRange(24, 160); size->setValue(64);
        bold = new QCheckBox(QStringLiteral("Negrita"), appearance); bold->setChecked(true);
        italic = new QCheckBox(QStringLiteral("Cursiva"), appearance);
        auto *colorButton = new QPushButton(QStringLiteral("Color del texto…"), appearance);
        backgrounds = new QComboBox(appearance);
        backgrounds->addItems({QStringLiteral("Azul profundo"), QStringLiteral("Amanecer"), QStringLiteral("Bosque"), QStringLiteral("Púrpura"), QStringLiteral("Imagen propia"), QStringLiteral("Video propio"), QStringLiteral("Transparente")});
        auto *imageButton = new QPushButton(QStringLiteral("Escoger imagen…"), appearance);
        auto *videoButton = new QPushButton(QStringLiteral("Escoger video…"), appearance);
        videoLabel = new QLabel(QStringLiteral("Sin video seleccionado"), appearance); videoLabel->setWordWrap(true);
        imageLabel = new QLabel(QStringLiteral("Sin imagen seleccionada"), appearance); imageLabel->setWordWrap(true);
        shade = new QSpinBox(appearance); shade->setRange(0, 90); shade->setSuffix(" %"); shade->setValue(35);
        styleForm->addRow(QStringLiteral("Tipografía"), fonts);
        styleForm->addRow(QStringLiteral("Tamaño de letra (máximo)"), size);
        styleForm->addRow(bold); styleForm->addRow(italic); styleForm->addRow(colorButton);
        bandOpacity = new QSpinBox(appearance); bandOpacity->setRange(0,100); bandOpacity->setValue(85); bandOpacity->setSuffix(" %");
        auto *bandButton = new QPushButton(QStringLiteral("Color de la franja…"),appearance);
        styleForm->addRow(bandButton); styleForm->addRow(QStringLiteral("Opacidad de franja"),bandOpacity);
        auto *hint = new QLabel(QStringLiteral("La apariencia actualiza el pasaje proyectado. El texto se ajusta al espacio disponible."), appearance);
        hint->setWordWrap(true); styleForm->addRow(hint);
        tabs->addTab(appearance, QStringLiteral("Apariencia")); layout->addWidget(tabs);
        auto *backgroundTab = new QWidget(tabs); auto *backgroundForm = new QFormLayout(backgroundTab);
        backgroundForm->addRow(QStringLiteral("Fondo"),backgrounds);
        backgroundForm->addRow(imageButton); backgroundForm->addRow(imageLabel);
        backgroundForm->addRow(videoButton); backgroundForm->addRow(videoLabel);
        backgroundForm->addRow(QStringLiteral("Oscurecer fondo"),shade);
        auto *videoHint = new QLabel(QStringLiteral("Videos locales MP4, MOV, MKV, WEBM o AVI. Se repiten en bucle, sin sonido. El video en movimiento se ve en la fuente de OBS."),backgroundTab);
        videoHint->setWordWrap(true); backgroundForm->addRow(videoHint);
        tabs->addTab(backgroundTab,QStringLiteral("Fondos"));
        preview = new QLabel(this); preview->setMinimumSize(240, 90); preview->setAlignment(Qt::AlignCenter);
        layout->addWidget(preview);
        status = new QLabel(this); status->setWordWrap(true); layout->addWidget(status);
        connect(lookup, &QPushButton::clicked, this, [this] { find(); });
        connect(reference, &QLineEdit::returnPressed, this, [this] { find(); });
        connect(project, &QPushButton::clicked, this, [this] {
            if (selectionDirty) { if (selectPassage()) projectCandidate(); }
            else if (find()) projectCandidate();
        });
        connect(verseList,&QListWidget::itemSelectionChanged,this,[this] {
            if (restoring || browsing) return;
            selectionDirty = true;
            if (selectPassage() && autoProject->isChecked()) projectCandidate();
        });
        connect(verseList,&QListWidget::itemDoubleClicked,this,[this](QListWidgetItem *) { if (selectPassage()) projectCandidate(); });
        connect(clear, &QPushButton::clicked, this, [this] { visible = false; render(); status->setText(QStringLiteral("Pasaje oculto.")); });
        connect(import, &QPushButton::clicked, this, [this] { importVersion(); });
        auto changed = [this] { if (!restoring) render(); };
        connect(fonts, &QFontComboBox::currentFontChanged, this, changed);
        connect(size, qOverload<int>(&QSpinBox::valueChanged), this, changed);
        connect(shade, qOverload<int>(&QSpinBox::valueChanged), this, changed);
        connect(bold, &QCheckBox::toggled, this, changed); connect(italic, &QCheckBox::toggled, this, changed);
        connect(backgrounds, qOverload<int>(&QComboBox::currentIndexChanged), this, changed);
        connect(presentation, qOverload<int>(&QComboBox::currentIndexChanged), this, changed);
        connect(bandOpacity,qOverload<int>(&QSpinBox::valueChanged),this,changed);
        connect(bandButton,&QPushButton::clicked,this,[this] { auto selected=QColorDialog::getColor(bandColor,this,QStringLiteral("Color de la franja")); if(selected.isValid()){ bandColor=selected; render(); } });
        connect(versions, qOverload<int>(&QComboBox::currentIndexChanged), this, [this] { verseList->clear(); selectionDirty=false; });
        connect(reference, &QLineEdit::textEdited, this, [this] { verseList->clear(); selectionDirty=false; });
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
        connect(videoButton,&QPushButton::clicked,this,[this] {
            const auto path=QFileDialog::getOpenFileName(this,QStringLiteral("Seleccionar video de fondo"),{},QStringLiteral("Videos (*.mp4 *.mov *.mkv *.webm *.avi *.m4v)"));
            if(path.isEmpty()) return;
            if(!QFileInfo(path).isFile() || !QFileInfo(path).isReadable()) { status->setText(QStringLiteral("No se puede leer el video.")); return; }
            videoPath=path; videoLabel->setText(QFileInfo(path).fileName()); backgrounds->setCurrentIndex(5); render();
        });
        setupExtensions(tabs, styleForm, searchLayout);
        loadVersions(); loadLibrary(); loadCachedFonts(configPath("fonts")); render();
        status->setText(QStringLiteral("Agrega la fuente «Biblia» a tu escena y busca un pasaje."));
        auto *mediaStatus=new QTimer(this); mediaStatus->setInterval(1000);
        connect(mediaStatus,&QTimer::timeout,this,[this] {
            obs_source_t *media;
            {std::lock_guard<std::mutex> lock(frameMutex); media=obs_source_get_ref(sharedMedia);}
            if(media){
                if(obs_source_media_get_state(media)==OBS_MEDIA_STATE_ERROR)
                    status->setText(QStringLiteral("No se pudo reproducir el video. Escoge otro archivo compatible con OBS."));
                obs_source_release(media);
            }
        }); mediaStatus->start();
    }

    QJsonObject save() const
    {
        return {{"maxLines",maxLines->value()},{"paginate",paginate->isChecked()},{"layoutPolicy",layoutPolicy->currentIndex()},{"css",css->toPlainText()},{"slide",slideIndex},{"transition",transition->currentIndex()}, {"version", versions->currentData().toString()}, {"reference", reference->text()},
                {"font", fonts->currentFont().family()}, {"size", size->value()}, {"bold", bold->isChecked()},
                {"italic", italic->isChecked()}, {"color", color.name()}, {"background", backgrounds->currentIndex()},
                {"image", imagePath}, {"video",videoPath}, {"presentation",presentation->currentIndex()},
                {"bandColor",bandColor.name()}, {"bandOpacity",bandOpacity->value()}, {"autoProject",autoProject->isChecked()},
                {"shade", shade->value()}, {"visible", visible},
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
        backgrounds->setCurrentIndex(qBound(0, o.value("background").toInt(), 6));
        presentation->setCurrentIndex(o.value("presentation").toInt(0));
        bandColor=QColor(o.value("bandColor").toString("#542966")); if(!bandColor.isValid()) bandColor=QColor("#542966");
        bandOpacity->setValue(o.value("bandOpacity").toInt(85)); autoProject->setChecked(o.value("autoProject").toBool(false));
        videoPath=o.value("video").toString(); videoLabel->setText(QFileInfo(videoPath).isFile()?QFileInfo(videoPath).fileName():QStringLiteral("Sin video disponible"));
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
        paginate->setChecked(o.value("paginate").toBool(true)); maxLines->setValue(o.value("maxLines").toInt(3));
        layoutPolicy->setCurrentIndex(qBound(0,o.value("layoutPolicy").toInt(0),2)); css->setPlainText(o.value("css").toString());
        transition->setCurrentIndex(qBound(0,o.value("transition").toInt(0),1)); slideIndex=qMax(0,o.value("slide").toInt());
        restoring = false; verseList->clear(); selectionDirty=false; status->clear(); render();
    }
private:
    QComboBox *versions, *backgrounds, *presentation;
    QComboBox *layoutPolicy, *transition, *themePicker, *listPicker;
    QCheckBox *paginate;
    QSpinBox *maxLines;
    QTextEdit *css;
    QLabel *slideLabel;
    QListWidget *savedPassages;
    QJsonArray themes, lists;
    int slideIndex=0, slideCount=1;
    QNetworkAccessManager *network=nullptr;
    QVariantAnimation *fade=nullptr;
    QImage targetFrame, previousFrame;
    bool libraryLoading=false;

    QLineEdit *reference;
    QFontComboBox *fonts;
    QSpinBox *size, *shade, *bandOpacity;
    QCheckBox *bold, *italic, *autoProject;
    QListWidget *verseList;
    QLabel *preview, *status, *imageLabel, *videoLabel, *passageTitle;
    QVector<Bible> bibles;
    Passage candidate, current;
    QString currentVersion, imagePath, videoPath;
    QColor color = Qt::white;
    QColor bandColor = QColor("#542966");
    QImage customImage;
    bool visible = false, restoring = false, browsing=false, selectionDirty=false;

    void setupExtensions(QTabWidget *tabs,QFormLayout *styleForm,QVBoxLayout *searchLayout)
    {
        paginate=new QCheckBox(QStringLiteral("Dividir automáticamente en diapositivas")); paginate->setChecked(true);
        maxLines=new QSpinBox; maxLines->setRange(1,20); maxLines->setValue(3);
        layoutPolicy=new QComboBox; layoutPolicy->addItems({QStringLiteral("Altura de cada diapositiva"),QStringLiteral("Altura de la más larga"),QStringLiteral("Todo el espacio disponible")});
        transition=new QComboBox; transition->addItems({QStringLiteral("Sin transición"),QStringLiteral("Disolver · 250 ms")});
        styleForm->addRow(paginate); styleForm->addRow(QStringLiteral("Máximo de líneas"),maxLines);
        styleForm->addRow(QStringLiteral("Altura del recuadro"),layoutPolicy); styleForm->addRow(QStringLiteral("Transición"),transition);
        auto *slideButtons=new QHBoxLayout; auto *previous=new QPushButton(QStringLiteral("‹ Diapositiva")); auto *next=new QPushButton(QStringLiteral("Diapositiva ›"));
        slideLabel=new QLabel(QStringLiteral("Diapositiva 1 / 1")); slideButtons->addWidget(previous); slideButtons->addWidget(next);
        searchLayout->addLayout(slideButtons); searchLayout->addWidget(slideLabel);
        connect(previous,&QPushButton::clicked,this,[this]{if(slideIndex>0){--slideIndex;render();}});
        connect(next,&QPushButton::clicked,this,[this]{if(slideIndex+1<slideCount){++slideIndex;render();}});
        auto changed=[this]{if(!restoring){slideIndex=0;render();}};
        connect(paginate,&QCheckBox::toggled,this,changed); connect(maxLines,qOverload<int>(&QSpinBox::valueChanged),this,changed);
        connect(layoutPolicy,qOverload<int>(&QComboBox::currentIndexChanged),this,changed);
        fade=new QVariantAnimation(this); fade->setStartValue(0.0); fade->setEndValue(1.0); fade->setDuration(250);
        connect(fade,&QVariantAnimation::valueChanged,this,[this](const QVariant &value){
            QImage frame(Width,Height,QImage::Format_RGBA8888_Premultiplied); frame.fill(Qt::transparent);
            QPainter painter(&frame); painter.setOpacity(1.0-value.toDouble()); painter.drawImage(0,0,previousFrame);
            painter.setCompositionMode(QPainter::CompositionMode_Plus); painter.setOpacity(value.toDouble()); painter.drawImage(0,0,targetFrame); painter.end();
            std::lock_guard<std::mutex> lock(frameMutex); sharedFrame=frame; ++sharedRevision;
        });
        connect(fade,&QVariantAnimation::finished,this,[this]{std::lock_guard<std::mutex> lock(frameMutex);sharedFrame=targetFrame;++sharedRevision;});

        auto *themeTab=new QWidget; auto *themeForm=new QFormLayout(themeTab);
        themePicker=new QComboBox; themePicker->addItems({QStringLiteral("Clásico púrpura"),QStringLiteral("Noche azul"),QStringLiteral("Luz cálida"),QStringLiteral("Minimalista")});
        auto *apply=new QPushButton(QStringLiteral("Aplicar tema")), *saveTheme=new QPushButton(QStringLiteral("Guardar como tema…"));
        css=new QTextEdit; css->setAcceptRichText(false); css->setMaximumHeight(180);
        css->setPlaceholderText(QStringLiteral("QLabel#verse { color: #fff; font-family: Georgia; }\nQLabel#reference { color: #ffd67a; }"));
        auto *applyCss=new QPushButton(QStringLiteral("Aplicar estilos"));
        themeForm->addRow(QStringLiteral("Tema"),themePicker); themeForm->addRow(apply); themeForm->addRow(saveTheme);
        auto *cssHint=new QLabel(QStringLiteral("CSS nativo de Qt: selectores #frame, #verse, #reference y #version. Colores, fuentes, bordes, fondos y degradados. El diseño y las transiciones se controlan en Apariencia.")); cssHint->setWordWrap(true);
        themeForm->addRow(cssHint); themeForm->addRow(QStringLiteral("Hoja de estilos Qt"),css); themeForm->addRow(applyCss);
        connect(applyCss,&QPushButton::clicked,this,[this]{if(css->toPlainText().size()>32000){status->setText(QStringLiteral("Máximo 32 000 caracteres de estilos."));return;} slideIndex=0;render();});
        connect(apply,&QPushButton::clicked,this,[this]{
            const int index=themePicker->currentIndex(); auto settings=save();
            if(index<4){
                static const char *colors[]={"#542966","#173553","#594031","#202020"};
                settings["bandColor"]=colors[index]; settings["bandOpacity"]=index==3?65:85;
                settings["background"]=index==1?0:index==2?1:index==3?6:3;
                settings["css"]=index==2?QStringLiteral("QLabel#reference { color:#ffdc91; } QLabel#verse { font-family:Georgia; }"):QString();
            }else{
                const auto style=themes[index-4].toObject().value("style").toObject();
                for(auto it=style.begin();it!=style.end();++it)settings[it.key()]=it.value();
            }
            restore(settings); status->setText(QStringLiteral("Tema aplicado."));
        });
        connect(saveTheme,&QPushButton::clicked,this,[this]{
            bool ok=false; auto name=QInputDialog::getText(this,QStringLiteral("Guardar tema"),QStringLiteral("Nombre"),QLineEdit::Normal,{},&ok).trimmed();
            if(!ok||name.isEmpty())return;
            if(themes.size()>=100){status->setText(QStringLiteral("Máximo 100 temas personales."));return;}
            QJsonObject style; const auto settings=save();
            for(const auto &key:styleKeys())style[key]=settings.value(key);
            themes.append(QJsonObject{{"name",name.left(100)},{"style",style}});
            if(saveLibrary()){themePicker->addItem(name.left(100));themePicker->setCurrentIndex(themePicker->count()-1);}
            else themes.removeLast();
        });
        network=new QNetworkAccessManager(this);
        auto *googleFamily=new QLineEdit; googleFamily->setPlaceholderText(QStringLiteral("Ejemplo: Lora"));
        auto *googleButton=new QPushButton(QStringLiteral("Descargar Google Font (Internet)…"));
        auto *fontButton=new QPushButton(QStringLiteral("Importar fuente TTF/OTF local…"));
        themeForm->addRow(QStringLiteral("Google Fonts"),googleFamily); themeForm->addRow(googleButton); themeForm->addRow(fontButton);
        connect(googleButton,&QPushButton::clicked,this,[this,googleFamily,googleButton]{
            googleButton->setEnabled(false); status->setText(QStringLiteral("Descargando fuente…"));
            downloadGoogleFont(network,googleFamily->text(),configPath("fonts"),[this,googleButton](QString family,QString error){
                googleButton->setEnabled(true); if(!error.isEmpty()){status->setText(error);return;}
                fonts->setCurrentFont(QFont(family));render();status->setText(QStringLiteral("Fuente guardada para uso sin conexión: ")+family);
            });
        });
        connect(fontButton,&QPushButton::clicked,this,[this]{
            auto path=QFileDialog::getOpenFileName(this,QStringLiteral("Importar tipografía"),{},QStringLiteral("Fuentes (*.ttf *.otf)")); if(path.isEmpty())return;
            QFile file(path); if(file.size()>10*1024*1024||!file.open(QIODevice::ReadOnly)){status->setText(QStringLiteral("No se pudo abrir la fuente (máximo 10 MB)."));return;}
            auto bytes=file.readAll(); int id=QFontDatabase::addApplicationFontFromData(bytes); auto families=QFontDatabase::applicationFontFamilies(id);
            if(id<0||families.isEmpty()){status->setText(QStringLiteral("Fuente no compatible."));return;}
            QDir().mkpath(configPath("fonts")); QSaveFile output(QDir(configPath("fonts")).filePath(QString::fromLatin1(QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex())+"."+QFileInfo(path).suffix().toLower()));
            if(!output.open(QIODevice::WriteOnly)||output.write(bytes)!=bytes.size()||!output.commit()){status->setText(QStringLiteral("No se pudo guardar la fuente."));return;}
            fonts->setCurrentFont(QFont(families.first())); render();
        });
        auto *themeScroll=new QScrollArea;themeScroll->setWidgetResizable(true);themeScroll->setWidget(themeTab);tabs->addTab(themeScroll,QStringLiteral("Temas"));
        auto *appearance=tabs->widget(1);tabs->removeTab(1);auto *appearanceScroll=new QScrollArea;appearanceScroll->setWidgetResizable(true);appearanceScroll->setWidget(appearance);tabs->insertTab(1,appearanceScroll,QStringLiteral("Apariencia"));
        auto *search=tabs->widget(0);tabs->removeTab(0);auto *searchScroll=new QScrollArea;searchScroll->setWidgetResizable(true);searchScroll->setWidget(search);tabs->insertTab(0,searchScroll,QStringLiteral("Biblia"));
        tabs->setCurrentIndex(0);

        auto *listTab=new QWidget;auto *listLayout=new QVBoxLayout(listTab);listPicker=new QComboBox;
        auto *createList=new QPushButton(QStringLiteral("Nueva lista…"));auto *add=new QPushButton(QStringLiteral("Añadir el pasaje preparado"));
        savedPassages=new QListWidget;auto *show=new QPushButton(QStringLiteral("Proyectar seleccionado"));
        auto *remove=new QPushButton(QStringLiteral("Quitar seleccionado")); auto *up=new QPushButton(QStringLiteral("Subir")),*down=new QPushButton(QStringLiteral("Bajar"));
        listLayout->addWidget(listPicker);listLayout->addWidget(createList);listLayout->addWidget(add);listLayout->addWidget(savedPassages,1);
        auto *order=new QHBoxLayout;order->addWidget(up);order->addWidget(down);listLayout->addLayout(order);listLayout->addWidget(show);listLayout->addWidget(remove);
        auto *listHint=new QLabel(QStringLiteral("Cada entrada guarda versión y referencia. Se consulta la Biblia local al proyectar. Las listas se conservan al cerrar OBS."));listHint->setWordWrap(true);listLayout->addWidget(listHint);
        tabs->addTab(listTab,QStringLiteral("Listas"));
        connect(listPicker,qOverload<int>(&QComboBox::currentIndexChanged),this,[this]{refreshList();});
        connect(createList,&QPushButton::clicked,this,[this]{
            bool ok=false;auto name=QInputDialog::getText(this,QStringLiteral("Nueva lista"),QStringLiteral("Nombre"),QLineEdit::Normal,{},&ok).trimmed();if(!ok||name.isEmpty())return;
            if(lists.size()>=100){status->setText(QStringLiteral("Máximo 100 listas."));return;}
            lists.append(QJsonObject{{"name",name.left(100)},{"entries",QJsonArray()}});
            if(saveLibrary()){listPicker->addItem(name.left(100));listPicker->setCurrentIndex(listPicker->count()-1);}else lists.removeLast();
        });
        connect(add,&QPushButton::clicked,this,[this]{
            if(listPicker->currentIndex()<0){status->setText(QStringLiteral("Crea una lista primero."));return;}
            if(selectionDirty?!selectPassage():!find())return;
            int index=listPicker->currentIndex();auto list=lists[index].toObject();auto entries=list["entries"].toArray();
            if(entries.size()>=5000){status->setText(QStringLiteral("Máximo 5000 entradas por lista."));return;}
            entries.append(QJsonObject{{"version",versions->currentData().toString()},{"reference",candidate.reference}});list["entries"]=entries;
            auto old=lists[index];lists[index]=list;if(!saveLibrary())lists[index]=old;refreshList();
        });
        auto projectSaved=[this]{
            int index=listPicker->currentIndex(),row=savedPassages->currentRow();if(index<0||row<0)return;
            auto entry=lists[index].toObject()["entries"].toArray()[row].toObject();int version=versions->findData(entry["version"].toString());
            if(version<0){status->setText(QStringLiteral("Importa la versión de esta entrada: ")+entry["version"].toString());return;}
            versions->setCurrentIndex(version);reference->setText(entry["reference"].toString());selectionDirty=false;if(find())projectCandidate();
        };
        connect(show,&QPushButton::clicked,this,projectSaved);connect(savedPassages,&QListWidget::itemDoubleClicked,this,[projectSaved](QListWidgetItem*){projectSaved();});
        connect(remove,&QPushButton::clicked,this,[this]{changeListEntry(0);});
        connect(up,&QPushButton::clicked,this,[this]{changeListEntry(-1);});connect(down,&QPushButton::clicked,this,[this]{changeListEntry(1);});
    }
    QStringList styleKeys() const
    {
        return {"font","size","bold","italic","color","background","image","video","presentation","bandColor","bandOpacity","shade","maxLines","paginate","layoutPolicy","css","transition"};
    }
    bool saveLibrary()
    {
        const auto directory=configPath("library");QDir().mkpath(directory);
        QSaveFile file(QDir(directory).filePath("library.json"));auto bytes=QJsonDocument(QJsonObject{{"themes",themes},{"lists",lists}}).toJson();
        if(!file.open(QIODevice::WriteOnly)||file.write(bytes)!=bytes.size()||!file.commit()){status->setText(QStringLiteral("No se pudo guardar temas y listas."));return false;}return true;
    }
    void loadLibrary()
    {
        QFile file(QDir(configPath("library")).filePath("library.json"));if(!file.exists())return;
        if(file.size()>8*1024*1024||!file.open(QIODevice::ReadOnly)){status->setText(QStringLiteral("No se pudo abrir la biblioteca local."));return;}
        QJsonParseError error;auto doc=QJsonDocument::fromJson(file.readAll(),&error);
        if(error.error!=QJsonParseError::NoError||!doc.isObject()){status->setText(QStringLiteral("Biblioteca local inválida."));return;}
        const auto object=doc.object();
        for(const auto &value:object["themes"].toArray()){
            auto item=value.toObject();if(themes.size()>=100)break;if(item["name"].toString().isEmpty()||!item["style"].isObject())continue;
            QJsonObject style;auto original=item["style"].toObject();for(const auto &key:styleKeys())if(original.contains(key))style[key]=original[key];
            if(style["css"].toString().size()>32000)continue;item["style"]=style;themes.append(item);themePicker->addItem(item["name"].toString());
        }
        for(const auto &value:object["lists"].toArray()){
            auto item=value.toObject();if(lists.size()>=100)break;if(item["name"].toString().isEmpty()||!item["entries"].isArray())continue;
            QJsonArray entries;for(const auto &value:item["entries"].toArray()){auto entry=value.toObject();if(entries.size()>=5000)break;if(!entry["version"].toString().isEmpty()&&!entry["reference"].toString().isEmpty())entries.append(entry);}
            item["entries"]=entries;lists.append(item);listPicker->addItem(item["name"].toString());
        }
        refreshList();
    }
    void refreshList()
    {
        savedPassages->clear();int index=listPicker->currentIndex();if(index<0||index>=lists.size())return;
        for(const auto &value:lists[index].toObject()["entries"].toArray()){auto entry=value.toObject();savedPassages->addItem(entry["reference"].toString()+QStringLiteral(" · ")+entry["version"].toString());}
    }
    void changeListEntry(int direction)
    {
        int index=listPicker->currentIndex(),row=savedPassages->currentRow();if(index<0||row<0)return;
        auto list=lists[index].toObject();auto entries=list["entries"].toArray();int next=row+direction;
        if(direction==0)entries.removeAt(row);
        else{if(next<0||next>=entries.size())return;auto value=entries[row];entries[row]=entries[next];entries[next]=value;}
        list["entries"]=entries;auto old=lists[index];lists[index]=list;if(!saveLibrary())lists[index]=old;
        refreshList();savedPassages->setCurrentRow(qMin(next,savedPassages->count()-1));
    }

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
            verseList->clear(); selectionDirty=false; status->setText(error); return false;
        }
        QVector<Passage> chapter;
        if (!bibles[index].chapterVerses(candidate.reference,chapter,error)) { status->setText(error); return false; }
        browsing=true; verseList->clear();
        const QString base=candidate.reference.section(':',0,0);
        const QString range=candidate.reference.section(':',1,1);
        const int first=range.isEmpty()?1:range.section('-',0,0).toInt();
        const int last=range.contains('-')?range.section('-',1,1).toInt():(range.isEmpty()?201:first);
        QListWidgetItem *firstItem=nullptr;
        for(const auto &verse:chapter){
            const int number=verse.reference.section(':',1,1).toInt();
            auto *item=new QListWidgetItem(QString::number(number)+QStringLiteral("   ")+verse.text,verseList);
            item->setData(Qt::UserRole,verse.reference);
            if(number>=first && number<=last){ item->setSelected(true); if(!firstItem) firstItem=item; }
        }
        if(firstItem) verseList->scrollToItem(firstItem,QAbstractItemView::PositionAtCenter);
        browsing=false; selectionDirty=false;
        passageTitle->setText(base+QStringLiteral(" · ")+bibles[index].name);
        status->setText(QStringLiteral("Pasaje encontrado. Pulsa Proyectar para mostrarlo.")); return true;
    }
    bool selectPassage()
    {
        const auto selected=verseList->selectedItems();
        if(selected.isEmpty()) { status->setText(QStringLiteral("Selecciona uno o varios versículos contiguos.")); return false; }
        int first=verseList->count(),last=-1;
        for(auto *item:selected){ int row=verseList->row(item); first=qMin(first,row); last=qMax(last,row); }
        if(last-first+1!=selected.size()){ status->setText(QStringLiteral("Selecciona un rango continuo de versículos.")); return false; }
        const auto start=verseList->item(first)->data(Qt::UserRole).toString();
        QString ref=start;
        if(last>first) ref+='-'+verseList->item(last)->data(Qt::UserRole).toString().section(':',1,1);
        const int index=versions->currentIndex(); QString error;
        if(index<0 || index>=bibles.size() || !bibles[index].lookup(ref,candidate,error)){ status->setText(error); return false; }
        reference->setText(candidate.reference);
        status->setText(QStringLiteral("Seleccionado %1. Pulsa Proyectar o haz doble clic.").arg(candidate.reference)); return true;
    }
    void projectCandidate()
    {
        const auto old=current; const auto oldVersion=currentVersion; const bool oldVisible=visible;
        const int oldSlide=slideIndex; slideIndex=0;
        current=candidate; currentVersion=versions->currentText(); visible=true;
        if(!render()){current=old; currentVersion=oldVersion; visible=oldVisible; slideIndex=oldSlide;}
        else status->setText(QStringLiteral("Proyectando %1 · %2").arg(current.reference,currentVersion));
    }
    void navigate(int direction,bool wholeChapter)
    {
        const int index=versions->currentIndex(); Passage next; QString error;
        if(index<0 || index>=bibles.size()) return;
        if(!bibles[index].adjacent(reference->text(),direction,wholeChapter,next,error)){ status->setText(QStringLiteral("No hay otro pasaje disponible en esa dirección.")); return; }
        reference->setText(next.reference); selectionDirty=false;
        if(find() && autoProject->isChecked()) projectCandidate();
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
        QImage image(Width, Height, QImage::Format_RGBA8888_Premultiplied); image.fill(Qt::transparent);
        if (visible && !current.text.isEmpty()) {
            if (backgrounds->currentIndex() == 4 && customImage.isNull()) {
                status->setText(QStringLiteral("Selecciona una imagen de fondo válida.")); return false;
            }
            if(backgrounds->currentIndex()==5 && (!QFileInfo(videoPath).isFile() || !QFileInfo(videoPath).isReadable())){
                status->setText(QStringLiteral("Selecciona un video de fondo válido.")); return false;
            }
            QPainter painter(&image); painter.setRenderHint(QPainter::Antialiasing); painter.setRenderHint(QPainter::TextAntialiasing);
            if (backgrounds->currentIndex() == 4) {
                const auto scaled = customImage.scaled(Width, Height, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
                painter.drawImage(QPoint((Width-scaled.width())/2, (Height-scaled.height())/2), scaled);
            } else if(backgrounds->currentIndex()<4) {
                static const char *starts[] = {"#07172f", "#3e2145", "#082c25", "#241244"};
                static const char *ends[] = {"#245b85", "#c57c46", "#467c58", "#795898"};
                const int index = qBound(0, backgrounds->currentIndex(), 3);
                QLinearGradient gradient(0, 0, Width, Height); gradient.setColorAt(0, QColor(starts[index])); gradient.setColorAt(1, QColor(ends[index]));
                painter.fillRect(image.rect(), gradient);
                painter.setPen(Qt::NoPen); painter.setBrush(QColor(255,255,255,12));
                painter.drawEllipse(QPoint(1600,200), 500,500); painter.drawEllipse(QPoint(150,1000), 600,600);
            }
            if(backgrounds->currentIndex()!=6) painter.fillRect(image.rect(), QColor(0,0,0,shade->value()*255/100));
            QFont font=fonts->currentFont(); font.setBold(bold->isChecked()); font.setItalic(italic->isChecked()); font.setPixelSize(size->value());
            const bool lower=presentation->currentIndex()==0;
            QWidget frame; frame.setObjectName("frame"); frame.setAttribute(Qt::WA_TranslucentBackground); frame.setAttribute(Qt::WA_StyledBackground);
            QLabel verse(&frame), ref(&frame), version(&frame);
            verse.setObjectName("verse"); ref.setObjectName("reference"); version.setObjectName("version");
            for(auto *label:{&verse,&ref,&version}) {label->setTextFormat(Qt::PlainText); label->setFont(font);}
            verse.setWordWrap(false); verse.setAlignment(lower?Qt::AlignLeft|Qt::AlignVCenter:Qt::AlignCenter);
            ref.setAlignment(Qt::AlignLeft|Qt::AlignVCenter); version.setAlignment(Qt::AlignRight|Qt::AlignVCenter);
            QString base=QStringLiteral("QLabel { color:%1; background:transparent; padding:0; border:0; } QLabel#reference {font-size:36px; font-weight:bold;} QLabel#version {font-size:28px;}").arg(color.name());
            frame.setStyleSheet(base+"\n"+css->toPlainText()); frame.ensurePolished(); verse.ensurePolished(); ref.ensurePolished(); version.ensurePolished();
            font=verse.font();
            const int textWidth=lower?Width-120:Width-260;
            const int availableHeight=lower?Height-190:Height-350;
            const int lineHeight=QFontMetrics(font).lineSpacing();
            if(lineHeight>availableHeight){status->setText(QStringLiteral("La fuente del tema es demasiado grande para el espacio disponible."));return false;}
            QStringList slides;
            int pixel=font.pixelSize()>0?font.pixelSize():size->value();
            if(paginate->isChecked()) {
                int effectiveLines=qMin(maxLines->value(),qMax(1,availableHeight/qMax(1,lineHeight)));
                slides=passageSlides(current.text,font,textWidth,effectiveLines);
            } else {
                const QSize area(textWidth,lower?168:Height-350);
                pixel=fitPassage(painter,font,current.text,area,size->value());
                if(!pixel){status->setText(QStringLiteral("Activa dividir en diapositivas o selecciona un pasaje más corto."));return false;}
                font.setPixelSize(pixel); verse.setFont(font);
                slides=passageSlides(current.text,font,textWidth,100000);
            }
            if(slides.isEmpty()){status->setText(QStringLiteral("No se pudo dividir el pasaje."));return false;}
            slideCount=slides.size(); slideIndex=qBound(0,slideIndex,slideCount-1);
            slideLabel->setText(QStringLiteral("Diapositiva %1 / %2").arg(slideIndex+1).arg(slideCount));
            int lines=slides[slideIndex].count('\n')+1;
            if(layoutPolicy->currentIndex()==1) for(const auto &slide:slides) lines=qMax(lines,int(slide.count('\n')+1));
            const int bodyHeight=layoutPolicy->currentIndex()==2?availableHeight:qMin(availableHeight,lines*QFontMetrics(font).lineSpacing()+32);
            const int frameHeight=bodyHeight+100;
            const int top=lower?Height-frameHeight-24:(Height-frameHeight)/2;
            frame.setGeometry(lower?24:94,top,lower?Width-48:Width-188,frameHeight);
            if(lower || layoutPolicy->currentIndex()!=2){
                QColor body=bandColor; body.setAlpha(bandOpacity->value()*255/100);
                painter.fillRect(frame.geometry(),body);
                QColor header=bandColor.lighter(140); header.setAlpha(body.alpha()); painter.fillRect(QRect(frame.x(),top,frame.width(),60),header);
            }
            verse.setGeometry(36,76,textWidth,bodyHeight);
            ref.setGeometry(36,0,850,60); version.setGeometry(946,0,frame.width()-982,60);
            verse.setText(slides[slideIndex]);
            ref.setText(current.reference+(slideCount>1?QStringLiteral(" · %1/%2").arg(slideIndex+1).arg(slideCount):QString()));
            version.setText(QFontMetrics(version.font()).elidedText(currentVersion,Qt::ElideRight,version.width()));
            frame.render(&painter,QPoint(frame.x(),frame.y()),QRegion(),QWidget::DrawWindowBackground|QWidget::DrawChildren);

        }
        QImage panelPreview=image;
        const QString desiredVideo=visible && backgrounds->currentIndex()==5?videoPath:QString();
        if(!desiredVideo.isEmpty()){
            panelPreview=QImage(Width,Height,QImage::Format_RGBA8888_Premultiplied); panelPreview.fill(QColor("#171c29"));
            QPainter p(&panelPreview); p.setPen(Qt::white); QFont f; f.setPixelSize(48); p.setFont(f);
            p.drawText(QRect(100,100,Width-200,500),Qt::AlignCenter|Qt::TextWordWrap,QStringLiteral("VIDEO · ")+QFileInfo(videoPath).fileName()+QStringLiteral("\nReproducción en la fuente de OBS"));
            p.drawImage(0,0,image);
        }
        preview->setPixmap(QPixmap::fromImage(panelPreview.scaled(280,158,Qt::KeepAspectRatio,Qt::SmoothTransformation)));
        obs_source_t *replacement=nullptr;
        bool replace=false;
        {std::lock_guard<std::mutex> lock(frameMutex); replace=desiredVideo!=sharedVideoPath;}
        if(replace && !desiredVideo.isEmpty()){
            auto *settings=obs_data_create(); const auto path=desiredVideo.toUtf8();
            obs_data_set_bool(settings,"is_local_file",true); obs_data_set_string(settings,"local_file",path.constData());
            obs_data_set_bool(settings,"looping",true); obs_data_set_bool(settings,"restart_on_activate",true);
            obs_data_set_bool(settings,"clear_on_media_end",false); obs_data_set_bool(settings,"close_when_inactive",true);
            replacement=obs_source_create_private("ffmpeg_source","Biblia · video de fondo",settings); obs_data_release(settings);
            if(!replacement){ status->setText(QStringLiteral("OBS no pudo crear el fondo de video. Comprueba el módulo multimedia.")); return false; }
            obs_source_set_muted(replacement,true); obs_source_set_volume(replacement,0.0f);
        }
        obs_source_t *previous=nullptr;
        {
            std::lock_guard<std::mutex> lock(frameMutex);
            if(replace){previous=sharedMedia; sharedMedia=replacement; sharedVideoPath=desiredVideo;}
            if(fade->state()==QAbstractAnimation::Running) fade->stop();
            previousFrame=sharedFrame; targetFrame=image;
            if(transition->currentIndex()==0 || previousFrame.isNull()){sharedFrame=image; ++sharedRevision;}
        }
        if(transition->currentIndex()==1 && !previousFrame.isNull()) fade->start();
        obs_source_release(previous); return true;
    }
};
QPointer<Panel> panel;

struct Source {
    gs_texture_t *texture=nullptr;
    gs_texrender_t *mediaCanvas=nullptr;
    uint64_t revision=0;
    obs_source_t *parent=nullptr, *media=nullptr;
    std::mutex mediaMutex;
};
const char *sourceName(void *) { return "Biblia"; }
void *createSource(obs_data_t *, obs_source_t *parent) { auto *source=new Source; source->parent=parent; return source; }
void tickSource(void *data,float)
{
    auto *source=static_cast<Source *>(data); obs_source_t *media;
    {std::lock_guard<std::mutex> lock(frameMutex); media=obs_source_get_ref(sharedMedia);}
    obs_source_t *previous=nullptr; bool changed=false;
    {
        std::lock_guard<std::mutex> lock(source->mediaMutex);
        if(media!=source->media){ previous=source->media; source->media=media; changed=true; }
    }
    if(!changed){obs_source_release(media); return;}
    if(previous){obs_source_remove_active_child(source->parent,previous); obs_source_release(previous);}
    if(media) obs_source_add_active_child(source->parent,media);
}
void enumerateMedia(void *data,obs_source_enum_proc_t callback,void *param)
{
    auto *source=static_cast<Source *>(data); obs_source_t *media;
    {std::lock_guard<std::mutex> lock(source->mediaMutex); media=obs_source_get_ref(source->media);}
    if(media){ callback(source->parent,media,param); obs_source_release(media); }
}
void destroySource(void *data)
{
    auto *source = static_cast<Source *>(data);
    if(source->media){obs_source_remove_active_child(source->parent,source->media); obs_source_release(source->media);}
    obs_enter_graphics();
    gs_texture_destroy(source->texture); gs_texrender_destroy(source->mediaCanvas); obs_leave_graphics(); delete source;
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
    obs_source_t *media;
    {std::lock_guard<std::mutex> lock(source->mediaMutex); media=obs_source_get_ref(source->media);}
    if(media){
        const auto width=obs_source_get_width(media),height=obs_source_get_height(media);
        if(width && height){
            if(!source->mediaCanvas) source->mediaCanvas=gs_texrender_create(GS_RGBA,GS_ZS_NONE);
            if(source->mediaCanvas){
                gs_texrender_reset(source->mediaCanvas);
                if(gs_texrender_begin(source->mediaCanvas,Width,Height)){
                    vec4 clear; vec4_zero(&clear); gs_clear(GS_CLEAR_COLOR,&clear,0.0f,0);
                    gs_ortho(0,float(Width),0,float(Height),-100.0f,100.0f);
                    const float scale=qMax(float(Width)/float(width),float(Height)/float(height));
                    gs_matrix_push(); gs_matrix_translate3f((Width-float(width)*scale)/2.0f,(Height-float(height)*scale)/2.0f,0.0f);
                    gs_matrix_scale3f(scale,scale,1.0f); obs_source_video_render(media); gs_matrix_pop();
                    gs_texrender_end(source->mediaCanvas);
                    obs_source_draw(gs_texrender_get_texture(source->mediaCanvas),0,0,Width,Height,false);
                }
            }
        }
        obs_source_release(media);
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
    info.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW | OBS_SOURCE_COMPOSITE | OBS_SOURCE_CAP_DONT_SHOW_PROPERTIES;
    info.get_name = sourceName; info.create = createSource; info.destroy = destroySource;
    info.get_width = getWidth; info.get_height = getHeight; info.video_render = renderSource;
    info.video_tick=tickSource; info.enum_active_sources=enumerateMedia; info.enum_all_sources=enumerateMedia;
    // Composite sources require this callback even when their video is muted.
    info.audio_render=[](void *,uint64_t *,obs_source_audio_mix *,uint32_t,size_t,size_t){return false;};
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
    obs_source_t *media;
    {std::lock_guard<std::mutex> lock(frameMutex); media=sharedMedia; sharedMedia=nullptr; sharedVideoPath.clear(); sharedFrame=QImage();}
    obs_source_release(media);
}
