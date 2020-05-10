#include "config.h"
#include "logging.hpp"
#include "pinentry.hpp"
#include <iostream>
#include <KF5/KI18n/KLocalizedString>
#include <KAboutData>
#include <KWidgetsAddons/KPasswordDialog>

int main(int argc, char* argv[])
{
    QApplication app{argc, argv};
    KLocalizedString::setApplicationDomain(PROJECT_NAME);
    QCommandLineParser parser;
    Pinentry pinentry;

    KAboutData about(
        PROJECT_NAME,
        i18n("Kpinentry"),
        PROJECT_VERSION,
        i18n("KDE version of pinentry that integrates with KWallet"),
        KAboutLicense::GPL,
        i18n("(C) 2020 Dmitri Goutnik"),
        "",
        "https://github.com/dmgk/kpinentry",
        "https://github.com/dmgk/kpinentry/issues"
    );
    about.addAuthor(i18n("Dmitri Goutnik"), i18n("Current author"), "dg@syrec.org");
    KAboutData::setApplicationData(about);

    pinentry.setupCommandLine(&parser);
    about.setupCommandLine(&parser);
    parser.process(app);
    pinentry.processCommandLine(&parser);
    about.processCommandLine(&parser);

    try {
        pinentry.run();
    }
    catch (const std::exception& e) {
        LOG_ERRORF("%s", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
