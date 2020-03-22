#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //设置图标题目
    setWindowTitle("linkmeup");
    setWindowIcon(QIcon(":images/images/myico.ico"));

    //初始化难度
    getDifficulty();

    //初始化图片、音效
    types = nullptr;
    btns = nullptr;
    for(int i=0; i<=16; i++)
        icons[i].load(QString::asprintf(":images/images/%d.jpg", i));
    imageLine[0].load(":images/images/line.jpg");
    QMatrix matrix;
    matrix.rotate(90.0);
    imageLine[1] = imageLine[0].transformed(matrix,
                                            Qt::FastTransformation);
    imageTurn[0].load(":images/images/turn.jpg");
    for(int i=1; i<=3; i++)
        imageTurn[i] = imageTurn[i-1].transformed(matrix,
                                                  Qt::FastTransformation);
    clickSound.setSource(QUrl::fromLocalFile(":sounds/sounds/clicks.wav"));
    endsSound.setSource(QUrl::fromLocalFile(":sounds/sounds/ends.wav"));

    //设置grid布局
    ui->gridLayout->setHorizontalSpacing(1);
    ui->gridLayout->setVerticalSpacing(1);
    ui->gridLayout->setSizeConstraint(QLayout::SetFixedSize);
    ui->gridLayout->activate();

    //清理场景
    stageClear();

    //设置场景
    setBlocks();
}

MainWindow::~MainWindow()
{
    stageClear();
    delete ui;
}

void MainWindow::getDifficulty()
{
    QStringList items;
    items << "简单" << "普通" << "困难";
    QString dlgTitle = "选择难度";
    QString txtLabel = "请选择难度";
    bool ok = false;

    //默认为普通难度
    QString text = QInputDialog::getItem(this, dlgTitle,
                                         txtLabel, items, 1, false, &ok);
    if(!ok)
        exit(0);

    if (text=="简单")
        setDifficulty(6, 6, 8);
    else if (text=="困难")
        setDifficulty(10, 14, 15);
    else
        setDifficulty(8, 10, 11);
}

void MainWindow::setDifficulty(int setHb, int setWb, int setFigure)
{
    hb = setHb;
    wb = setWb;
    figures = setFigure;
}

void MainWindow::btnsClicked() {
    //获取被点击的按钮的坐标
    QObject *obj = qobject_cast<QPushButton *>(sender());
    int thisH=0, thisW=0;
    for(int i=1; i<=hb; i++)
        for(int j=1; j<=wb; j++)
            if(btns[i][j]==obj) {
                thisH = i;
                thisW = j;
                break;
            }

    //点击已经消除的无效
    if(types[thisH][thisW]==0)
        return ;

    //如果没有已经点击的按钮，就记录当前按钮为第一个点击的按钮
    if (!lastClickedH && !lastClickedW) {
        lastClickedH = thisH;
        lastClickedW = thisW;
        return ;
    }

    //点击已经点过的无效
    if(lastClickedH==thisH && lastClickedW==thisW)
        return ;

    //点击不同类型的无效
    if(types[lastClickedH][lastClickedW]!=types[thisH][thisW]) {
        lastClickedH = thisH;
        lastClickedW = thisW;
        return ;
    }

    //判定这次点击配对的合法性
    LinkPoints lp;
    if(canNoCorner(lastClickedH, lastClickedW, thisH, thisW, lp)
            || canOneCorner(lastClickedH, lastClickedW, thisH, thisW, lp)
            || canTwoCorner(lastClickedH, lastClickedW, thisH, thisW, lp)) {
        //如果没有开始游戏，就开始游戏。同时开始计时
        if(!isStarted) {
            isStarted = true;
            pTime->start();
            pTimer->start();
        }

        //点击音效
        clickSound.play();

        //画线连接两张图片
        drawLines(lp);

        //设置两张图片为被点击了
        types[lastClickedH][lastClickedW] = types[thisH][thisW] = 0;
        btns[thisH][thisW]->setIcon(icons[0]);
        ui->gridLayout->addWidget(btns[thisH][thisW],
                                  thisH, thisW, 1, 1,
                                  Qt::AlignCenter | Qt::AlignHCenter);
        btns[lastClickedH][lastClickedW]->setIcon(icons[0]);
        ui->gridLayout->addWidget(btns[lastClickedH][lastClickedW],
                                  lastClickedH, lastClickedW, 1, 1,
                                  Qt::AlignCenter | Qt::AlignHCenter);

        //重置点击按钮
        lastClickedH = lastClickedW = 0;

        //修改进度
        remains -= 2;
        ui->progressBar->setValue(100*(hb*wb-remains)/(hb*wb));

        //结束了，要不要继续玩
        if (remains<=0) {
            pTimer->stop();
            endsSound.play();
            QMessageBox::StandardButton result =
                    QMessageBox::question(this, "完成",
                                  "恭喜你通过本关！\n要再来一局吗？",
                                  QMessageBox::Yes | QMessageBox::No,
                                  QMessageBox::NoButton);
            if(result==QMessageBox::No)
                exit(0);
            on_actNewGame_triggered();
        }
    }
    else {
        //不合法的配对
        lastClickedH = lastClickedW = 0;
    }
}

