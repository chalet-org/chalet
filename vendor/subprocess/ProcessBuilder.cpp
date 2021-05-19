#include "ProcessBuilder.hpp"

#ifdef _WIN32

#else
#include <spawn.h>
#ifdef __APPLE__
#include <sys/wait.h>
#else
#include <wait.h>
#endif
#include <errno.h>
#include <signal.h>
#endif

#include <string.h>
#include <thread>
#include <mutex>
#include <chrono>
#include <cstring>

#include "shell_utils.hpp"
#include "utf8_to_utf16.hpp"


#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable : 4996)
#endif

// TODO: throw exceptions on various os errors.

namespace subprocess {
    namespace details {
        void throw_os_error(const char* function, int errno_code) {
            if (errno_code == 0)
                return;
            std::string message = function;
            message += " failed: " + std::to_string(errno) + ": ";
            message += std::strerror(errno_code);
            throw OSError(message);
        }
    }
    double monotonic_seconds() {
        static bool needs_init = true;
        static std::chrono::steady_clock::time_point begin;
        static double last_value = 0;
        if (needs_init) {
            begin = std::chrono::steady_clock::now();
            needs_init = false;
        }
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        std::chrono::duration<double> duration = now - begin;
        double result = duration.count();

        // some OS's have bugs and not exactly monotonic. Or perhaps there is
        // floating point errors or something. I don't know.
        if (result < last_value)
            return last_value;
        last_value = result;
        return result;
    }

    double sleep_seconds(double seconds) {
        StopWatch watch;
        std::chrono::duration<double> duration(seconds);
        std::this_thread::sleep_for(duration);
        return watch.seconds();
    }

    struct AutoClosePipe {
        AutoClosePipe(PipeHandle handle, bool autoclose) {
            mHandle = autoclose? handle : kBadPipeValue;
        }
        ~AutoClosePipe() {
            close();
        }
        void close() {
            if (mHandle != kBadPipeValue) {
                pipe_close(mHandle);
                mHandle = kBadPipeValue;
            }
        }
    private:
        PipeHandle mHandle;
    };
    static void pipe_thread(PipeHandle input, std::ostream* output) {
        std::thread thread([=]() {
            std::vector<char> buffer(2048);
            while (true) {
                ssize_t transfered = pipe_read(input, &buffer[0], buffer.size());
                if (transfered <= 0)
                    break;
                output->write(&buffer[0], transfered);
            }
        });
        thread.detach();
    }

    static void pipe_thread(PipeHandle input, FILE* output) {
        std::thread thread([=]() {
            std::vector<char> buffer(2048);
            while (true) {
                ssize_t transfered = pipe_read(input, &buffer[0], buffer.size());
                if (transfered <= 0)
                    break;
                fwrite(&buffer[0], 1, transfered, output);
            }
        });
        thread.detach();
    }
    static void pipe_thread(FILE* input, PipeHandle output, bool bautoclose) {
        std::thread thread([=]() {
            AutoClosePipe autoclose(output, bautoclose);
            std::vector<char> buffer(2048);
            while (true) {
                ssize_t transfered = fread(&buffer[0], 1, buffer.size(), input);
                if (transfered <= 0)
                    break;
                pipe_write(output, &buffer[0], transfered);
            }
        });
        thread.detach();
    }
    static void pipe_thread(std::string& input, PipeHandle output, bool bautoclose) {
        std::thread thread([input(move(input)), output, bautoclose]() {
            AutoClosePipe autoclose(output, bautoclose);

            std::size_t pos = 0;
            while (pos < input.size()) {
                ssize_t transfered = pipe_write(output, input.c_str()+pos, input.size() - pos);
                if (transfered <= 0)
                    break;
                pos += transfered;
            }
        });
        thread.detach();
    }
    static void pipe_thread(std::istream* input, PipeHandle output, bool bautoclose) {
        std::thread thread([=]() {
            AutoClosePipe autoclose(output, bautoclose);
            std::vector<char> buffer(2048);
            while (true) {
                input->read(&buffer[0], buffer.size());
                ssize_t transfered = input->gcount();
                if (input->bad())
                    break;
                if (transfered <= 0) {
                    if (input->eof())
                        break;
                    continue;
                }
                pipe_write(output, &buffer[0], transfered);
            }
        });
        thread.detach();
    }
    static void setup_redirect_stream(PipeHandle input, PipeVar& output) {
        PipeVarIndex index = static_cast<PipeVarIndex>(output.index());

        switch (index) {
        case PipeVarIndex::handle:
        case PipeVarIndex::option: return;
        case PipeVarIndex::string: // doesn't make sense
        case PipeVarIndex::istream: // dousn't make sense
            throw std::domain_error("expected something to output to");
        case PipeVarIndex::ostream:
            pipe_thread(input, std::get<std::ostream*>(output));
            break;
        case PipeVarIndex::file:
            pipe_thread(input, std::get<FILE*>(output));
            break;
        }
    }

