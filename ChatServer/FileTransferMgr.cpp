#include "FileTransferMgr.h"
#include <boost/filesystem.hpp>
#include <stdexcept>

namespace fs = boost::filesystem;

FileTransferMgr& FileTransferMgr::Inst()
{
    static FileTransferMgr ins;
    return ins;
}

FileTransferMgr::FileTransferMgr()
{
    fs::create_directories("./store");     // 确保目录存在
}

auto FileTransferMgr::createOrGet(
    const std::string& id,
    const std::string& path,
    int64_t total) -> std::shared_ptr<TransferCtx>
{
    std::lock_guard<std::mutex> lk(_mtx);

    auto it = _ctx_map.find(id);
    if (it != _ctx_map.end())
        return it->second;

    auto ctx = std::make_shared<TransferCtx>();
    ctx->file_id = id;
    ctx->file_path = path;
    ctx->total_size = total;
    ctx->ofs.open(path, std::ios::binary | std::ios::app);
    if (!ctx->ofs.is_open())
        throw std::runtime_error("open file failed: " + path);

    ctx->written = static_cast<int64_t>(ctx->ofs.tellp());
    _ctx_map[id] = ctx;
    return ctx;
}

int64_t FileTransferMgr::append(
    const std::string& id,
    int64_t offset,
    const char* buf,
    size_t len)
{
    auto ctx = createOrGet(id, "./store/" + id + ".tmp", 0);
    if (offset != ctx->written)
        throw std::runtime_error("offset mismatch");

    ctx->ofs.write(buf, static_cast<std::streamsize>(len));
    ctx->written += static_cast<int64_t>(len);
    return ctx->written;
}

int64_t FileTransferMgr::query(const std::string& id)
{
    std::lock_guard<std::mutex> lk(_mtx);
    auto it = _ctx_map.find(id);
    return it == _ctx_map.end() ? 0 : it->second->written.load();
}

void FileTransferMgr::finish(const std::string& id)
{
    std::lock_guard<std::mutex> lk(_mtx);
    auto it = _ctx_map.find(id);
    if (it == _ctx_map.end()) return;

    it->second->ofs.close();
    fs::rename(it->second->file_path, "./store/" + id);   // 去掉 .tmp
    _ctx_map.erase(it);
}
