## Program Explanation
* `process-run.py`
  可以查看进程状态在CPU上运行时如何变化，进程可以处于以下几种不同的状态：
  * `RUNNING`：进程正在使用CPU
  * `READY`：进程现在可以使用CPU但其他进程正在使用
  * `BLOCKED`：进程正在等待I/O（例如，它向磁盘发送请求）
  * `DONE`：进程已经执行完成
## QA
1. 运行命令 `process-run.py -l 5:100`,5:100。CPU 利用率应该是多少（例如，CPU 使用时间的百分比）？你为什么知道这个？使用 -c 和 -p 标志来查看你是否正确。