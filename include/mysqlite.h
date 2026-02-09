#ifndef MYSQLITE_H
#define MYSQLITE_H

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVector>
#include <QStringList>
#include <QDebug>

namespace DatabaseCover
{

class MySqLite
{
public:
    MySqLite(QString db)
    {
        if (QSqlDatabase::contains("qt_sql_default_connection"))
        {
            mDatabase = QSqlDatabase::database("qt_sql_default_connection");
        }
        else
        {
            mDatabase = QSqlDatabase::addDatabase("QSQLITE");
            mDatabase.setDatabaseName(db);
            mDatabase.setUserName("drose");
            mDatabase.setPassword("drose");
            open();
        }
    }

    bool open()
    {
        if (!mDatabase.open())
        {
            qDebug() << "Error: Failed to connect database." << mDatabase.lastError();
            return false;
        }
        else
        {
            qDebug() << "Connect database success.";
        }

        return true;
    }

    void close()
    {
        mDatabase.close();
    }

    bool insert(QString table, QVector<QStringList> tQvector)
    {
        QSqlQuery sql_query;
        QString insert_sql = "INSERT INTO " + table;
        insert_sql += " (";
        for (int i = 0; i < tQvector.size(); i++)
        {
            insert_sql += tQvector[i][0];
            if (i != tQvector.size() - 1)
            {
                insert_sql += ", ";
            }
        }
        insert_sql += ") VALUES (";

        for (int i = 0; i < tQvector.size(); i++)
        {
            insert_sql += tQvector[i][1];
            if (i != tQvector.size() - 1)
            {
                insert_sql += ", ";
            }
        }
        insert_sql += ")";

        if (!sql_query.exec(insert_sql))
        {
            qDebug() << sql_query.lastError();
            return false;
        }
        else
        {
            qDebug() << "Inserted ok!";
            return true;
        }
    }

    void select()
    {
        QSqlQuery sql_query;
        QString tSql = "select * from drose;";

        if (!sql_query.exec(tSql))
        {
            qDebug() << sql_query.lastError();
        }
        else
        {
            qDebug() << "Select ok!";
        }
    }

private:
    QSqlDatabase mDatabase;
};

}  // namespace DatabaseCover

#endif // MYSQLITE_H
