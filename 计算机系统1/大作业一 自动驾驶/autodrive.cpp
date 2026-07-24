#include <iostream>
#include <string>

using namespace std;

// 传感器数据结构体
struct SensorData {
    float p[3];           // 视觉概率 [左,中,右]
    string move[3];       // 运动趋势："forward","backward","left","right","still"
    float d[3];           // 激光雷达距离 (米)
    bool is_raining;      // 是否下雨
    float speed;          // 车速 (km/h)
    bool is_fault;        // 故障标志
};

/**
 * 决策函数
 * sensors   输入：传感器数据
 * slow_down 输出：是否减速
 * stop      输出：是否停车
 */
void decide(const SensorData& sensors, bool& slow_down, bool& stop) {
    // ---------- 阈值 ----------
    const float PROB_THRESH = 0.6f;      // 视觉概率阈值
    const float DIST_MID_THRESH = 10.0f; // 正前方有物体距离阈值
    const float DIST_CLOSE = 5.0f;       // 正前方过近停车阈值
    const float DIST_SIDE_THRESH = 8.0f; // 侧前方有物体距离阈值
    const float HIGH_SPEED = 80.0f;      // 高速阈值 (km/h)
    const float RAIN_SPEED = 30.0f;      // 雨天减速速度阈值

    // ---------- 中间量----------
    
    // 逻辑：正前方有物体 = 视觉概率高 或 激光雷达测距近
    bool obj_mid = (sensors.p[1] > PROB_THRESH) || (sensors.d[1] < DIST_MID_THRESH);
    // 逻辑：正前方物体向后移动（对向行驶）
    bool move_backward = (sensors.move[1] == "backward");
    // 逻辑：正前方距离过近
    bool close_mid = (sensors.d[1] < DIST_CLOSE);
    // 逻辑：左前方有物体
    bool obj_left = (sensors.p[0] > PROB_THRESH) || (sensors.d[0] < DIST_SIDE_THRESH);
    // 逻辑：左前方物体向右移动（横穿）
    bool move_right = (sensors.move[0] == "right");
    // 逻辑：右前方有物体
    bool obj_right = (sensors.p[2] > PROB_THRESH) || (sensors.d[2] < DIST_SIDE_THRESH);
    // 逻辑：右前方物体向左移动（横穿）
    bool move_left = (sensors.move[2] == "left");
    // 逻辑：是否下雨
    bool rain = sensors.is_raining;
    // 逻辑：是否高速行驶
    bool high_speed = (sensors.speed > HIGH_SPEED);
    // 逻辑：是否超过雨天限速阈值
    bool speed_gt30 = (sensors.speed > RAIN_SPEED);
    // 逻辑：故障标志
    bool fault = sensors.is_fault;

    // ---------- 输出逻辑表达式 ----------
    // 停车条件：故障 或 (正前方有物体且距离过近)
    stop = fault || (obj_mid && close_mid);

    // 减速条件：未停车 且 (五个条件之一成立)
    // 条件1: 正前方物体向后移动（对向行驶）
    // 条件2: 正前方有物体且车速过高（>80km/h）
    // 条件3: 下雨且车速>30km/h
    // 条件4: 左前方有物体且该物体向右移动
    // 条件5: 右前方有物体且该物体向左移动
    if (!stop) {
        bool cond1 = obj_mid && move_backward;
        bool cond2 = obj_mid && high_speed;
        bool cond3 = rain && speed_gt30;
        bool cond4 = obj_left && move_right;
        bool cond5 = obj_right && move_left;
        slow_down = cond1 || cond2 || cond3 || cond4 || cond5;
    } else {
        slow_down = false;
    }
}

// ---------- 主函数 ----------
int main() {
    // 测试1：正常行驶
    SensorData s1 = {
        {0.2f, 0.1f, 0.1f},
        {"still", "still", "still"},
        {20.0f, 30.0f, 25.0f},
        false, 60.0f, false
    };
    bool slow, stop;
    decide(s1, slow, stop);
    cout << "测试1 正常行驶: slow_down=" << slow << ", stop=" << stop << endl;

    // 测试2：正前方对向行驶
    SensorData s2 = {
        {0.2f, 0.9f, 0.1f},
        {"still", "backward", "still"},
        {20.0f, 15.0f, 25.0f},
        false, 60.0f, false
    };
    decide(s2, slow, stop);
    cout << "测试2 对向行驶: slow_down=" << slow << ", stop=" << stop << endl;

    // 测试3：正前方距离过近（4米）
    SensorData s3 = {
        {0.2f, 0.9f, 0.1f},
        {"still", "still", "still"},
        {20.0f, 4.0f, 25.0f},
        false, 40.0f, false
    };
    decide(s3, slow, stop);
    cout << "测试3 前方过近: slow_down=" << slow << ", stop=" << stop << endl;

    // 测试4：车辆故障
    SensorData s4 = {
        {0.2f, 0.1f, 0.1f},
        {"still", "still", "still"},
        {20.0f, 30.0f, 25.0f},
        false, 60.0f, true
    };
    decide(s4, slow, stop);
    cout << "测试4 故障: slow_down=" << slow << ", stop=" << stop << endl;

    // 测试5：下雨且车速90km/h
    SensorData s5 = {
        {0.2f, 0.1f, 0.1f},
        {"still", "still", "still"},
        {20.0f, 30.0f, 25.0f},
        true, 90.0f, false
    };
    decide(s5, slow, stop);
    cout << "测试5 下雨且车速90km/h: slow_down=" << slow << ", stop=" << stop << endl;

    // 测试6：左前方有物体向右移动（横穿）
    SensorData s6 = {
        {0.8f, 0.1f, 0.1f},
        {"right", "still", "still"},
        {9.0f, 20.0f, 20.0f},
        false, 50.0f, false
    };
    decide(s6, slow, stop);
    cout << "测试6 左前方横穿: slow_down=" << slow << ", stop=" << stop << endl;

    return 0;
}