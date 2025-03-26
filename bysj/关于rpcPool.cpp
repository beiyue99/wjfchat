
RPConPool 里创建了一批 Stub 并放入连接池，这样就能高效复用这些连接，避免重复创建 gRPC 连接，提高性能。

VarifyService::NewStub(channel) 
channel：表示 gRPC 连接，Stub 通过它来发送请求，返回的是 std::unique_ptr<VarifyService::Stub>，
也就是 VarifyService 这个 gRPC 服务的客户端 Stub。这个 Stub 用于发送 gRPC 请求。



假设你有一个 gRPC 服务 VarifyService，它有一个 VerifyUser 方法：
verify.proto（gRPC 服务定义）
service VarifyService{
	rpc VerifyUser(VerifyRequest) returns(VerifyResponse);
}
当你用 protoc 生成 C++ 代码后，gRPC 会生成一个 VarifyService::Stub 类，并提供 NewStub() 方法来创建它。




std::shared_ptr<Channel> channel = grpc::CreateChannel(host + ":" + port,
	grpc::InsecureChannelCredentials());
connections_.push(VarifyService::NewStub(channel));
这行代码的含义是：
创建一个 gRPC 连接(channel)，连接到 host : port 服务器。
使用 NewStub(channel) 创建 VarifyService::Stub，用于向远程服务器发送请求。
把 Stub 存入连接池，供后续使用。