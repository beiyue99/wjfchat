


服务器端：

//先创建io_contex，然后创建server启动异步监听，创建新连接后，创建HttpConnection类管理连接。
//调用HttpConnection的start函数，就是异步读，
// 读完后（将数据存入 _buffer，解析后存入 _request）调用HandleReq处理请求
//根据get post进行不同的处理（通过LogicSystem类注册了相应的请求以及对应的处理函数）
//如果是获取验证码的post请求，服务器就会与验证服务发请求，
// 调用grpc生成的GetVarifyCode方法获取验证码,发送至邮箱(通过node.js实现)




客户端：

//http管理类在请求完成后发出http_finish信号（根据模块携带不同参数），然后自身调用slot_http_finish
//槽函数进行处理，根据携带的模块信息，调用不同的回调函数。如果是注册模块，请求完成发出的信号携带
//服务器回应的信息，这个回调函数会把这个信息等通过信号发出（reg_mod_finish)
//注册类slot_reg_mod_finish会进行相应的处理：如果成功解析服务器回的数据，就通知客户端验证码已经发送！
//然后将解析的数据通过消息id，找到对应的消息处理器处理。（消息id和对应的回调存在map里：_handlers）
//怎么处理就很简单，就是从json数据提取email，然后简单的打印，提示用户验证码已发送到邮箱

