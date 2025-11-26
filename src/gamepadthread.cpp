#include "gamepadthread.h"
#include <QDebug>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <linux/input.h>

GamepadThread::GamepadThread(const QString &devicePath, QObject *parent)
    : QThread(parent), m_devicePath(devicePath), m_running(false), m_fd(-1)
{
}

GamepadThread::~GamepadThread()
{
    stop();
}

void GamepadThread::stop()
{
    m_running = false;
    if (m_fd >= 0) {
        close(m_fd);
        m_fd = -1;
    }
    wait();
}

void GamepadThread::run()
{
    m_fd = open(m_devicePath.toLocal8Bit().constData(), O_RDONLY);
    if (m_fd < 0) {
        qDebug() << "Failed to open gamepad device:" << m_devicePath;
        return;
    }

    m_running = true;
    struct input_event ev;

    while (m_running) {
        ssize_t bytes = read(m_fd, &ev, sizeof(ev));
        if (bytes < 0) {
            if (errno == EINTR) {
                continue;
            }
            qDebug() << "Error reading from gamepad:" << strerror(errno);
            break;
        }

        if (bytes == sizeof(ev)) {
            // Emit signal with event data
            emit inputReceived(ev.type, ev.code, ev.value);
        }
    }

    if (m_fd >= 0) {
        close(m_fd);
        m_fd = -1;
    }
}