    static void setup_redirect_stream(PipeVar& input, PipeHandle output) {
        PipeVarIndex index = static_cast<PipeVarIndex>(input.index());

        switch (index) {
        case PipeVarIndex::handle:
        case PipeVarIndex::option: return;
        case PipeVarIndex::string:
            pipe_thread(std::get<std::string>(input), output, true);
            break;
        case PipeVarIndex::istream:
            pipe_thread(std::get<std::istream*>(input), output, true);
            break;
        case PipeVarIndex::ostream:
            throw std::domain_error("reading from std::ostream doesn't make sense");
        case PipeVarIndex::file:
            pipe_thread(std::get<FILE*>(input), output, true);
            break;
        }
    }
    Popen::Popen(CommandLine& inCommand, RunOptions& optionsIn) {
        init(inCommand, optionsIn);
    }

    void Popen::init(CommandLine& inCommand, RunOptions& options) {
        ProcessBuilder builder;

        builder.cin_option  = get_pipe_option(options.cin);
        builder.cout_option = get_pipe_option(options.cout);
        builder.cerr_option = get_pipe_option(options.cerr);

        if (builder.cin_option == PipeOption::specific) {
            builder.cin_pipe = std::get<PipeHandle>(options.cin);
            if (builder.cin_pipe == kBadPipeValue)
                throw std::invalid_argument("bad pipe value for cin");
        }
        if (builder.cout_option == PipeOption::specific) {
            builder.cout_pipe = std::get<PipeHandle>(options.cout);
            if (builder.cout_pipe == kBadPipeValue)
                throw std::invalid_argument("Popen constructor: bad pipe value for cout");
        }
        if (builder.cerr_option == PipeOption::specific) {
            builder.cerr_pipe = std::get<PipeHandle>(options.cerr);
            if (builder.cout_pipe == kBadPipeValue)
                throw std::invalid_argument("Popen constructor: bad pipe value for cout");
        }

        builder.new_process_group = options.new_process_group;
        builder.env = options.env;
        builder.cwd = options.cwd;

        *this = builder.run_command(inCommand);

        setup_redirect_stream(options.cin, cin);
        setup_redirect_stream(cout, options.cout);
        setup_redirect_stream(cerr, options.cerr);
    }

    Popen::Popen(Popen&& other) noexcept {
        *this = std::move(other);
    }

    Popen& Popen::operator=(Popen&& other) noexcept {
        close();
        cin = other.cin;
        cout = other.cout;
        cerr = other.cerr;

        pid = other.pid;
        returncode = other.returncode;
        args = std::move(other.args);

#ifdef _WIN32
        process_info = other.process_info;
	    ZeroMemory(&other.process_info, sizeof(other.process_info));
#endif

        other.cin = kBadPipeValue;
        other.cout = kBadPipeValue;
        other.cerr = kBadPipeValue;
        other.pid = 0;
        other.returncode = kBadReturnCode;
        return *this;
    }

    Popen::~Popen() {
        close();
    }

