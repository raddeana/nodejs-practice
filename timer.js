// 执行顺序不确定
setImmediate(function () {
	console.log('immediate');
});

setTimeout(function () {
	console.log('timeout');
});