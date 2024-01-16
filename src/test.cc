//#include <gtest/gtest.h>
//#include "Mgard300_Handler.h"
//#include <thread>
//#define DET_STATE_SLEEP 0x1b
//class Mgard300_HandlerTest : public ::testing::Test {
//protected:
//    sockpp::tcp_acceptor acceptor;
//    Mgard300_Handler handler;
//
//    Mgard300_HandlerTest() : handler(acceptor) {}
//
//
//};
//
//TEST_F(Mgard300_HandlerTest, IncrementNumberConnection) {
//    handler.increment_number_connection();
//    EXPECT_EQ(handler.get_number_connection(), 1);
//}
//
//TEST_F(Mgard300_HandlerTest, DecrementNumberConnection) {
//    handler.increment_number_connection();
//    handler.decrement_number_connection();
//    EXPECT_EQ(handler.get_number_connection(), 0);
//}
//
//TEST_F(Mgard300_HandlerTest, IncrementDecrementNumberConnectionMultithreaded) {
//    // Mô phỏng việc sử dụng nhiều luồng để tăng và giảm số kết nối
//    std::thread t1([&]() {
//        handler.increment_number_connection();
//        handler.increment_number_connection();
//    });
//
//    std::thread t2([&]() {
//        handler.decrement_number_connection();
//        handler.decrement_number_connection();
//    });
//
//    t1.join();
//    t2.join();
//
//    EXPECT_EQ(handler.get_number_connection(), 0);
//}
//TEST_F(Mgard300_HandlerTest, ParseMsgClient) {
//	unsigned char validMsg[] = {0xAA, DET_STATE_SLEEP, 0x00, 0x00, 0x55, 0x00};
//}
////TEST_F(Mgard300_HandlerTest, )
//
//
//
