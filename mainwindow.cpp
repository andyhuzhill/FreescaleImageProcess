#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "QString"
#include "QDebug"
#include "QMessageBox"
#include "string.h"

//绘图相关
#include "QPainter"
#include "QPixmap"

//文件相关
#include "QFileDialog"
#include "QFile"
#include "QTextStream"

#include "pid.h"


extern PidObject pidSteer;
extern PidObject pidMotor;

#define MAX_SPEED 7
#define MIN_SPEED 7

#define IMG_MID  (IMG_W/2)
#define ABS(x)   ((x) >= 0 ? (x): (-x))
#define MAX(x,y) ((x) >= (y) ? (x) : (y))
#define MIN(x,y) ((x) >= (y) ? (y) : (x))


//int8_t jiaozheng[] = {22, 24 ,26 ,28 ,29 ,30 ,32 ,33 ,34 ,35 ,36 ,37 ,38 ,39 ,41 ,41 ,43 ,43 ,45 ,46 ,47 ,48 ,50 ,50 ,52 ,52 ,54 ,55 ,56 ,57 ,58 ,59 ,60 ,61 ,62 ,64 ,64 ,65 ,66 ,67 ,68 ,69 ,69 ,71 ,71 ,73 ,73 ,74 ,75 ,75 ,76 ,76 ,76 ,77 ,77 ,77 ,81 ,81 ,81 ,81 };

const int8_t IMG_H = 60;
const int8_t IMG_W = 80;
int8_t srcImg[IMG_H][IMG_W/8];
int8_t img[IMG_H][IMG_W];

int8_t leftBlack[IMG_H];
int8_t middle[IMG_H];
int8_t rightBlack[IMG_H];

int8_t leftFalling[IMG_H][IMG_W] = {0};
int8_t rightRising[IMG_H][IMG_W] = {0};

int8_t lostRow;
int8_t startRow;
int8_t endRow;

int8_t max=0;
int8_t min=0;

class paint: public QWidget
{
public:
    paint(const int8_t *content,int imgcount);
    ~paint(void);

protected:
    void paintEvent(QPaintEvent *);

private:
    int8_t img[IMG_H][IMG_W/8];
    int imgccc;
};

paint::paint(const int8_t *content, int imgcount)
{
    memcpy(img, content, sizeof(img));

    imgccc = imgcount;
}

paint::~paint()
{

}

void paint::paintEvent(QPaintEvent *)
{
    QPixmap pixmap(800,600);
    pixmap.fill(Qt::white);

    QPainter painter(&pixmap);

    for (int row = 0; row < IMG_H; ++row) {
        for (int col = 0; col < (IMG_W/8); ++col) {
            for (int i = 0; i < 8; ++i) {
                if(img[row][col] & (1 << i)){
                    painter.fillRect((col*8+(7-i))*10,(row)*10, 10, 10, Qt::black);
                }
            }
        }
    }


    painter.setPen(Qt::darkYellow);
    for (int i = 0; i < 6 ; ++i) {
        painter.drawLine(0,i*100,800,i*100);
        painter.drawLine(i*100,0, i*100, 600);
    }
    painter.drawLine(6*100, 0, 6*100,600);
    painter.drawLine(7*100, 0, 7*100,600);

    if(pixmap.save(QString("img[%1].bmp").arg(imgccc),"BMP",100)){
        qDebug() << "img save succes" ;
    }else{
        qDebug() << "img save failed";
    }

    QPainter finalPainter(this);
    finalPainter.drawPixmap(0,0,pixmap);

}


class paint2: public QWidget
{
public:
    paint2(const int8_t *content, const int8_t* leftdat, const int8_t *middat, const int8_t *rightdat, int8_t lostr);
    ~paint2(void);

protected:
    void paintEvent(QPaintEvent *);

private:
    int8_t img[IMG_H][IMG_W];
    int8_t left[IMG_H];
    int8_t mid[IMG_H];
    int8_t rig[IMG_H];
    int8_t rowlost;
};

