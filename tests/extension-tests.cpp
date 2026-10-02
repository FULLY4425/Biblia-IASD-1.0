#include "slides.hpp"
#include "native-style.hpp"
#include "presentation-options.hpp"
#include "openlp.hpp"
#include "bible.hpp"
#include <QApplication>
#include <QFile>
#include <QJsonDocument>
#include <QLabel>
#include <sqlite3.h>
#include <QTemporaryDir>
#include <iostream>

int main(int argc,char **argv)
{
    QApplication app(argc,argv); int failures=0;
    auto check=[&](bool value,const char *name){std::cout<<(value?"PASS ":"FAIL ")<<name<<'\n';if(!value)++failures;};
    check(cleanPassage("1. Uno[1]\r\ncontinuación\r\n2. Dos[2]",true,true)==QStringLiteral("1. Uno continuación\n2. Dos"),"cleanup preserves verse boundaries");
    check(cleanPassage("Texto[1]\nsegundo",false,false)=="Texto[1]\nsegundo","cleanup can be disabled");
    check(slideLetter(0)=="a" && slideLetter(25)=="z" && slideLetter(26)=="aa","slide suffix extends beyond z");
    QFont font("Arial");font.setPixelSize(64);
    QString text=QStringLiteral("Porque de tal manera amó Dios al mundo, que ha dado a su Hijo unigénito, para que todo aquel que en él cree, no se pierda, mas tenga vida eterna. ").repeated(20);
    auto slides=passageSlides(text,font,800,3);
    check(slides.size()>10,"long passage creates multiple slides");
    QString reconstructed;bool lines=true;
    for(const auto &slide:slides){lines &= slide.count('\n')<3;auto copy=slide;copy.remove('\n');reconstructed+=copy;}
    check(lines,"maximum three lines per slide");check(reconstructed==text,"pagination preserves every character");
    check(passageSlides("abc",font,0,3).isEmpty(),"invalid width rejected");
    auto unicode=passageSlides(QString::fromUtf8("שלום 世界 😀 ").repeated(25),font,200,2);
    QString joined=unicode.join(QString());joined.remove('\n');
    check(joined==QString::fromUtf8("שלום 世界 😀 ").repeated(25),"RTL CJK and emoji preserved");
    QWidget frame;QLabel label(&frame);label.setObjectName("verse");label.setFont(font);
    app.setStyleSheet("QLabel {font-size:13px;}");
    frame.setStyleSheet(verseFontStyle(font,64));frame.ensurePolished();label.ensurePolished();
    check(label.font().pixelSize()==64,"verse size overrides the host OBS stylesheet");
    frame.setStyleSheet("QLabel#verse {font-size:48px;color:#ffd67a;}");frame.ensurePolished();label.ensurePolished();
    check(label.font().pixelSize()==48,"native CSS font size applies to verse");
    check(label.palette().color(QPalette::WindowText)==QColor("#ffd67a"),"native CSS verse color applies");
    QTemporaryDir directory;QString path=directory.filePath("bible.sqlite");
    sqlite3 *db=nullptr;check(sqlite3_open(path.toUtf8().constData(),&db)==SQLITE_OK,"SQLite fixture opens");
    sqlite3_exec(db,"CREATE TABLE book(id INTEGER PRIMARY KEY,name TEXT); CREATE TABLE verse(id INTEGER PRIMARY KEY,book_id INTEGER,chapter INTEGER,verse INTEGER,text TEXT); INSERT INTO book VALUES(1,'Juan'); INSERT INTO verse VALUES(1,1,3,16,'Texto de prueba');",nullptr,nullptr,nullptr);
    sqlite3_close(db);
    QFile before(path);before.open(QIODevice::ReadOnly);auto original=before.readAll();before.close();
    QJsonObject converted;QString error;check(convertOpenLp(path,"test","Prueba","Fixture",converted,error),"OpenLP schema converts");
    Bible bible;Passage passage;check(bible.load(QJsonDocument(converted).toJson(),error)&&bible.lookup("Juan 3:16",passage,error)&&passage.text=="Texto de prueba","generated Bible can be queried");
    QFile after(path);after.open(QIODevice::ReadOnly);check(after.readAll()==original,"conversion leaves SQLite unchanged");after.close();
    check(!convertOpenLp(path,"test","Prueba","",converted,error),"license required");
    sqlite3_open(path.toUtf8().constData(),&db);sqlite3_exec(db,"INSERT INTO verse VALUES(2,1,3,16,'Duplicado')",nullptr,nullptr,nullptr);sqlite3_close(db);
    check(!convertOpenLp(path,"test","Prueba","Fixture",converted,error),"duplicate verses rejected");
    check(!convertOpenLp(directory.filePath("missing.sqlite"),"test","Prueba","Fixture",converted,error),"missing input rejected without creating database");
    return failures?1:0;
}
