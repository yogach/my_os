#include <QCoreApplication>
#include <QList>
#include <iostream>
#include <ctime>
#include <QQueue>

using namespace std;

#define PAGE_NUM  (0xFF + 1)
#define FRAME_NUM (0x04)
#define FP_NONE   (-1)

struct FrameItem
{
    int pid;    // 哪个任务使用了这个页框
    int pnum;   //

    FrameItem()
    {
        pid = FP_NONE;
        pnum = FP_NONE;
    }
};

class PageTable
{
    int m_pt[PAGE_NUM];
public:
    PageTable()
    {
        for(int i=0; i<PAGE_NUM; i++)
        {
            m_pt[i] = FP_NONE;
        }
    }

    int& operator[] (int i)
    {
        if( (0 <= i) && (i < length()) )
        {
            return m_pt[i];
        }
        else
        {
            QCoreApplication::exit(-1);
        }
    }

    int length()
    {
        return PAGE_NUM;
    }

};

class PCB
{
    int m_pid;      //任务id
    PageTable m_pageTable; //任务对应的页表
    int* m_pageSerial;     //模拟任务访问页表的序列
    int m_pageSerialCount; //访问页表次数
    int m_next;            //下一个要访问的页表
public:
    PCB(int pid)
    {
        m_pid = pid;
        m_pageSerialCount = qrand() % 5 + 5;
        m_pageSerial = new int[m_pageSerialCount];

        for(int i=0; i<m_pageSerialCount; i++)
        {
            m_pageSerial[i] = qrand() % 8;
        }

        m_next = 0;
    }

    int getPID()
    {
        return m_pid;
    }

    PageTable& getPageTable()
    {
        return m_pageTable;
    }

    int getNextPage()
    {
        int ret = m_next++;

        if( ret < m_pageSerialCount)
        {
            ret = m_pageSerial[ret];
        }
        else
        {
            ret = FP_NONE;
        }

        return ret;
    }

    bool running()
    {
        return (m_next < m_pageSerialCount);
    }

    void printPageSerial()
    {
        QString s = "";

        for(int i=0; i<m_pageSerialCount; i++)
        {
            s += QString::number(m_pageSerial[i]) + " ";
        }

        cout << ("Task" + QString::number(m_pid) + " : " + s).toStdString() << endl;
    }

    ~PCB()
    {
        delete[] m_pageSerial;
    }
};

FrameItem FrameTable[FRAME_NUM];  //全局页框表
QList<PCB*> TaskTable;       //任务列表
QQueue<int> MoveOut;

int GetFrameItem();          //获取空闲的页框表项
void AccessPage(PCB& pcb);   //模仿任务访问页面
int RequestPage(int pid, int page); //请求页框
int SwapPage();              //交换页框
void ClearFrameItem(PCB& pcb);
void ClearFrameItem(int frame);
int Random();
int FIFO();
int LRU();
void PrintLog(QString log);
void PrintPageMap(int pid, int page, int frame);
void PrintFatalError(QString s, int pid, int page);

int GetFrameItem()
{
    int ret = FP_NONE;

    for(int i=0; i<FRAME_NUM; i++)
    {
        //找到空闲的页框
        if( FrameTable[i].pid == FP_NONE )
        {
            ret = i;
            break;
        }
    }

    return ret;
}

//模拟页面访问
void AccessPage(PCB& pcb)
{
    int pid = pcb.getPID();
    PageTable& pageTable = pcb.getPageTable();
    int page = pcb.getNextPage();

    if( page != FP_NONE )
    {
        PrintLog("Access Task" + QString::number(pid) + " for Page" + QString::number(page));

        //任务页表的对应项不为空 代表已经存在页映射
        if( pageTable[page] != FP_NONE )
        {
            PrintLog("Find target page in page table.");
            PrintPageMap(pid, page, pageTable[page]);
        }
        else
        {
            //任务页表的对应项为空 需要进行页请求
            PrintLog("Target page is NOT found, need to request page ...");

            pageTable[page] = RequestPage(pid, page);

            if( pageTable[page] != FP_NONE )
            {
                PrintPageMap(pid, page, pageTable[page]);
            }
            else
            {
                PrintFatalError("Can NOT request page from disk...", pid, page);
            }

        }
    }
    else
    {
        //任务已结束
        PrintLog("Task" + QString::number(pid) + " is finished!");
    }
}

