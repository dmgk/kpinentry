#ifndef KPINENTRY_ASSUAN_HPP
#define KPINENTRY_ASSUAN_HPP

#include <exception>
#include <string>
#include <sstream>
#include <assuan.h>

class AssuanError : public std::exception
{
public:
    explicit AssuanError(std::string msg)
        : m_msg{std::move(msg)}
    {}

    AssuanError(std::string msg, gpg_error_t err)
        : m_gpgError{err}
    {
        std::ostringstream oss{msg};
        oss << ": " << gpg_strerror(err);
        msg = oss.str();
    }

    [[nodiscard]] const char* what() const noexcept override
    {
        return m_msg.c_str();
    }

    [[nodiscard]] gpg_error_t gpgError() const noexcept
    {
        return m_gpgError;
    }

private:
    std::string m_msg;
    gpg_error_t m_gpgError = GPG_ERR_NO_ERROR;
};

class Assuan
{
public:
    explicit Assuan(void* userData)
    {
        gpg_error_t err = assuan_new(&m_ctx);
        if (err != GPG_ERR_NO_ERROR)
            throw AssuanError("failed to initialize ctx", err);
        assuan_set_pointer(m_ctx, userData);
    }

    ~Assuan()
    {
        assuan_release(m_ctx);
    }

    Assuan(const Assuan&) = delete;
    Assuan& operator=(const Assuan&) = delete;

    Assuan(Assuan&& assuan) noexcept
    {
        *this = std::move(assuan);
    }

    Assuan& operator=(Assuan&& assuan) noexcept
    {
        if (this != &assuan) {
            assuan_release(m_ctx);
            m_ctx = nullptr;
            std::swap(m_ctx, assuan.m_ctx);
        }
        return *this;
    }

    void initServer()
    {
        assuan_fd_t fds[] = {
            assuan_fdopen(STDIN_FILENO),
            assuan_fdopen(STDOUT_FILENO)
        };
        gpg_error_t err = assuan_init_pipe_server(m_ctx, fds);
        if (err != GPG_ERR_NO_ERROR)
            throw AssuanError("failed to initialize server", err);
    }

    bool accept()
    {
        gpg_error_t err = assuan_accept(m_ctx);
        if (err == static_cast<gpg_error_t>(EOF))
            return false;
        if (err != GPG_ERR_NO_ERROR)
            throw AssuanError("failed to accept", err);
        return true;
    }

    void process()
    {
        gpg_error_t err = assuan_process(m_ctx);
        if (err != GPG_ERR_NO_ERROR)
            throw AssuanError("failed to process", err);
    }

    void registerCommandHandler(const char* name, assuan_handler_t handler)
    {
        gpg_error_t err = assuan_register_command(m_ctx, name, handler, nullptr);
        if (err != GPG_ERR_NO_ERROR)
            throw AssuanError("failed to register command handler", err);
    }

    void registerOptionHandler(gpg_error_t (* handler)(assuan_context_t, const char*, const char*))
    {
        gpg_error_t err = assuan_register_option_handler(m_ctx, handler);
        if (err != GPG_ERR_NO_ERROR)
            throw AssuanError("failed to register option handler", err);
    }

    void registerResetHandler(assuan_handler_t handler)
    {
        gpg_error_t err = assuan_register_reset_notify(m_ctx, handler);
        if (err != GPG_ERR_NO_ERROR)
            throw AssuanError("failed to register reset handler", err);
    }

    void sendData(const void* buffer, size_t length)
    {
        gpg_error_t err = assuan_send_data(m_ctx, buffer, length);
        if (err != GPG_ERR_NO_ERROR)
            throw AssuanError("failed to send data", err);
    }

    void writeStatus(const char* key, const char* text = nullptr)
    {
        gpg_error_t err = assuan_write_status(m_ctx, key, text);
        if (err != GPG_ERR_NO_ERROR)
            throw AssuanError("failed to write status", err);
    }

private:
    assuan_context_t m_ctx = nullptr;
};

#endif //KPINENTRY_ASSUAN_HPP
