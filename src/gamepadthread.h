#ifndef GAMEPADTHREAD_H
#define GAMEPADTHREAD_H

#include <QThread>
#include <QObject>

class GamepadThread : public QThread
{
    Q_OBJECT

public:
    explicit GamepadThread(const QString &devicePath = "/dev/input/event1", QObject *parent = nullptr);
    ~GamepadThread();

    void stop();

signals:
    void inputReceived(int type, int code, int value);

protected:
    void run() override;

private:
    QString m_devicePath;
    bool m_running;
    int m_fd;
};

#endif // GAMEPADTHREAD_H

