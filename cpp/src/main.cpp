#include "AppConfig.h"
#include "Database.h"
#include "Errors.h"
#include "Repositories.h"
#include "ui/Application.h"
#include "ui/MainWindow.h"
#include "ui/Messages.h"

#include <memory>

int main(int argc, char *argv[])
{
    Application app(argc, argv);
    configureApplication(app);

    std::unique_ptr<Database> database;
    try {
        database = std::make_unique<Database>(AppConfig::loadDatabaseSettings());
        database->checkConnection();
    } catch (const AppError &error) {
        Messages::showError(nullptr,
                            QStringLiteral("Не удалось подключиться к базе данных «Чудо Обувь».\n\n"
                                           "Проверьте, что сервер PostgreSQL запущен, база "
                                           "развёрнута скриптом database/deploy.bat, а параметры "
                                           "в config.ini указаны верно.\n\nПодробности: %1")
                                .arg(error.message()));
        return 1;
    }

    UserRepository users(*database);
    ProductRepository products(*database);
    OrderRepository orders(*database);
    AppContext context(users, products, orders);
    MainWindow window(context);
    window.show();
    return app.exec();
}
