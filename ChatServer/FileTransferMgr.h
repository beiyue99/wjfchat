#pragma once
#include <unordered_map>
#include <memory>
#include <fstream>
#include <mutex>
#include <atomic>
#include <string>

/**
 * 负责单机磁盘落盘 + 断点续传状态维护
 * 使用 createOrGet → append → finish 三步
 */
class FileTransferMgr
{
public:
    struct TransferCtx {
        std::string  file_id;               // uuid
        std::string  file_path;             // ./store/<file_id>.tmp
        std::ofstream ofs;                  // 追加模式
        std::atomic<int64_t> written{ 0 };    // 已写入
        int64_t       total_size = 0;       // 总大小
    };

    /** 单例 */
    static FileTransferMgr& Inst();

    /** 如已存在则复用 */
    std::shared_ptr<TransferCtx>
        createOrGet(const std::string& file_id,
            const std::string& abs_path,
            int64_t total_size);

    /** 追加分片。返回“写完后的累计字节数”；offset 不一致则抛异常 */
    int64_t append(const std::string& file_id,
        int64_t offset,
        const char* data,
        size_t len);

    /** 查询已写入 */
    int64_t query(const std::string& file_id);

    /** 完成写入并把 .tmp → 正式文件名 */
    void finish(const std::string& file_id);

private:
    FileTransferMgr();                                    // ctor 私有

    std::mutex _mtx;
    std::unordered_map<std::string,
        std::shared_ptr<TransferCtx>> _ctx_map;
};
