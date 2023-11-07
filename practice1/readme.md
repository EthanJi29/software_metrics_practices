**Practice 1**

**522023330037 姬怡深**

1. 下载gnuit-4.9.5并解压。
```shell
curl http://ftp.gnu.org/gnu/git/gnuit-4.9.5.tar.gz 
tar -xvf data/gnuit-4.9.5.tar.gz 
```
understand新建项目打开gnuit，生成und文件。

1. 导入understand库。
   要使用understand库有两个途径：一是使用软件附带的upython解释器，其自带understand库；
   二是对软件目录中的understand.so使用stubgen生成.pyi文件，放在同一目录中，在脚本中添加路径：
```python
import sys
sys.path.append('/Applications/Understand.app/Contents/MacOS/Python')
import understand
```
本项目使用第二种途径。

3. 脚本编写参考官方文档：
   https://documentation.scitools.com/html/python/index.html
