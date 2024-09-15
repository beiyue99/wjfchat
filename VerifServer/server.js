const grpc = require('@grpc/grpc-js')
const message_proto = require("./proto")
const const_module = require('./const')
const { v4: uuidv4 } = require('uuid');
const emailModule = require('./email')


// 定义异步函数 GetVarifyCode，用于处理客户端的 gRPC 请求
// 'call' 参数包含请求信息，'callback' 用于响应客户端
async function GetVarifyCode(call, callback) {
    // 打印客户端发送的邮箱地址
    console.log("email is ", call.request.email);
    
    try {
        // 生成唯一的验证码（UUID）
        uniqueId = uuidv4();
        console.log("uniqueId is ", uniqueId);
        
        // 定义邮件内容，包含验证码和提示信息
        let text_str = '您的验证码为' + uniqueId + '，请三分钟内完成注册';
        
        // 构建邮件选项，包括发件人、收件人、主题和邮件正文
        let mailOptions = {
            from: '18137575298@163.com', // 发件人邮箱地址
            to: call.request.email, // 客户端请求中的目标邮箱地址
            subject: '验证码', // 邮件主题
            text: text_str, // 邮件正文，包含验证码
        };

        // 使用自定义的 emailModule 发送邮件，并等待异步响应
        let send_res = await emailModule.SendMail(mailOptions);
        console.log("send res is ", send_res);
        
        // 如果邮件发送成功，调用回调函数，返回成功信息给客户端
        callback(null, { 
            email: call.request.email, // 返回客户端发送的邮箱地址
            error: const_module.Errors.Success // 返回成功状态码
        }); 
    } catch (error) {
        // 如果发生错误，打印错误信息并返回异常状态给客户端
        console.log("catch error is ", error);
        callback(null, { 
            email: call.request.email, // 返回客户端发送的邮箱地址
            error: const_module.Errors.Exception // 返回异常状态码
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