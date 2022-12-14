#include "avpacket.h"

#define MEM_ITEM_SIZE (20 * 1024 * 102)
#define AVPACKET_LOOP_COUNT 1000
// 测试 内存泄漏
/**
 * @brief 测试av_packet_alloc和av_packet_free的配对使用
 */
void av_packet_test1()
{
    AVPacket *pkt = NULL;
    int ret = 0;

    pkt = av_packet_alloc();
    ret = av_new_packet(pkt, MEM_ITEM_SIZE); // 引用计数初始化为1
    memccpy(pkt->data, (void *)&av_packet_test1, 1, MEM_ITEM_SIZE);
    av_packet_unref(pkt); // 要不要调用。调用av_packet_free，就不需要调用av_packet_unref，因为av_packet_free内部调用了av_packet_unref。但是重复调用也没事，因为av_packet_unref中处理了重复调用的情况
    av_packet_free(&pkt); // 如果不free将会发生内存泄漏。
}

/**
 * @brief 测试误用av_init_packet将会导致内存泄漏
 */
void av_packet_test2()
{
    AVPacket *pkt = NULL;
    int ret = 0;

    pkt = av_packet_alloc();
    ret = av_new_packet(pkt, MEM_ITEM_SIZE);
    memccpy(pkt->data, (void *)&av_packet_test1, 1, MEM_ITEM_SIZE);
    //    av_init_packet(pkt);        // 这个时候init就会导致内存无法释放。为什么调用av_init_packet，就无法使用 av_packet_free释放内存??
                                        // 答：我觉得：av_init_packet(pkt)将pkt中的buf指向空，但pkt中buf指向的那一块内存还没释放，从而导致了内存泄漏
    av_packet_free(&pkt);
}

/**
 * @brief 测试av_packet_move_ref后，可以av_init_packet
 */
void av_packet_test3()
{
    AVPacket *pkt = NULL;
    AVPacket *pkt2 = NULL;
    int ret = 0;

    pkt = av_packet_alloc();
    ret = av_new_packet(pkt, MEM_ITEM_SIZE);
    memccpy(pkt->data, (void *)&av_packet_test1, 1, MEM_ITEM_SIZE);
    pkt2 = av_packet_alloc();      // 必须先alloc
    av_packet_move_ref(pkt2, pkt); //内部其实也调用了av_init_packet
    av_init_packet(pkt);
    av_packet_free(&pkt); // test2中无法在av_init_packet之后调用av_packet_free，这里为什么又可以了？？
                          // 答：这里pkt中buf指向的内存，由pkt2的buf接管，可以通过pkt2的buf对内存进行释放。【内存泄漏：有内存空间没有释放】
    av_packet_free(&pkt2);
}
/**
 * @brief 测试av_packet_clone
 */
void av_packet_test4()
{
    AVPacket *pkt = NULL;
    // av_packet_alloc()没有必要，因为av_packet_clone内部有调用 av_packet_alloc
    AVPacket *pkt2 = NULL;
    int ret = 0;

    pkt = av_packet_alloc();
    ret = av_new_packet(pkt, MEM_ITEM_SIZE);
    memccpy(pkt->data, (void *)&av_packet_test1, 1, MEM_ITEM_SIZE);
    pkt2 = av_packet_clone(pkt); // av_packet_alloc()+av_packet_ref()
    av_init_packet(pkt);
    av_packet_free(&pkt);
    av_packet_free(&pkt2);
}

/**
 * @brief 多次调用av_packet_ref导致的内存泄漏
 */
void av_packet_test5()
{
    AVPacket *pkt = NULL;
    AVPacket *pkt2 = NULL;
    int ret = 0;

    pkt = av_packet_alloc(); //
    if (pkt->buf)            // 打印referenc-counted，必须保证传入的是有效指针
    {
        printf("%s(%d) ref_count(pkt) = %d\n", __FUNCTION__, __LINE__,
               av_buffer_get_ref_count(pkt->buf));
    }

    ret = av_new_packet(pkt, MEM_ITEM_SIZE);
    if (pkt->buf) // 打印referenc-counted，必须保证传入的是有效指针
    {
        printf("%s(%d) ref_count(pkt) = %d\n", __FUNCTION__, __LINE__,
               av_buffer_get_ref_count(pkt->buf));
    }
    memccpy(pkt->data, (void *)&av_packet_test1, 1, MEM_ITEM_SIZE);

    pkt2 = av_packet_alloc();      // 必须先alloc
    av_packet_move_ref(pkt2, pkt); // av_packet_move_ref
                                   //    av_init_packet(pkt);  //av_packet_move_ref

    av_packet_ref(pkt, pkt2);
    av_packet_ref(pkt, pkt2); // 多次ref如果没有对应多次unref将会内存泄漏。现在此内存的引用计数为3
    if (pkt->buf)             // 打印referenc-counted，必须保证传入的是有效指针
    {
        printf("%s(%d) ref_count(pkt) = %d\n", __FUNCTION__, __LINE__,
               av_buffer_get_ref_count(pkt->buf));
    }
    if (pkt2->buf) // 打印referenc-counted，必须保证传入的是有效指针
    {
        printf("%s(%d) ref_count(pkt) = %d\n", __FUNCTION__, __LINE__,
               av_buffer_get_ref_count(pkt2->buf));
    }
    av_packet_unref(pkt); // 现在引用计数为2
    av_packet_unref(pkt); // 由于pkt的buf已经在前面使用了av_packet_unref(pkt)进行置空，所以再使用一次av_packet_unref(pkt)，将不起到任何作用。内存的引用次数还是2
    if (pkt->buf)
        printf("pkt->buf没有被置NULL\n");
    else
        printf("pkt->buf已经被置NULL\n");
    if (pkt2->buf) // 打印referenc-counted，必须保证传入的是有效指针
    {
        printf("%s(%d) ref_count(pkt) = %d\n", __FUNCTION__, __LINE__,
               av_buffer_get_ref_count(pkt2->buf)); // 为2
    }
    av_packet_unref(pkt2);  // 内存的引用次数为1。由于内存的引用次数无法降低到零，从而导致内存无法释放，从而导致内存泄漏

    av_packet_free(&pkt);  
    av_packet_free(&pkt2);
}

/**
 * @brief 测试AVPacket整个结构体赋值, 和av_packet_move_ref类似
 */
void av_packet_test6()
{
    AVPacket *pkt = NULL;
    AVPacket *pkt2 = NULL;
    int ret = 0;

    pkt = av_packet_alloc();
    ret = av_new_packet(pkt, MEM_ITEM_SIZE);
    memccpy(pkt->data, (void *)&av_packet_test1, 1, MEM_ITEM_SIZE);

    pkt2 = av_packet_alloc(); // 必须先alloc
    *pkt2 = *pkt;             // 有点类似  pkt可以重新分配内存
    av_init_packet(pkt);

    av_packet_free(&pkt);
    av_packet_free(&pkt2);
}

void av_packet_test()
{
    av_packet_test1();
    //    av_packet_test2();
    //    av_packet_test3();
    //    av_packet_test4();
    //    av_packet_test5();
    // av_packet_test6();
}