void MainWindow::onTimeOut()
{
    //获取流逝的时间并更新lcd
    int elapsedMSec = pTime->elapsed();
    if(elapsedMSec>60000)
        ui->lcdNumber->display(QString::asprintf("%d:%d.%3d",
                                                 elapsedMSec / 60000,
                                                 (elapsedMSec / 1000) % 60,
                                                 elapsedMSec % 1000));
    else
        ui->lcdNumber->display(QString::asprintf("%d.%3d",
                                                 elapsedMSec / 1000,
                                                 elapsedMSec % 1000));
}

void MainWindow::setBlocks()
{
    //重置剩余块
    remains = hb * wb;

    //初始化计时器
    pTimer = new QTimer;
    pTime = new QTime;
    pTimer->setInterval(60);
    connect(pTimer, SIGNAL(timeout()), this, SLOT(onTimeOut()));

    //初始化按钮数组，按钮类型。长宽加2，边上留空
    btns = new QPushButton **[hb+2];
    types = new int *[hb+2];
    for(int i=0; i<=hb+1; i++) {
        types[i] = new int [wb+2];
        btns[i] = new QPushButton *[wb+2];
    }
    int *typeList = new int [hb*wb];
    for(int i=0; i<hb*wb; i+=2)
        typeList[i] = typeList[i+1] = i/2 % figures + 1;

    //生成随机地图
    qsrand(QTime(0,0,0).secsTo(QTime::currentTime()));
    for(int i=0; i<hb*wb; i++) {
        int idx=qrand()%(hb*wb-i)+i;
        int t=typeList[idx];
        typeList[idx] = typeList[i];
        typeList[i] = t;
        types[i/wb+1][i%wb+1] = typeList[i];
    }

    //设置地图
    for(int i=0; i<=hb+1; i++)
        for(int j=0; j<=wb+1; j++) {
            btns[i][j] = new QPushButton;
            if(i==0 || j==0 || i==hb+1 || j==wb+1) {
                types[i][j] = 0;
                btns[i][j]->setIcon(icons[types[i][j]]);
            }
            else {
                btns[i][j]->setIcon(icons[types[i][j]]);
                connect(btns[i][j], SIGNAL(clicked()),
                        this, SLOT(btnsClicked()));
            }
            btns[i][j]->setMinimumWidth(60);
            btns[i][j]->setMinimumHeight(60);
            btns[i][j]->setMaximumWidth(60);
            btns[i][j]->setMaximumHeight(60);
            btns[i][j]->setIconSize(QSize(60,60));
            ui->gridLayout->addWidget(btns[i][j],
                                      i, j, 1, 1,
                                      Qt::AlignCenter | Qt::AlignHCenter);
        }
}

