####################################################################
# 停止数据中心后台服务程序的脚本
####################################################################

# 强行终止调度程序
killall -9 procctl

# 通知所有的服务程序退出
killall gzipfiles crtsurfdata deletefiles ftpgetfiles ftpputfiles tcpputfiles tcpgetfiles fileserver
killall obtcodetodb obtmindtodb execsql dminingmysql xmltodb syncupdate syncincrement
killall deletetable migratetable xmltodb_oracle migratetable_oracle deletetable_oracle
killall syncupdate_oracle syncincrement_oracle 

sleep 3

# 让所有的服务程序强制退出
killall -9 gzipfiles crtsurfdata deletefiles ftpgetfiles ftpputfiles tcpputfiles tcpgetfiles fileserver
killall -9 obtcodetodb obtmindtodb execsql dminingmysql xmltodb syncupdate syncincrement
killall -9 deletetable migratetable xmltodb_oracle migratetable_oracle deletetable_oracle
killall -9 syncupdate_oracle syncincrement_oracle
