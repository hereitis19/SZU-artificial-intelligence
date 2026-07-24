/**
 * 自适应缓存替换策略模拟器（C++实现）
 * 支持 FIFO、LRU、LFU 三种替换算法
 * 自适应：用前10次访问测试，选择命中率最高的策略（平局时 LRU > LFU > FIFO）
 * 缓存容量固定为4块
 * 
 * 编译：g++ -std=c++11 -o cache_sim cache_sim.cpp
 */

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <iomanip>

using namespace std;

// -------------------------- 日志结构 --------------------------
struct LogEntry {
    int step;           // 步数
    int address;        // 访问地址
    bool hit;           // 是否命中
    vector<int> cache;  // 当前缓存内容（-1表示空）
};

// -------------------------- 基类：缓存模拟器 --------------------------
class CacheSimulator {
protected:
    int capacity;               // 缓存容量（固定为4）
    vector<int> cache;          // 当前缓存的地址，未满时用-1表示空位
    int hit_count;              // 命中次数
    int miss_count;             // 缺失次数
    vector<LogEntry> log;       // 每一步的日志

public:
    CacheSimulator(int cap = 4) : capacity(cap), hit_count(0), miss_count(0) {
        cache.resize(capacity, -1);  // 初始全空
    }

    virtual ~CacheSimulator() {}

    // 处理一次地址访问（纯虚函数，子类实现具体替换逻辑）
    void access(int address, int step) {
        // 查找是否命中
        int idx = findInCache(address);
        bool is_hit = (idx != -1);
        
        if (is_hit) {
            hit_count++;
            hitUpdate(idx, address);   // 命中时更新策略相关数据
        } else {
            miss_count++;
            // 找第一个空位（未满）
            int empty_idx = findEmpty();
            if (empty_idx != -1) {
                // 缓存未满，直接放入空位
                cache[empty_idx] = address;
                addNew(empty_idx, address);
            } else {
                // 缓存已满，需要替换
                int victim = chooseVictim();
                cache[victim] = address;
                replace(victim, address);
            }
        }
        
        // 记录日志
        LogEntry entry;
        entry.step = step;
        entry.address = address;
        entry.hit = is_hit;
        entry.cache = cache;
        log.push_back(entry);
    }

    // 获取统计信息
    void getStats(int &total, int &hits, int &misses, double &hitRate) const {
        total = hit_count + miss_count;
        hits = hit_count;
        misses = miss_count;
        hitRate = (total > 0) ? (double)hit_count / total : 0.0;
    }

    // 获取日志
    const vector<LogEntry>& getLog() const { return log; }

    // 重置模拟器状态（用于自适应测试）
    void reset() {
        fill(cache.begin(), cache.end(), -1);
        hit_count = 0;
        miss_count = 0;
        log.clear();
        resetExtra();
    }

    // 获取当前命中率（用于自适应选择）
    double getHitRate() const {
        int total = hit_count + miss_count;
        return (total > 0) ? (double)hit_count / total : 0.0;
    }

protected:
    // 在缓存中查找地址，返回索引，未找到返回-1
    int findInCache(int address) const {
        for (int i = 0; i < capacity; ++i) {
            if (cache[i] == address) return i;
        }
        return -1;
    }

    // 查找第一个空位（值为-1），返回索引，无空位返回-1
    int findEmpty() const {
        for (int i = 0; i < capacity; ++i) {
            if (cache[i] == -1) return i;
        }
        return -1;
    }

    // 虚函数：子类实现
    virtual void hitUpdate(int idx, int address) = 0;
    virtual void addNew(int idx, int address) = 0;
    virtual int chooseVictim() = 0;
    virtual void replace(int idx, int address) = 0;
    virtual void resetExtra() = 0;
};