    void Popen::close() {
        if (cin != kBadPipeValue)
            pipe_close(cin);
        if (cout != kBadPipeValue)
            pipe_close(cout);
        if (cerr != kBadPipeValue)
            pipe_close(cerr);
        cin = cout = cerr = kBadPipeValue;

        // do this to not have zombie processes.
        if (pid) {
            wait();

#ifdef _WIN32
            CloseHandle(process_info.hProcess);
            CloseHandle(process_info.hThread);
#endif
        }

        pid = 0;
        args.clear();
    }

#ifdef _WIN32
    static std::string lastErrorString() {
        LPTSTR lpMsgBuf = nullptr;
        DWORD dw        = GetLastError();

        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            dw,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR) &lpMsgBuf,
            0, NULL );
        std::string message = lptstr_to_string((LPTSTR)lpMsgBuf);
        LocalFree(lpMsgBuf);
        return message;
    }

    bool Popen::poll() {
        return this->wait(0.0);
    }

    int Popen::wait(double timeout) {
        if (returncode != kBadReturnCode)
            return returncode;

        DWORD ms = timeout < 0.0 ? INFINITE : static_cast<DWORD>(timeout * 1000.0);
        DWORD result = WaitForSingleObject(process_info.hProcess, ms);
        if (result == WAIT_TIMEOUT) {
            if (timeout == 0.0)
                return false;

            throw TimeoutExpired("timeout of " + std::to_string(ms) + " expired");
        } else if (result == WAIT_ABANDONED) {
            DWORD error = GetLastError();
            throw OSError("WAIT_ABANDONED error:" + std::to_string(error));
        } else if (result == WAIT_FAILED) {
            DWORD error = GetLastError();
            throw OSError("WAIT_FAILED error:" + std::to_string(error) + ":" + lastErrorString());
        }

        if (result != WAIT_OBJECT_0) {
            throw OSError("WaitForSingleObject failed: " + std::to_string(result));
        }

        DWORD exit_code;
        int ret = GetExitCodeProcess(process_info.hProcess, &exit_code);
        if (ret == 0) {
            DWORD error = GetLastError();
            throw OSError("GetExitCodeProcess failed: " + std::to_string(error) + ":" + lastErrorString());
        }

        returncode = exit_code;
        return returncode;
    }

    bool Popen::send_signal(int signum) {
        if (returncode != kBadReturnCode)
            return false;
        bool success = false;
        if (signum == PSIGKILL) {
            // 137 just like when process is killed SIGKILL
            return TerminateProcess(process_info.hProcess, 137);
        } else if (signum == PSIGINT) {
            // pid can be used as process group id. The signals are sent
            // to the entire process group, including parents.
            success = GenerateConsoleCtrlEvent(CTRL_C_EVENT, pid);
        } else {
            success = GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, pid);
        }
        if (!success) {
            std::string str = lastErrorString();
            std::cout << "error: " << str << "\n";
        }
        return success;
    }
#else
    bool Popen::poll() {
        if (returncode != kBadReturnCode)
            return true;
        int exit_code;
        auto child = waitpid(pid, &exit_code, WNOHANG);
        if (child == 0)
            return false;
        if (child > 0) {
            if(WIFEXITED(exit_code)) {
                returncode = WEXITSTATUS(exit_code);
            } else if (WIFSIGNALED(exit_code)) {
                returncode = -WTERMSIG(exit_code);
            } else {
                returncode = 1;
            }
        }
        return child > 0;
    }
    int Popen::wait(double timeout) {
        if (returncode != kBadReturnCode)
            return returncode;
        if (timeout < 0) {
            int exit_code;
            while (true) {
                pid_t child = waitpid(pid, &exit_code,0);
                if (child == -1 && errno == EINTR) {
                    continue;
                }
                if (child == -1) {
                    // TODO: throw oserror(errno)
                }
                break;
            }
            if(WIFEXITED(exit_code)) {
                returncode = WEXITSTATUS(exit_code);
            } else if (WIFSIGNALED(exit_code)) {
                returncode = -WTERMSIG(exit_code);
            } else {
                returncode = 1;
            }
            return returncode;
        }
        StopWatch watch;

        while (watch.seconds() < timeout) {
            if (poll())
                return returncode;
            sleep_seconds(0.00001);
        }
        throw TimeoutExpired("no time");
    }

    bool Popen::send_signal(int signum) {
        if (returncode != kBadReturnCode)
            return false;
        return ::kill(pid, signum) == 0;
    }
