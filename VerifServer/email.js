// 引入 Nodemailer 模块，用于发送电子邮件
const nodemailer = require('nodemailer');

// 引入自定义的配置模块，其中包含邮箱用户和授权码信息
const config_module = require("./config")

// 创建一个邮件发送的传输实例，配置邮件服务器信息
// 使用 `nodemailer.createTransport()` 方法来定义发送邮件的配置
let transport = nodemailer.createTransport({
    host: 'smtp.163.com', // 使用 163 邮箱的 SMTP 服务器
    port: 465,            // 465 端口用于 SSL 加密连接的 SMTP 服务
    secure: true,         // 启用安全连接（SSL/TLS）
    auth: {               // 配置认证信息
        user: config_module.email_user, // 发送方邮箱地址，从配置模块中读取
        pass: config_module.email_pass  // 邮箱的授权码（不是登录密码），用于安全登录
    }
});

/**
 * 发送邮件的函数，返回一个 Promise
 * @param {*} mailOptions_ 传递的邮件选项参数，包含收件人、主题和正文等信息
 * @returns {Promise} 成功发送时 resolve，失败时 reject
 */
function SendMail(mailOptions_) {
    // 使用 Promise 包装异步操作，处理邮件发送结果
    return new Promise(function(resolve, reject) {
        // 调用 nodemailer 的 sendMail 方法发送邮件
        transport.sendMail(mailOptions_, function(error, info) {
            if (error) {
                // 如果发送失败，打印错误信息并 reject
                console.log(error);
                reject(error);
            } else {
                // 如果发送成功，打印成功信息并 resolve
                console.log('邮件已成功发送：' + info.response);
                resolve(info.response); // 返回邮件发送结果
            }
        });
    });
}

// 导出 SendMail 函数，以便其他模块可以使用此函数来发送邮件
module.exports.SendMail = SendMail;


//因为transport.SendMail相当于一个异步函数，调用该函数后发送的结果是通过回调函数通知的，所以我们没办法同步使用，需要用Promise封装这个调用，抛出Promise给外部，那么外部就可以通过await或者then catch的方式处理了。