paint2::paint2(const int8_t *content, const int8_t *leftdat, const int8_t *middat, const int8_t *rightdat, int8_t lostr)
{
    memcpy(img, content, sizeof(img));
    memcpy(left, leftdat, sizeof(left));
    memcpy(mid, middat, sizeof(mid));
    memcpy(rig, rightdat, sizeof(rig));
    rowlost = lostr;
}

paint2::~paint2()
{

}

void paint2::paintEvent(QPaintEvent *)
{
    QPixmap pixmap(800,600);
    pixmap.fill(Qt::white);

    QPainter painter(&pixmap);

    painter.setPen(Qt::black);

    for (int row = 0; row < IMG_H; ++row) {
        for (int col = 0; col < IMG_W; ++col) {
            if(img[row][col]){
                painter.fillRect((col)*10,(row)*10, 10, 10, Qt::blue);
            }
        }
    }

    for (int row = 0; row < IMG_H; ++row) {
        painter.fillRect(left[row]*10, row*10,5,5,Qt::yellow);
        painter.fillRect(mid[row]*10, row*10, 5,5, Qt::red);
        painter.fillRect(rig[row]*10, row*10, 5,5, Qt::green);

        painter.drawText(1,row*10,QString("%1").arg(left[row]));
        painter.drawText(IMG_W/2*10,row*10, QString("%1").arg(mid[row]));
        painter.drawText((IMG_W-3)*10,row*10,QString("%1").arg(rig[row]));
        painter.setPen(Qt::gray);
        painter.drawLine(0,row*10, IMG_W*10, row*10);
        painter.setPen(Qt::black);
    }

    int b;
    b = MAX(rowlost, 3);
    if(b >= 50) b = 3;
    int sum=0;
    int average;
    for (int i = b ; i < 50; ++i) {
        sum += mid[i];
    }
    average = sum /(50-b);

    int error = average - 40;

    if(error > max){
        max = error;
    }
    if(error < min){
        min = error;
    }

    if(ABS(error) <= 3){
        pidSteer.kp = error*error/10 + 10;
        pidSteer.kd = 200;
        pidSteer.prevError = 0;
    }else{
        pidSteer.kp = error*error/5 + 50;
        pidSteer.kd = 500;
    }

    int ret = UpdatePID(&pidSteer, error);

    ret += 5000;

    int speed = MAX_SPEED-error*error*(MAX_SPEED-MIN_SPEED)/(1600);

    painter.drawText(250,225,QString("lostRow=%1").arg(rowlost));
    painter.drawText(250,250, QString("middle[lostRow]=%1").arg(mid[rowlost]));
    painter.drawText(250,300,QString("average=%1").arg(average));

    painter.drawText(250,350,QString("steer=%1").arg(ret));

    painter.drawText(250,400,QString("speed=%1").arg(speed));


    painter.setPen(Qt::darkYellow);
    for (int i = 0; i < 6 ; ++i) {
        painter.drawLine(0,i*100,800,i*100);
        painter.drawLine(i*100,0, i*100, 600);
    }
    painter.drawLine(6*100, 0, 6*100,600);
    painter.drawLine(7*100, 0, 7*100,600);

    QPainter finalPainter(this);
    finalPainter.drawPixmap(0,0,pixmap);

}


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    pidInit(&pidSteer,0,10,0,20);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("打开数据文件"),".",tr("图像文件(*.img *.IMG)"));
    if (filename.isEmpty()) {
        QMessageBox::warning(this, tr("警告"),tr("未提供文件名"), QMessageBox::Yes);
    } else {
        QFile file(filename);
        if (!file.open(QFile::ReadOnly)){
            QMessageBox::warning(this, tr("警告"),tr("无法打开文件%1").arg(filename), QMessageBox::Yes);
            return ;
        }
        QTextStream iostream(&file);

        QString content = iostream.readAll();

        QStringList splitcontent = content.split("img", QString::SkipEmptyParts);

        ui->goIndex->setMaximum(splitcontent.length());
        ui->progressBar->setMaximum(splitcontent.length());

        int imgcnt = 0;
        foreach(QString line, splitcontent){
            QStringList dat = line.split(",", QString::SkipEmptyParts);
            int row = 0, col =0;
            foreach(QString item, dat){
                srcImg[row][col++] = item.toInt();
                if (col == 10) {
                    col = 0;
                    row ++;
                }
                if (row == 60) break;
            }

            imgcnt++;

            ui->tabWidget->addTab(new paint((int8_t *)srcImg,imgcnt),QString(tr("原始图像[%1]")).arg(imgcnt));

            imgResize();
            //            imgFilter();
            //            findLine();
            imgFindLine();
            getMidLine();

            ui->tabWidget->addTab(new paint2((int8_t*)img, (int8_t*)leftBlack, (int8_t*)middle,(int8_t*)rightBlack, lostRow), QString(tr("处理后图像[%1]")).arg(imgcnt
                                                                                                                                                           ));
            ui->tabWidget->setCurrentIndex(2*imgcnt-1);
            ui->progressBar->setValue(imgcnt);
        }
    }
}