bool MainWindow::canNoCorner(int lstH, int lstW,
                             int thisH, int thisW, LinkPoints &lp)
{
    lp.cnt = 0;
    lp.pt[0][0] = lstH; lp.pt[0][1] = lstW;
    lp.pt[1][0] = thisH; lp.pt[1][1] = thisW;
    if (lstW==thisW) {
        int minH = lstH<thisH?lstH:thisH;
        int maxH = lstH + thisH - minH;
        for(int i=minH+1; i<=maxH-1; i++)
            if(types[i][lstW])
                return false;
        return true;
    }
    else if (lstH==thisH) {
        int minW = lstW<thisW?lstW:thisW;
        int maxW = lstW + thisW - minW;
        for(int i=minW+1; i<=maxW-1; i++)
            if(types[lstH][i])
                return false;
        return true;
    }
    return false;
}

bool MainWindow::canOneCorner(int lstH, int lstW,
                              int thisH, int thisW, LinkPoints &lp)
{
    lp.cnt = 1;
    lp.pt[0][0] = lstH; lp.pt[0][1] = lstW;
    lp.pt[2][0] = thisH; lp.pt[2][1] = thisW;
    if(lstH==thisH || lstW==thisW)  return false;
    LinkPoints rubbish;
    if(types[lstH][thisW]==0) {
        lp.pt[1][0] = lstH; lp.pt[1][1] = thisW;
        if(canNoCorner(lstH, lstW, lstH, thisW, rubbish)
                && canNoCorner(lstH, thisW, thisH, thisW, rubbish))
            return true;
    }
    if(types[thisH][lstW]==0) {
        lp.pt[1][0] = thisH; lp.pt[1][1] = lstW;
        if(canNoCorner(lstH, lstW, thisH, lstW, rubbish)
                && canNoCorner(thisH, lstW, thisH, thisW, rubbish))
            return true;
    }
    return false;
}

bool MainWindow::canTwoCorner(int lstH, int lstW,
                              int thisH, int thisW, LinkPoints &lp)
{
    lp.cnt = 2;
    lp.pt[0][0] = lstH; lp.pt[0][1] = lstW;
    lp.pt[3][0] = thisH; lp.pt[3][1] = thisW;
    LinkPoints rubbish;
    for(int i=0; i<=hb+1; i++) {
        if(i!=lstH && i!=thisH
                && !types[i][lstW] && !types[i][thisW]
                && canNoCorner(lstH, lstW, i, lstW, rubbish)
                && canOneCorner(i, lstW, thisH, thisW, rubbish)) {
            lp.pt[1][0] = i; lp.pt[1][1] = lstW;
            lp.pt[2][0] = i; lp.pt[2][1] = thisW;
            return true;
        }

        if(i!=thisH && i!=lstH
                && !types[i][thisW] && !types[i][lstW]
                && canOneCorner(lstH, lstW, i, thisW, rubbish)
                && canNoCorner(i, thisW, thisH, thisW, rubbish)) {
            lp.pt[1][0] = i; lp.pt[1][1] = thisW;
            lp.pt[2][0] = i; lp.pt[2][1] = lstW;
            return true;
        }
    }
    for(int i=0; i<=wb+1; i++) {
        if(i!=lstW && i!=thisW
                && !types[lstH][i] && !types[thisH][i]
                && canNoCorner(lstH, lstW, lstH, i, rubbish)
                && canOneCorner(lstH, i, thisH, thisW, rubbish)) {
            lp.pt[1][0] = lstH; lp.pt[1][1] = i;
            lp.pt[2][0] = thisH; lp.pt[2][1] = i;
            return true;
        }
        if(i!=thisW && i!=lstW
                && !types[thisH][i] && !types[lstH][i]
                && canOneCorner(lstH, lstW, thisH, i, rubbish)
                && canNoCorner(thisH, i, thisH, thisW, rubbish)) {
            lp.pt[1][0] = thisH; lp.pt[1][1] = i;
            lp.pt[2][0] = lstH; lp.pt[2][1] = i;
            return true;
        }
    }
    return false;
}

