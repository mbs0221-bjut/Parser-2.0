# Parser

开发环境：VS2013 C++

一个简单的脚本解释器

在Parser-1.0基础上去掉了代码生成而实现的一个解释器

Text.txt是用该脚本语言写的测试程序代码，编译运行整个工程文件，就可以运行这个脚本程序了

这个脚本解释器里，删减了表达式语法分析

增加了JSONObject和JSONArray的动态访问，支持一维数组访问

支持简单控制流语句，比如if-else for while do-while和case语句的执行

暂时还未实现continue和break语句

暂时未完善函数定义和调用
