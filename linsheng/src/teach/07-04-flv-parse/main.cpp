// g++ main.cpp -g FlvMetaData.cpp FlvParser.cpp vadbg.cpp Videojj.cpp -o  main
// ./main dump.flv out.flv # 本代码利用解析得到的信息，重新封装得到一个新的flv文件。

#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <fstream>

#include "FlvParser.h"

using namespace std;

void Process(fstream &fin, const char *filename);

int main(int argc, char *argv[])
{
    cout << "Hi, this is FLV parser test program!\n";

    if (argc != 3)
    {
        cout << "FlvParser.exe [input flv] [output flv]" << endl;
        return 0;
    }

    fstream fin;
    fin.open(argv[1], ios_base::in | ios_base::binary);
    if (!fin)
        return 0;

    Process(fin, argv[2]);

    fin.close();

    return 1;
}

void Process(fstream &fin, const char *filename)
{
    CFlvParser parser;

    int nBufSize = 2*1024 * 1024; // 代表nBufSize个字节，即2MB。
    int nFlvPos = 0;              // 代表待处理数据为多少个字节
    uint8_t *pBuf, *pBak;
    pBuf = new uint8_t[nBufSize];
    pBak = new uint8_t[nBufSize];

    while (1) // 这里是在循环读什么数据？
    {
        int nReadNum = 0;   // 
        int nUsedLen = 0;   // 代表已处理数据为多少个字节
        fin.read((char *)pBuf + nFlvPos, nBufSize - nFlvPos); // 每次循环将 nBufSize - nFlvPos个字节的数据读入到pBuf + nFlvPos所在的位置。每次最多读取2MB
        nReadNum = fin.gcount();
        if (nReadNum == 0)
            break;
        nFlvPos += nReadNum;

        parser.Parse(pBuf, nFlvPos, nUsedLen);
        if (nFlvPos != nUsedLen) // 如果nFlvPos != nUsedLen，则代表还有 nFlvPos - nUsedLen个字节大小的数据没有被Parse处理
        {
            memcpy(pBak, pBuf + nUsedLen, nFlvPos - nUsedLen); // 将未处理的出数据暂存在pBak中
            memcpy(pBuf, pBak, nFlvPos - nUsedLen); // 将未处理的出数据存入pBuf中
        }
        nFlvPos -= nUsedLen; // 
    }
    parser.PrintInfo();
    parser.DumpH264("parser.264");
    parser.DumpAAC("parser.aac");

    //dump into flv
    parser.DumpFlv(filename);

    delete []pBak;
    delete []pBuf;
}
