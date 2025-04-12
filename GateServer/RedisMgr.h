#pragma once
#include "const.h"
#include "hiredis.h"
#include <queue>
#include <atomic>
#include <mutex>
#include "Singleton.h"




class RedisConPool {
public:
    // 构造函数：初始化连接池，创建指定数量的 Redis 连接并验证身份，同时启动定时健康检查线程
    RedisConPool(size_t poolSize, const char* host, int port, const char* pwd)
        : poolSize_(poolSize), host_(host), port_(port), b_stop_(false), pwd_(pwd), counter_(0) {
        for (size_t i = 0; i < poolSize_; ++i) {
            auto* context = redisConnect(host, port);
            if (context == nullptr || context->err != 0) {
                if (context != nullptr) redisFree(context);
                continue;
            }

            auto reply = (redisReply*)redisCommand(context, "AUTH %s", pwd);
            if (reply->type == REDIS_REPLY_ERROR) {
                std::cout << "认证失败" << std::endl;
                freeReplyObject(reply);
                continue;
            }

            freeReplyObject(reply);
            connections_.push(context);
        }

        // 每 1 秒计数一次，累计 60 次后检查连接健康
        check_thread_ = std::thread([this]() {
            while (!b_stop_) {
                counter_++;
                if (counter_ >= 60) {
                    checkThread();
                    counter_ = 0;
                }
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            });
    }

    // 析构函数：清理资源
    ~RedisConPool() {}

    // 清空连接池中所有连接
    void ClearConnections() {
        std::lock_guard<std::mutex> lock(mutex_);
        while (!connections_.empty()) {
            auto* context = connections_.front();
            redisFree(context);
            connections_.pop();
        }
    }

    // 获取一个连接，如果连接池为空则等待
    redisContext* getConnection() {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this] {
            if (b_stop_) return true;
            return !connections_.empty();
            });
        if (b_stop_) return nullptr;
        auto* context = connections_.front();
        connections_.pop();
        return context;
    }

    // 将连接归还到连接池
    void returnConnection(redisContext* context) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (b_stop_) return;
        connections_.push(context);
        cond_.notify_one();
    }

    // 关闭连接池并终止健康检查线程
    void Close() {
        b_stop_ = true;
        cond_.notify_all();
        check_thread_.join();
    }

private:
    // 定期检查连接是否正常，异常则重新连接并认证
    void checkThread() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (b_stop_) return;
        auto pool_size = connections_.size();
        for (int i = 0; i < pool_size && !b_stop_; i++) {
            auto* context = connections_.front();
            connections_.pop();
            try {
                auto reply = (redisReply*)redisCommand(context, "PING");
                if (!reply) {
                    std::cout << "reply is null, redis ping failed" << std::endl;
                    connections_.push(context);
                    continue;
                }
                freeReplyObject(reply);
                connections_.push(context);
            }
            catch (std::exception& exp) {
                std::cout << "Error keeping connection alive: " << exp.what() << std::endl;
                redisFree(context);
                context = redisConnect(host_, port_);
                if (context == nullptr || context->err != 0) {
                    if (context != nullptr) redisFree(context);
                    continue;
                }

                auto reply = (redisReply*)redisCommand(context, "AUTH %s", pwd_);
                if (reply->type == REDIS_REPLY_ERROR) {
                    std::cout << "认证失败" << std::endl;
                    freeReplyObject(reply);
                    continue;
                }

                freeReplyObject(reply);
                std::cout << "认证成功" << std::endl;
                connections_.push(context);
            }
        }
    }

    std::atomic<bool> b_stop_;
    size_t poolSize_;
    const char* host_;
    const char* pwd_;
    int port_;
    std::queue<redisContext*> connections_;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::thread check_thread_;
    int counter_;
};




class RedisMgr : public Singleton<RedisMgr>,
    public std::enable_shared_from_this<RedisMgr> {
    friend class Singleton<RedisMgr>;

public:
    ~RedisMgr();

    // 获取指定 key 的值
    bool Get(const std::string& key, std::string& value);

    // 设置 key 的值
    bool Set(const std::string& key, const std::string& value);

    // 向列表头部插入元素
    bool LPush(const std::string& key, const std::string& value);

    // 弹出列表头部元素
    bool LPop(const std::string& key, std::string& value);

    // 向列表尾部插入元素
    bool RPush(const std::string& key, const std::string& value);

    // 弹出列表尾部元素
    bool RPop(const std::string& key, std::string& value);

    // 设置哈希表字段的值
    bool HSet(const std::string& key, const std::string& hkey, const std::string& value);

    // 设置哈希表字段的值（带长度）
    bool HSet(const char* key, const char* hkey, const char* hvalue, size_t hvaluelen);

    // 获取哈希表字段的值
    std::string HGet(const std::string& key, const std::string& hkey);

    // 删除哈希表字段
    bool HDel(const std::string& key, const std::string& field);

    // 删除指定 key
    bool Del(const std::string& key);

    // 判断 key 是否存在
    bool ExistsKey(const std::string& key);

    // 关闭连接池
    void Close() {
        _con_pool->Close();
        _con_pool->ClearConnections();
    }

private:
    // 构造函数：初始化 Redis 连接池
    RedisMgr();
    std::unique_ptr<RedisConPool>  _con_pool;
};
