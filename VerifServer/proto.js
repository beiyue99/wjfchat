

// 导入 Node.js 内置的 'path' 模块，用于处理和转换文件路径
const path = require('path');

// 导入 gRPC 模块，使用 '@grpc/grpc-js' 实现 gRPC 服务
const grpc = require('@grpc/grpc-js');

// 导入 'proto-loader' 模块，用于加载 .proto 文件并生成定义
const protoLoader = require('@grpc/proto-loader');

// 定义 .proto 文件的路径，使用 path 模块的 join 方法来兼容不同系统的路径格式
const PROTO_PATH = path.join(__dirname, 'message.proto');

// 使用 protoLoader 加载 .proto 文件
// 'loadSync' 方法用于同步加载 .proto 文件，并将其转换为 JavaScript 代码可使用的对象
// 其中的选项 'keepCase' 保持字段名称不变，'longs' 和 'enums' 的转换规则设置为字符串，
// 'defaults' 表示为所有消息类型设置默认值，'oneofs' 启用 oneof 字段支持
const packageDefinition = protoLoader.loadSync(PROTO_PATH, {
    keepCase: true, // 保持 proto 文件中的字段名称不变
    longs: String,  // 将 'long' 类型转换为字符串
    enums: String,  // 将枚举类型转换为字符串
    defaults: true, // 设置 proto 文件中消息的默认值
    oneofs: true    // 启用 oneof 字段的支持
});

// 将解析后的 packageDefinition 转换为可用于 gRPC 的 protoDescriptor
// 使用 gRPC 提供的 'loadPackageDefinition' 方法来加载 proto 文件定义并生成对象
const protoDescriptor = grpc.loadPackageDefinition(packageDefinition);

// 访问 protoDescriptor 中定义的 'message' 服务/消息对象
// 'message' 是从 proto 文件中获取的服务或消息定义
const message_proto = protoDescriptor.message;

// 导出 message_proto，以便在其他模块中可以导入并使用该服务/消息对象
module.exports = message_proto;
