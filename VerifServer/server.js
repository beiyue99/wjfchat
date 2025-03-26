const grpc = require('@grpc/grpc-js')
const message_proto = require("./proto")
const const_module = require('./const')
const { v4: uuidv4 } = require('uuid');
const emailModule = require('./email')
const redis_module = require('./redis')

// 定义异步函数 GetVarifyCode，用于处理客户端的 gRPC 请求
// 'call' 参数包含请求信息，'callback' 用于响应客户端
async function GetVarifyCode(call, callback) {
    console.log("email is ", call.request.email)
    try {
        let query_res = await redis_module.GetRedis(const_module.code_prefix + call.request.email);
        console.log("query_res is ", query_res)
        if (query_res == null) {
        }
        let uniqueId = query_res;
        if (query_res == null) {
            uniqueId = uuidv4();
            if (uniqueId.length > 4) {
                uniqueId = uniqueId.substring(0, 4);
            }
            let bres = await redis_module.SetRedisExpire(const_module.code_prefix + call.request.email, uniqueId, 600)
            if (!bres) {
                callback(null, {
                    email: call.request.email,
                    error: const_module.Errors.RedisErr
                });
                return;
            }
        }
        console.log("uniqueId is ", uniqueId)
        let text_str = '您的验证码为' + uniqueId + '请三分钟内完成注册'
        //发送邮件
        let mailOptions = {
            from: '18137575298@163.com',
            to: call.request.email,
            subject: '验证码',
            text: text_str,
        };
        let send_res = await emailModule.SendMail(mailOptions);
        console.log("send res is ", send_res)
        callback(null, {
            email: call.request.email,
            error: const_module.Errors.Success
        });
    } catch (error) {
        console.log("catch error is ", error)
        callback(null, {
            email: call.request.email,
            error: const_module.Errors.Exception
        });
    }
}

// 定义主函数，用于创建并启动 gRPC 服务器
function main() {
    // 创建一个 gRPC 服务器实例
    var server = new grpc.Server();

    // 将服务注册到服务器，绑定服务方法 GetVarifyCode 到 VarifyService 服务中
    server.addService(message_proto.VarifyService.service, { 
        GetVarifyCode: GetVarifyCode // 将 GetVarifyCode 方法绑定到 gRPC 服务
    });

    // 绑定服务器地址和端口，使用不安全的 gRPC 连接（不加密）
    server.bindAsync('0.0.0.0:50051', grpc.ServerCredentials.createInsecure(), () => {
        //    server.start();  启动 gRPC 服务器
       
        console.log('grpc server started'); // 服务器启动后输出提示
    });
}

// 调用主函数，启动服务器
main();


//启动grpc server  GetVarifyCode声明为async是为了能在内部调用await。