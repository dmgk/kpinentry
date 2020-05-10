#ifndef KPINENTRY_PINENTRY_HPP
#define KPINENTRY_PINENTRY_HPP

#include <exception>
#include <memory>
#include <sstream>
#include <string>
#include <assuan.hpp>
#include <QtCore/QCommandLineParser>
#include <QtGui/QWidgetSet>
#include <QtWidgets/QApplication>

class Pinentry
{
public:
    Pinentry();
    ~Pinentry() = default;

    Pinentry(const Pinentry&) = delete;
    Pinentry& operator=(const Pinentry&) = delete;

    bool setupCommandLine(QCommandLineParser* parser);
    void processCommandLine(const QCommandLineParser* parser);

    void run();

private:
    void registerCommandHandlers();
    gpg_error_t setdescCommand(char* line);
    gpg_error_t setpromptCommand(char* line);
    gpg_error_t setkeyinfoCommand(char* line);
    gpg_error_t getpinCommand(char* line);
    gpg_error_t seterrorCommand(char* line);
    gpg_error_t clearpassphraseCommand(char* line);
    gpg_error_t getinfoCommand(char* line);

    void registerOptionHandler();
    gpg_error_t optionHandler(const char* key, const char* value);

    QString showPasswordDialog();

    QString readCachedPassword();
    void storeCachedPassword(const QString& password);
    void removeCachedPassword();

    Assuan m_assuan;
    QString m_ttyname;
    QString m_ttytype;
    QString m_lcCtype;
    QString m_lcMessages;
    QString m_desc;
    QString m_prompt;
    QString m_keyinfo;
    QString m_error;
    bool m_noGlobalGrab = false;
    bool m_allowExternalPasswordCache = false;
    bool m_cachePassword = false;
};

#endif // KPINENTRY_PINENTRY_HPP