void MainWindow::on_GoButton_clicked()
{
    ui->tabWidget->setCurrentIndex(ui->goIndex->value()*2);
}

void MainWindow::on_prev_clicked()
{
    ui->tabWidget->setCurrentIndex((ui->tabWidget->currentIndex() -1 < 0)?0:ui->tabWidget->currentIndex() -1);
}

void MainWindow::on_next_clicked()
{
    ui->tabWidget->setCurrentIndex((ui->tabWidget->currentIndex() +1 > ui->tabWidget->count())?0:ui->tabWidget->currentIndex()+1);
}

void MainWindow::imgResize(void)
{
    int row, col, i;
    for(row=0; row<IMG_H; ++row){
        for(col=0; col<IMG_W/8; ++col){
            for(i=7;i>=0;--i){
                if(srcImg[row][col] & (1 << i)){
                    img[row][col*8+(7-i)] = 1;
                }else{
                    img[row][col*8+(7-i)] = 0;
                }
            }
        }
    }
}

//void MainWindow::findLine()
//{
//    int8_t getLeft=0, getRight=0;
//    int8_t row, col;
//    int8_t start, end;
//    int8_t slop1, slop2;

//    memset(leftBlack, -1, sizeof(leftBlack));
//    memset(rightBlack, IMG_W, sizeof(rightBlack));
//    memset(middle, (IMG_W/2), sizeof(middle));

//    lostRow = 3;

//    for(row=IMG_H-1; row > (IMG_H-8); --row){
//        getLeft = getRight = 0;

//        for(col=IMG_W/2; col>0; --col){
//            if((img[row][col] == 0) && (img[row][col-1] != 0)){
//                getLeft = 1;
//                leftBlack[row] = col - 1;
//                break;
//            }
//        }
//        if(!getLeft){
//            for(col=IMG_W/2; col < IMG_W; ++col){
//                if(img[row][col] != 0 && (img[row][col+1] == 0)){
//                    getLeft = 1;
//                    leftBlack[row] = col;
//                    break;
//                }
//            }
//        }

//        for(col=IMG_W/2; col < IMG_W; ++col){
//            if((img[row][col]==0) && (img[row][col+1] != 0)){
//                getRight = 1;
//                rightBlack[row] = col + 1;
//                break;
//            }
//        }
//        if(!getRight){
//            for(col=IMG_W/2; col > 0; --col){
//                if((img[row][col+1] != 0) && (img[row][col] == 0)){
//                    getRight = 1;
//                    rightBlack[row] = col+1;
//                    break;
//                }
//            }
//        }