int RequestPage(int pid, int page)
{
    int frame = GetFrameItem(); //获取空闲的页框

    if( frame != FP_NONE )
    {
        //获取到空闲的页框
        PrintLog("Get a frame to hold page content: Frame" + QString::number(frame));
    }
    else
    {
        //没有获取到空闲的页框
        PrintLog("No free frame to allocate, need to swap page out.");

        frame = SwapPage(); //执行页交换

        if( frame != FP_NONE )
        {
            PrintLog("Succeed to swap lazy page out.");
        }
        else
        {
            PrintFatalError("Failed to swap page out.", pid, FP_NONE);
        }

    }

    PrintLog("Load content from disk to Frame" + QString::number(frame));

    //记录使用页框的任务和对应的页表
    FrameTable[frame].pid = pid;
    FrameTable[frame].pnum = page;

    MoveOut.enqueue(frame); //申请页面成功后 将其加入队列

    return frame;
}

//在一个任务结束后 释放页框表与之有关的东西
void ClearFrameItem(PCB& pcb)
{
    for(int i=0; i<FRAME_NUM; i++)
    {
        if( FrameTable[i].pid == pcb.getPID() )
        {
            FrameTable[i].pid = FP_NONE;
            FrameTable[i].pnum = FP_NONE;
        }
    }

}

void ClearFrameItem(int frame)
{
    //清空页框
    FrameTable[frame].pid = FP_NONE;
    FrameTable[frame].pnum = FP_NONE;

    //查找每个任务内的页表中是否有对应页框 如果有清空
    for(int i=0, f=0; (i<TaskTable.count()) && !f; i++)
    {
        PageTable& pt = TaskTable[i]->getPageTable();

        for(int j=0; j<pt.length(); j++)
        {
            if( pt[j] == frame )
            {
                pt[j] = FP_NONE;
                f = 1;
                break;
            }
        }
    }
}

int Random()
{
    //按简单的做 直接随机选择一个页框
    int obj = qrand() % FRAME_NUM;

    PrintLog("Random select a frame to swap page content out: Frame" + QString::number(obj));

    //实际上选择一个要交换的页框后 需要将数据写回到硬盘
    PrintLog("Write the selected page content back to disk.");

    ClearFrameItem(obj);

    return obj;
}

int FIFO()
{
    int obj = MoveOut.dequeue();

    PrintLog("Select a frame to swap page content out: Frame" + QString::number(obj));
    PrintLog("Write the selected page content back to disk.");

    ClearFrameItem(obj);

    return obj;
}



int SwapPage()
{
    return FIFO();
}

void PrintLog(QString log)
{
    cout << log.toStdString() << endl;
}

void PrintPageMap(int pid, int page, int frame)
{
    QString s = "Task" + QString::number(pid) + " : ";

    s += "Page" + QString::number(page) + " ==> Frame" + QString::number(frame);

    cout << s.toStdString() << endl;
}

void PrintFatalError(QString s, int pid, int page)
{
    s += " Task" + QString::number(pid) + ": Page" + QString::number(page);

    cout << s.toStdString() << endl;

    QCoreApplication::exit(-2);
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    int index = 0;

    qsrand(time(NULL));

    TaskTable.append(new PCB(1));
    TaskTable.append(new PCB(2));

    PrintLog("Task Page Serial:");

    for(int i=0; i<TaskTable.count(); i++)
    {
        TaskTable[i]->printPageSerial();
    }

    PrintLog("==== Running ====");

    while( true )
    {
        if( TaskTable.count() > 0 )
        {
            if( TaskTable[index]->running() )
            {
                AccessPage(*TaskTable[index]);
            }
            else
            {
                PrintLog("Task" + QString::number(TaskTable[index]->getPID()) + " is finished!");

                PCB* pcb = TaskTable[index];

                TaskTable.removeAt(index);

                ClearFrameItem(*pcb);

                delete pcb;
            }
        }

        if( TaskTable.count() > 0 )
        {
            index = (index + 1) % TaskTable.count();
        }

        cin.get(); //等待键盘的空格输入
    }


    return a.exec();
}
