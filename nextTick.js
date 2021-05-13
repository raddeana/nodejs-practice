/**
 * next tick
 * @author Chenxiangyu
 */

// process.nextTick()属于idle观察者
// setImmediate()属于check观察者
// 在每一轮循环检查中，idle观察者先于I/O观察者，I/O观察者先于check观察者
// process.nextTick()的回调函数保存在一个数组中
// setImmediate()的结果则是保存在链表中

process.nextTick(function(){
  console.log("nextTick延迟执行1");
});

process.nextTick(function(){
  console.log("nextTick延迟执行2");
});

setImmediate(function(){
  console.log("setImmediate延迟执行1");

  process.nextTick(function(){
    console.log("强势插入");
  });
});

setImmediate(function(){
  console.log("setImmediate延迟执行2");
});

console.log("正常执行");

// nextTick延迟执行1
// nextTick延迟执行2
// setImmediate延迟执行1
// 强势插入
// setImmediate延迟执行2
// 正常执行