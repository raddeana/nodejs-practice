// 执行顺序不确定
setTimeout(function () {
	console.log('setTimeout');
}, 0)

setImmediate(function () {
	console.log('setImmediate');
})