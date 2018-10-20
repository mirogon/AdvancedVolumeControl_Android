#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    mainLayout = new QGridLayout();

    mainWidget = new QWidget();
    mainWidget->setLayout(mainLayout);

    scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);

    scrollAreaLayout = new QGridLayout();

    scrollAreaContainer = new QWidget();
    scrollAreaContainer->setLayout(scrollAreaLayout);

    this->setCentralWidget(mainWidget);

    lineEdit_serverIP = new QLineEdit();

    pushButton_connect = new QPushButton("Connect to Windows host");
    connect(pushButton_connect, SIGNAL(pressed()), this, SLOT(ConnectToHost()));

    slider = *new std::vector<QSlider*>();
    spinBoxes = *new std::vector<QSpinBox*>();
    labels = *new std::vector<QLabel*>();

    mapper_slider = new QSignalMapper(this);
    connect(mapper_slider, SIGNAL( mapped( int ) ), this, SLOT(SliderChanged(int)));

    mapper_spinBoxes = new QSignalMapper(this);
    connect( mapper_spinBoxes, SIGNAL(mapped(int)), this, SLOT( SpinboxChanged(int) ) );

    timer_receive = new QTimer(this);
    connect( timer_receive, SIGNAL(timeout()), this, SLOT(ReceivePackage()) );

    socket = new QTcpSocket(this);
    connect( socket, SIGNAL(disconnected()), this, SLOT(DisconnectedFromHost()) );

    ReadSavedIP();

    mainLayout->addWidget(lineEdit_serverIP, 0, 0);
    mainLayout->addWidget(pushButton_connect, 1, 0);

}

void MainWindow::ConnectToHost()
{
    __android_log_write(ANDROID_LOG_FATAL, "W10VC", "Connecting to host");

    QHostAddress address;
    address.setAddress(lineEdit_serverIP->text());

    connect(socket, SIGNAL(connected()), this, SLOT(ConnectedToHost()));

    socket->connectToHost(address, 55555);
}

void MainWindow::ConnectedToHost()
{
    __android_log_write(ANDROID_LOG_FATAL, "W10VC", "Connected to host");

    WriteSavedIP();

    mainLayout->removeWidget(lineEdit_serverIP);
    mainLayout->removeWidget(pushButton_connect);

    lineEdit_serverIP->setVisible(false);
    pushButton_connect->setVisible(false);

    mainLayout->addWidget(scrollArea);

    timer_receive->start(10);
}

void MainWindow::DisconnectedFromHost()
{

    audioSessions.clear();
    audioSessions.shrink_to_fit();

    for(int i = 0; i < labels.size(); i++)
    {
        scrollAreaLayout->removeWidget(labels.at(i));
        SAFE_DELETE(labels.at(i));
    }
    labels.clear();
    labels.shrink_to_fit();

    for(int i = 0; i < slider.size(); i++)
    {
        scrollAreaLayout->removeWidget(slider.at(i));
        SAFE_DELETE(slider.at(i));
    }
    slider.clear();
    slider.shrink_to_fit();

    for(int i = 0; i < spinBoxes.size(); i++)
    {
        scrollAreaLayout->removeWidget(spinBoxes.at(i));
        SAFE_DELETE(spinBoxes.at(i));
    }
    spinBoxes.clear();
    spinBoxes.shrink_to_fit();

    mainLayout->removeWidget(scrollArea);
    mainLayout->addWidget(lineEdit_serverIP, 0, 0);
    mainLayout->addWidget(pushButton_connect, 1, 0);

    lineEdit_serverIP->setVisible(true);
    pushButton_connect->setVisible(true);

}

