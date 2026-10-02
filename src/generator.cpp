#include "openlp.hpp"
#include <QApplication>
#include <QFileDialog>
#include <QFormLayout>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSaveFile>
#include <QWidget>

int main(int argc, char **argv)
{
    QApplication app(argc,argv); QWidget window;
    window.setWindowTitle(QStringLiteral("Generador de Biblias · OBS Biblia")); window.resize(560,320);
    auto *form = new QFormLayout(&window);
    auto *path = new QLineEdit, *id = new QLineEdit, *name = new QLineEdit, *license = new QLineEdit;
    auto *choose = new QPushButton(QStringLiteral("Seleccionar Biblia OpenLP…"));
    auto *generate = new QPushButton(QStringLiteral("Generar archivo para OBS…"));
    auto *status = new QLabel(QStringLiteral("Convierte Biblias SQLite locales de OpenLP. No utiliza Internet. Importa el JSON resultante desde el panel Biblia de OBS.")); status->setWordWrap(true);
    form->addRow(choose); form->addRow(QStringLiteral("Archivo SQLite"),path);
    form->addRow(QStringLiteral("Identificador único"),id); form->addRow(QStringLiteral("Nombre de la versión"),name);
    form->addRow(QStringLiteral("Licencia / permiso del texto"),license); form->addRow(generate); form->addRow(status);
    QObject::connect(choose,&QPushButton::clicked,&window,[&] {
        auto selected=QFileDialog::getOpenFileName(&window,QStringLiteral("Biblia OpenLP"),{},QStringLiteral("Biblia SQLite (*.sqlite *.sqlite3 *.db);;Todos los archivos (*)"));
        if(!selected.isEmpty()) path->setText(selected);
    });
    QObject::connect(generate,&QPushButton::clicked,&window,[&] {
        QJsonObject output; QString error;
        if(!convertOpenLp(path->text(),id->text(),name->text(),license->text(),output,error)){ status->setText(error); return; }
        auto target=QFileDialog::getSaveFileName(&window,QStringLiteral("Guardar Biblia JSON"),id->text()+".json",QStringLiteral("Biblia JSON (*.json)"));
        if(target.isEmpty()) return;
        QSaveFile file(target); auto bytes=QJsonDocument(output).toJson();
        if(!file.open(QIODevice::WriteOnly)||file.write(bytes)!=bytes.size()||!file.commit()){status->setText(QStringLiteral("No se pudo guardar el archivo."));return;}
        status->setText(QStringLiteral("Biblia generada. En OBS: Biblia → Importar versión JSON."));
    });
    window.show(); return app.exec();
}