//        if(getLeft && getRight
//                && (leftBlack[row] < rightBlack[row])
//                && (ABS(rightBlack[row]-leftBlack[row]) > 20)
//                ){            //找到两边黑线
//            middle[row] = (leftBlack[row]+rightBlack[row])/2;
//        }else if(getLeft && getRight
//                 && ((leftBlack[row] > rightBlack[row]) && leftBlack[row] > IMG_W/2)
//                 ){
//            middle[row] = rightBlack[row] / 2;
//        }else if (getLeft && getRight
//                  && ((leftBlack[row] > rightBlack[row]) && rightBlack[row] < IMG_W/2)
//                  ){
//            middle[row] = (leftBlack[row] + IMG_W-1)/2;
//        }else if(getLeft && !getRight){     //丢失右边黑线
//            middle[row] = (leftBlack[row]+IMG_W)/2;
//        }else if(!getLeft && getRight){     //丢失左边黑线
//            middle[row] = rightBlack[row]/2;
//        }else if(!getLeft && !getRight){    //两边丢线
//            // ??
//        }

//        if((middle[row] <3) || (middle[row] > (IMG_W-2))){
//            lostRow = row;
//            return;
//        }
//    }

//    for(row = (IMG_H-8); row > 0; --row){
//        getLeft = getRight = 0;
//        start   = middle[row+1];
//        end     = leftBlack[row+1] - 5;
//        if (end < 0) end = 0;
//        for(col=start; col > end; --col){   //从右往左搜索
//            if((img[row][col] == 0) && (img[row][col-1] != 0)){
//                getLeft = 1;
//                leftBlack[row] = col-1;
//                break;
//            }
//        }

//        end     = rightBlack[row+1] + 5;
//        if (end > IMG_W-1) end = IMG_W-1;
//        for(col=start; col < end; ++ col){   //从左往右搜索
//            if((img[row][col] == 0) && (img[row][col+1] != 0)){
//                getRight = 1;
//                rightBlack[row] = col+1;
//                break;
//            }
//        }

//        if(getLeft && getRight
//                && (leftBlack[row] < rightBlack[row])
//                && ((rightBlack[row]-leftBlack[row]) > 10)
//                && (ABS(leftBlack[row]-leftBlack[row+1]) < 10)
//                && (ABS(rightBlack[row]-rightBlack[row+1]) < 10)
//                ){            //找到两边黑线
//            middle[row] = (leftBlack[row]+rightBlack[row])/2;
//        }else if(getLeft && !getRight
//                 && (ABS(leftBlack[row]-leftBlack[row+1]) < 10)
//                 ){         //丢失右边黑线
//            middle[row] = middle[row+1] + (leftBlack[row] - leftBlack[row+1]);
//        }else if(getLeft && !getRight){
//            middle[row] = (leftBlack[row] + IMG_W) / 2;
//        }else if(!getLeft && getRight
//                 && (ABS(rightBlack[row]-rightBlack[row+1]) < 10)
//                 ){     //丢失左边黑线
//            middle[row] = middle[row+1] + (rightBlack[row] - rightBlack[row+1]);
//        }else if(!getLeft && getRight){
//            middle[row] = rightBlack[row] / 2;
//        }else if(!getLeft && !getRight){    //两边丢线
//            // ??
//            if(middle[row+1] != 0){
//                middle[row] = middle[row+1];
//            }else{
//                middle[row] = IMG_W/2;
//            }
//        }

//        if((middle[row] <3) || (middle[row] > (IMG_W-2))){
//            lostRow = row;
//            return;
//        }
//    }

//    for(row=IMG_H-4;row>=lostRow+2; ++row){
//        middle[row] = (middle[row-1]+middle[row]+middle[row+1])/3;
//    }
//}