// -------------------------- FIFO 策略 --------------------------
class FIFOSimulator : public CacheSimulator {
private:
    vector<int> insertOrder;   // 每个索引对应的插入顺序（序号）
    int nextOrder;             // 下一个可用的序号

public:
    FIFOSimulator(int cap = 4) : CacheSimulator(cap), nextOrder(0) {
        insertOrder.resize(capacity, -1);
    }

protected:
    void hitUpdate(int idx, int address) override {
        // FIFO 命中不改变任何顺序
    }

    void addNew(int idx, int address) override {
        insertOrder[idx] = nextOrder++;
    }

    int chooseVictim() override {
        // 找插入顺序最小的块（最早进入）
        int minOrder = insertOrder[0];
        int victim = 0;
        for (int i = 1; i < capacity; ++i) {
            if (insertOrder[i] < minOrder) {
                minOrder = insertOrder[i];
                victim = i;
            }
        }
        return victim;
    }

    void replace(int idx, int address) override {
        insertOrder[idx] = nextOrder++;
    }

    void resetExtra() override {
        fill(insertOrder.begin(), insertOrder.end(), -1);
        nextOrder = 0;
    }
};

// -------------------------- LRU 策略 --------------------------
class LRUSimulator : public CacheSimulator {
private:
    vector<int> lastAccessTime;   // 每个索引的最后访问时间戳
    int globalTime;               // 全局时间戳

public:
    LRUSimulator(int cap = 4) : CacheSimulator(cap), globalTime(0) {
        lastAccessTime.resize(capacity, -1);
    }

protected:
    void hitUpdate(int idx, int address) override {
        lastAccessTime[idx] = ++globalTime;
    }

    void addNew(int idx, int address) override {
        lastAccessTime[idx] = ++globalTime;
    }

    int chooseVictim() override {
        // 找最后访问时间最小的（最久未使用）
        int minTime = lastAccessTime[0];
        int victim = 0;
        for (int i = 1; i < capacity; ++i) {
            if (lastAccessTime[i] < minTime) {
                minTime = lastAccessTime[i];
                victim = i;
            }
        }
        return victim;
    }

    void replace(int idx, int address) override {
        lastAccessTime[idx] = ++globalTime;
    }

    void resetExtra() override {
        fill(lastAccessTime.begin(), lastAccessTime.end(), -1);
        globalTime = 0;
    }
};

// -------------------------- LFU 策略 --------------------------
class LFUSimulator : public CacheSimulator {
private:
    vector<int> freq;           // 每个索引的访问频率
    vector<int> insertOrder;    // 每个索引的插入顺序（用于频率相同时的FIFO）
    int nextOrder;

public:
    LFUSimulator(int cap = 4) : CacheSimulator(cap), nextOrder(0) {
        freq.resize(capacity, 0);
        insertOrder.resize(capacity, -1);
    }

protected:
    void hitUpdate(int idx, int address) override {
        freq[idx]++;   // 命中时增加频率
    }

    void addNew(int idx, int address) override {
        freq[idx] = 1;               // 新块初始访问次数为1（本次算一次）
        insertOrder[idx] = nextOrder++;
    }

    int chooseVictim() override {
        // 先找最小频率
        int minFreq = freq[0];
        for (int i = 1; i < capacity; ++i) {
            if (freq[i] < minFreq) minFreq = freq[i];
        }
        // 收集所有频率等于minFreq的候选
        vector<int> candidates;
        for (int i = 0; i < capacity; ++i) {
            if (freq[i] == minFreq) candidates.push_back(i);
        }
        if (candidates.size() == 1) return candidates[0];
        // 多个候选，选插入顺序最小的（FIFO）
        int minOrder = insertOrder[candidates[0]];
        int victim = candidates[0];
        for (size_t i = 1; i < candidates.size(); ++i) {
            int idx = candidates[i];
            if (insertOrder[idx] < minOrder) {
                minOrder = insertOrder[idx];
                victim = idx;
            }
        }
        return victim;
    }

    void replace(int idx, int address) override {
        freq[idx] = 1;                 // 新替换的块初始频率为1
        insertOrder[idx] = nextOrder++;
    }

