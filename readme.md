# 类Unix V6++二级文件系统使用指南
## 开发语言
* C++
  
## 系统支持
* CentOS 7 64位
  
## 库需求
* 32位C++标准编译库  
    
  在CentOS 7下执行语句  
  `yum install glibc-devel.i686` 与  `yum install libstdc++-devel.i686`  
  安装对应库即可  
## 文件清单
头文件类：
    
    Buf.h
    BufferManager.h
    File.h
    FileManager.h
    FileSystem.h
    INode.h
    Kernel.h
    OpenFileManager.h
    SecondFileSystem.h
    User.h
cpp文件：

    BufferManager.cpp
    File.cpp
    FileManager.cpp
    FileSystem.cpp
    Inode.cpp
    Kernel.cpp
    OpenFileManager.cpp
    SecondFileSystem.cpp
    _main1.cpp
Makefile文件：

    Makefile
测试用文件：

    img_in.png
    
## 系统安装与运行
  * 进入文件系统所在文件夹，执行  `make` 语句，得到文件**SecondFileSystem**，再在同目录下输入语句 `./SecondFileSystem` 即可运行文件系统
  
## 支持语法
### 通用操作
* `-help`  
 查询支持语句及句式

* `-fopen [filename] [mode]`  
  以mode模式打开当前目录下的filename文件  
  **mode = 1 为只读， mode = 2 为只写， mode = 3 为读写（下同）**  
  返回值将是文件对应的句柄fd，用于读写

* `-fclose [fd]`  
  关闭句柄fd

* `-fread [fd] [length]`  
  从句柄fd中读取长度为length字节的内容  
  *将于屏幕直接输出*  
  返回值为成功写入的字节数

* `-fwrite [fd] [source] [length]`  
  向句柄fd中写入source，长度为length  
  *source由屏幕内直接输入，长度过短将截断，过长则不保证超出部分的内容合法性*

* `-fseek [fd] [position] [ptrname]`  
  对句柄fd内文件指针进行移动，偏移量为seek，方式为ptrname  
  *ptrname = 0为由文件头开始，ptrname = 1由文件末开始，ptrname = 2则对整个文件所占空间进行操作，若为3~5，则偏移量单位将由字节变为文件块大小（本系统中为512字节）*  

* `-fcreate [filename] [mode]`  
  在当前目录创建文件名为filename的文件  
  返回值为文件句柄（注意与读写句柄区分）  

* `-fdelete [filename]`  
  删除当前目录下文件名为filename的文件  

* `-ls`  
  查看当前目录下的所有文件  

* `-mkdir [dirname]`  
  在当前目录下创建名为dirname的文件夹  

* `-cd [dirname]`  
  进入名为dirname的文件夹  
  **当dirname为../ ， 即命令为cd ../时为返回上级目录**  
  返回值为操作后所处目录  
  *非文件夹将返回报错*  

* `-quit`  
  退出系统，当前磁盘情况将被记录于同名目录下myDisk.disk文件中  

### 测试用操作  
* `-fread -2 [fd] [length]`  
  同-fread，但读取的内容将会被存入与宿主机进行交换用的暂存buffer中  
  *若已进行过-fread -2操作但未进行-save操作，则buffer内的内容不会被替换，操作无效*  
  *若已进行过-load操作但未进行-fwrite -2操作，则buffer内的内容不会被替换，操作无效*  

* `-fwrite -2 [fd]` 
  同-fwrite，但写入的source来源于与宿主机进行交换用的暂存buffer中  
  *若已进行过-fread -2操作但未进行-save操作，则buffer内的内容不会被替换，操作无效*  
  *若未进行过-load操作，则本次操作失败*  

* `-load [filename]`  
  从与二级文件系统同名目录中的filename文件中装载内容至buffer内  
  *若buffer已被load过，则不会替换，操作无效*  
  *若buffer等待被save，则不会替换，操作无效*  
  *若不存在filename则会报错*  

* `-save [filename]`  
  向与二级文件系统同名目录中的filename文件中写入buffer内的内容  
  *若buffer为空，则本次操作失败*  
  *若buffer已被load，则本次操作失败，操作无效*  

* `-check`  
  检查buffer内的当前状况，可查看buffer是否为空，并且是等待写入文件系统还是等待向宿主系统输出数据
