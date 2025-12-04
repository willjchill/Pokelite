#ifndef GAMEPAD_H
#define GAMEPAD_H

#include <QThread>
#include <QObject>

class Gamepad : public QThread
{
    Q_OBJECT

public:
    explicit Gamepad(const QString &devicePath = "/dev/input/event1", QObject *parent = nullptr);
    ~Gamepad();

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

#endif // GAMEPAD_H

