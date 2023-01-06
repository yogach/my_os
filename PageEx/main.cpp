#include <QCoreApplication>
#include <QList>
#include <iostream>
#include <ctime>

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
QList<PCB*> TaskTable;   //任务列表

int GetFrameItem();  //获取空闲的页框表项
void AccessPage(PCB& pcb);   //模仿任务访问页面
int RequestPage(int pid, int page); //请求页框
int SwapPage();    //交换页框
void PrintLog(QString log);
void PrintPageMap(int pid, int page, int frame);
void PrintFatalError(QString s, int pid, int page);

int GetFrameItem()
{
    int ret = FP_NONE;

    for(int i=0; i<FRAME_NUM; i++)
    {
        //找到空闲的页框表项
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

}

int Random()
{

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

    return a.exec();
}
