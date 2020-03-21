#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGridLayout>
#include <QPushButton>
#include <QInputDialog>
#include <QDebug>
#include <QTime>
#include <QMessageBox>
#include <QtTest/QtTest>
#include <sys/stat.h>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void getDifficulty();
private:
    Ui::MainWindow *ui;
    QPushButton ***btns;
    int hb, wb;//高多少个格子，宽多少个格子
    int lastClickedH, lastClickedW;//上一个点的哪儿
    int remains;//剩余多少
    int **types;//哪种类型的图片。0代表被清理了
    QPixmap icons[8];//图片，多少种写死了
    QPixmap imageLine[2], imageTurn[4];
    void setDifficulty(int setHb, int setWb);
    void setBlocks();

    struct LinkPoints{
        int cnt;
        int pt[4][2];//0是第一个端点，cnt+1是第二个端点，其他是中间点
    };

    //判定能否链接
    bool canNoCorner(int lstH, int lstW, int thisH, int thisW, LinkPoints &lp);
    bool canOneCorner(int lstH, int lstW, int thisH, int thisW, LinkPoints &lp);
    bool canTwoCorner(int lstH, int lstW, int thisH, int thisW, LinkPoints &lp);
    void stageClear();
    int getDirection(int lstH, int lstW, int thisH, int thisW, int midH, int midW);
    void drawALine(int lstH, int lstW, int thisH, int thisW, bool isW);
    void drawLines(LinkPoints lp);

private slots:
    void btnsClicked();
};

#endif // MAINWINDOW_H