#endif
    bool Popen::terminate() {
        return send_signal(PSIGTERM);
    }
    bool Popen::kill() {
        return send_signal(PSIGKILL);
    }










    std::string ProcessBuilder::windows_command() {
        return this->command[0];
    }

    std::string ProcessBuilder::windows_args() {
        return this->windows_args(this->command);
    }
    std::string ProcessBuilder::windows_args(const CommandLine& inCommand) {
        std::string args;
        for(std::size_t i = 0; i < inCommand.size(); ++i) {
            if (i > 0)
                args += ' ';
            args += escape_shell_arg(inCommand[i]);
        }
        return args;
    }

    CompletedProcess run(Popen& popen, bool check) {
        CompletedProcess completed;
        std::thread cout_thread;
        std::thread cerr_thread;
        if (popen.cout != kBadPipeValue) {
            cout_thread = std::thread([&]() {
                try {
                    completed.cout = pipe_read_all(popen.cout);
                } catch (...) {
                }
                pipe_close(popen.cout);
                popen.cout = kBadPipeValue;
            });
        }
        if (popen.cerr != kBadPipeValue) {
            cerr_thread = std::thread([&]() {
                try {
                    completed.cerr = pipe_read_all(popen.cerr);
                } catch (...) {
                }
                pipe_close(popen.cerr);
                popen.cerr = kBadPipeValue;
            });
        }

        if (cout_thread.joinable()) {
            cout_thread.join();
        }
        if (cerr_thread.joinable()) {
            cerr_thread.join();
        }

        popen.wait();
        completed.returncode = popen.returncode;
        completed.args = CommandLine(popen.args.begin()+1, popen.args.end());
        if (check) {
            CalledProcessError error("failed to execute " + popen.args[0]);
            error.cmd           = popen.args;
            error.returncode    = completed.returncode;
            error.cout          = std::move(completed.cout);
            error.cerr          = std::move(completed.cerr);
            throw error;
        }
        return completed;
    }

    CompletedProcess run(CommandLine& inCommand, RunOptions options) {
        Popen popen(inCommand, options);
        CompletedProcess completed;
        std::thread cout_thread;
        std::thread cerr_thread;
        if (popen.cout != kBadPipeValue) {
            cout_thread = std::thread([&]() {
                try {
                    completed.cout = pipe_read_all(popen.cout);
                } catch (...) {
                }
                pipe_close(popen.cout);
                popen.cout = kBadPipeValue;
            });
        }
        if (popen.cerr != kBadPipeValue) {
            cerr_thread = std::thread([&]() {
                try {
                    completed.cerr = pipe_read_all(popen.cerr);
                } catch (...) {
                }
                pipe_close(popen.cerr);
                popen.cerr = kBadPipeValue;
            });
        }

        if (cout_thread.joinable()) {
            cout_thread.join();
        }
        if (cerr_thread.joinable()) {
            cerr_thread.join();
        }

        popen.wait();
        completed.returncode = popen.returncode;
        completed.args = inCommand;
        if (options.check && completed.returncode != 0) {
            CalledProcessError error("failed to execute " + inCommand[0]);
            error.cmd           = inCommand;
            error.returncode    = completed.returncode;
            error.cout          = std::move(completed.cout);
            error.cerr          = std::move(completed.cerr);
            throw error;
        }
        return completed;
    }

}

#ifdef _MSC_VER
	#pragma warning(pop)
#endif
