#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "QString"
#include "QMessageBox"
#include "QDebug"
#include "QThread"

//绘图相关
#include "QPainter"
#include "QPixmap"

#include "qwt_plot.h"
#include "qwt_plot_canvas.h"
#include "qwt_plot_curve.h"

//串口相关
#include "qextserialport.h"
#include "qextserialenumerator.h"
#include "QTimer"

//文件相关
#include "QFile"
#include "QFileDialog"
#include "QTextStream"

const int IMG_W = 50;
const int IMG_H = 50;

#define ABS(x)   ((x) >= 0 ? (x): (-x))
#define MAX(x,y) ((x) >= (y) ? (x) : (y))
#define MIN(x,y) ((x) >= (y) ? (y) : (x))

int motorcnt = 0;


int8_t img[IMG_H][IMG_W];
int8_t baudImg[IMG_H][IMG_W];
int8_t leftBlack[IMG_H];
int8_t rightBlack[IMG_H];
volatile int8_t middle[IMG_H];
int8_t middle2[IMG_H];

int8_t avr;

qint32 refline;
int8_t lostRow;

//int8_t leftLostRow=0, rightLostRow =0;

static const int8_t jiaozheng[IMG_H] = {6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 12,
                                        12, 13, 13, 14, 14, 15, 15, 16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 21,
                                        22, 23, 23, 24, 24, 25, 25, 26, 26, 27, 28};


void
imgLagrange(int row1,int row2,int row3);



///////////////////////////////////////////////////////////////////////////////
//
//  绘图类
//
///////////////////////////////////////////////////////////////////////////////

class paint: public QWidget
{
public:
    paint(const int8_t *content, const int8_t *leftBlack, const int8_t *rightBlack, const int8_t *middle, float k,int8_t row, int8_t col, int8_t avr);
    ~paint(void);

protected:
    void paintEvent(QPaintEvent *);

private:
    int dat[IMG_H][IMG_W];
    int8_t left[IMG_H];
    int8_t right[IMG_H];
    int8_t mid[IMG_H];
    int8_t hang,lie;
    float kf;
    int8_t average;
};

paint::paint(const int8_t *content,const int8_t *leftBlack, const int8_t *rightBlack, const int8_t *middle,float k, int8_t row, int8_t col, int8_t avr)
{
    for (int row = 0; row < IMG_H; ++row) {
        for (int col = 0; col < IMG_W; ++col) {
            dat[row][col] = *(content + row*IMG_W+col);
        }
        left[row] = *(leftBlack + row);
        right[row] = *(rightBlack + row);
        mid[row] = *(middle + row);
    }
    kf = k;

    hang = row;
    lie = col;
    average = avr;
}

paint::~paint()
{

}

void paint::paintEvent(QPaintEvent *)
{
    QPixmap pixmap(500,500);
    pixmap.fill(Qt::white);

    QPainter painter(&pixmap);

    qint8 rowxi = 500/hang;
    qint8 colxi = 500/lie;


    painter.setPen(Qt::gray);
    for (int i = 0; i < 6 ; ++i) {
        painter.drawLine(0,i*100,500,i*100);
        painter.drawLine(i*100,0, i*100, 500);
    }

    for (int row = 0; row < IMG_H; ++row) {
        for (int col = 0; col < IMG_W; ++col) {
            if (dat[row][col]) {
                painter.fillRect(col*colxi, row*rowxi, 10, 10, Qt::blue);
            }
        }
        painter.setPen(Qt::black);
        painter.fillRect(left[row]*colxi, row*rowxi, 5,5,Qt::yellow);
        painter.drawText(1*colxi,row*rowxi,QString("%1").arg(left[row]));
        painter.fillRect(right[row]*colxi, row*rowxi, 5, 5, Qt::green);
        painter.drawText(IMG_W*colxi-25,row*rowxi,QString("%1").arg(right[row]));
        painter.fillRect(mid[row]*colxi, row*rowxi, 5,5, Qt::red);
        painter.drawText(IMG_W/2*colxi, row*rowxi, QString("%1").arg(mid[row]));
    }

    painter.setPen(Qt::black);
    if(kf != 0)
        painter.drawText(400,400,QString("k=%1").arg(kf));
    painter.setPen(Qt::darkGreen);
    painter.drawLine(250,0,250,500);
    painter.drawLine(0,refline*rowxi,500,refline*rowxi);
    painter.setPen(Qt::darkBlue);
    painter.drawLine(average*10,0,average*10,500);
    painter.drawText(average*10, 300, QString("average = %1").arg(average));
    painter.drawText(average*10, 250, QString("lostRow = %1").arg(lostRow));

    QPainter finalPainter(this);
    finalPainter.drawPixmap(0,0,pixmap);
}