void MainWindow::ReceivePackage()
{

    static S_AudioSession_Network incomingSession;
    static char buffer[sizeof(S_AudioSession_Network)];
    static uint8_t packetType;

    if(socket != nullptr && socket->state() == QAbstractSocket::ConnectedState && socket->bytesAvailable() >= 1 )
    {
        socket->read(buffer, 1);

        memcpy(&packetType, buffer, 1);

        //Check packet type
        if(packetType == pT_audioSession)
        {
            socket->read(buffer, sizeof(S_AudioSession_Network));
        }

        else if(packetType == pT_resetAudioSessions)
        {
            __android_log_write(ANDROID_LOG_FATAL, "W10VC", "Reset audio sessions!");
            ResetAudioSessions();
            return;
        }

        else
        {

            __android_log_write(ANDROID_LOG_FATAL, "W10VC", "Unknown packet type!");
            return;
        }

        memcpy(&incomingSession, buffer, sizeof(S_AudioSession_Network));

        while(audioSessions.size() <= incomingSession.index)
        {
            audioSessions.push_back(S_AudioSession_Network());
        }

        audioSessions.at(incomingSession.index).index = incomingSession.index;
        audioSessions.at(incomingSession.index).currentVolumeLevel = incomingSession.currentVolumeLevel;
        strcpy(audioSessions.at(incomingSession.index).displayName, incomingSession.displayName);
        audioSessions.at(incomingSession.index).isMuted = incomingSession.isMuted;

        while(labels.size() <= incomingSession.index)
        {
            labels.push_back(new QLabel());
            labels.at(labels.size()-1)->setFont(QFont("Arial", 20, QFont::Bold));
        }
        while(slider.size() <= incomingSession.index)
        {
            slider.push_back(new QSlider());
            slider.at(slider.size()-1)->setMinimum(0);
            slider.at(slider.size()-1)->setMaximum(100);
            slider.at(slider.size()-1)->setOrientation(Qt::Horizontal);
            QSizePolicy p = slider.at(slider.size() -1)->sizePolicy();
            p.setHorizontalStretch(5);
            slider.at(slider.size()-1)->setSizePolicy(p);

            connect( slider.at(slider.size() - 1), SIGNAL(sliderReleased()), mapper_slider, SLOT( map() ) );
            mapper_slider->setMapping(slider.at(slider.size() -1), slider.size()-1);

        }
        while(spinBoxes.size() <= incomingSession.index)
        {
            spinBoxes.push_back(new QSpinBox());
            spinBoxes.back()->setFixedHeight(this->height() * 0.1);
            spinBoxes.at(spinBoxes.size()-1)->setMinimum(0);
            spinBoxes.at(spinBoxes.size()-1)->setMaximum(100);
            spinBoxes.at(spinBoxes.size() -1)->setFont(QFont("Arial", 10, QFont::Bold));
            QSizePolicy p = spinBoxes.at(spinBoxes.size() -1)->sizePolicy();
            p.setHorizontalStretch(2);
            spinBoxes.at(spinBoxes.size() -1)->setSizePolicy(p);

            connect(spinBoxes.at(spinBoxes.size() - 1), SIGNAL(editingFinished()), mapper_spinBoxes, SLOT(map()) );
            mapper_spinBoxes->setMapping(spinBoxes.at(spinBoxes.size() -1), spinBoxes.size()-1);
        }
        SyncUI();
    }

    timer_receive->start(10);

}

void MainWindow::SliderChanged(int index)
{

    audioSessions.at(index).currentVolumeLevel = slider.at(index)->value();

    if(slider.at(index)->value() != spinBoxes.at(index)->value())
    {
        spinBoxes.at(index)->setValue(slider.at(index)->value());
    } 

    SendSession(index);

}

void MainWindow::SpinboxChanged(int index)
{

    audioSessions.at(index).currentVolumeLevel = spinBoxes.at(index)->value();

    if(slider.at(index)->value() != spinBoxes.at(index)->value())
    {
        slider.at(index)->setValue(spinBoxes.at(index)->value());
    }

    SendSession(index);


}

void MainWindow::SendSession(int index)
{
    static S_AudioSession_Network session;
    static char buffer[sizeof(S_AudioSession_Network)];

    session.index = audioSessions.at(index).index;
    strcpy(session.displayName, audioSessions.at(index).displayName);
    session.currentVolumeLevel = audioSessions.at(index).currentVolumeLevel;
    session.isMuted = audioSessions.at(index).isMuted;

    memcpy(buffer, &session, sizeof(S_AudioSession_Network));

    if(socket != nullptr && socket->state() == QAbstractSocket::ConnectedState)
    {
        __android_log_write(ANDROID_LOG_FATAL, "W10VC", "Sending package!");
        socket->write(buffer, sizeof(S_AudioSession_Network));
    }

}

