

std::unique_ptr<VarifyService::Stub> getConnection() {
	std::unique_lock<std::mutex> lock(mutex_);
	cond_.wait(lock, [this] {
		if (b_stop_) {
			return true;   //如果停止则直接返回true,继续执行
		}
		return !connections_.empty(); //如果不为空则返回true,继续执行,否则等待
		});
	//如果停止则直接返回空指针
	if (b_stop_) {
		return  nullptr;
	}
	auto context = std::move(connections_.front());
	connections_.pop();
	return context;
}

执行流程：
先释放 mutex_（防止死锁）。
阻塞当前线程，直到：条件 为 true，或其他线程调用 cond_.notify_one() / cond_.notify_all() 唤醒。
被唤醒后，自动重新获取 mutex_，然后继续执行。否则一直阻塞，不占用CPU
注意：
它不会反复轮询 条件，而是等待事件触发，相比while检查机制，避免 CPU 资源浪费。