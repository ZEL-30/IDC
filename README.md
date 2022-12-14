# Meteorological-Data-Center


# 一. 功能模块

## 1. 数据采集子系统

- ftp客户端,采用ftp协议,采集数据文件
- http客户端,采用http协议,从WEB服务接口采集数据
- 直连数据源的数据库,从表中抽取数据

## 2. 数据处理和加工子系统

- 把各种格式的原始数据解码转换成xml格式的数据文件
- 对原始数据进行二次加工,生成高可用的数据集

## 3. 数据入库子系统

- 把数百种数据存储到数据中心的表中

## 4. 数据同步子系统

- MySQL的高可用方案只能解决部分问题,不够灵活,效率不高
- 把核心数据库(Slave)表中的数据按条件同步到业务数据库中
- 把核心数据库(Slave)表中的数据增量同步到业务数据库中

## 5. 数据管理子系统

- 清理(删除)历史数据
- 把历史数据备份和归档

## 6. 数据交换子系统

- 把数据中心的数据从表中导出来,生成数据文件
- 采用ftp协议,把数据文件推送至对方的ftp服务器
- 基于tcp协议的快速文件传输系统

## 7. 数据服务总线

- 用C++开发WEB服务,为业务系统提供数据访问接口
- 效率极高(数据库连接池和线程池)

## 8. 网络代理服务

- 用于运维而开发的一个工具
- I/O服用技术(select\poll\epoll)

# 二. 重点和难点

- 服务程序的稳定性
- 数据处理和服务的效率
- 功能模块的通用性

# 三. 开发环境

- Centos7    ssh客户端    vscode
- MySQL 5.7    客户端Navicat Premium
- gcc    g++
- 字符集utf-8
