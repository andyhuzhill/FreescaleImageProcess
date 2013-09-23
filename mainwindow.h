#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

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
    void on_pushButton_clicked();
    void on_GoButton_clicked();
    void on_prev_clicked();
    void on_next_clicked();

    void on_pushButton_2_clicked();

    void on_pushButton_3_clicked();

private:
    Ui::MainWindow *ui;
    void imgResize(void);
//    void findLine();
    void getMidLine();
    void imgFilter(void);
    void imgGetStartLine(void);
    void imgFindLine(void);
};

#endif // MAINWINDOW_H
