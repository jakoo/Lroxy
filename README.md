# Lroxy
##Intro:
Lroxy is a socks4 proxy builded with [muduo](https://github.com/chenshuo/muduo).

##Usage:
```
$ cd makefile  
$ make  
$ ./socks4 [config file path]
```

Search config file in current dir by default.  
There is a defalt config in [makefile dir](https://github.com/jakoo/Lroxy/blob/master/makefile/config).
   
   
Has been tested in behind environment:

Linux 3.19.8

g++ (GCC) 4.8.3

boost 1.54.0
######To do list:
1.Not fully tested yet. BIND method isn't tested.

2.Add support to Socks4a.

######License:
Copyright (c) 2015, Jiehong Zhang. All rights reserved.

Use of this source code is governed by a BSD-style license that can be found in the License file.