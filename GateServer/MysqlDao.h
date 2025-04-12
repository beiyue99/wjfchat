#pragma once
#include "const.h"
#include <thread>
#include <jdbc/mysql_driver.h>
#include <jdbc/mysql_connection.h>
#include <jdbc/cppconn/prepared_statement.h>
#include <jdbc/cppconn/resultset.h>
#include <jdbc/cppconn/statement.h>
#include <jdbc/cppconn/exception.h>

// SqlConnection类用于封装mysql的连接对象和最后一次操作时间
class SqlConnection {
public:
	SqlConnection(sql::Connection* con, int64_t lasttime):_con(con), _last_oper_time(lasttime){}
	~SqlConnection() {
		if (_con) {
			try {
				_con->close();  // 显式关闭连接
			}
			catch (sql::SQLException& e) {
				// 记录错误但继续析构过程
				std::cerr << "Error closing connection: " << e.what() << std::endl;
			}
		}
	}
	std::unique_ptr<sql::Connection> _con;
	int64_t _last_oper_time;
};

class MySqlPool {
public:
	MySqlPool(const std::string& url, const std::string& user, const std::string& pass, const std::string& schema, int poolSize)
		: url_(url), user_(user), pass_(pass), schema_(schema), poolSize_(poolSize), b_stop_(false){
		try {
			for (int i = 0; i < poolSize_; ++i) {
				sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();//获取mysql驱动
				auto* con = driver->connect(url_, user_, pass_); //创建连接
				con->setSchema(schema_); //设置数据库
				auto currentTime = std::chrono::system_clock::now().time_since_epoch();// 获取当前时间戳
				long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();// 将时间戳转换为秒
				pool_.push(std::make_unique<SqlConnection>(con, timestamp)); //将连接放入连接池
			}

			_check_thread = std::thread([this]() {
				while (!b_stop_) {
					checkConnection();
					std::this_thread::sleep_for(std::chrono::seconds(60)); //每隔60s检查一次连接是否存活
				}
			});
			_check_thread.detach(); //分离线程,不需要等待线程结束,避免阻塞
		}
		catch (sql::SQLException& e) {
			std::cout << "mysql pool init failed, error is " << e.what()<< std::endl;
		}
	}

	void checkConnection() {
		std::lock_guard<std::mutex> guard(mutex_);
		int poolsize = pool_.size();
		auto currentTime = std::chrono::system_clock::now().time_since_epoch();// 获取当前时间戳
		long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();// 将时间戳转换为秒
		for (int i = 0; i < poolsize; i++) {
			auto con = std::move(pool_.front()); //取出连接
			pool_.pop();//从池子删除废掉的连接
			Defer defer([this, &con]() {
				pool_.push(std::move(con)); //将连接放回连接池，defer在作用域结束时执行传入的函数
			});

			if (timestamp - con->_last_oper_time < 300) { //如果距离上次操作时间小于300s,则不检查
				continue;
			}
			
			try {
				std::unique_ptr<sql::Statement> stmt(con->_con->createStatement()); //创建一个Statement对象,用于执行sql语句
				stmt->executeQuery("SELECT 1"); //执行一个查询语句,保证连接存活
				con->_last_oper_time = timestamp; //更新最后操作时间
				//std::cout << "execute timer alive query SELECT 1 , cur is " << timestamp << std::endl;
			}
			catch (sql::SQLException& e) {
				std::cout << "Error keeping connection alive: " << e.what() << std::endl;
				// 重新创建连接并替换旧的连接
				sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
				auto* newcon = driver->connect(url_, user_, pass_);
				newcon->setSchema(schema_);
				con->_con.reset(newcon);
				con->_last_oper_time = timestamp; //更新最后操作时间
			}
		}
	}

	std::unique_ptr<SqlConnection> getConnection() {
		std::unique_lock<std::mutex> lock(mutex_);
		cond_.wait(lock, [this] { 
			if (b_stop_) {
				return true;
			}		
			return !pool_.empty(); });
		if (b_stop_) {
			return nullptr;
		}
		std::unique_ptr<SqlConnection> con(std::move(pool_.front()));
		pool_.pop();
		return con;
	}

	void returnConnection(std::unique_ptr<SqlConnection> con) {
		std::unique_lock<std::mutex> lock(mutex_);
		if (b_stop_) {
			return;
		}
		pool_.push(std::move(con));
		cond_.notify_one();
	}

	void Close() {
		b_stop_ = true;
		cond_.notify_all();
	}

	~MySqlPool() {
		std::unique_lock<std::mutex> lock(mutex_);
		while (!pool_.empty()) {
			pool_.pop();//析构时清空连接池,会自动调用SqlConnection的析构函数，因为调用pop时会调用unique_ptr的析构函数
		}
	}

private:
	std::string url_;  // mysql连接地址
	std::string user_; // mysql用户名
	std::string pass_; // mysql密码
	std::string schema_; // mysql数据库
	int poolSize_;    // 连接池大小
	std::queue<std::unique_ptr<SqlConnection>> pool_;  // 连接池
	std::mutex mutex_;
	std::condition_variable cond_;
	std::atomic<bool> b_stop_; // 是否停止连接池,atomic保证线程安全,不需要加锁,提高效率
	std::thread _check_thread; // 定时检查连接是否存活的线程
};

// UserInfo结构体用于保存用户信息
struct UserInfo {
	std::string name;
	std::string pwd;
	int uid;
	std::string email;
	std::string icon;
};


// MysqlDao类用于封装mysql的操作
class MysqlDao
{
public:
	MysqlDao();
	~MysqlDao();
	// 注册用户
	int RegUser(const std::string& name, const std::string& email, const std::string& pwd);
	// 注册用户事务
	int RegUserTransaction(const std::string& name, const std::string& email, const std::string& pwd, const std::string& icon);
	bool CheckEmail(const std::string& name, const std::string & email);
	bool UpdatePwd(const std::string& name, const std::string& newpwd);
	bool CheckPwd(const std::string& name, const std::string& pwd, UserInfo& userInfo);
	bool TestProcedure(const std::string& email, int& uid, std::string& name);
private:
	std::unique_ptr<MySqlPool> pool_;
};


