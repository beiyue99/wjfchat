const nodemailer = require('nodemailer');
const config_module = require("./config")
/**
 * 创建发送邮件的代理
 */
let transport = nodemailer.createTransport({
    host: 'smtp.163.com',
    port: 465,
    secure: true,
    auth: {
        user: config_module.email_user, // 发送方邮箱地址
        pass: config_module.email_pass // 邮箱授权码或者密码
    }
});

/**
 * 发送邮件的函数
 * @param {*} mailOptions_ 发送邮件的参数
 * @returns 
 */
function SendMail(mailOptions_){
    return new Promise(function(resolve, reject){
        transport.sendMail(mailOptions_, function(error, info){
            if (error) {
                console.log(error);
                reject(error);
            } else {
                console.log('邮件已成功发送：' + info.response);
                resolve(info.response)
            }
        });
    })
}
module.exports.SendMail = SendMail

//因为transport.SendMail相当于一个异步函数，调用该函数后发送的结果是通过回调函数通知的，所以我们没办法同步使用，需要用Promise封装这个调用，抛出Promise给外部，那么外部就可以通过await或者then catch的方式处理了。