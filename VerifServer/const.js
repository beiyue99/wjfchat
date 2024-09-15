// 定义一个前缀字符串，用于标识某些代码（例如 Redis 存储键前缀）
let code_prefix = "code_";

// 定义一个错误代码对象，包含三个不同的错误类型
const Errors = {
    Success : 0,   // 成功，返回代码 0
    RedisErr : 1,  // Redis 操作错误，返回代码 1
    Exception : 2, // 异常错误，返回代码 2
};

// 导出 code_prefix 和 Errors 以便在其他文件中使用
module.exports = { code_prefix, Errors };