void getBaud(void)
{
    int nr;
    memcpy((void*)baudImg,(void*)img, sizeof(img));

    for (int row = 1; row < IMG_H; ++row) {
        for (int col = 1; col < IMG_W; ++col) {
            if(img[row][col] == 0){
                continue;
            }else{
                nr = img[row-1][col]*img[row+1][col]*img[row][col+1]*img[row][col-1];
                if(nr!=0){
                    baudImg[row][col] = 0;
                }
            }
        }
    }
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QList <QextPortInfo> ports = QextSerialEnumerator::getPorts();

    foreach(QextPortInfo port, ports){
        ui->SerialName->addItem(port.portName.toLatin1(), port.physName.toLatin1());
    }

    ui->SerialName->setEditable(true);

    ui->SerBaudRate->addItem("1200", BAUD1200);
    ui->SerBaudRate->addItem("2400", BAUD2400);
    ui->SerBaudRate->addItem("4800", BAUD4800);
    ui->SerBaudRate->addItem("9600", BAUD9600);
    ui->SerBaudRate->addItem("19200", BAUD19200);
    ui->SerBaudRate->addItem("38400", BAUD38400);
    ui->SerBaudRate->addItem("57600", BAUD57600);
    ui->SerBaudRate->addItem("115200", BAUD115200);

    ui->SerBaudRate->setCurrentIndex(7);

    ui->SerParity->addItem(tr("无"),PAR_NONE);
    ui->SerParity->addItem(tr("奇"),PAR_ODD);
    ui->SerParity->addItem(tr("偶"),PAR_EVEN);

    ui->SerDataBit->addItem("5", DATA_5);
    ui->SerDataBit->addItem("6", DATA_6);
    ui->SerDataBit->addItem("7", DATA_7);
    ui->SerDataBit->addItem("8", DATA_8);
    ui->SerDataBit->setCurrentIndex(3);

    ui->SerStopBit->addItem("1", STOP_1);
#ifdef Q_OS_WIN32
    ui->SerStopBit->addItem("1.5", STOP_1_5);
#endif
    ui->SerStopBit->addItem("2", STOP_2);


    timer = new QTimer(this);
    timer->setInterval(40);
    handleTimer = new QTimer(this);
    handleTimer->setInterval(1000);


    PortSettings setttings = {BAUD115200, DATA_8, PAR_NONE, STOP_1, FLOW_OFF, 10};

    port = new QextSerialPort(ui->SerialName->itemData(ui->SerialName->currentIndex()).toString(), setttings, QextSerialPort::Polling);

    connect(ui->SerBaudRate, SIGNAL(currentIndexChanged(int)), this, SLOT(onBaudRateChanged(int)));
    connect(ui->SerDataBit, SIGNAL(currentIndexChanged(int)), this, SLOT(onDataBitChanged(int)));
    connect(ui->SerParity, SIGNAL(currentIndexChanged(int)), this, SLOT(onParityChanged(int)));
    connect(ui->SerStopBit, SIGNAL(currentIndexChanged(int)), this, SLOT(onStopBitChanged(int)));

    connect(timer, SIGNAL(timeout()), this, SLOT(onReadyRead()));
#ifdef Q_OS_WIN32
    //    connect(port, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
#endif

    connect(ui->QuitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
    connect(ui->OpenPortAction, SIGNAL(triggered()), this, SLOT(on_OpenClosePort_clicked()));
    connect(ui->aboutAction,SIGNAL(triggered()), qApp, SLOT(aboutQt()));

    imgcnt = 0;
    motorcnt = 0;

    portSettingEnabled(true);
    ui->progressBar->hide();
    refline = ui->spinBox->value();
    ui->GoButton->setEnabled(false);
    myCurve = new QwtPlotCurve("PID");

    QPen pen =  QPen(QColor(255,0,0));
    myCurve->setPen(pen);

    ui->qwtPlot->replot();

}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_handlerButton_clicked()
{
    QString dat = ui->textEdit->toPlainText();

    if(dat.contains("img")){
        ui->progressBar->show();
        ui->progressBar->setValue(0);
        ui->GoButton->setEnabled(true);

        if(dat.isEmpty()){
            if(!handleTimer->isActive()){
                QMessageBox::warning(this, tr("警告"),tr("接收到数据为空"),QMessageBox::Yes);
                ui->progressBar->hide();
                return;
            }
        }

        QLabel label(tr("正在处理^^^^^^"));
        ui->statusBar->addWidget(&label);

        ui->tabWidget->clear();
        imgcnt = 0;

        QStringList datlist = dat.split("img\n",QString::SkipEmptyParts);

        ui->progressBar->setMaximum(datlist.length());
        ui->goIndex->setMaximum(datlist.length());

        foreach(QString str, datlist){
#if 0
            memcpy(img, str.toAscii(), 2500);
#else
            str.replace("\n","");

            QStringList list = str.split(",");

            int row = 0, col =0;

            qDebug() << list;


            foreach(QString item, list)
            {
                img[row][col++] = item.toInt();
                if(col == IMG_W){
                    col = 0;
                    row ++;
                }
            }
#endif

            imgGetLine();
            imgGetMidLine();
            imgcnt ++;

            ui->progressBar->setValue(imgcnt);
            int start = MAX(lostRow, 20);
            int sum = 0;
            for(int i=start; i < 40; i++){
                sum += middle[i];
            }

            avr = sum / (40-start);

            ui->tabWidget->addTab(new paint((int8_t *)img,(int8_t *)leftBlack,(int8_t *)rightBlack, (int8_t *)middle,0,ui->RowSB->value(), ui->colSB->value(),avr),QString(tr("原始图像[%1]")).arg(imgcnt));

            float k;
            int8_t b;
            imgLeastsq(MAX(lostRow,20),40,&k,&b);



            for (int i = 0; i < IMG_H; ++i) {
                middle2[i] = (int8_t) (k*i+b);
            }

            ui->tabWidget->addTab(new paint((int8_t *)img,(int8_t *)leftBlack,(int8_t *)rightBlack, (int8_t *)middle2,k,ui->RowSB->value(), ui->colSB->value(), avr),QString(tr("最小二乘法[%1]")).arg(imgcnt));

            //            getBaud();
            //            memcpy((void*)img,(void*)baudImg,sizeof(img));
            //            ui->tabWidget->addTab(new paint((int8_t *)baudImg,(int8_t *)leftBlack,(int8_t *)rightBlack, (int8_t *)middle2,k,ui->RowSB->value(), ui->colSB->value()),QString(tr("边界")));

            ui->tabWidget->setCurrentIndex(2*imgcnt - 2);

            ui->debugOut->insertPlainText(QString("\nimg[%1]\n").arg(imgcnt));
            ui->debugOut->insertPlainText(QString("lostRow=%1\n").arg(lostRow));
            ui->debugOut->insertPlainText(QString("diff=%1\n").arg(middle[refline]-IMG_W/2));
            ui->debugOut->insertPlainText(QString("average is %1\n").arg(avr));
            ui->debugOut->moveCursor(QTextCursor::End);
        }
        ui->progressBar->hide();
    }else {

        motorcnt = 0;
        xData.clear();
        yData.clear();

        QStringList list =  dat.split("\n", QString::SkipEmptyParts);

        qDebug() <<"list is" <<  list;
        foreach(QString Qstr, list){
            motorcnt ++;
            xData.append(motorcnt);
            QStringList motdat = Qstr.split(",",QString::SkipEmptyParts);
            qDebug() << "motdat is" << motdat;
            int32_t tmp = 0;
            foreach(QString st, motdat){
                tmp =(int32_t)(tmp * (1 << 8) + st.toInt());
                qDebug() << st.toInt();
            }

            qDebug() << "tmp is " << tmp;

            yData.append(tmp);
        }

        myCurve->setSamples(xData, yData);
        myCurve->attach(ui->qwtPlot);

        ui->qwtPlot->replot();
    }
}


/**
 *  使用最小二乘法计算跑道方向
 *  输入变量:  BaseLine起始行 < FinalLine终止行
 *  输出变量:  k, 斜率 b 常数项
 *  返回值:  最小二乘法拟合的残差和
 */
int8_t MainWindow::imgLeastsq(int8_t BaseLine, int8_t FinalLine, float *k, int8_t *b)
{
    int32_t sumX=0, sumY=0;
    int32_t averageX=0, averageY=0;
    int8_t i;
    int8_t availableLines = FinalLine - BaseLine;
    float error=0;

    if(availableLines == 0){
        *k = 0;
        *b = 0;
        return 0;
    }

    for (i = BaseLine; i < FinalLine; ++i) {
        sumX += i;
        sumY += middle[i];
    }

    averageX = sumX / availableLines;
    averageY = sumY / availableLines;

    sumX = 0;
    sumY = 0;
    for (i = BaseLine; i < FinalLine; ++i) {
        sumX += (i-averageX)*(middle[i]-averageY);
        sumY += (i-averageX)*(i-averageX);
    }

    if(sumY == 0){
        *k = 0.0;
    }else{
        *k = (float) sumX / sumY;
    }
    *b = (int8_t) averageY -*k*averageX;

    for (i = BaseLine; i < FinalLine; ++i) {
        error += (*k*i+*b - middle[i])*(*k*i+*b-middle[i]);
    }

    return error;
}




void MainWindow::imgGetLine(void)
{
#if 1
    int8_t row, col;

    int8_t leftStart, leftEnd, rightStart, rightEnd;
    int8_t getLeftBlack=0, getRightBlack=0;  //标志是否找到黑线
    int8_t leftLostCnt =0, rightLostCnt=0;

    memset((void *)leftBlack, -1, sizeof(leftBlack));
    memset((void *)rightBlack, IMG_W, sizeof(rightBlack));

    row = IMG_H -2;
    getRightBlack = getLeftBlack = 0;
    do{
        for (col = (IMG_W /2); col >= 0; --col) {  // 先找左边黑线
            if (img[row][col] != 0){
                leftBlack[row] = col;               //记录下黑线的列数
                getLeftBlack ++;
                break;
            }
        }

        for (col = (IMG_W /2); col <= (IMG_W -1); ++col) {  // 再找右边黑线
            if (img[row][col] != 0){     //发现黑线
                rightBlack[row] = col;   //记录下黑线的列数
                getRightBlack ++;
                break;
            }
        }
        row --;
    }while(getLeftBlack != 5 && getRightBlack != 5);

    if(ABS(leftBlack[row+1] - rightBlack[row+1]) <= 3){
        if((leftBlack[row+1] - leftBlack[row+4]) > 0){    //丢右线
            rightBlack[row+1] = rightBlack[row+2] = rightBlack[row+3] = rightBlack[row+4] = IMG_W;
        }
        if((leftBlack[row+1] - leftBlack[row+4]) < 0){    //丢左线
            leftBlack[row+1] = leftBlack[row+2] = leftBlack[row+3] = leftBlack[row+4] = -1;
        }
    }

    leftStart = leftEnd = leftBlack[row+1];
    rightStart = rightEnd = rightBlack[row+1];

    do{  //找左边黑线
        getLeftBlack = 0;
        leftStart += 3;
        leftEnd -= 3;
        if (leftEnd < 0) leftEnd = 0;
        if (leftStart > IMG_W -1) leftStart = IMG_W-1;//避免数组访问越界

        for (col = leftStart; col >= leftEnd ; --col) { //从右向左搜索
            if (img[row][col] != 0 ){//找到黑线
                leftBlack[row] = col;
                leftStart = leftEnd = col;
                getLeftBlack = 1;
                if(leftLostCnt == 1){  //补线
                    leftBlack[row+1] = (leftBlack[row+2] + leftBlack[row]) /2;
                }
                if(leftLostCnt == 2){ //补线
                    leftBlack[row+1] = leftBlack[row]   - (leftBlack[row]-leftBlack[row+3])/3;
                    leftBlack[row+2] = leftBlack[row+3] + (leftBlack[row]-leftBlack[row+3])/3;
                }
                if(leftLostCnt == 3 && ABS(col-rightBlack[row+1])>=3){
                    leftBlack[row+1] = leftBlack[row]   - (leftBlack[row]-leftBlack[row+4])/4;
                    leftBlack[row+2] = leftBlack[row+4] + (leftBlack[row]-leftBlack[row+4])/2;
                    leftBlack[row+3] = leftBlack[row+4] + (leftBlack[row]-leftBlack[row+4])/4;
                }
                if(leftLostCnt == 4 && ABS(col-rightBlack[row+1])>=3){
                    leftBlack[row+1] = leftBlack[row] - (leftBlack[row]-leftBlack[row+5])/5;
                    leftBlack[row+2] = leftBlack[row] - (leftBlack[row]-leftBlack[row+5])*2/5;
                    leftBlack[row+3] = leftBlack[row+5] + (leftBlack[row]-leftBlack[row+5])*3/5;
                    leftBlack[row+4] = leftBlack[row+5] + (leftBlack[row]-leftBlack[row+5])*2/5;
                    leftBlack[row+5] = leftBlack[row+5] + (leftBlack[row]-leftBlack[row+5])/5;
                }
                leftLostCnt = 0;
                break;  //找到黑线退出for循环
            }
        }
        if(getLeftBlack != 1){ //没有找到黑线
            leftLostCnt ++;
            leftBlack[row] = -1;
            if(leftLostCnt > 4){
                leftStart -= 3;
                leftEnd += 3;
            }
        }

        getRightBlack = 0;    //找右边黑线
        rightStart -= 3;
        rightEnd += 3;
        if (rightStart < 0) rightStart = 0;
        if (rightEnd > IMG_W -1) rightEnd = IMG_W-1; //避免数组访问越界

        for (col = rightStart; col <= rightEnd; ++col){  //从左往右搜索
            if (img[row][col]!=0  ){ //找到黑线
                rightBlack[row] = col;
                rightStart = rightEnd = col;
                getRightBlack = 1;
                if(rightLostCnt == 1){ //补线
                    rightBlack[row+1] = (rightBlack[row+2]+rightBlack[row]) /2;
                }
                if(rightLostCnt == 2){  //补线
                    rightBlack[row+1] = rightBlack[row] - (rightBlack[row]-rightBlack[row+3])/3;
                    rightBlack[row+2] = rightBlack[row+3] + (rightBlack[row]-rightBlack[row+3])/3;
                }
                if(rightLostCnt == 3  && ABS(col-leftBlack[row+1])>=3){
                    rightBlack[row+1] = rightBlack[row] - (rightBlack[row]-rightBlack[row+4])/4;
                    rightBlack[row+2] = rightBlack[row+4] + (rightBlack[row]-rightBlack[row+4])/2;
                    rightBlack[row+3] = rightBlack[row+4] + (rightBlack[row]-rightBlack[row+4])/4;
                }
                if(rightLostCnt == 4 && ABS(col-leftBlack[row+1])>=3){
                    rightBlack[row+1] = rightBlack[row] - (rightBlack[row]-rightBlack[row+5])/5;
                    rightBlack[row+2] = rightBlack[row] - (rightBlack[row]-rightBlack[row+5])*2/5;
                    rightBlack[row+3] = rightBlack[row+5] + (rightBlack[row]-rightBlack[row+5])*3/5;
                    rightBlack[row+4] = rightBlack[row+5] + (rightBlack[row]-rightBlack[row+5])*2/5;
                    rightBlack[row+5] = rightBlack[row+5] + (rightBlack[row]-rightBlack[row+5])/5;
                }
                break;      //跳出for循环
            }
        }
        if (getRightBlack != 1){ //没有找到黑线
            rightBlack[row] = IMG_W;
            rightLostCnt ++;
            if(rightLostCnt > 4){
                rightStart += 3;
                rightEnd -= 3;
            }
        }
        row --;
    }while(row > 0);

#else
    int8_t getLeftBlack, getRightBlack;
    int8_t leftStart, leftEnd, rightStart, rightEnd;
    int8_t leftLostCnt, rightLostCnt;

    memset(leftBlack, -1, sizeof(leftBlack));
    memset(rightBlack, IMG_W, sizeof(rightBlack));

    for (int row = (IMG_H-2) ; row > (IMG_H-7); --row) {  //先扫靠近车的前五行
        //先找左线
        for (int col = (IMG_W/2); col >= 0; --col) {       //从右往左扫描
            if(img[row][col] != 0){                       //找到黑线
                leftBlack[row] = col;                     //记录黑线列数
                break;
            }
        }
        //再找右线
        for (int col = (IMG_W/2); col <= (IMG_W-1); ++col) {   //右线从左往右扫描
            if(img[row][col] != 0){                       //找到黑线
                leftBlack[row] = col;
                break;
            }
        }
    }

    leftStart = leftEnd = leftBlack[IMG_H - 6];
    rightStart = rightEnd = rightBlack[IMG_H -6];

    for (int row = (IMG_H-6); row > 2; --row) {
        //先找左线
        getLeftBlack = 0;
        if(img[row][leftBlack[row+1]]!=0){
            leftBlack[row] = leftBlack[row+1];
            getLeftBlack = 1;
            leftLostCnt = 0;
        }else{
            leftStart += 3;
            if(leftStart >= IMG_W) leftStart = (IMG_W-1);
            leftEnd -= 3;
            if(leftEnd <= -1) leftEnd = 0;
            for (int col = leftStart; col > leftEnd; --col) {  //从右往左扫描
                if(img[row][col] != 0){                        //找到黑线
                    leftBlack[row] = col;
                    leftStart = leftEnd = col;
                    getLeftBlack = 1;
                    if(leftLostCnt == 1){  //补线
                        leftBlack[row+1] = (leftBlack[row+2] + leftBlack[row]) /2;
                    }
                    if(leftLostCnt == 2){ //补线
                        leftBlack[row+1] = leftBlack[row]   - (leftBlack[row]-leftBlack[row+3])/3;
                        leftBlack[row+2] = leftBlack[row+3] + (leftBlack[row]-leftBlack[row+3])/3;
                    }
                    if(leftLostCnt == 3 && ABS(col-rightBlack[row+1])>=5){
                        leftBlack[row+1] = leftBlack[row]   - (leftBlack[row]-leftBlack[row+4])/4;
                        leftBlack[row+2] = leftBlack[row+4] + (leftBlack[row]-leftBlack[row+4])/2;
                        leftBlack[row+3] = leftBlack[row+4] + (leftBlack[row]-leftBlack[row+4])/4;
                    }
                    leftLostCnt = 0;
                    break;                                      //跳出for循环
                }
            }   //左线扫描for循环的终止
        }
        if(getLeftBlack != 1){                              //没有找到黑线
            leftLostCnt ++;
            if (leftLostCnt >= 5) {
                leftStart -= 3;
                leftEnd += 3;
            }
        }

        //找右线
        getRightBlack = 0;

        if(img[row][rightBlack[row+1]] != 0){
            rightBlack[row] = rightBlack[row+1];
            getRightBlack = 1;
            rightLostCnt = 0;
        }else{
            rightStart -= 3;
            if (rightStart <= -1) rightStart = 0;
            rightEnd  += 3;
            if (rightEnd >= IMG_W) rightEnd = (IMG_W -1);
            for (int col = rightStart; col < rightEnd; ++col) {     //从左往右扫描
                if(img[row][col] != 0){                             //找到黑线
                    rightBlack[row] = col;
                    rightStart = rightEnd = col;
                    getRightBlack = 1;
                    if(rightLostCnt == 1){ //补线
                        rightBlack[row+1] = (rightBlack[row+2]+rightBlack[row]) /2;
                    }
                    if(rightLostCnt == 2){  //补线
                        rightBlack[row+1] = rightBlack[row] - (rightBlack[row]-rightBlack[row+3])/3;
                        rightBlack[row+2] = rightBlack[row+3] + (rightBlack[row]-rightBlack[row+3])/3;
                    }
                    if(rightLostCnt == 3  && ABS(col-leftBlack[row+1])>=5){
                        rightBlack[row+1] = rightBlack[row] - (rightBlack[row]-rightBlack[row+4])/4;
                        rightBlack[row+2] = rightBlack[row+4] + (rightBlack[row]-rightBlack[row+4])/2;
                        rightBlack[row+3] = rightBlack[row+4] + (rightBlack[row]-rightBlack[row+4])/4;
                    }
                    rightLostCnt = 0;
                    break;
                }
            }//扫描for循环的终止
        }
        if (getRightBlack != 1){ //没有找到黑线
            rightLostCnt ++;
            if(rightLostCnt >= 5){
                rightStart += 3;
                rightEnd -= 3;
            }
        }
    }

#endif
}



/*
 *两位数转化为字符串
 *
 */

char *
MainWindow::intToChar(const int ch)
{
    static char str[3];
    int tmp = ch;
    int ge,shi;
    ge = tmp %10;
    shi = tmp /10;

    str[0] = shi+0x30;
    str[1] = ge+0x30;
    str[2] = '\0';

    return str;
}

void
MainWindow::imgGetMidLine(void)
{
#if 1
    int leftCnt=0, rightCnt=0;
    lostRow = 10;
    int slop1 = 0, slop2 = 0;

    memset((void *)middle, IMG_W/2 , sizeof(middle));

    for (int row = IMG_H-3; row > 2; --row)  {
        if(leftBlack[row] != -1 && rightBlack[row] != IMG_W && (leftBlack[row] < rightBlack[row])){
            middle[row] = (leftBlack[row] + rightBlack[row]) /2;
            leftCnt = 0;
            rightCnt = 0;
            continue;
        }
        if(leftBlack[row] == rightBlack[row] || (leftBlack[row] > rightBlack[row])){
            if(lostRow == 10){
                lostRow = row;
            }
            if(leftBlack[row+3] != -1 && rightBlack[row+3] == IMG_W) {        //右线找到左线
                rightBlack[row] = IMG_W;
            }else if(rightBlack[row+3] != IMG_W && leftBlack[row+3] == -1){   //左线找到右线
                leftBlack[row] = -1;
            }else if((leftBlack[row+3] == -1) && (rightBlack[row+3] == IMG_W)){
                if((leftBlack[row-3] != -1) && (rightBlack[row-3] == IMG_W)){   //右线找到左线
                    rightBlack[row] = IMG_W;
                }else if((leftBlack[row-3] == -1) && (rightBlack[row-3] != IMG_W)){ // 左线找到右线
                    leftBlack[row] = -1;
                }else{
                    return ;
                }
            }
        }

        if(leftBlack[row] ==-1 && rightBlack[row] != IMG_W){           //丢失左线
            leftCnt ++;
            if(row < 30 && leftCnt >= 2){
                middle[row] = (middle[row+1] + (rightBlack[row] - rightBlack[row+1]) +middle[row+1])/2;
            }else{
                middle[row] = (rightBlack[row]/2 + middle[row+1])/2;
            }
        }else if(leftBlack[row] != -1 && rightBlack[row] == IMG_W){     //丢失右线
            rightCnt ++;
            if(row < 30 && rightCnt >= 2){
                middle[row] = (middle[row+1] + (leftBlack[row] - leftBlack[row+1]) + middle[row+1])/2;
            }else{
                middle[row] = ((IMG_W+leftBlack[row])/2 + middle[row+1]) /2;
            }
        }else if((leftBlack[row] == -1) && (rightBlack[row] == IMG_W)){    //十字丢线
            middle[row] = middle[row+1];
        }

        if((middle[row] <= 3) || (middle[row] >= (IMG_W-3)) || (ABS(middle[row] - middle[row+1]) >= 10)&&(row < 30)){
            if(lostRow == 10){
//                lostRow = row + 2;
            }
        }
    }

    for (int row = IMG_H-2; row > 1; --row)  {      //平滑中线
        if(row < 35){
            slop1 = middle[row+2] - middle[row];
            slop2 = middle[row] - middle[row-2];

            if(slop1*slop2 < 0){       // middle[row]是拐点
                if((ABS(slop1)+ABS(slop2)) > 5){
                    middle[row-1] = middle[row];
                }
            }
        }
        middle[row] = (middle[row-1] + middle[row+1])/2;
    }
#else

    memset((void *)middle, -1, sizeof(middle));

#endif
}


void MainWindow::on_clearButton_clicked()
{
    ui->textEdit->clear();
    ui->tabWidget->clear();
    ui->debugOut->clear();
    memset(img,0,sizeof(img));
    memset(leftBlack, 0, sizeof(leftBlack));
    memset(rightBlack,0, sizeof(rightBlack));
    memset((void *)middle, 0, sizeof(middle));
    imgcnt = 0;
    motorcnt  = 0;
    ui->GoButton->setEnabled(false);

    myCurve->detach();
    xData.clear();
    yData.clear();

    ui->qwtPlot->replot();
    motorcnt = 0;
}

void MainWindow::on_refreshPort_clicked()
{
    QList <QextPortInfo> ports = QextSerialEnumerator::getPorts();
    ui->SerialName->clear();

    foreach(QextPortInfo port, ports){
        ui->SerialName->addItem(port.portName.toLatin1(), port.physName.toLatin1());
    }

    ui->SerialName->setEditable(true);
}

void MainWindow::on_OpenClosePort_clicked()
{
    if(!port->isOpen()){
        port->setPortName(ui->SerialName->itemData(ui->SerialName->currentIndex()).toString());
        if(port->open(QIODevice::ReadWrite)){
            QMessageBox::information(this, tr("通知"), tr("串口已成功打开！" + port->portName().toLatin1()) ,QMessageBox::Ok);
            portSettingEnabled(false);
            ui->OpenClosePort->setText(tr("关闭串口"));
            if(port->isOpen()){
                timer->start();
                handleTimer->start();
            }else{
                timer->stop();
                handleTimer->stop();
            }
        }else{
            QMessageBox::critical(this, tr("错误"),tr("串口未成功打开！"), QMessageBox::Ok);
            portSettingEnabled(true);
            return ;
        }
    }else{
        port->close();
        timer->stop();
        handleTimer->stop();
        ui->OpenClosePort->setText(tr("打开串口"));
        portSettingEnabled(true);
    }
}

void MainWindow::onBaudRateChanged(int index)
{
    port->setBaudRate((BaudRateType)ui->SerBaudRate->itemData(index).toInt());
}

void MainWindow::onParityChanged(int index)
{
    port->setParity((ParityType)ui->SerParity->itemData(index).toInt());
}

void MainWindow::onDataBitChanged(int index)
{
    port->setDataBits((DataBitsType)ui->SerDataBit->itemData(index).toInt());
}

void MainWindow::onStopBitChanged(int index)
{
    port->setStopBits((StopBitsType)ui->SerStopBit->itemData(index).toInt());
}

void MainWindow::portSettingEnabled(bool en)
{
    ui->SerialName->setEnabled(en);
    ui->SerBaudRate->setEnabled(en);
    ui->SerDataBit->setEnabled(en);
    ui->SerParity->setEnabled(en);
    ui->SerStopBit->setEnabled(en);
}

void MainWindow::onPortNameChanged(const QString )
{
    if (port->isOpen()) {
        port->close();
        ui->OpenClosePort->setText(tr("打开串口"));
    }
}

void MainWindow::on_saveData_clicked()
{
    QString filename = QFileDialog::getSaveFileName(this,tr("保存为"),".","Image dat(*.img)");
    if(filename.isEmpty()){
        return ;
    }

    QFile file(filename);
    if (!file.open(QFile::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("保存失败"),tr("无法打开%1文件进行保存").arg(filename),QMessageBox::Yes);
    } else {
        QTextStream io(&file);
        io << ui->textEdit->toPlainText();
        file.close();
    }
}

void MainWindow::onReadyRead()
{
    if(port->bytesAvailable())
    {
        QString dat = QString::fromLatin1(port->readAll());
        ui->textEdit->moveCursor(QTextCursor::End);
        ui->textEdit->insertPlainText(dat);
    }
}

void MainWindow::on_pushButton_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("打开已保存数据文件"),".",tr("Image Dat(*.img *.IMG)"));

    if(filename.isEmpty()){
        return ;
    }

    QFile file(filename);
    if (!file.open(QFile::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("打开失败"),tr("无法打开%1文件").arg(filename),QMessageBox::Yes);
    } else {
        QTextStream io(&file);
        QString str = io.readAll();
        ui->textEdit->insertPlainText(str);
        ui->textEdit->moveCursor(QTextCursor::End);
        file.close();
    }
}

