



//io_context又叫io_service，是asio库的核心类，用于管理异步I/O操作。
//io_context 本质上维护一个任务队列。当你将 I/O 操作（例如异步 socket 操作）提交给 io_context 时，
// 任务会被放入队列中。
// 由于ioService不支持拷贝构造，所以要在初始化列表中初始化，也因此不能用std::make_shared<io_context>创建对象，
// 只能用new，因为make_shared会调用拷贝构造
//每次调用 io_context::run()，它会不断从队列中取出任务并执行，同时监听这些操作的完成状态。
// 当某个异步操作完成时，io_context 会将对应的回调函数放入任务队列
//io_context::run() 会阻塞当前线程，直到队列为空或者调用了 io_context::stop()。

//using Work = boost::asio::io_context::work;  这个work对象的作用是保证io_context不会在没有任务的时候退出run()函数
//如果绑定work对象，io_context会在没有任务的时候退出run()函数，导致程序结束
//如果没有绑定work对象，io_context会一直运行，直到work对象被析构，或者调用了io_context的stop()函数

