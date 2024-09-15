// 引入 Node.js 的文件系统模块，用于读取文件
const fs = require('fs');

// 读取并解析配置文件 config.json，文件内容被读取为 UTF-8 编码格式
let config = JSON.parse(fs.readFileSync('config.json', 'utf8'));

// 从解析的配置中提取邮箱、MySQL 和 Redis 的配置项
let email_user = config.email.user;      // 邮箱地址
let email_pass = config.email.pass;      // 邮箱授权码
let mysql_host = config.mysql.host;      // MySQL 主机地址
let mysql_port = config.mysql.port;      // MySQL 端口
let redis_host = config.redis.host;      // Redis 主机地址
let redis_port = config.redis.port;      // Redis 端口
let redis_passwd = config.redis.passwd;  // Redis 密码

// 定义 Redis 存储验证码的键前缀
let code_prefix = "code_";

// 导出所有提取的配置信息，以便在其他模块中使用
module.exports = {
    email_pass,  // 邮箱授权码
    email_user,  // 邮箱地址
    mysql_host,  // MySQL 主机
    mysql_port,  // MySQL 端口
    redis_host,  // Redis 主机
    redis_port,  // Redis 端口
    redis_passwd,// Redis 密码
    code_prefix  // 存储验证码的键前缀
};
