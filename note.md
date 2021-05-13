#### js调用c++
- process、buffer等全局变量
在js层调用global其实就是调用了c++层的global对象，执行global.process = process的时候，即在c++层面给c++层的global对象新增了一个属性process，值是传进来的c++对象process。所以当我们在js层面访问process的时候，v8会在c++层面的global对象里查找process属性，这时候就会找到传进来的c++对象。

- process.binding
在js里直接调用c++是不可以的，但是js最终是要编译成二进制代码的。在二进制的世界里，js代码和c++代码就可以通信了，因为nodejs定义的那些c++模块和c++变量都是基于v8的架构的，比如定义了一个process对象，或者Binding函数，都是利用了v8的规则和接口。