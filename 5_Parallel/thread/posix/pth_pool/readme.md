

### 思路 (忙等版)

简单的模拟线程池的写法，main 线程进行分配任务，然后定义一个全局变量
num 作为存储任务的，各线程从 num 中进行抢任务，然后还会创建 N 各线程。

当 num 中的任务被抢走后，那么将它的值置为 0，告诉 main 和其它线程说
明任务已被抢，这样 main 才号分配任务，当 num 中的值大于 0 时说明有任
务在，可以抢，当 num 值为 -1 时，说明全部任务已经分配完，各线程执行
完后可以退出了。

