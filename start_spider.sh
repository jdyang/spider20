cd $(dirname $0)
ulimit -c 1024000
killall spider
mv ../log ../log_bak
mkdir ../log
nohup ./spider ../log/spider.conf 2>err 1>log & 