void MainWindow::stageClear()
{
    lastClickedH = lastClickedW = 0;
    if(types==nullptr || btns==nullptr)
        return ;
    for(int i=0; i<=hb+1; i++) {
        if(btns[i]!=nullptr) {
            for(int j=0; j<=wb+1; j++) {
                if(btns[i][j]!=nullptr)
                    delete btns[i][j];
            }
            delete btns[i];
        }
        if(types[i]!=nullptr)   delete types[i];
    }
    delete types;
    delete btns;
    isStarted = false;
    if(pTimer!=nullptr) delete pTimer;
    if(pTime!=nullptr) delete pTime;
    ui->progressBar->setValue(0);
    ui->lcdNumber->display(0);
}

int MainWindow::getDirection(int lstH, int lstW, int thisH,
                             int thisW, int midH, int midW)
{
    Q_UNUSED(midW)
    if(lstH>thisH) {
        int t=lstH; lstH = thisH;   thisH = t;
        t = lstW;   lstW = thisW;   thisW = t;
    }
    if(lstW<thisW) {
        if(midH==lstH)  return 2;
        else    return 0;
    }
    else {
        if(midH==lstH)  return 1;
        else    return 3;
    }
}

void MainWindow::drawALine(int lstH, int lstW, int thisH,
                           int thisW, bool isW)
{
    if(lstW==thisW) {
        int minH=lstH<thisH?lstH:thisH;
        int maxH=lstH+thisH-minH;
        for(int i=minH+1; i<=maxH-1; i++) {
            btns[i][lstW]->setIcon(isW?icons[0]:imageLine[0]);
            ui->gridLayout->addWidget(btns[i][lstW], i, lstW,
                                      1, 1,
                                      Qt::AlignCenter | Qt::AlignHCenter);
        }
    }
    else {
        int minW=lstW<thisW?lstW:thisW;
        int maxW=lstW+thisW-minW;
        for(int i=minW+1; i<=maxW-1; i++) {
            btns[lstH][i]->setIcon(isW?icons[0]:imageLine[1]);
            ui->gridLayout->addWidget(btns[lstH][i], lstH, i,
                                      1, 1,
                                      Qt::AlignCenter | Qt::AlignHCenter);
        }
    }
}

void MainWindow::drawLines(MainWindow::LinkPoints lp)
{
    //画线连接
    for(int i=0; i<=lp.cnt; i++)
        drawALine(lp.pt[i][0], lp.pt[i][1],
                lp.pt[i+1][0], lp.pt[i+1][1], false);
    for(int i=1; i<=lp.cnt; i++) {
        btns[lp.pt[i][0]][lp.pt[i][1]]->setIcon(imageTurn[
                getDirection(lp.pt[i-1][0],
                lp.pt[i-1][1],
                lp.pt[i+1][0], lp.pt[i+1][1],
                lp.pt[i][0], lp.pt[i][1])]);
        ui->gridLayout->addWidget(btns[lp.pt[i][0]][lp.pt[i][1]],
                lp.pt[i][0], lp.pt[i][1],
                1, 1,
                Qt::AlignCenter | Qt::AlignHCenter);
    }

    //等待0.2s
    QTest::qWait(200);

    //取消画线（画白块）
    for(int i=0; i<=lp.cnt; i++)
        drawALine(lp.pt[i][0], lp.pt[i][1],
                lp.pt[i+1][0], lp.pt[i+1][1], true);
    for(int i=1; i<=lp.cnt; i++) {
        btns[lp.pt[i][0]][lp.pt[i][1]]->setIcon(icons[0]);
        ui->gridLayout->addWidget(btns[lp.pt[i][0]][lp.pt[i][1]],
                lp.pt[i][0], lp.pt[i][1],
                1, 1,
                Qt::AlignCenter | Qt::AlignHCenter);
    }
}

void MainWindow::on_actNewGame_triggered()
{
    stageClear();
    getDifficulty();
    setBlocks();
}

void MainWindow::on_actAbout_triggered()
{
    QString strInfo = "poorpool 开发的连连看小游戏\n使用Qt开发";
    QMessageBox::about(this, "关于", strInfo);
}
