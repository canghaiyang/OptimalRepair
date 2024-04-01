环境搭建-测试命令-调试命令-代码协助

----------------------------------------------------------------------
环境搭建

#1. 必备软件安装
sudo apt update && sudo apt install -y gcc g++ cmake make net-tools gdb git iperf openssh-client openssh-server

#2. SSH免密登录：每个机器都要轮流分发到对应机器
ssh-keygen 						//一直回车，在当前用户~下生成.ssh文件夹
ssh-copy-id -i ~/.ssh/id_rsa.pub root@192.168.7.101	//给需要免密登录的节点执行

#3. 创建ych文件夹并上传测试文档
cd /home && mkdir ych
winscp上传文件


#可选1. iperf网速测试节点之间网络
iperf3 -s			//在服务器节点192.168.7.102运行
iperf3 -c 192.168.7.102		//在非服务器节点运行

#可选2：通过切分文件准备测试文件
cd /home/ych/OptimalRepair/test_file/write && gcc -o cut_file_test cut_file_test.c && ./cut_file_test 1.58GB.mp4
#可选3：直接生成，无需准备测试文件64*3MB的文件生成
dd if=/dev/urandom iflag=fullblock of=file.txt bs=64M count=3 

----------------------------------------------------------------------
测试命令

#1. 准备测试运行环境：创建环境和删除环境（仅首次搭建使用，后续无需运行）
cd /home/ych/OptimalRepair && cd script && chmod a+x *
./create_env_storagenodes.sh 101 106
./delete_env_storagenodes.sh 101 106

#2. 编译命令 + 发送可执行文件 + 运行可执行文件
cd /home/ych/OptimalRepair && cd script && chmod a+x * && ./make.sh

#3. 测试命令
./start_client.sh -w 2MB_src 2MB_dst 		//2MB_src是client本地保存文件，2MB_dst是存储在分布式存储的文件名
./start_client.sh -r 2MB_src 2MB_dst 
./start_client.sh -trtra 2MB_repair 2MB_dst	//2MB_repair是新节点或故障节点修复得到的文件，2MB_dst是存储在分布式存储的文件名

#可选1.make.sh实际执行
cmake . && make  && cd script && ./kill_all_storagenodes.sh 102 106 && ./scp_storagenodes.sh 102 106 && ./start_all_storagenodes.sh 102 106

#可选2.单点故障
./kill_ip_storagenodes.sh 103


#可选3.网络带宽限制
./limit_network.sh 102 106 ens5 100000     	//ens5需要查看网卡名确定100Mbps
./unlimit_network.sh 102 106 ens5 

#可选4.网络带宽测试
./start_close_iperf3_server.sh 102 106 1
./start_close_iperf3_server.sh 102 106 0  关闭服务器
 ./start_iperf3_test.sh 102 106 1 

----------------------------------------------------------------------
调试命令

#可选1.GDB手动调试
gdb --args  ../build/client -r 2MB_src 2MB_dst

#可选2：GDB+VSCODE远程调试
优点:远程直接修改文件，添加print log和打断点以及图形化的调试，直到问题解决。避免反复传输执行文件
操作：修改task.json和launch.json

#可选3：内存调试：查看报错前面的变化
./valgrind.sh client -r 2MB_src 2MB_dst 

#可选4：实时查看文档信息
	tail -f ../log/log.txt

#可选5：网络端口查看
	netstat -tuln

#可选6：网络流量查看
iftop -i wlan0 -BnP -f 'port 8800'  //http://www.360doc.com/content/23/0630/18/21693298_1086847598.shtml 按T查看单次数据量
rates：分别表示过去 2s 10s 40s 的平均流量
----------------------------------------------------------------------
代码协助

# 1. git安装和库下载
1. 下载win版git，并注册gitee账号（账号发给我）
2. 右键打开git bash，进行以下操作
3. 初始化git信息
git config --global user.name "xxx"
git config --global user.email "你的邮箱地址"
ssh-keygen -t rsa -C "自己的邮箱地址"
4. 在~目录下生成文件夹.ssh，其中包含私钥和公钥（_pub结尾）。将pub文件内容粘贴至gitee的ssh设置中
比如C:\Users\22806\.ssh
5. git clone https://gitee.com/ychyhx/OptimalRepair.git

# 2. git每次使用
git pull	//每次使用前拉去远端看看是否更改
git add . 
git commit -m "提交信息" 	//提交 
git push
	若提交失败，则git pull查看冲突，修改后再提交[商量]
	- 内容冲突，打开冲突文件保留1个结果即可【即重新编辑冲突内容】
	- 修改/删除冲突，保留修改就直接add+commit+push，保留删除git rm filename+commit+push

#3. git其他
git log 查看日志

