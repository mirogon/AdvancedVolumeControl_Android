#ifndef MAINWINDOW_H
#define MAINWINDOW_H



#include <vector>
#include <string>
#include <sstream>

#include <android/log.h>

#include <QMainWindow>
#include <QNetworkInterface>
#include <QTcpSocket>
#include <QPushButton>
#include <QLineEdit>
#include <QGridLayout>
#include <QSpinBox>
#include <QSlider>
#include <QLabel>
#include <QTimer>
#include <QSignalMapper>
#include <QKeyEvent>
#include <QScrollArea>

template <typename T>
inline std::string to_string(T value)
{
    std::ostringstream os;
    os << value;
    return os.str();
}

#define SAFE_DELETE(x) if(x != nullptr){delete x; x = nullptr;}

enum E_PacketType{pT_audioSession = 0, pT_resetAudioSessions = 1};

struct S_AudioSession_Network
{
    uint8_t index;
    char displayName[128];
    uint8_t currentVolumeLevel;
    bool isMuted;

};


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:

    Ui::MainWindow *ui;

    QWidget* mainWidget;
    QGridLayout* mainLayout;

    QScrollArea* scrollArea;
    QWidget* scrollAreaContainer;
    QGridLayout* scrollAreaLayout;

    QLineEdit* lineEdit_serverIP;
    QPushButton* pushButton_connect;

    std::vector<QLabel*> labels;
    std::vector<QSlider*> slider;
    std::vector<QSpinBox*> spinBoxes;

    std::vector<S_AudioSession_Network> audioSessions;

    QTimer* timer_receive;
    QTcpSocket* socket;

    QSignalMapper* mapper_slider;
    QSignalMapper* mapper_spinBoxes;

    //FUNCTIONS

    void keyReleaseEvent(QKeyEvent* e);

    void ResetAudioSessions();
    void SyncUI();

    void SendSession(int index);

    void ReadSavedIP();
    void WriteSavedIP();

    private slots:

    void SliderChanged(int index);
    void SpinboxChanged(int index);

    void ConnectToHost();
    void DisconnectedFromHost();

    void ReceivePackage();

    void ConnectedToHost();

};

#endif // MAINWINDOW_H
