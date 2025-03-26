

我们代码中RPConPool的构造函数如下：

RPConPool(size_t poolSize, std::string host, std::string port)
	: poolSize_(poolSize), host_(host), port_(port), b_stop_(false) {
	for (size_t i = 0; i < poolSize_; ++i) {
		std::shared_ptr<Channel> channel = grpc::CreateChannel(host + ":" + port,
			grpc::InsecureChannelCredentials());
		connections_.push(VarifyService::NewStub(channel));
	}



	connections_.push(VarifyService::NewStub(channel)); //这里是移动构造
	void push(const value_type & val);  // 传左值，拷贝构造
	void push(value_type && val);       // 传右值，移动构造
	这实际上是可以的，因为 NewStub(channel) 返回的是临时右值，push() 会自动选择移动构造版本。

	错误示范：
	auto stub = VarifyService::NewStub(channel);
	connections_.push(stub);  // ? 错误！尝试拷贝 unique_ptr

	正确写法：
	auto stub = VarifyService::NewStub(channel);
	connections_.push(std::move(stub));  // ? 这样才会调用移动构造

