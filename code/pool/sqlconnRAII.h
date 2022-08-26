#ifndef SQLCONNRAII_H
#define SQLCONNRAII_H
#include "sqlconnpool.h"

/*
RAII [1]  （Resource Acquisition Is Initialization）,也称为“资源获取就是初始化”，是C++语言的一种管理资源、避免泄漏的惯用法。
C++标准保证任何情况下，已构造的对象最终会销毁，即它的析构函数最终会被调用。
简单的说，RAII 的做法是使用一个对象，在其构造时获取资源，在对象生命期控制对资源的访问使之始终保持有效，最后在对象析构的时候释放资源。
*/

/* 资源在对象构造初始化 资源在对象析构时释放*/
class SqlConnRAII {
public:
    SqlConnRAII(MYSQL** sql, SqlConnPool *connpool) {
        assert(connpool);
        *sql = connpool->GetConn(); // 获取资源
        sql_ = *sql;
        connpool_ = connpool;
    }
    
    ~SqlConnRAII() {
        if(sql_) { connpool_->FreeConn(sql_); } // 释放资源
    }
    
private:
    MYSQL *sql_;
    SqlConnPool* connpool_;
};

#endif //SQLCONNRAII_H