void MainWindow::getMidLine()
{
    lostRow = 3;

    memset((void *)middle, 0 , sizeof(middle));

    for (int row = IMG_H-5; row > 0; --row) {
        if(leftBlack[row] != -1 && rightBlack[row] != IMG_W && (leftBlack[row] < rightBlack[row])){
            middle[row] = (leftBlack[row] + rightBlack[row])/2;
            continue;
        }else if(leftBlack[row] <= -1 && rightBlack[row] != IMG_W){     //丢失左线
            if(row > 50){
                middle[row] = rightBlack[row] / 2 ;
            }else if((rightBlack[row+1] != IMG_W) && (rightBlack[row+2] != IMG_W)){
                middle[row] = middle[row+1] + (rightBlack[row+1] - rightBlack[row+2]);
            }else{
                middle[row] = middle[row+1];
            }
        }else if(leftBlack[row] != -1 && rightBlack[row] >= IMG_W-1){     //丢失右线
            if(row > 50){
                middle[row] = (leftBlack[row]+IMG_W) /2 ;
            }else if((leftBlack[row+1] != -1) && (leftBlack[row+2] != -1)){
                middle[row] = middle[row+1] + (leftBlack[row+1] - leftBlack[row+2]);
            }else{
                middle[row] = middle[row+1];
            }
        }else if(leftBlack[row] == -1 && rightBlack[row] == IMG_W){
            if(row > 50){
                middle[row] = IMG_MID;
            }else{
                middle[row] = middle[row+1];
            }
        }else{
            if((leftBlack[row+5] == -1) &&(rightBlack[row+1]!=IMG_W) &&(rightBlack[row+2] != IMG_W)){
                middle[row] = middle[row+1] + rightBlack[row+1] - rightBlack[row+2];
            }else if((rightBlack[row+5] == IMG_W)&&(leftBlack[row+1]!=-1) &&(leftBlack[row+2] != -1)){
                middle[row] = middle[row+1] + leftBlack[row+1] - leftBlack[row+2];
            }
        }
    }

    for(int row = IMG_H-5; row > 1; --row){
        middle[row]= (middle[row+1] + middle[row] + middle[row-1])/3;

        if(middle[row]<3 || middle[row] > (IMG_W-3)){
            if(lostRow == 3){
                lostRow = row;
            }
        }
    }
}


void MainWindow::imgFilter(void)
{
    int8_t row, col;
    for (row = 1; row < IMG_H-1; ++row) {
        for (col = 1; col < IMG_W-1; ++col) {
            if (img[row][col-1]==1 && img[row][col]==0 && img[row][col+1]==1) {
                img[row][col] = 1;
            }
            if (img[row][col-1]==0 && img[row][col]==1 && img[row][col+1]==0) {
                img[row][col] = 0;
            }
        }
    }
}

void MainWindow::imgGetStartLine(void)
{
    int row, col;
    int count = 0;
    int tiaobian[8] = {0};

    for(row = IMG_H-1; row > (2); --row){
        count = 0;
        for(col=0;col<(IMG_W-1); ++col){
            if(img[row][col]!= img[row][col+1]){
                tiaobian[count++] = row;
                if(count >= 5){
                    if(ABS((tiaobian[2]-tiaobian[1])-(tiaobian[4]-tiaobian[3])) <= 2){
                        QMessageBox::warning(this,tr("警告"),tr("起跑线"),QMessageBox::Yes);
                        return ;
                    }
                }
            }
        }
    }
}

