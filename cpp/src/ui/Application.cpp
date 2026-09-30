#include "ui/Application.h"

#include "Errors.h"
#include "ui/Messages.h"
#include "ui/Style.h"

#include <QFont>
#include <QIcon>
#include <QLibraryInfo>
#include <QLocale>
#include <QStyleFactory>
#include <QStyleHints>
#include <QTranslator>

namespace {

// Русские подписи стандартных кнопок и диалогов Qt: «Да», «Нет», «Отмена»
void installRussianTranslation(QApplication &app)
{
    auto *translator = new QTranslator(&app);
    if (translator->load(QLocale(QLocale::Russian), QStringLiteral("qtbase"), QStringLiteral("_"),
                         QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
        app.installTranslator(translator);
    }
}

} // namespace

bool Application::notify(QObject *receiver, QEvent *event)
{
    try {
        return QApplication::notify(receiver, event);
    } catch (const DatabaseError &error) {
        Messages::showDatabaseError(nullptr, error.message());
    } catch (const std::exception &error) {
        Messages::showError(nullptr, QStringLiteral("Произошла непредвиденная ошибка: %1\n\n"
                                                    "Повторите действие. Если ошибка повторяется, "
                                                    "перезапустите приложение.")
                                         .arg(QString::fromUtf8(error.what())));
    }
    return false;
}

void configureApplication(QApplication &app)
{
    app.setApplicationName(QStringLiteral("Чудо Обувь"));
    // Fusion выглядит одинаково на Windows 10 и 11 и не добавляет системный
    // акцентный цвет, которого нет в руководстве по стилю
    app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    // Оформление заказчика светлое: при тёмной теме Windows Qt иначе
    // берёт палитру с белым текстом, и он сливается с белым фоном
    app.styleHints()->setColorScheme(Qt::ColorScheme::Light);
    app.setWindowIcon(QIcon(QStringLiteral(":/icon.ico")));
    app.setFont(QFont(QString::fromLatin1(Style::fontFamily), 11));
    app.setStyleSheet(Style::styleSheet());
    installRussianTranslation(app);
}