void MainWindow::SyncUI()
{

    for(int i = 0; i < audioSessions.size();i++)
    {

        labels.at(i)->setText( audioSessions.at(i).displayName );
        slider.at(i)->setValue(audioSessions.at(i).currentVolumeLevel);
        spinBoxes.at(i)->setValue(slider.at(i)->value());

        scrollAreaLayout->removeWidget(labels.at(i));
        scrollAreaLayout->removeWidget(slider.at(i));
        scrollAreaLayout->removeWidget(spinBoxes.at(i));

        scrollAreaLayout->addWidget(labels.at(i), i, 0, Qt::AlignTop);
        scrollAreaLayout->addWidget(slider.at(i), i, 0, Qt::AlignBottom);
        scrollAreaLayout->addWidget(spinBoxes.at(i), i, 1, Qt::AlignBottom);
    }
    scrollArea->setWidget(scrollAreaContainer);
}



void MainWindow::ResetAudioSessions()
{

    audioSessions.clear();
    audioSessions.shrink_to_fit();

    for(int i= 0; i < labels.size(); i++)
    {
        SAFE_DELETE(labels.at(i));
    }
    labels.clear();
    labels.shrink_to_fit();

    for(int i= 0; i < slider.size(); i++)
    {
        disconnect(slider.at(i), 0, 0, 0);
        mapper_slider->removeMappings(slider.at(i));
        SAFE_DELETE(slider.at(i));
    }
    disconnect(mapper_slider, 0, 0, 0);

    slider.clear();
    slider.shrink_to_fit();

    for(int i= 0; i < spinBoxes.size(); i++)
    {
        disconnect(spinBoxes.at(i), 0, 0, 0);
        mapper_spinBoxes->removeMappings(spinBoxes.at(i));
        SAFE_DELETE(spinBoxes.at(i));
    }
    disconnect(mapper_spinBoxes, 0, 0, 0);

    spinBoxes.clear();
    spinBoxes.shrink_to_fit();

    connect(mapper_slider, SIGNAL( mapped( int ) ), this, SLOT(SliderChanged(int)));
    connect( mapper_spinBoxes, SIGNAL(mapped(int)), this, SLOT( SpinboxChanged(int) ) );

}

void MainWindow::ReadSavedIP()
{
    QFile file("hostIP.txt");
    file.open(QIODevice::ReadOnly);
    QDataStream in(&file);

    QString inStr;

    in >> inStr;

    if(inStr.size() <= 0)
    {
        inStr = "Insert host address here";
    }
    file.close();

    lineEdit_serverIP->setText(inStr);

}

void MainWindow::WriteSavedIP()
{
    QFile file("hostIP.txt");
    file.open(QIODevice::WriteOnly);
    QDataStream out(&file);

    out << lineEdit_serverIP->text();

}

MainWindow::~MainWindow()
{
    SAFE_DELETE(ui);
    SAFE_DELETE(mainLayout);
    SAFE_DELETE(mainWidget);
    SAFE_DELETE(scrollArea);
    SAFE_DELETE(scrollAreaContainer);
    SAFE_DELETE(scrollAreaLayout);

    SAFE_DELETE(lineEdit_serverIP);
    SAFE_DELETE(pushButton_connect);

    for(int i = 0; i < labels.size(); i++)
    {
        SAFE_DELETE(labels.at(i));
    }
    labels.clear();
    for(int i = 0; i < slider.size(); i++)
    {
        SAFE_DELETE(slider.at(i));
    }
    slider.clear();
    for(int i = 0; i < spinBoxes.size(); i++)
    {
        SAFE_DELETE(spinBoxes.at(i));
    }
    spinBoxes.clear();

    audioSessions.clear();

    SAFE_DELETE(timer_receive);
    SAFE_DELETE(socket);

    SAFE_DELETE(mapper_slider);
    SAFE_DELETE(mapper_spinBoxes);

}

void MainWindow::keyReleaseEvent(QKeyEvent *e)
{
    if(e->key() == Qt::Key_Back)
    {

    }

    e->accept();
}
