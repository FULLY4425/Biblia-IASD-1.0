#include "openlp.hpp"
#include "bible.hpp"
#include <QFileInfo>
#include <QJsonDocument>
#include <sqlite3.h>
#include <memory>

bool convertOpenLp(const QString &path, const QString &id, const QString &name,
                   const QString &license, QJsonObject &output, QString &error)
{
    if (!QFileInfo(path).isFile() || QFileInfo(path).size()>64*1024*1024 || id.trimmed().isEmpty() || name.trimmed().isEmpty() || license.trimmed().isEmpty()) {
        error = QStringLiteral("Selecciona una Biblia local (máximo 64 MB) y completa identificador, nombre y licencia."); return false;
    }
    sqlite3 *raw=nullptr;
    const int opened=sqlite3_open_v2(path.toUtf8().constData(),&raw,SQLITE_OPEN_READONLY,nullptr);
    std::unique_ptr<sqlite3,decltype(&sqlite3_close)> db(raw,&sqlite3_close);
    if(opened!=SQLITE_OK){error=QStringLiteral("No se pudo abrir SQLite: ")+QString::fromUtf8(raw?sqlite3_errmsg(raw):"error");return false;}
    sqlite3_busy_timeout(raw,1000);
    sqlite3_stmt *statement=nullptr;
    const char *sql="SELECT book.name, verse.chapter, verse.verse, verse.text FROM verse LEFT JOIN book ON book.id = verse.book_id ORDER BY book.id, verse.chapter, verse.verse";
    const int prepared=sqlite3_prepare_v2(raw,sql,-1,&statement,nullptr);
    std::unique_ptr<sqlite3_stmt,decltype(&sqlite3_finalize)> query(statement,&sqlite3_finalize);
    if(prepared!=SQLITE_OK){error=QStringLiteral("Se requiere una Biblia SQLite sin conexión de OpenLP (tablas book y verse): ")+QString::fromUtf8(sqlite3_errmsg(raw));return false;}
    QJsonObject books; int count=0,step=SQLITE_OK;
    auto column=[&](int index){const auto *value=sqlite3_column_text(statement,index);return value?QString::fromUtf8(reinterpret_cast<const char*>(value)):QString();};
    while((step=sqlite3_step(statement))==SQLITE_ROW){
        const QString book=column(0).trimmed(),text=column(3).trimmed();bool chapterOk=false,verseOk=false;
        const int chapter=column(1).toInt(&chapterOk),verse=column(2).toInt(&verseOk);
        auto chapters=books.value(book).toObject();auto verses=chapters.value(QString::number(chapter)).toObject();
        if(book.isEmpty()||text.isEmpty()||!chapterOk||!verseOk||chapter<1||verse<1||verses.contains(QString::number(verse))||++count>100000){
            error=QStringLiteral("La Biblia contiene registros vacíos, duplicados o inválidos.");return false;
        }
        verses.insert(QString::number(verse),text);chapters.insert(QString::number(chapter),verses);books.insert(book,chapters);
    }
    if(step!=SQLITE_DONE){error=QString::fromUtf8(sqlite3_errmsg(raw));return false;}
    QJsonObject candidate{{"id",id.trimmed()},{"name",name.trimmed()},{"license",license.trimmed()},{"books",books}};
    auto bytes=QJsonDocument(candidate).toJson();Bible bible;
    if(bytes.size()>32*1024*1024){error=QStringLiteral("El resultado supera el máximo de 32 MB.");return false;}
    if(!bible.load(bytes,error))return false;
    output=candidate;return true;
}
