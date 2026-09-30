#include "AppConfig.h"
#include "Database.h"
#include "Errors.h"

#include <QApplication>
#include <QLabel>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    try {
        Database database(AppConfig::loadDatabaseSettings());
        database.checkConnection();
        QSqlQuery query = database.prepare(QStringLiteral("SELECT count(*) FROM products"));
        database.exec(query);
        query.next();
        QLabel label(QStringLiteral("Товаров в каталоге: %1").arg(query.value(0).toInt()));
        label.show();
        return app.exec();
    } catch (const AppError &error) {
        QMessageBox::critical(nullptr, QStringLiteral("Ошибка"), error.message());
        return 1;
    }
}
