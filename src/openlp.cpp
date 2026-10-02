#include "openlp.hpp"
#include "bible.hpp"
#include <QFileInfo>
#include <QJsonDocument>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QUuid>

bool convertOpenLp(const QString &path, const QString &id, const QString &name,
                   const QString &license, QJsonObject &output, QString &error)
{
    if (!QFileInfo(path).isFile() || id.trimmed().isEmpty() || name.trimmed().isEmpty() || license.trimmed().isEmpty()) {
        error = QStringLiteral("Selecciona una Biblia local y completa identificador, nombre y licencia."); return false;
    }
    const auto connection = QUuid::createUuid().toString();
    bool success = false;
    {
        auto db = QSqlDatabase::addDatabase("QSQLITE", connection);
        db.setConnectOptions("QSQLITE_OPEN_READONLY"); db.setDatabaseName(path);
        if (!db.open()) error = QStringLiteral("No se pudo abrir SQLite: ") + db.lastError().text();
        else {
            QSqlQuery query(db); query.setForwardOnly(true);
            if (!query.exec("SELECT book.name, verse.chapter, verse.verse, verse.text FROM verse LEFT JOIN book ON book.id = verse.book_id ORDER BY book.id, verse.chapter, verse.verse"))
                error = QStringLiteral("Se requiere una Biblia SQLite sin conexión de OpenLP (tablas book y verse): ") + query.lastError().text();
            else {
                QJsonObject books; int count = 0; bool valid = true;
                while (query.next()) {
                    const QString book = query.value(0).toString().trimmed(), text = query.value(3).toString().trimmed();
                    bool chapterOk = false, verseOk = false;
                    const int chapter = query.value(1).toInt(&chapterOk), verse = query.value(2).toInt(&verseOk);
                    auto chapters = books.value(book).toObject(); auto verses = chapters.value(QString::number(chapter)).toObject();
                    if (book.isEmpty() || text.isEmpty() || !chapterOk || !verseOk || chapter < 1 || verse < 1 || verses.contains(QString::number(verse)) || ++count > 100000) {
                        error = QStringLiteral("La Biblia contiene registros vacíos, duplicados o inválidos."); valid = false; break;
                    }
                    verses.insert(QString::number(verse), text); chapters.insert(QString::number(chapter), verses); books.insert(book, chapters);
                }
                if (query.lastError().isValid()) { error = query.lastError().text(); valid = false; }
                if (valid) {
                    QJsonObject candidate{{"id",id.trimmed()},{"name",name.trimmed()},{"license",license.trimmed()},{"books",books}};
                    const auto bytes = QJsonDocument(candidate).toJson(); Bible bible;
                    if (bytes.size() > 32 * 1024 * 1024) error = QStringLiteral("El resultado supera el máximo de 32 MB.");
                    else if (bible.load(bytes,error)) { output = candidate; success = true; }
                }
            }
        }
    }
    QSqlDatabase::removeDatabase(connection); return success;
}