void MainWindow::on_spinBox_valueChanged(int arg1)
{
    refline = arg1;
}

void MainWindow::on_GoButton_clicked()
{
    ui->tabWidget->setCurrentIndex(ui->goIndex->value()*2);
}

void MainWindow::on_realtimeProcessCB_toggled(bool checked)
{
    if(checked){
        connect(handleTimer,SIGNAL(timeout()),this,SLOT(on_handlerButton_clicked()));
        if(port->isOpen()) {
            handleTimer->start();
        }
    }else{
        if(handleTimer->isActive()){
            handleTimer->stop();
        }
        disconnect(handleTimer, SIGNAL(timeout()),this,SLOT(on_handlerButton_clicked()));
    }
}

void MainWindow::on_RowSB_valueChanged(int arg1)
{
    //    IMG_H = arg1;
}

void MainWindow::on_colSB_valueChanged(int arg1)
{
    //    IMG_W = arg1;
}

void MainWindow::on_prev_clicked()
{
    ui->tabWidget->setCurrentIndex((ui->tabWidget->currentIndex() -1 < 0)?0:ui->tabWidget->currentIndex() -1);
}

void MainWindow::on_next_clicked()
{
    ui->tabWidget->setCurrentIndex((ui->tabWidget->currentIndex() +1 > ui->tabWidget->count())?0:ui->tabWidget->currentIndex()+1);
}

void MainWindow::on_pushButton_2_clicked()
{
    myCurve->detach();
    xData.clear();
    yData.clear();

    ui->qwtPlot->replot();
    motorcnt = 0;
    ui->textEdit->clear();
}
