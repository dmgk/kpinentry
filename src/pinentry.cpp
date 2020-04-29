#include "pinentry.hpp"
#include "logging.hpp"
#include <QDesktopWidget>
#include <QUrl>
#include <KI18n/KLocalizedString>
#include <KWallet/KWallet>
#include <KPasswordDialog>

Pinentry::Pinentry()
    : m_assuan{this}
{
    registerCommandHandlers();
    registerOptionHandler();
}

bool Pinentry::setupCommandLine(QCommandLineParser* parser)
{
    return parser->addOptions({
        {{"d", "debug"},          i18n("Turn on debugging output.")},
        {{"T", "ttyname"},        i18n("Set the tty terminal node name."), "FILE"},
        {{"N", "ttytype"},        i18n("Set the tty terminal type."),      "NAME"},
        {{"C", "lc-ctype"},       i18n("Set the tty LC_CTYPE value."),     "STRING"},
        {{"M", "lc-messages"},    i18n("Set the tty LC_MESSAGES value."),  "STRING"},
        {{"g", "no-global-grab"}, i18n("Grab keyboard only while window is focused.")},
    });
}

void Pinentry::processCommandLine(const QCommandLineParser* parser)
{
    QLoggingCategory::setFilterRules(QString{LOG_KPINENTRY().categoryName()}
        + "=" + (parser->isSet("debug") ? "true" : "false"));

    m_ttyname = parser->value("ttyname");
    m_ttytype = parser->value("ttytype");
    m_lcCtype = parser->value("lc-ctype");
    m_lcMessages = parser->value("lc-messages");
    m_noGlobalGrab = parser->isSet("no-global-grab");
}

void Pinentry::run()
{
    m_assuan.initServer();
    for (;;) {
        if (!m_assuan.accept())
            break;
        m_assuan.process();
    }
}

using AssuanCommandFn = assuan_handler_t;
using HandlerFn = gpg_error_t (Pinentry::*)(char*);

template<HandlerFn FN>
static constexpr AssuanCommandFn wrap()
{
    return [](assuan_context_t ctx, char* line) -> gpg_error_t {
        return ((*reinterpret_cast<Pinentry*>(assuan_get_pointer(ctx))).*(FN))(line);
    };
}

using AssuanOptionFn = gpg_error_t (*)(assuan_context_t, const char*, const char*);
using OptionFn = gpg_error_t (Pinentry::*)(const char*, const char*);

template<OptionFn FN>
static constexpr AssuanOptionFn wrap()
{
    return [](assuan_context_t ctx, const char* key, const char* value) -> gpg_error_t {
        return ((*reinterpret_cast<Pinentry*>(assuan_get_pointer(ctx))).*(FN))(key, value);
    };
}

void Pinentry::registerCommandHandlers()
{
    static struct
    {
        const char* name;
        AssuanCommandFn handler;

    } commands[] = {
        {"SETDESC",         wrap<&Pinentry::setdescCommand>()},
        {"SETPROMPT",       wrap<&Pinentry::setpromptCommand>()},
        {"SETKEYINFO",      wrap<&Pinentry::setkeyinfoCommand>()},
        {"SETERROR",        wrap<&Pinentry::seterrorCommand>()},
        {"GETPIN",          wrap<&Pinentry::getpinCommand>()},
        {"CLEARPASSPHRASE", wrap<&Pinentry::clearpassphraseCommand>()},
    };

    for (const auto& command : commands)
        m_assuan.registerCommandHandler(command.name, command.handler);
}

static QString percentUnescape(const char* str)
{
    return QString{QUrl::fromPercentEncoding(QString{str}.toUtf8())};
}

gpg_error_t Pinentry::setdescCommand(char* line)
{
    qCDebug(LOG_KPINENTRY, "setdesc: %s", line);

    m_desc = percentUnescape(line);
    return GPG_ERR_NO_ERROR;
}

gpg_error_t Pinentry::setpromptCommand(char* line)
{
    qCDebug(LOG_KPINENTRY, "setprompt: %s", line);

    m_prompt = percentUnescape(line);
    return GPG_ERR_NO_ERROR;
}

gpg_error_t Pinentry::setkeyinfoCommand(char* line)
{
    qCDebug(LOG_KPINENTRY, "setkeyinfo: %s", line);

    m_keyinfo = percentUnescape(line);
    return GPG_ERR_NO_ERROR;
}

gpg_error_t Pinentry::getpinCommand(char* line)
{
    qCDebug(LOG_KPINENTRY, "getpin: %s", line);

    QString password;

    // if we allowed to use the external cache, try reading password from it first
    if (m_allowExternalPasswordCache)
        password = readCachedPassword();

    // no password was read, either because we're not allowed to use the cache
    // or because it wasn't found ine the cache - show the password dialog
    if (password.isEmpty())
        password = showPasswordDialog();

    if (!password.isEmpty()) {
        const QByteArray escapedPasswordData = QUrl::toPercentEncoding(password.toUtf8());
        m_assuan.sendData(escapedPasswordData.constData(), static_cast<size_t>(escapedPasswordData.length()));

        // if we allowed to use the external cache, cache the password
        if (m_allowExternalPasswordCache && m_cachePassword)
            storeCachedPassword(password);
    }

    return GPG_ERR_NO_ERROR;
}