    void resetExtra() override {
        fill(freq.begin(), freq.end(), 0);
        fill(insertOrder.begin(), insertOrder.end(), -1);
        nextOrder = 0;
    }
};

// -------------------------- 自适应策略选择 --------------------------
string chooseBestStrategy(const vector<int>& accessSeq, int testLen = 10) {
    // 实际测试长度不能超过序列长度
    int len = (int)accessSeq.size();
    if (testLen > len) testLen = len;
    
    vector<int> testSeq(accessSeq.begin(), accessSeq.begin() + testLen);
    
    FIFOSimulator fifo;
    LRUSimulator lru;
    LFUSimulator lfu;
    
    // 分别模拟前 testLen 次
    for (int addr : testSeq) {
        fifo.access(addr, 0);
        lru.access(addr, 0);
        lfu.access(addr, 0);
    }
    
    double rate_fifo = fifo.getHitRate();
    double rate_lru  = lru.getHitRate();
    double rate_lfu  = lfu.getHitRate();
    
    // 找出最高命中率
    double bestRate = max({rate_fifo, rate_lru, rate_lfu});
    
    // 按优先级 LRU > LFU > FIFO 选择
    if (rate_lru == bestRate) return "LRU";
    if (rate_lfu == bestRate) return "LFU";
    return "FIFO";
}

// -------------------------- 运行模拟并输出结果 --------------------------
void runAndPrint(const vector<int>& accessSeq, const string& caseName) {
    // 自适应选择最佳策略
    int testLen = min(10, (int)accessSeq.size());
    string bestStrategy = chooseBestStrategy(accessSeq, testLen);
    
    // 根据选中的策略创建模拟器
    CacheSimulator* sim = nullptr;
    if (bestStrategy == "FIFO") sim = new FIFOSimulator();
    else if (bestStrategy == "LRU") sim = new LRUSimulator();
    else sim = new LFUSimulator();
    
    // 运行完整序列
    for (size_t i = 0; i < accessSeq.size(); ++i) {
        sim->access(accessSeq[i], i+1);
    }
    
    // 输出标题
    cout << "\n========== " << caseName << " 使用策略: " << bestStrategy << " ==========\n";
    
    // 输出每一步
    const auto& log = sim->getLog();
    for (const auto& entry : log) {
        // 格式化缓存显示：将 -1 显示为 _
        cout << "步数 " << setw(2) << entry.step << " | 访问 " << setw(2) << entry.address << " | ";
        if (entry.hit) cout << "命中";
        else cout << "缺失";
        cout << " | 缓存 [";
        for (size_t i = 0; i < entry.cache.size(); ++i) {
            if (entry.cache[i] == -1) cout << "_";
            else cout << entry.cache[i];
            if (i != entry.cache.size()-1) cout << " ";
        }
        cout << "]\n";
    }
    
    // 输出统计
    int total, hits, misses;
    double hitRate;
    sim->getStats(total, hits, misses, hitRate);
    cout << "\n最终统计：\n";
    cout << "总访问次数: " << total << "\n";
    cout << "总命中次数: " << hits << "\n";
    cout << "总缺失次数: " << misses << "\n";
    cout << "整体命中率: " << fixed << setprecision(2) << hitRate * 100 << "%\n";
    
    delete sim;
}

// -------------------------- 主函数：测试三个用例 --------------------------
int main() {
    // 测试用例
    vector<vector<int>> testCases = {
        {1,2,3,2,1,2,3,4,1,2,3,4},                 // 用例1：局部性强
        {1,2,3,4,1,2,3,4,1,2,3,4},                 // 用例2：循环访问
        {1,1,2,2,1,1,2,2,3,3,3,4}                  // 用例3：高频重复
    };
    vector<string> caseNames = {"用例1（局部性强）", "用例2（循环访问）", "用例3（高频重复）"};
    
    for (size_t i = 0; i < testCases.size(); ++i) {
        runAndPrint(testCases[i], caseNames[i]);
    }
    
    return 0;
}