void MainWindow::imgFindLine(void)
{
#if 0
    int8_t row, col;

    int8_t leftStart, leftEnd, rightStart, rightEnd;
    int8_t getLeftBlack=0, getRightBlack=0;  //标志是否找到黑线
    int8_t leftLostCnt =0, rightLostCnt=0;
    int8_t leftadder, rightadder;

    memset((void *)leftBlack, -1, sizeof(leftBlack));
    memset((void *)rightBlack, IMG_W, sizeof(rightBlack));

    row = IMG_H -3;
    getRightBlack = getLeftBlack = 0;
    do{
        for (col = IMG_W/2 ; col >= 0; --col) {  // 先找左边黑线
            if ((img[row][col] != 0) && (img[row][col+1]==0)){
                leftBlack[row] = col;               //记录下黑线的列数
                if(col <= 20 || (ABS(leftBlack[row+1]-col) <= 20)){
                    getLeftBlack ++;
                    break;
                }
            }
        }
        if((leftBlack[row] == -1) && (leftBlack[row + 1] != -1)){
            leftBlack[row] = leftBlack[row+1];
        }

        for (col = IMG_W/2; col <= (IMG_W -1); ++col) {  // 再找右边黑线
            if ((img[row][col] != 0) && (img[row][col-1] == 0)){     //发现黑线
                rightBlack[row] = col;   //记录下黑线的列数
                if(col >= 60 || (ABS(rightBlack[row+1]-col) <= 20)){
                    getRightBlack ++;
                    break;
                }
            }
        }
        if((rightBlack[row] == IMG_W) && (rightBlack[row+1] != IMG_W)){
            rightBlack[row] = rightBlack[row+1];
        }
        row --;
    }while(getLeftBlack != 5 && getRightBlack != 5);

    leftStart = leftEnd = leftBlack[row+1];
    rightStart = rightEnd = rightBlack[row+1];

    do{  //找左边黑线
        getLeftBlack = 0;
        leftStart += 3;
        leftEnd -= 3;
        if (leftEnd < 0) leftEnd = 0;
        if (leftStart > IMG_W -1) leftStart = IMG_W-1;//避免数组访问越界

        for (col = leftStart; col >= leftEnd ; --col) { //从右向左搜索
            if ((img[row][col] != 0) && (img[row][col+1] == 0) ){//找到黑线
                leftBlack[row] = col;
                leftStart = leftEnd = col ;
                getLeftBlack = 1;
                break;  //找到黑线退出for循环
            }
        }
        if(getLeftBlack != 1){ //没有找到黑线
            leftBlack[row] = leftBlack[row+1]+ (leftBlack[row+1] -leftBlack[row+4])/3;
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
            if ((img[row][col]!=0) && (img[row][col-1] == 0) ){ //找到黑线
                rightBlack[row] = col;
                rightStart = rightEnd = col;
                getRightBlack = 1;
                break;      //跳出for循环
            }
        }
        if (getRightBlack != 1){ //没有找到黑线
            rightBlack[row] = rightBlack[row+1] + (rightBlack[row+1]- rightBlack[row+4])/3;

            if(rightLostCnt > 4){
                rightStart += 3;
                rightEnd -= 3;
            }
        }
        row --;
    }while(row > 0);
#else
    int8_t row, col;

    int8_t leftStart, leftEnd, rightStart, rightEnd;
    int8_t getLeftBlack=0, getRightBlack=0;  //标志是否找到黑线
    int8_t leftLostCnt =0, rightLostCnt=0;
    int8_t slop1, slop2;

    memset((void *)leftBlack, -1, sizeof(leftBlack));
    memset((void *)rightBlack, IMG_W, sizeof(rightBlack));

    row = IMG_H -3;
    getRightBlack = getLeftBlack = 0;
    do{
        for (col = IMG_W/2 ; col >= 0; --col) {  // 先找左边黑线
            if ((img[row][col] != 0) && (img[row][col+1]==0)){
                leftBlack[row] = col;               //记录下黑线的列数
                if(col <= 20 || (ABS(leftBlack[row+1]-col) <= 20)){
                    getLeftBlack ++;
                    break;
                }
            }
        }

        for (col = IMG_W/2; col <= (IMG_W -1); ++col) {  // 再找右边黑线
            if ((img[row][col] != 0) && (img[row][col-1] == 0)){     //发现黑线
                rightBlack[row] = col;   //记录下黑线的列数
                if(rightBlack[row] < leftBlack[row] || (ABS(rightBlack[row]-leftBlack[row]) < 10)){
                    rightBlack[row] = IMG_W;
                    continue;
                }
                if(col >= 60 || (ABS(rightBlack[row+1]-col) <= 20)){
                    getRightBlack ++;
                    break;
                }
            }
        }

        row --;
    }while(getLeftBlack != 5 && getRightBlack != 5);

    leftStart = leftEnd = leftBlack[row+1];
    rightStart = rightEnd = rightBlack[row+1];

    do{  //找左边黑线
        getLeftBlack = 0;
        leftStart += 3;
        leftEnd -= 3;
        if (leftEnd < 0) leftEnd = 0;
        if (leftStart > IMG_W -1) leftStart = IMG_W-1;//避免数组访问越界

        for (col = leftStart; col >= leftEnd ; --col) { //从右向左搜索
            if ((img[row][col] != 0) && (img[row][col+1] == 0) ){//找到黑线
                leftBlack[row] = col;
                if((ABS(leftBlack[row]-leftBlack[row+5]) > 20) && leftBlack[row+5] != -1){
                    leftBlack[row] = -1;
                    continue;
                }
                leftStart = leftEnd = col;
                if(leftBlack[row] != -1 && leftBlack[row+2] != -1 && leftBlack[row+4] != -1){
                    slop1 = leftBlack[row] - leftBlack[row+2];
                    slop2 = leftBlack[row+2] - leftBlack[row+4];
                    if(slop1*slop2 < 0){
                        leftBlack[row+1] = leftBlack[row+2] + slop2 /2;
                        leftBlack[row]   = leftBlack[row+2] + slop2;
                    }
                }

                getLeftBlack = 1;
                leftLostCnt = 0;
                break;  //找到黑线退出for循环
            }
        }
        if(getLeftBlack != 1){ //没有找到黑线
            leftBlack[row] = leftBlack[row+1]+ (leftBlack[row+2] -leftBlack[row+4])/2;
            if(leftBlack[row] <= 0){
                leftBlack[row] = -1;
            }

            if(leftLostCnt > 3){
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
            if ((img[row][col]!=0) && (img[row][col-1] == 0) ){ //找到黑线
                rightBlack[row] = col;
                if(rightBlack[row] < leftBlack[row] || (ABS(rightBlack[row]-leftBlack[row]) < 10) || ((ABS(rightBlack[row]-rightBlack[row+5]) > 20) && rightBlack[row+5] != IMG_W)){
                    rightBlack[row] = IMG_W;
                    continue;
                }
                rightStart = rightEnd = col;
                rightLostCnt = 0;
                getRightBlack = 1;
                if(rightBlack[row] != IMG_W && rightBlack[row+2] != IMG_W && rightBlack[row+4]!= IMG_W){
                    slop1 = rightBlack[row] - rightBlack[row+2];
                    slop2 = rightBlack[row+2] - rightBlack[row+4];
                    if(slop1*slop2 < 0){
                        rightBlack[row+1] = rightBlack[row+2] + slop2 /2;
                        rightBlack[row]   = rightBlack[row+2] + slop2;
                    }
                }
                break;      //跳出for循环
            }
        }
        if (getRightBlack != 1){ //没有找到黑线
            rightBlack[row] = rightBlack[row+1] + (rightBlack[row+2]- rightBlack[row+4])/2;
            if(rightBlack[row] >= IMG_W){
                rightBlack[row] = IMG_W;
            }
            if(rightLostCnt > 3){
                rightStart += 3;
                rightEnd -= 3;
            }
        }
        row --;
    }while(row > 0);
#endif
}

void MainWindow::on_pushButton_2_clicked()
{
    ui->tabWidget->clear();
}

void MainWindow::on_pushButton_3_clicked()
{
    QMessageBox::information(this, tr("偏差范围"),QString("max = %1, min=%2").arg(max).arg(min), QMessageBox::Yes);
}