gpg_error_t Pinentry::seterrorCommand(char* line)
{
    qCDebug(LOG_KPINENTRY, "seterror: %s", line);

    m_error = percentUnescape(line);
    if (m_error.startsWith("Bad Passphrase"))   // XXX
        removeCachedPassword();

    return GPG_ERR_NO_ERROR;
}

gpg_error_t Pinentry::clearpassphraseCommand(char* line)
{
    qCDebug(LOG_KPINENTRY, "clearpassphrase: %s", line);

    removeCachedPassword();
    return GPG_ERR_NO_ERROR;
}

void Pinentry::registerOptionHandler()
{
    m_assuan.registerOptionHandler(wrap<&Pinentry::optionHandler>());
}

gpg_error_t Pinentry::optionHandler(const char* key, const char* value)
{
    qCDebug(LOG_KPINENTRY, "option: %s %s", key, value);

    if (strcasecmp(key, "ttyname") == 0)
        m_ttyname = value;
    if (strcasecmp(key, "ttytype") == 0)
        m_ttytype = value;
    if (strcasecmp(key, "lc-ctype") == 0)
        m_lcCtype = value;
    if (strcasecmp(key, "lc-messages") == 0)
        m_lcMessages = value;
    if (strcasecmp(key, "no-grab") == 0)
        m_noGlobalGrab = true;
    if (strcasecmp(key, "allow-external-password-cache") == 0)
        m_allowExternalPasswordCache = true;

    return GPG_ERR_NO_ERROR;
}

QString Pinentry::showPasswordDialog()
{
    KPasswordDialog::KPasswordDialogFlags flags{KPasswordDialog::NoFlags};
    if (m_allowExternalPasswordCache)
        flags |= KPasswordDialog::ShowKeepPassword;

    KPasswordDialog passwordDialog{QApplication::desktop(), flags};
    passwordDialog.setWindowTitle(i18n("Kpinentry"));
    passwordDialog.setPrompt(m_desc);
    passwordDialog.setKeepPassword(m_cachePassword);
    if (!m_error.isEmpty())
        passwordDialog.showErrorMessage(m_error);

    QString password;
    if (passwordDialog.exec() == QDialog::Accepted) {
        password = passwordDialog.password();
        m_cachePassword = passwordDialog.keepPassword();
    }
    return password;
}

QString Pinentry::readCachedPassword()
{
    const auto folderName = QApplication::applicationName();
    const auto key = QString{m_keyinfo};

    KWallet::Wallet* wallet = KWallet::Wallet::openWallet(
        KWallet::Wallet::LocalWallet(),
        QApplication::desktop()->winId());

    if (!wallet) {
        qCCritical(LOG_KPINENTRY, "kwalletCacheLoad: failed to open wallet");
        return "";
    }

    if (wallet->hasFolder(folderName)) {
        if (!wallet->setFolder(folderName)) {
            qCCritical(LOG_KPINENTRY, "kwalletCacheLoad: failed to set folder name");
            return "";
        }
        if (wallet->hasEntry(key)) {
            QString password;
            if (wallet->readPassword(key, password)) {
                qCCritical(LOG_KPINENTRY, "kwalletCacheLoad: failed to read password");
                return "";
            }
            return password;
        }
    }

    return "";
}

void Pinentry::storeCachedPassword(const QString& password)
{
    const auto folderName = QApplication::applicationName();
    const auto key = QString{m_keyinfo};

    KWallet::Wallet* wallet = KWallet::Wallet::openWallet(
        KWallet::Wallet::LocalWallet(),
        QApplication::desktop()->winId());

    if (!wallet) {
        qCCritical(LOG_KPINENTRY, "kwalletCacheStore: failed to open wallet");
        return;
    }

    if (!wallet->hasFolder(folderName)) {
        if (!wallet->createFolder(folderName)) {
            qCCritical(LOG_KPINENTRY, "kwalletCacheStore: failed to set folder name");
            return;
        }
    }

    if (!wallet->setFolder(folderName)) {
        qCCritical(LOG_KPINENTRY, "kwalletCacheStore: failed to set folder name");
        return;
    }
    if (wallet->writePassword(key, password))
        qCCritical(LOG_KPINENTRY, "kwalletCacheStore: failed to write");
}

void Pinentry::removeCachedPassword()
{
    const auto folderName = QApplication::applicationName();

    KWallet::Wallet* wallet = KWallet::Wallet::openWallet(
        KWallet::Wallet::LocalWallet(),
        QApplication::desktop()->winId());

    if (!wallet) {
        qCCritical(LOG_KPINENTRY, "kwalletCacheStore: failed to open wallet");
        return;
    }

    if (wallet->hasFolder(folderName)) {
        if (!wallet->removeFolder(folderName))
            qCCritical(LOG_KPINENTRY, "kwalletCacheStore: failed to remove folder");
    }
}
