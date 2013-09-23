#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "stdint.h"
#include "string.h"

class QTimer;
class QextSerialPort;
class QwtPlotCurve;


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    

private slots:
    void on_handlerButton_clicked();

    void on_clearButton_clicked();

    void on_refreshPort_clicked();

    void on_OpenClosePort_clicked();
    void onPortNameChanged(const QString);
    void onBaudRateChanged(int);
    void onParityChanged(int);
    void onDataBitChanged(int);
    void onStopBitChanged(int);

    void on_saveData_clicked();
    void onReadyRead(void);

    void on_pushButton_clicked();

    void on_spinBox_valueChanged(int arg1);

    void on_GoButton_clicked();

    void on_realtimeProcessCB_toggled(bool checked);

    void on_RowSB_valueChanged(int arg1);

    void on_colSB_valueChanged(int arg1);

    void on_prev_clicked();

    void on_next_clicked();

    void on_pushButton_2_clicked();

private:
    Ui::MainWindow *ui;

    QextSerialPort *port;
    QTimer *timer;
    QTimer *handleTimer;

    qint32 imgcnt;


    char* intToChar(const int ch);
    void imgGetMidLine(void);

    void imgGetLine(void);
    void portSettingEnabled(bool en);

    int8_t imgLeastsq(int8_t BaseLine, int8_t FinalLine, float *k, int8_t *b);

    float imgArea(int row1,int row2, int row3,int *area);


    QwtPlotCurve *myCurve;

    QVector <double> xData;

    QVector <double> yData;
};

#endif // MAINWINDOW_H
